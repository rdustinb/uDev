// Libraries Needed
#include <SPI.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "Timer.h"
#include <EEPROM.h>

// Used for debugging
//#define STATEDECODE
//#define DATAFLOW
#define CONSUMERDATA
//#define FUNCALLS
#define ADDREGISTRATION

// Teensy 3.1, Polling All Other Devices
#define NRFCSn      10
#define NRFIRQn     6
#define NRFCE       4
#define AIN         23

// Address Registration Parameters
#define ADDRESSCOUNT 32

// This parameter specifies the air data-rate that this device will communicate
// with. Both ends of the rF pipe must communicate with the same data-rate.
#define DATARATE 250
// This parameter sets the transmitting power of this device, available values
// are: 0, -6, -12 or -18 (representing dBm of power).
#define RF_PWR 0
// 4000us * # Retries allowed
#define ARD 4000
#define ARC 15
// This timeout value waiting for a Tx result from the nRF is dependent
// on the above 2 values. This delay is in ms, while the above values
// are in us.
#define TXFAILUREDELAY ((ARD*ARC*2)/1000)
// This delay occurs when this device is waiting for a response from the remote
// device. This device has already sent a successful transmission (according
// to the nRF device) and is now waiting for the handshaking to send a response.
// To reduce complexity it may be better to make this the same function as the
// above value since the remote device goes through the above steps anyway.
#define RSPFAILUREDELAY ((ARD*ARC*3)/1000)
// This delay is how long to wait after this device initiates a handshake and
// there is any kind of failure.
#define FAILUREDELAY 5000
// This delay is how long the device waits after receiving data to when it 
// tries to transmit a response.
#define DELAYTXMILLIS 20
// This delay is consumer-specific, how long between sensor sample requests 
// in milliseconds.
#define REQTIMERMILIS 60000

// Control Words
#define REQADDR             0xF0
#define REQSENSOR           0xFE

// nRF Command Set
#define R_REGISTER          0x00 // 0x00 - 0x1C
#define W_REGISTER          0x20 // 0x20 - 0x3C
#define R_RX_PAYLOAD        0x61
#define W_TX_PAYLOAD        0xA0
#define FLUSH_TX            0xE1
#define FLUSH_RX            0xE2
#define REUSE_TX_PL         0xE3
#define ACTIVATE            0x50
#define R_RX_PL_WID         0x60
#define W_ACK_PAYLOAD       0xA8 // 0xA8 - 0xAF
#define W_TX_PAYLOAD_NOACK  0xB0
#define NOP                 0xFF

// nRF Register Space
#define CONFIG              0x00
#define EN_AA               0x01
#define EN_RXADDR           0x02
#define SETUP_AW            0x03
#define SETUP_RETR          0x04
#define RF_CH               0x05
#define RF_SETUP            0x06
#define STATUS              0x07
#define OBSERVE_TX          0x08
#define CARRIERDETECT       0x09
#define RX_ADDR_P0          0x0A
#define RX_ADDR_P1          0x0B
#define RX_ADDR_P2          0x0C
#define RX_ADDR_P3          0x0D
#define RX_ADDR_P4          0x0E
#define RX_ADDR_P5          0x0F
#define TX_ADDR             0x10
#define RX_PW_P0            0x11
#define RX_PW_P1            0x12
#define RX_PW_P2            0x13
#define RX_PW_P3            0x14
#define RX_PW_P4            0x15
#define RX_PW_P5            0x16
#define FIFO_STATUS         0x17
#define DYNPLD              0x1C
#define FEATURE             0x1D

// Advanced Types
enum States {IDLE,RXTOTX_RXCOMMAND,RXTOTX_DELAYTOTX,RXTOTX_TXRESPONSE,
  RXTOTX_DELAYSTATUS,RXTOTX_DECODESTATUS,RXTOTX_TXFAILURE,TXTORX_DELAYTOTX,
  TXTORX_TXCOMMAND,TXTORX_DELAYSTATUS,TXTORX_DECODESTATUS,TXTORX_SETUPRX,
  TXTORX_DELAYRX,TXTORX_RXRESPONSE,TXTORX_TXFAILURE} state;
Timer reqTimer;

// Globals
uint8_t defaultAddress[5] = {0x01,0x23,0x45,0x67,0x89};
uint8_t* p_defaultAddress = defaultAddress;
volatile unsigned long enterTxMillis;
volatile char timeChar;
volatile unsigned long currentTime;
volatile unsigned long currentBootTime;
volatile unsigned int totalHandshakes;
volatile unsigned int failedHandshakes;
volatile unsigned int timerExpired;
volatile unsigned int nrfResults;
// Main Address Registration
// How to make this dynamically adjustable?
struct Producers {
  uint8_t address[ADDRESSCOUNT][5];
  uint8_t* p_address[ADDRESSCOUNT];
  uint8_t serial_number[ADDRESSCOUNT][5];
  uint8_t sequential_failed_samples[ADDRESSCOUNT];
  uint8_t registered_count;
  uint8_t service_count;
} producer_list;

/*********************************/
/********* Req Timer ISR *********/
/*********************************/
void timerISR(){
  cli();
  #ifdef STATEDECODE
  Serial.println(F("timerISR: timer expired!"));
  #endif
  timerExpired = 1;
  sei();
}

/*********************************/
/******** nRF Result ISR *********/
/*********************************/
void nrfISR(){
  cli();
  #ifdef STATEDECODE
  Serial.println(F("nrfISR: interrupt seen!"));
  #endif
  nrfResults = 1;
  sei();
}

/*********************************/
/********** Main Setup ***********/
/*********************************/
void setup(){
  // Setup Serial Monitor
  Serial.begin(115200);
  // Clear Globals
  timeChar = 0;
  currentTime = 0;
  currentBootTime = 0;
  totalHandshakes = 0;
  failedHandshakes = 0;
  timerExpired = 0;
  nrfResults = 0;
  state = IDLE;
  // Setup the nRF Pins
  pinMode(NRFCSn, OUTPUT);
  pinMode(NRFIRQn, INPUT_PULLUP);
  pinMode(NRFCE, OUTPUT);
  // Begin SPI for the nRF Device
  SPI.begin();
  // Setup the nRF Device Configuration
  nrfSetup();
  Serial.println(F("nRF Setup Complete!"));
  // Setup the request timer
  reqTimer.every(REQTIMERMILIS, timerISR);
  // Setup the Interrupt Pin, parameterized
  attachInterrupt(NRFIRQn, nrfISR, FALLING);
  Serial.println(F("Core Setup Complete!"));
}

/*********************************/
/*********** Main Loop ***********/
/*********************************/
void loop(){
  // Loop Variables
  int status;

  // This "updates" the timer, kind of lame but apparently it works
  // may need to factor in variance when other code is running.
  reqTimer.update();

  // Service User Input
  getUserInput();

  switch(state){
    case IDLE:
      if(nrfResults == 1){
        // Data Received asynchronously on generic address port
        nrfResults = 0;
        #ifdef STATEDECODE
        Serial.println(F("IDLE->RXTOTX_RXCOMMAND"));
        #endif
        state = RXTOTX_RXCOMMAND;
      }else if(timerExpired == 1){
        // Round Robin Timer Expired, sample all registered producers
        timerExpired = 0;
        // Only start sampling if we have at least one registered producer
        if(producer_list.registered_count != 0){
          // Snapshot Current time value
          enterTxMillis = millis();
          #ifdef STATEDECODE
          Serial.println(F("IDLE->TXTORX_DELAYTOTX"));
          #endif
          totalHandshakes++;
          state = TXTORX_DELAYTOTX;
        }
      }
      break;
    /******************************************
          Receive First then Transmit Flow
    ******************************************/
    case RXTOTX_RXCOMMAND:
      // Check the status
      status = decode_status();
      if(status == 1){
        // Determine what needs to happen
        decode_rx_fifo();
      }else{
        #ifdef STATEDECODE
        Serial.println(F("nRf comm failure, no Rx response received.")); 
        #endif
      }
      // Clear FIFOs
      clear_fifos();
      // Clear Flags
      clear_flags();
      // Power Down the Transceiver
      power_down();
      #ifdef STATEDECODE
      Serial.println(F("RXTOTX_RXCOMMAND->RXTOTX_DELAYTOTX")); 
      #endif
      state = RXTOTX_DELAYTOTX;
      break;
    case RXTOTX_DELAYTOTX:
      // Standard Delay (Functionalize this?)
      if((millis() - enterTxMillis) >= DELAYTXMILLIS){
        #ifdef STATEDECODE
        Serial.println(F("RXTOTX_DELAYTOTX->RXTOTX_TXRESPONSE"));
        #endif
        state = RXTOTX_TXRESPONSE;
      }
      break;
    case RXTOTX_TXRESPONSE:
      // Power Up in Tx
      power_up_tx();
      // Clear FIFOs
      clear_fifos();
      // Clear Flags
      clear_flags();
      // The producer needs an address for communicating with the consumer and
      // the consumer needs to know to request data from that particular
      // producer. Generate a random address and increment the registered 
      // producer pointer.
      register_producer();
      // Get the current Milisecond time to allow for a maximum wait time
      enterTxMillis = millis();
      #ifdef STATEDECODE
      Serial.println(F("RXTOTX_TXRESPONSE->RXTOTX_DELAYSTATUS"));
      #endif
      state = RXTOTX_DELAYSTATUS;
      break;
    case RXTOTX_DELAYSTATUS:
      if(nrfResults == 1){
        nrfResults = 0;
        #ifdef STATEDECODE
        Serial.println(F("RXTOTX_DELAYSTATUS->RXTOTX_DECODESTATUS"));
        #endif
        state = RXTOTX_DECODESTATUS;
      }else if(tx_timeout_reached(enterTxMillis)){
        #ifdef STATEDECODE
        Serial.println(F("RXTOTX_DELAYSTATUS->RXTOTX_TXFAILURE"));
        #endif
        Serial.println(F("Tx never received a result from the local nRf, failing."));
        enterTxMillis = millis();
        state = RXTOTX_TXFAILURE;
      }
      break;
    case RXTOTX_DECODESTATUS:
      // Check the status
      status = decode_status();
      // Clear FIFOs
      clear_fifos();
      // Power Down
      power_down();
      // Set back to the default address
      set_addr_pipe(p_defaultAddress,5);
      if(status == -1){
        #ifdef STATEDECODE
        Serial.println(F("Tx failed with nRf Max Retries, failing.")); 
        Serial.println(F("RXTOTX_DECODESTATUS->RXTOTX_TXFAILURE")); 
        #endif
        enterTxMillis = millis();
        // Clear FIFOs
        clear_fifos();
        // Clear Flags
        clear_flags();
        state = RXTOTX_TXFAILURE;
      }else if(status == 0){
        #ifdef STATEDECODE
        Serial.println(F("RXTOTX_DECODESTATUS->IDLE")); 
        #endif
        // Increment the Registered Producer Count
        producer_list.registered_count++;
        // Increment the EEPROM registered value
        EEPROM.update(0, producer_list.registered_count);
        #ifdef ADDREGISTRATION
        Serial.print(F("Incrementing register producer count: ")); 
        Serial.println(producer_list.registered_count, DEC);
        #endif
        state = IDLE;
      }
      break;
    case RXTOTX_TXFAILURE:
      #ifdef STATEDECODE
      Serial.println(F("RXTOTX_TXFAILURE->IDLE")); 
      #endif
      power_up_rx();
      state = IDLE;
      break;
    /******************************************
          Transmit First then Receive Flow
    ******************************************/
    case TXTORX_DELAYTOTX:
      // Standard Delay (Functionalize this?)
      if((millis() - enterTxMillis) >= DELAYTXMILLIS){
        #ifdef STATEDECODE
        Serial.println(F("TXTORX_DELAYTOTX->TXTORX_TXCOMMAND"));
        #endif
        state = TXTORX_TXCOMMAND;
      }
      break;
    case TXTORX_TXCOMMAND:
      // Switch Pipe to next device
      set_addr_pipe(producer_list.p_address[producer_list.service_count],5);
      // Power Up in Tx
      power_up_tx();
      // Clear FIFOs
      clear_fifos();
      #ifdef DATAFLOW
      Serial.print(F("Requesting data!"));
      get_addr_pipe();
      #endif
      // Request Sensor Data
      request_sensor_data();
      // Get the current Milisecond time to allow for a maximum wait time
      enterTxMillis = millis();
      #ifdef STATEDECODE
      Serial.println(F("TXTORX_TXCOMMAND->TXTORX_DELAYSTATUS"));
      #endif
      state = TXTORX_DELAYSTATUS;
      break;
    case TXTORX_DELAYSTATUS:
      if(nrfResults == 1){
        nrfResults = 0;
        #ifdef STATEDECODE
        Serial.println(F("TXTORX_DELAYSTATUS->TXTORX_DECODESTATUS"));
        #endif
        state = TXTORX_DECODESTATUS;
      }else if(tx_timeout_reached(enterTxMillis)){
        #ifdef STATEDECODE
        Serial.println(F("TXTORX_DELAYSTATUS->TXTORX_TXFAILURE"));
        #endif
        Serial.println(F("Tx never received a result from the local nRf, failing."));
        enterTxMillis = millis();
        state = TXTORX_TXFAILURE;
      }
      break;
    case TXTORX_DECODESTATUS:
      // Check the status
      status = decode_status();
      // Clear FIFOs
      clear_fifos();
      // Power Down
      power_down();
      if(status == -1){
        #ifdef STATEDECODE
        Serial.println(F("Tx failed with nRf Max Retries, failing.")); 
        Serial.println(F("TXTORX_DECODESTATUS->TXTORX_TXFAILURE")); 
        #endif
        enterTxMillis = millis();
        state = TXTORX_TXFAILURE;
      }else if(status == 0){
        #ifdef STATEDECODE
        Serial.println(F("TXTORX_DECODESTATUS->TXTORX_SETUPRX")); 
        #endif
        state = TXTORX_SETUPRX;
      }
      break;
    case TXTORX_SETUPRX:
      // Power Rx
      power_up_rx();
      // Get the current Milisecond time to allow for a maximum wait time
      enterTxMillis = millis();
      #ifdef STATEDECODE
      Serial.println(F("TXTORX_SETUPRX->TXTORX_DELAYRX"));
      #endif
      state = TXTORX_DELAYRX;
      break;
    case TXTORX_DELAYRX:
      if(nrfResults == 1){
        nrfResults = 0;
        #ifdef STATEDECODE
        Serial.println(F("TXTORX_DELAYRX->TXTORX_RXRESPONSE"));
        #endif
        state = TXTORX_RXRESPONSE;
      }else if(rsp_timeout_reached(enterTxMillis)){
        #ifdef STATEDECODE
        Serial.println(F("TXTORX_DELAYRX->TXTORX_TXFAILURE"));
        #endif
        Serial.println(F("Maximum timeout for a response received, failing."));
        enterTxMillis = millis();
        state = TXTORX_TXFAILURE;
      }
      break;
    case TXTORX_RXRESPONSE:
      // Check the status
      status = decode_status();
      if(status == 1){
        // Print Current Time
        print_current_time(currentTime+millis());
        // Print Stats
        print_comm_stats(totalHandshakes, failedHandshakes);
        // Simply Print the Rx FIFO Contents
        decode_rx_fifo();
      }else{
        #ifdef STATEDECODE
        Serial.println(F("nRf comm failure, no Rx response received.")); 
        #endif
      }
      // Power Down for interface changes
      power_down();
      // Increment servicing counter to next device
      producer_list.service_count++;
      // Branch if there are more devices to sample
      if(producer_list.service_count < producer_list.registered_count){
        // Snapshot Current time value
        enterTxMillis = millis();
        #ifdef STATEDECODE
        Serial.println(F("TXTORX_RXRESPONSE->TXTORX_DELAYTOTX")); 
        #endif
        state = TXTORX_DELAYTOTX;
      }else{
        // Clear servicing count
        producer_list.service_count = 0;
        // Set back to generic address pipe
        set_addr_pipe(p_defaultAddress,5);
        // When done servicing, return to Rx mode with generic address pipe
        power_up_rx();
        #ifdef STATEDECODE
        Serial.println(F("TXTORX_RXRESPONSE->IDLE")); 
        #endif
        state = IDLE;
      }
      break;
    case TXTORX_TXFAILURE:
      failedHandshakes++;
      state = IDLE;
      break;
  }
}

/*********************************/
/********* nRf Functions *********/
/*********************************/
void clear_fifos(){
  #ifdef FUNCALLS
  Serial.println(F("clear_fifos()")); 
  #endif
  // Clear Rx FIFOs
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(FLUSH_RX);
  digitalWrite(NRFCSn, HIGH);
  // Clear Tx FIFOs
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(FLUSH_TX);
  digitalWrite(NRFCSn, HIGH);
}

void clear_flags(){
  #ifdef FUNCALLS
  Serial.println(F("clear_flags()")); 
  #endif
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(STATUS+W_REGISTER);
  SPI.transfer(0x70);
  digitalWrite(NRFCSn, HIGH);
}

void power_down(){
  #ifdef FUNCALLS
  Serial.println(F("power_down()")); 
  #endif
  // Disable Chip Enable (Kills Rx Mode, mostly)
  digitalWrite(NRFCE, LOW);
  delay(1);
  // Power Down
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(CONFIG+W_REGISTER);  // Write Reg (001x_xxxx)
  SPI.transfer(0x00);               // Power Down
  digitalWrite(NRFCSn, HIGH);
  // Wait 1.5ms to allow power down to occur
  delay(2);
}

void power_up_rx(){
  #ifdef FUNCALLS
  Serial.println(F("power_up_rx()")); 
  #endif
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(CONFIG+W_REGISTER);  // Write Reg (001x_xxxx)
  SPI.transfer(0x0B);               // CRC Enabled, Powerup Enabled, PRIM_RX Hi
  digitalWrite(NRFCSn, HIGH);
  // Wait for 1.5ms after Power Up Command
  delay(2);
  // Setup Rx, CE set HI until data received
  digitalWrite(NRFCE, HIGH);
}

void power_up_tx(){
  #ifdef FUNCALLS
  Serial.println(F("power_up_tx()")); 
  #endif
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(CONFIG+W_REGISTER);  // Write Reg (001x_xxxx)
  SPI.transfer(0x0A);               // CRC Enabled, Powerup Enabled, PRIM_RX Lo
  digitalWrite(NRFCSn, HIGH);
  // Wait for 1.5ms after Power Up Command
  delay(2);
}

uint8_t get_status(){
  #ifdef FUNCALLS
  Serial.println(F("get_status()")); 
  #endif
  uint8_t temp;
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(STATUS);
  temp = SPI.transfer(NOP);
  digitalWrite(NRFCSn, HIGH);
  return temp;
}

uint8_t get_top_rx_fifo_count(){
  #ifdef FUNCALLS
  Serial.println(F("get_top_rx_fifo_count()")); 
  #endif
  uint8_t temp;
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(R_RX_PL_WID);
  temp = SPI.transfer(NOP);
  digitalWrite(NRFCSn, HIGH);
  return temp;
}

bool get_carrier_detect_flag(){
  #ifdef FUNCALLS
  Serial.println(F("get_carrier_detect_flag()")); 
  #endif
  uint8_t temp;
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(CARRIERDETECT);
  temp = SPI.transfer(NOP);
  digitalWrite(NRFCSn, HIGH);
  if(temp == 0x0){
    return false;
  }else{
    return true;
  }
}

/*********************************/
/*********** nRF Steup ***********/
/*********************************/
void nrfSetup(){
  digitalWrite(NRFCSn, HIGH);
  digitalWrite(NRFCE, LOW);
  // Wait for 10.3ms for nRF to come up
  delay(11);
  // Change to parameterization set above
  uint8_t RF_DATARATE_VAL = (((DATARATE == 250) ? 0x4 : 
    ((DATARATE == 1) ? 0x0 : 
    ((DATARATE == 2) ? : 0x1)))<<3);
  uint8_t RF_POWER_VAL = ((RF_PWR == 0) ? 0x3 : 
   ((RF_PWR == -6) ? 0x2 : 
   ((RF_PWR == -12) ? 0x1 : 
   ((RF_PWR == -18) ? 0x0 : 0x3))))<<1;
  // OR-combine values to create the entire 
  uint8_t RF_SETUP_VAL = RF_DATARATE_VAL | RF_POWER_VAL;
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(RF_SETUP+W_REGISTER);
  SPI.transfer(RF_SETUP_VAL);
  digitalWrite(NRFCSn, HIGH);
  // Change default retry timeout, retry count values from above
  digitalWrite(NRFCSn, LOW);
  uint8_t SETUP_RETR_VAL = (((ARD/250-1)<<4)+(ARC));
  SPI.transfer(SETUP_RETR+W_REGISTER);
  SPI.transfer(SETUP_RETR_VAL);
  digitalWrite(NRFCSn, HIGH);
  // Change to Channel 1
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(RF_CH+W_REGISTER);
  SPI.transfer(0x1);
  digitalWrite(NRFCSn, HIGH);
  // Activate Extended Access
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(ACTIVATE);
  SPI.transfer(0x73); // Magic Value for Activate
  digitalWrite(NRFCSn, HIGH);
  // Enable Dynamic Payload on Rx Pipe
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(DYNPLD+W_REGISTER);
  SPI.transfer(0x1); // Dynamic Payload per Pipe, Pipe 0 Only
  digitalWrite(NRFCSn, HIGH);
  // Enable Dynamic Payload
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(FEATURE+W_REGISTER);
  SPI.transfer(0x4); // Dynamic Payload Enable
  digitalWrite(NRFCSn, HIGH);
  // Initialize the Address List Pointers
  for(int i=0; i<ADDRESSCOUNT; i++){
    producer_list.p_address[i] = &producer_list.address[i][0];
  }
  // Initialize the list of registered devices
  producer_list.registered_count = EEPROM.read(0);
  producer_list.service_count = 0;
  #ifdef ADDREGISTRATION
  Serial.print(F("Registered producer count: ")); 
  Serial.println(producer_list.registered_count, DEC);
  #endif
  /*
    0   Registered Device Count
    1   Dev 0, Byte 0
    2   Dev 0, Byte 1
    3   Dev 0, Byte 2
    4   Dev 0, Byte 3
    5   Dev 0, Byte 4
    6   Dev 1, Byte 0
    7   Dev 1, Byte 1
    8   Dev 1, Byte 2
    9   Dev 1, Byte 3
    10  Dev 1, Byte 4
    11  Dev 2, Byte 0
    12  Dev 2, Byte 1
  */
  for(int i=0; i<producer_list.registered_count; i++){
    #ifdef ADDREGISTRATION
    Serial.print(F("Registered address is: 0x"));
    #endif
    for(int j=0; j<5; j++){
      uint8_t addrByte = EEPROM.read((i)*5+1+j);
      producer_list.address[i][j] = addrByte;
      #ifdef ADDREGISTRATION
      Serial.print(addrByte, HEX);
      #endif
    }
    #ifdef ADDREGISTRATION
    Serial.println(F(" "));
    #endif
  }
  // Always Power up in Rx Mode
  power_up_rx();

  // Setup Pipe
  set_addr_pipe(p_defaultAddress,5);
  // Clear STATUS Flags
  clear_flags();
  // Clear FIFOs
  clear_fifos();
}

/*********************************/
/****** Low Level Functions ******/
/*********************************/
void request_sensor_data(){
  // Request Sensor Data
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(W_TX_PAYLOAD);
  SPI.transfer(REQSENSOR);
  digitalWrite(NRFCSn, HIGH);
  // Set CE Hi (>10us) to Enable Tx
  digitalWrite(NRFCE, HIGH);
  delay(10);
  digitalWrite(NRFCE, LOW);
}

bool tx_timeout_reached(unsigned long refMillis){
  // Expecting a very fast response from the local nRf
  if((millis()-refMillis) > TXFAILUREDELAY){
    return true;
  }else{
    return false;
  }
}

bool rsp_timeout_reached(unsigned long refMillis){
  if((millis()-refMillis) > RSPFAILUREDELAY){
    return true;
  }else{
    return false;
  }
}

bool delay_after_failure(unsigned long refMillis){
  if((millis()-refMillis) > FAILUREDELAY){
    return true;
  }else{
    return false;
  }
}

int decode_status(){
  #ifdef FUNCALLS
  Serial.println(F("decode_status()")); 
  #endif
  // Check Status Register for Results
  uint8_t resultStatus = get_status();
  // Clear STATUS Flags
  clear_flags();
  // Tx Failed
  if((resultStatus & 0x10) == 0x10){
    return -1;
  }
  // Tx of Command Succeeded
  else if((resultStatus & 0x20) == 0x20){
    return 0;
  }
  // Rx Data Present
  else if((resultStatus & 0x40) == 0x40){
    return 1;
  }else{
    return -2;
  }
}

void set_addr_pipe(uint8_t* address, uint8_t length){
  #ifdef ADDREGISTRATION
  Serial.println(F("Changing address of PIPE"));
  #endif
  // Set Tx Address
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(TX_ADDR+W_REGISTER); // Write Reg (001x_xxxx)
  for(int i=0; i<length; i++){
    SPI.transfer(address[i]);
  }
  digitalWrite(NRFCSn, HIGH);
  // Set Rx0 Address
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(RX_ADDR_P0+W_REGISTER); // Write Reg (001x_xxxx)
  for(int i=0; i<length; i++){
    SPI.transfer(address[i]);
  }
  digitalWrite(NRFCSn, HIGH);
}

void get_addr_pipe(){
  #ifdef FUNCALLS
  Serial.println(F("get_addr_pipe()")); 
  #endif
  #ifdef DATAFLOW
  uint8_t current_tx_address[5];
  uint8_t current_rx_address[5];
  // Set Tx Address
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(TX_ADDR);
  for(int i=0; i<5; i++){
    current_tx_address[i] = SPI.transfer(NOP);
  }
  digitalWrite(NRFCSn, HIGH);
  Serial.print(F("Current Tx Address is: 0x"));
  for(int i=0; i<5; i++){
    Serial.print(current_tx_address[i], HEX);
  }
  Serial.println(F(" "));
  // Set Rx0 Address
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(RX_ADDR_P0);
  for(int i=0; i<5; i++){
    current_rx_address[i] = SPI.transfer(NOP);
  }
  digitalWrite(NRFCSn, HIGH);
  Serial.print(F("Current Rx0 Address is: 0x"));
  for(int i=0; i<5; i++){
    Serial.print(current_rx_address[i], HEX);
  }
  Serial.println(F(" "));
  #endif
}

void register_producer(){
  #ifdef ADDREGISTRATION
  Serial.println(F("register_producer()")); 
  #endif
  // Generate the random address
  randomSeed(analogRead(AIN));
  // Store the new address in EEPROM and in the working structure
  for(int i=0; i<5; i++){
    // Store at the address offset based on number of currently registered addresses
    /*
      0   Registered Device Count
      1   Dev 0, Byte 0
      2   Dev 0, Byte 1
      3   Dev 0, Byte 2
      4   Dev 0, Byte 3
      5   Dev 0, Byte 4
      6   Dev 1, Byte 0
      7   Dev 1, Byte 1
      8   Dev 1, Byte 2
      9   Dev 1, Byte 3
      10  Dev 1, Byte 4
      11  Dev 2, Byte 0
      12  Dev 2, Byte 1
    */
    byte addrByte = random(256);
    EEPROM.update(((EEPROM.read(0)*5)+i+1), addrByte);
    producer_list.address[producer_list.registered_count][i] = addrByte;
  }
  #ifdef ADDREGISTRATION
  Serial.print(F("Registered address is: 0x"));
  for(int i=0; i<5; i++){
    Serial.print(producer_list.address[producer_list.registered_count][i], HEX);
  }
  Serial.println(F(" "));
  #endif
  // Send the new address
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(W_TX_PAYLOAD);
  SPI.transfer(REQADDR);
  for(int i=0; i<5; i++){
    SPI.transfer((uint8_t)(producer_list.address[producer_list.registered_count][i]));
  }
  digitalWrite(NRFCSn, HIGH);
  // Set CE Hi (>10us) to Enable Tx
  digitalWrite(NRFCE, HIGH);
  delay(10);
  digitalWrite(NRFCE, LOW);
}

void decode_rx_fifo(){
  #ifdef FUNCALLS
  Serial.println(F("decode_rx_fifo()")); 
  #endif
  // Get number of bytes received
  byte payloadByteCountTop = get_top_rx_fifo_count();
  // Temp Storage
  byte rxControl = NOP;
  byte payloadBytes[(payloadByteCountTop-1)];
  // Read out Received Bytes
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(R_RX_PAYLOAD);
  for(int i=0; i<payloadByteCountTop; i++){
    if(i==0){
      rxControl = SPI.transfer(NOP);
    }else{
      payloadBytes[(i-1)] = SPI.transfer(NOP);
    }
  }
  digitalWrite(NRFCSn, HIGH);
  // Decode the Incoming Command
  switch(rxControl){
    case REQADDR:
      // When a request for address is performed, nothing is actually done in
      // the consumer at this point. Maybe in the future it will register
      // the requesting producer's serial number and/or it's capabilities
      break;
    case REQSENSOR:
      // The DHT11 currently returns 1 Byte for the 
      // integral part and 1 Byte for the franctional
      // part of each Humidity and Temp, however the
      // DHT11 doesn't actually return those parts so
      // they are omitted in the DHT11 Library, 
      // returning only a single byte.
      #ifdef CONSUMERDATA
        int temperature = payloadBytes[0] + payloadBytes[1]*256;
        int humidity = payloadBytes[2] + payloadBytes[3]*256;
        print_sensor_data(temperature, humidity);
      #endif
      break;
  }
  // Clear FIFOs
  clear_fifos();
}

/*********************************/
/******** Debug Functions ********/
/*********************************/
void print_current_time(unsigned long currentMillis){
  Serial.print(F("Time of last update [HH:MM]: "));
  if((currentMillis/1000/60/60)%24 < 10){
    Serial.print(F("0"));
  }
  Serial.print((currentMillis/1000/60/60)%24, DEC);
  Serial.print(F(":"));
  if(((currentMillis/1000/60)%60) < 10){
    Serial.print(F("0"));
  }
  Serial.println(((currentMillis/1000/60)%60), DEC);
}

void print_comm_stats(int total, int failed){
  Serial.println(F("--- Comm Stats ---"));
  Serial.print(F("Throughput Rate [%]: "));
  Serial.println((((float)(total)-(float)(failed))/(float)(total))*100.0, 2);
  Serial.print(F("Total Comms: "));
  Serial.println(total, DEC);
  Serial.print(F("Failed Comms: "));
  Serial.println(failed, DEC);
}

void print_sensor_data(int temperature, int humidity){
  // Magic Constants for Calculated Dew Point
  float A = 8.1332;
  float B = 1762.39;
  float C = 235.66;
  // Temperature and Humidity Values
  float temp_C = (float)(-46.85) + (float)(175.72) * ((float)(temperature) / (float)(65536));
  float temp_F = (((float)(-46.85) + (float)(175.72) * ((float)(temperature) / (float)(65536)))*9/5 + 32);
  float humid = (float)(-6.0) + (float)(125.0) * ((float)(humidity) / (float)(65536));
  // Intermediate Values for Calculated Dew Point
  float pp_tamb = pow(10,(A - (B / (temp_C + C))));
  // Calculated Dew Point
  float tdew = (float)(-((B / ((log(humid * (pp_tamb / 100))) - A)) + C));
  Serial.print(F("Internal Temp [F]: "));
  Serial.println(temp_F, 2);
  Serial.print(F("Humidity Value [%RH]: "));
  Serial.println(humid, 2);
  Serial.print(F("Calculated Dew Point [F]: "));
  Serial.println(tdew,2);
  Serial.println(F(" "));
}

void getUserInput(){
  String content = "";
  char character;

  while(Serial.available()) {
      character = Serial.read();
      content.concat(character);
  }

  if(content != "") {
    if(content == "clear EEPROM Entries"){
      Serial.println("Clearing the EEPROM Entries...");
      for(int i=0; i<10; i++){
        EEPROM.update(i, 0);
      }
      for(int i=0; i<producer_list.registered_count; i++){
        for(int j=0; j<5; j++){
          producer_list.address[i][j] = 0;
          producer_list.serial_number[i][j] = 0;
        }
        producer_list.sequential_failed_samples[i] = 0;
      }
      producer_list.registered_count = 0;
    }
    else if(content == "print Registered Producers"){
      Serial.println("Registered Producer Struct:");
      Serial.println(" Registered Addresses");
      for(int i=0; i<3; i++){
        Serial.print("  0x");
        for(int j=0; j<5; j++){
          Serial.print(producer_list.address[i][j], HEX);
        }
        Serial.println(" ");
      }
      Serial.println(" Registered Serial Numbers");
      for(int i=0; i<3; i++){
        Serial.print("  0x");
        for(int j=0; j<5; j++){
          Serial.print(producer_list.serial_number[i][j], HEX);
        }
        Serial.println(" ");
      }
      Serial.println(" Sequential Failed Sample Count");
      for(int i=0; i<3; i++){
        Serial.print("  ");
        Serial.println(producer_list.sequential_failed_samples[i], DEC);
      }
      Serial.println(" Registered Count");
      Serial.print("  ");
      Serial.println(producer_list.registered_count, DEC);
    }
    else if(content == "print EEPROM Entries"){
      Serial.println("EEPROM Values");
      for(int i=0; i<10; i++){
        Serial.print(i, DEC);
        Serial.print(" --- 0x");
        Serial.println(EEPROM.read(i), HEX);
      }
    }
    else{
      // Default sets the current time
      Serial.print("Setting the time to: ");
      Serial.println(content);
    }
  }
}

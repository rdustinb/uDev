// Libraries Needed
#include <SPI.h>
#include <Wire.h>

// Used for debugging
//#define STATEDECODE
//#define FUNCALLS
//#define DATAFLOW

// Arduino Pro Mini Pins
#define NRFCSn      10
#define NRFIRQn     3
#define NRFCE       4

// Handshaking Time Values
#define TXFAILUREDELAY 100
#define RSPFAILUREDELAY 500
#define FAILUREDELAY 5000
#define DELAYTXMILLIS 20
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
enum States {IDLE,RXTOTX_DELAYRX,RXTOTX_RXCOMMAND,RXTOTX_DELAYTOTX,
  RXTOTX_TXRESPONSE,RXTOTX_DELAYSTATUS,RXTOTX_DECODESTATUS,RXTOTX_TXFAILURE,
  TXTORX_DELAYTOTX,TXTORX_TXCOMMAND,TXTORX_DELAYSTATUS,TXTORX_DECODESTATUS,
  TXTORX_SETUPRX,TXTORX_DELAYRX,TXTORX_RXRESPONSE,TXTORX_TXFAILURE,DEAD} state;

// Globals
byte defaultAddress[5] = {0x01,0x23,0x45,0x67,0x89};
byte* p_defaultAddress = defaultAddress;
volatile unsigned long enterTxMillis;
volatile uint8_t addressSet = 0;
volatile uint8_t nrfResults = 0;

/*********************************/
/******** nRF Result ISR *********/
/*********************************/
void nrfISR(){
  cli();
  #ifdef STATEDECODE
  Serial.println(F("irq!"));
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
  // Setup the nRF Pins
  pinMode(NRFCSn, OUTPUT);
  pinMode(NRFIRQn, INPUT_PULLUP);
  pinMode(NRFCE, OUTPUT);
  // Begin SPI for the nRF Device
  SPI.begin();
  // Setup the nRF Device Configuration
  nrfSetup();
  // Setup the Interrupt Pin, parameterized
  attachInterrupt(digitalPinToInterrupt(NRFIRQn), nrfISR, FALLING);

  Serial.println(F("Core Setup Complete!"));
}

/*********************************/
/*********** Main Loop ***********/
/*********************************/
void loop(){
  // Loop Variables
  int status;

  // Loop Decode
  switch(state){
    case IDLE:
      if(addressSet == 0){
        enterTxMillis = millis();
        #ifdef STATEDECODE
        Serial.println(F("IDLE->TXTORX_DELAYTOTX"));
        #endif
        state = TXTORX_DELAYTOTX;
      }else{
        power_up_rx();
        #ifdef STATEDECODE
        Serial.println(F("IDLE->RXTOTX_DELAYRX"));
        #endif
        state = RXTOTX_DELAYRX;
      }
      break;
    /******************************************
          Receive First then Transmit Flow
    ******************************************/
    case RXTOTX_DELAYRX:
      if(nrfResults == 1){
        nrfResults = 0;
        #ifdef STATEDECODE
        Serial.println(F("RXTOTX_DELAYRX->RXTOTX_RXCOMMAND"));
        #endif
        state = RXTOTX_RXCOMMAND;
      }
      break;
    case RXTOTX_RXCOMMAND:
      // Check the status
      status = decode_status();
      if(status == 1){
        // Sample Sensor Data
        decode_rx_fifo();
      }else{
        #ifdef STATEDECODE
        Serial.println(F("nRf comm failure, no Rx response received.")); 
        #endif
      }
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
      // Send Sensor Data
      send_sensor_data();
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
      if(status == -1){
        #ifdef STATEDECODE
        Serial.println(F("Tx failed with nRf Max Retries, failing.")); 
        Serial.println(F("RXTOTX_DECODESTATUS->RXTOTX_TXFAILURE")); 
        #endif
        enterTxMillis = millis();
        state = RXTOTX_TXFAILURE;
      }else if(status == 0){
        #ifdef STATEDECODE
        Serial.println(F("RXTOTX_DECODESTATUS->IDLE")); 
        #endif
        state = IDLE;
      }
      // Clear FIFOs
      clear_fifos();
      // Power Down
      power_down();
      break;
    case RXTOTX_TXFAILURE:
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
      // Power Up in Tx
      power_up_tx();
      // Clear FIFOs
      clear_fifos();
      // Request Address
      request_address();
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
        // Power Down
        power_down();
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
        // Set the new address
        decode_rx_fifo();
        // Flag that our address is set
        addressSet = 1;
      }else{
        #ifdef STATEDECODE
        Serial.println(F("nRf comm failure, no Rx response received.")); 
        #endif
      }
      // Power Down, then power back up in Rx
      power_down();
      power_up_rx();
      // Check my new addresses
      get_addr_pipe();
      #ifdef STATEDECODE
      Serial.println(F("TXTORX_RXRESPONSE->IDLE")); 
      #endif
      state = IDLE;
      //state = DEAD;
      break;
    case TXTORX_TXFAILURE:
      // Try again in 5s
      if(delay_after_failure(enterTxMillis)){
        state = IDLE;
      }
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
  // Change to 1Mbps
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(RF_SETUP+W_REGISTER);
  SPI.transfer(0x7); // 0dB, LNA Gain, 1Mbps
  digitalWrite(NRFCSn, HIGH);
  // Change to 4000us Retry Delay
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(SETUP_RETR+W_REGISTER);
  SPI.transfer(0xF3);
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
int sample_sensor(byte select){
  #ifdef FUNCALLS
  Serial.println(F("sample_sensor()")); 
  #endif
  int data = 0;
  uint8_t checksum = 0;
  int address = 0;

  // Decode which data to pull
  if(select == 0){
    // Temperature
    address = 0xE3;
  }else{
    // Humidity
    address = 0xE5;
  }

  // Get Sensor Data based on register request selection above
  Wire.beginTransmission(0x40);
  Wire.write(address);
  Wire.endTransmission(0);
  Wire.requestFrom(0x40, 3);
  while(3 > Wire.available());
  for(int i=0; i<3; i++){
    if(i != 2){
      // This acts like an acumulator + byte shifter
      data = Wire.read() + data*256;
    }else{
      // At some point use this checksum?
      checksum = (uint8_t)(Wire.read());
    }
  }

  // Cleanup Data
  if(select == 0){
    // Temperature is 14-bits
    data = data & 0xFFFC;
  }else{
    // Humidity is 12-bits
    data = data & 0xFFF0;
  }

  // Return the Data
  return data;
}

void request_address(){
  #ifdef FUNCALLS
  Serial.println(F("request_address()")); 
  #endif
  // Request Address Registration
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(W_TX_PAYLOAD);
  SPI.transfer(REQADDR);
  digitalWrite(NRFCSn, HIGH);
  // Set CE Hi (>10us) to Enable Tx
  digitalWrite(NRFCE, HIGH);
  delay(10);
  digitalWrite(NRFCE, LOW);
}

void send_sensor_data(){
  #ifdef FUNCALLS
  Serial.println(F("send_sensor_data()")); 
  #endif
  int temperature = sample_sensor(0);
  int humidity = sample_sensor(1);
  // Publish Sensor Data
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(W_TX_PAYLOAD);
  SPI.transfer(REQSENSOR);
  SPI.transfer((uint8_t)(temperature&0x00FF));
  SPI.transfer((uint8_t)((temperature&0xFF00)>>8));
  SPI.transfer((uint8_t)(humidity&0x00FF));
  SPI.transfer((uint8_t)((humidity&0xFF00)>>8));
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
  }
}

void set_addr_pipe(byte* address, byte length){
  #ifdef FUNCALLS
  Serial.println(F("set_addr_pipe()")); 
  #endif
  // Power Down, Else the Address will not be applied
  power_down();
  #ifdef DATAFLOW
  Serial.print(F("Length is: "));
  Serial.println(length, DEC);
  Serial.print(F("Setting address to: 0x"));
  for(int i=0; i<5; i++){
    Serial.print(address[i], HEX);
  }
  Serial.println(F(" "));
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
  uint8_t current_tx_address[5];
  uint8_t current_rx_address[5];
  // Set Tx Address
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(TX_ADDR);
  for(int i=0; i<5; i++){
    current_tx_address[i] = SPI.transfer(NOP);
  }
  digitalWrite(NRFCSn, HIGH);
  #ifdef DATAFLOW
  Serial.print(F("Current Tx Address is: 0x"));
  for(int i=0; i<5; i++){
    Serial.print(current_tx_address[i], HEX);
  }
  Serial.println(F(" "));
  #endif
  // Set Rx0 Address
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(RX_ADDR_P0);
  for(int i=0; i<5; i++){
    current_rx_address[i] = SPI.transfer(NOP);
  }
  digitalWrite(NRFCSn, HIGH);
  #ifdef DATAFLOW
  Serial.print(F("Current Rx0 Address is: 0x"));
  for(int i=0; i<5; i++){
    Serial.print(current_rx_address[i], HEX);
  }
  Serial.println(F(" "));
  #endif
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
  byte* p_payloadBytes = payloadBytes;
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
      set_addr_pipe(p_payloadBytes,5);
      break;
    case REQSENSOR:
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
  Serial.print(F("Total Comms: "));
  Serial.println(total, DEC);
  Serial.print(F("Failed Comms: "));
  Serial.println(failed, DEC);
}

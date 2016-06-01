// Used for debugging
#define DATAFLOW
#define STATEDECODE
#define PRODUCERDATA

// Needed for SPI Communications to nRF
#include <SPI.h>

#define NRFCSn      10
#define NRFIRQn     3
#define NRFCE       4
#define TXFAILUREDELAY 100
#define DELAYTXMILLIS 20

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

// Producer Globals
#include <Wire.h>
word humidity;
word temperature;

int newAddress[5] = {0x01,0x23,0x45,0x67,0x89};
enum producerStates {REQDELAY,REQADDR,REQWAIT,REQDECODE,REQFAILURE,SETUPREQRX,
  WAITFORREQRX,REQRX,SETADDRESS,IDLE,WAITFORRX,RXCOMMAND,SAMPLEDATA,CDSETUP,
  CARRIERCHECK,DELAYTOTX,TXDATA,WAITFORSTATUS,DECODESTATUS,CARRIERFAILURE,
  RETRYFAILURE} state;
unsigned long enterTxMillis;
uint8_t resultStatus = 0x20;
uint8_t rand_delay;
uint8_t register_address = 1;

/*********************************/
/******** nRF Result ISR *********/
/*********************************/
volatile unsigned int nrfResults = 0;
void nrfISR(){
  cli();
  #ifdef STATEDECODE
  Serial.println(F("irq!"));
  #endif
  nrfResults = 1;
  sei();
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
  // Change to 750us Retry Delay
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(SETUP_RETR+W_REGISTER);
  SPI.transfer(0x83);
  digitalWrite(NRFCSn, HIGH);
  // Change to Channel 76
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(RF_CH+W_REGISTER);
  SPI.transfer(0x4c);
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
  // Set Tx Address
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(TX_ADDR+W_REGISTER); // Write Reg (001x_xxxx)
  for(int i=0; i<5; i++){
    SPI.transfer(newAddress[i]);
  }
  digitalWrite(NRFCSn, HIGH);
  // Set Rx0 Address
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(RX_ADDR_P0+W_REGISTER); // Write Reg (001x_xxxx)
  for(int i=0; i<5; i++){
    SPI.transfer(newAddress[i]);
  }
  digitalWrite(NRFCSn, HIGH);
  // Clear STATUS Flags
  clear_flags();
  // Clear FIFOs
  clear_fifos();

  Serial.println(F("nRF Setup Complete!"));
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
  // Get the current milliseconds since boot
  enterTxMillis = millis();
  // Generate a random count, between 25 and 125
  rand_delay = rand() % 100 + 25;
  // Branch on stored address
  if(register_address == 1){
    state = IDLE;
  }else{
    state = REQDELAY;
  }

  Serial.println(F("Core Setup Complete!"));
}

/*********************************/
/*********** Main Loop ***********/
/*********************************/
void loop(){
  // Loop Variables
  uint8_t rdCnt = 0;
  uint8_t payloadByteCountTop = 0;

  // Loop Decode
  switch(state){
    case REQDELAY:
      if((uint8_t)(millis() - enterTxMillis) >= rand_delay){
        // Wait a random amount of time, prevents collisions if other producers 
        // are attempting to request from the consumer simultaneously
        #ifdef STATEDECODE
        Serial.println(F("REQDELAY->REQADDR"));
        #endif
        state = REQADDR;
      }
      break;
    case REQADDR:
      // Power Up in Tx
      power_up_tx();
      // Wait for 1.5ms after Power Up Command
      delay(2);
      // Clear FIFOs
      clear_fifos();
      // Write Data to the Payload FIFO
      digitalWrite(NRFCSn, LOW);
      SPI.transfer(W_TX_PAYLOAD); // Write Reg (001x_xxxx)
      SPI.transfer(TX_ADDR); // The Address request uses the TX_ADDR API Call
      digitalWrite(NRFCSn, HIGH);
      // Set CE Hi (>10us) to Enable Tx
      digitalWrite(NRFCE, HIGH);
      delay(10);
      digitalWrite(NRFCE, LOW);
      // Send the same command set as what the nRf uses, meaning TX_ADDR
      #ifdef STATEDECODE
      Serial.println(F("REQADDR->REQWAIT"));
      #endif
      state = REQWAIT;
      break;
    case REQWAIT:
      if(nrfResults == 1){
        nrfResults = 0;
        // Check Status Register for Results
        resultStatus = get_status();
        // Wait for the nRf to interrupt
        #ifdef STATEDECODE
        Serial.println(F("REQWAIT->REQDECODE"));
        #endif
        state = REQDECODE;
      }
      break;
    case REQDECODE:
      // Clear STATUS Flags
      clear_flags();
      // Clear FIFOs
      clear_fifos();
      // Decode status
      if((resultStatus & 0x20) == 0x20){
        #ifdef STATEDECODE
        Serial.println(F("REQDECODE->SETUPREQRX"));
        #endif
        state = SETUPREQRX;
      }else{
        // Power Down
        power_down();
        #ifdef STATEDECODE
        Serial.println(F("REQDECODE->REQFAILURE"));
        #endif
        state = REQFAILURE;
      }
      break;
    case REQFAILURE:
      // Get the current milliseconds since boot
      enterTxMillis = millis();
      // Generate a random count, between 25 and 125
      rand_delay = rand() % 100 + 25;
      #ifdef STATEDECODE
      Serial.println(F("REQFAILURE->REQDELAY"));
      #endif
      state = REQDELAY;
      break;
    case SETUPREQRX:
      digitalWrite(NRFCE, LOW);
      // Power Rx
      power_up_rx();
      // Wait for 1.5ms after Power Up Command
      delay(2);
      // Setup Rx, CE set HI until data received
      digitalWrite(NRFCE, HIGH);
      #ifdef STATEDECODE
      Serial.println(F("SETUPREQRX->WAITFORREQRX"));
      #endif
      state = WAITFORREQRX;
      break;
    case WAITFORREQRX:
      if(nrfResults == 1){
        nrfResults = 0;
        #ifdef STATEDECODE
        Serial.println(F("WAITFORREQRX->REQRX"));
        #endif
        state = REQRX;
      }
      break;
    case REQRX:
      // Clear STATUS Flags
      clear_flags();
      // Disable Rx
      digitalWrite(NRFCE, LOW);
      // Get number of bytes received
      payloadByteCountTop = get_top_rx_fifo_count();
      #ifdef DATAFLOW
      Serial.print(F("Bytes reported in top FIFO entry is: "));
      Serial.println(payloadByteCountTop, DEC);
      #endif
      // Temp Storage
      uint8_t payloadBytes[payloadByteCountTop];
      // Read out Received Bytes
      digitalWrite(NRFCSn, LOW);
      SPI.transfer(R_RX_PAYLOAD);
      for(int i=0; i<payloadByteCountTop; i++){
        newAddress[i] = SPI.transfer(NOP);
      }
      digitalWrite(NRFCSn, HIGH);
      // nRf must be powered off to set address
      power_down();
      // Delay a bit
      delay(5);
      #ifdef STATEDECODE
      Serial.println(F("REQRX->SETADDRESS"));
      #endif
      state = SETADDRESS;
      break;
    case SETADDRESS:
      // Set Tx Address
      digitalWrite(NRFCSn, LOW);
      SPI.transfer(TX_ADDR+W_REGISTER); // Write Reg (001x_xxxx)
      for(int i=0; i<5; i++){
        SPI.transfer(newAddress[i]);
      }
      digitalWrite(NRFCSn, HIGH);
      // Set Rx0 Address
      digitalWrite(NRFCSn, LOW);
      SPI.transfer(RX_ADDR_P0+W_REGISTER); // Write Reg (001x_xxxx)
      for(int i=0; i<5; i++){
        SPI.transfer(newAddress[i]);
      }
      digitalWrite(NRFCSn, HIGH);
      // Clear STATUS Flags
      clear_flags();
      // Clear FIFOs
      clear_fifos();
      #ifdef STATEDECODE
      Serial.println(F("SETADDRESS->IDLE"));
      #endif
      register_address = 1;
      state = IDLE;
      break;
    case IDLE:
      digitalWrite(NRFCE, LOW);
      // Power Up in Rx Mode
      power_up_rx();
      // Wait for 1.5ms after Power Up Command
      delay(2);
      // Setup Rx, CE set HI until data received
      digitalWrite(NRFCE, HIGH);
      #ifdef STATEDECODE
      Serial.println(F("IDLE->WAITFORRX"));
      #endif
      state = WAITFORRX;
      break;
    case WAITFORRX:
      if(nrfResults == 1){
        nrfResults = 0;
        #ifdef STATEDECODE
        Serial.println(F("WAITFORRX->RXCOMMAND"));
        #endif
        state = RXCOMMAND;
      }
      break;
    case RXCOMMAND:
      // Get number of bytes received
      payloadByteCountTop = get_top_rx_fifo_count();
      #ifdef DATAFLOW
      Serial.print(F("Bytes in top Rx FIFO: "));
      Serial.println(payloadByteCountTop, DEC);
      #endif
      // Temporary Storage
      uint8_t payloadBytes0[payloadByteCountTop];
      // Read out Received Bytes
      digitalWrite(NRFCSn, LOW);
      SPI.transfer(R_RX_PAYLOAD);
      for(unsigned int i=0; i<payloadByteCountTop; i++){
        payloadBytes0[i] = SPI.transfer(NOP);
      }
      digitalWrite(NRFCSn, HIGH);
      /***********************/
      //  Authenticate Here
      /***********************/
      // Disable Rx
      digitalWrite(NRFCE, LOW);
      // Clear STATUS Flags
      clear_flags();
      // Clear FIFOs
      clear_fifos();
      // Clear the Retry Counter for Later
      //retryCount = 0;
      #ifdef STATEDECODE
      Serial.println(F("RXCOMMAND->SAMPLEDATA"));
      #endif
      state = SAMPLEDATA;
      break;
    case SAMPLEDATA:
      // Get Humidity Data
      Wire.beginTransmission(0x40);
      Wire.write(0xE5);
      Wire.endTransmission(0);
      Wire.requestFrom(0x40, 3);
      while(3 > Wire.available());
      while(Wire.available()){
        if(1 == rdCnt){
          humidity = (word)(humidity * 256) + (word)(Wire.read());
        }else if(0 == rdCnt){
          humidity = (word)(Wire.read());
        }else{
          int checksum = (int)(Wire.read());
        }
        rdCnt++;
      }
      rdCnt = 0;
      // Final Cleanup
      if((temperature & 0x3) == 0x2){
        // Upper 12-bits is the actual humidity
        humidity = (word)(humidity & 0xFFF0);
      }else{
        #ifdef DATAFLOW
        Serial.println(F("Invalid humidity sample."));
        #endif
      }

      // Get Temperature Data
      Wire.beginTransmission(0x40);
      Wire.write(0xE3);
      Wire.endTransmission(0);
      Wire.requestFrom(0x40, 3);
      while(3 > Wire.available());
      while(Wire.available()){
        if(1 == rdCnt){
          temperature = (word)(temperature * 256) + (word)(Wire.read());
        }else if(0 == rdCnt){
          temperature = (word)(Wire.read());
        }else{
          int checksum = (int)(Wire.read());
        }
        rdCnt++;
      }
      // Final Cleanup
      if((temperature & 0x3) == 0x0){
        // Upper 12-bits is the actual temperature
        temperature = (word)(temperature & 0xFFFC);
      }else{
        #ifdef DATAFLOW
        Serial.println(F("Invalid temperature sample."));
        #endif
      }
      rdCnt = 0;
      #ifdef STATEDECODE
      Serial.println(F("SAMPLEDATA->DELAYTOTX"));
      #endif
      state = DELAYTOTX;
      break;
    case CDSETUP:
      if(nrfResults == 1){
        nrfResults = 0;
      }
      // Power up in Rx Mode for Carrier Detection
      power_up_rx();
      // Wait for 1.5ms after Power Up Command
      delay(2);
      // Snapshot the current Millis count for later timeout
      enterTxMillis = millis();
      #ifdef STATEDECODE
      Serial.println(F("CDSETUP->CARRIERCHECK"));
      #endif
      state = CARRIERCHECK;
      break;
    case CARRIERCHECK:
      if(nrfResults == 1){
        nrfResults = 0;
      }
      // Grab Carrier Detection flag
      if(get_carrier_detect_flag()){
        #ifdef DATAFLOW
        Serial.println(F("CARRIERCHECK waiting"));
        #endif
        if((uint8_t)(millis() - enterTxMillis) >= (byte)(TXFAILUREDELAY)){
          // Failed CD, power down and go to fail state
          power_down();
          // Wait for 1.5ms after Power Down Command
          delay(2);
          #ifdef STATEDECODE
          Serial.println(F("CARRIERCHECK->CARRIERFAILURE"));
          #endif
          state = CARRIERFAILURE;
        }
      }else{
        enterTxMillis = millis();
        #ifdef STATEDECODE
        Serial.println(F("CARRIERCHECK->DELAYTOTX"));
        #endif
        state = DELAYTOTX;
      }
      break;
    case DELAYTOTX:
      // Due to Async timing, delay a bit to make sure the other device is in
      // Rx Mode before trying to send
      if((uint8_t)(millis() - enterTxMillis) >= (uint8_t)(DELAYTXMILLIS)){
        enterTxMillis = millis();
        #ifdef STATEDECODE
        Serial.println(F("DELAYTOTX->TXDATA"));
        #endif
        state = TXDATA;
      }
      break;
    case TXDATA:
      if(nrfResults == 1){
        nrfResults = 0;
      }
      // Setup the Device in Tx Mode
      power_up_tx();
      // Wait for 1.5ms after Power Up Command
      delay(2);
      // Clear FIFOs
      clear_fifos();
      #ifdef PRODUCERDATA
      // Print out sampled data
      Serial.print(F("[%RH]: "));
      Serial.println(((float)(-6.0) + (float)(125.0) * ((float)(humidity) / (float)(65536))), 2);
      Serial.print(F("[C]: "));
      Serial.println(((float)(-46.85) + (float)(175.72) * ((float)(temperature) / (float)(65536))), 2);
      #endif
      // Write Data to the Payload FIFO
      digitalWrite(NRFCSn, LOW);
      SPI.transfer(W_TX_PAYLOAD); // Write Reg (001x_xxxx)
      // The HTU21D has 12-bits for Humidity (word type) and 14-bits for the
      // temperature (word type)
      SPI.transfer((char)(humidity&0xFF));
      SPI.transfer((char)((humidity&0xFF00)>>8));
      SPI.transfer((char)(temperature&0xFF));
      SPI.transfer((char)((temperature&0xFF00)>>8));
      digitalWrite(NRFCSn, HIGH);
      // Set CE Hi (>10us) to Enable Tx
      digitalWrite(NRFCE, HIGH);
      delay(1);
      digitalWrite(NRFCE, LOW);
      #ifdef STATEDECODE
      Serial.println(F("TXDATA->WAITFORSTATUS"));
      #endif
      state = WAITFORSTATUS;
      break;
    case WAITFORSTATUS:
      if(nrfResults == 1){
        nrfResults = 0;
        // Check Status Register for Results
        resultStatus = get_status();
        // Clear flags
        clear_flags();
        #ifdef STATEDECODE
        Serial.println(F("WAITFORSTATUS->DECODESTATUS"));
        #endif
        state = DECODESTATUS;
      }
      break;
    case DECODESTATUS:
      // Clear FIFOs
      clear_fifos();
      // Next sample the sensor data
      state = SAMPLEDATA;
      // Tx Failed
      if((resultStatus & 0x10) == 0x10){
          #ifdef STATEDECODE
          Serial.println(F("DECODESTATUS->RETRYFAILURE"));
          #endif
          state = RETRYFAILURE;
      }
      // Tx Succeeded
      if((resultStatus & 0x20) == 0x20){
        #ifdef STATEDECODE
        Serial.println(F("DECODESTATUS->IDLE"));
        Serial.println(F(" "));
        #endif
        state = IDLE;
      }
      break;
    case CARRIERFAILURE:
      // Dead State, the Carrier detection state failed hard, never
      // allowing this device to Tx sensor data
      #ifdef STATEDECODE
      Serial.println(F("CARRIERFAILURE->IDLE"));
      Serial.println(F(" "));
      #endif
      state = IDLE;
      break;
    case RETRYFAILURE:
      // Dead State, the Retry of Sensor Data attempts exceeded the allowed
      // number of retries
      #ifdef STATEDECODE
      Serial.println(F("RETRYFAILURE->IDLE"));
      Serial.println(F(" "));
      #endif
      state = IDLE;
      break;
    default:
      nrfResults = 0;
      enterTxMillis = millis();
      power_down();
      state = IDLE;
      break;
  }
}

/*********************************/
/********* nRf Functions *********/
/*********************************/
void clear_fifos(){
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
  // Clear STATUS Flags
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(STATUS+W_REGISTER);
  SPI.transfer(0x70);
  digitalWrite(NRFCSn, HIGH);
}

void power_down(){
  // Disable Chip Enable (Kills Rx Mode, mostly)
  digitalWrite(NRFCE, LOW);
  delay(5);
  // Power Down
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(CONFIG+W_REGISTER);  // Write Reg (001x_xxxx)
  SPI.transfer(0x00);               // Power Down
  digitalWrite(NRFCSn, HIGH);
}

void power_up_rx(){
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(CONFIG+W_REGISTER);  // Write Reg (001x_xxxx)
  SPI.transfer(0x0B);               // CRC Enabled, Powerup Enabled, PRIM_RX Hi
  digitalWrite(NRFCSn, HIGH);
}

void power_up_tx(){
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(CONFIG+W_REGISTER);  // Write Reg (001x_xxxx)
  SPI.transfer(0x0A);               // CRC Enabled, Powerup Enabled, PRIM_RX Lo
  digitalWrite(NRFCSn, HIGH);
}

char get_status(){
  char temp;
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(STATUS);
  temp = SPI.transfer(NOP);
  digitalWrite(NRFCSn, HIGH);
  return temp;
}

char get_top_rx_fifo_count(){
  char temp;
  digitalWrite(NRFCSn, LOW);
  SPI.transfer(R_RX_PL_WID);
  temp = SPI.transfer(NOP);
  digitalWrite(NRFCSn, HIGH);
  return temp;
}

bool get_carrier_detect_flag(){
  char temp;
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

// We're using the higher-precision HTU21D
#define HTU21D

// Used for debugging
//#define STATEDECODE
//#define DATAFLOW
#define CONSUMERDATA

// Needed for SPI Communications to nRF
#include <SPI.h>

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

// Consumer Globals
#include <avr/io.h>
#include <avr/interrupt.h>

// Teensy 3.1, Polling All Other Devices
#define NRFCSn      10
#define NRFIRQn     6
#define NRFCE       4
int newAddress[5] = {0x01,0x23,0x45,0x67,0x89};
enum consumerStates {IDLE,CDSETUP,CARRIERCHECK,DELAYTOTX,TXCOMMAND,WAITFORSTATUS,
  DECODESTATUS,SETUPRX,WAITFORRX,RXDATA,CARRIERFAILURE,RETRYFAILURE};
consumerStates state = IDLE;
volatile unsigned long enterTxMillis;
volatile unsigned int retryCount;
volatile char resultStatus = 0x20;
#define TXFAILUREDELAY 100
#define DELAYTXMILLIS 20

volatile char timeChar = 0;
volatile unsigned long currentTime = 0;
volatile unsigned long currentBootTime = 0;

volatile unsigned long totalHandshakes;
volatile unsigned long failedHandshakes;

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

/*********************************/
/********* Req Timer ISR *********/
/*********************************/
#include "Timer.h"
// Request Data Time Delay in miliseconds
#define REQTIMERMILIS 60000
Timer reqTimer;
volatile unsigned int timerExpired = 0;
void timerISR(){
  timerExpired = 1;
}

/*********************************/
/******** nRF Result ISR *********/
/*********************************/
volatile unsigned int nrfResults = 0;
void nrfISR(){
  cli();
  #ifdef STATEDECODE
  Serial.println("nrfISR: interrupt seen!");
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
  // Set the Rx Pipe to 32 Bytes (even though Dynamic is active)
  //digitalWrite(NRFCSn, LOW);
  //SPI.transfer(RX_PW_P0+W_REGISTER);
  //SPI.transfer(0x2);
  //digitalWrite(NRFCSn, HIGH);
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
  // Activate Extended Access
  //digitalWrite(NRFCSn, LOW);
  //SPI.transfer(ACTIVATE);
  //SPI.transfer(0x73); // Magic Value for Deactivate
  //digitalWrite(NRFCSn, HIGH);
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

  Serial.println("nRF Setup Complete!");
}

/*********************************/
/********** Main Setup ***********/
/*********************************/
void setup(){
  // Setup Serial Monitor
  Serial.begin(115200);
  totalHandshakes = 0;
  failedHandshakes = 0;
  // Setup the nRF Pins
  pinMode(NRFCSn, OUTPUT);
  pinMode(NRFIRQn, INPUT_PULLUP);
  pinMode(NRFCE, OUTPUT);
  // Begin SPI for the nRF Device
  SPI.begin();
  // Setup the nRF Device Configuration
  nrfSetup();
  // Setup the request timer
  reqTimer.every(REQTIMERMILIS, timerISR);
  // Setup the Interrupt Pin, parameterized
  attachInterrupt(NRFIRQn, nrfISR, FALLING);

  Serial.println("Core Setup Complete!");
}

/*********************************/
/*********** Main Loop ***********/
/*********************************/
void loop(){
  // Loop Variables
  char payloadByteCountTop = 0;
  word humidity = 0;
  word temperature = 0;

  // This "updates" the timer, kind of lame but apparently it works
  // may need to factor in variance when other code is running.
  reqTimer.update();

  // Service User Input
  getUserInput();

  switch(state){
    case IDLE:
      if(timerExpired == 1){
        retryCount = 0;
        timerExpired = 0;
        #ifdef STATEDECODE
        Serial.println("Consumer Loop >> IDLE going to DELAYTOTX");
        #endif
        state = DELAYTOTX;
      }
      break;
    case CDSETUP:
      if(nrfResults == 1){
        nrfResults = 0;
      }
      // Power Up in Tx
      power_up_rx();
      // Wait for 1.5ms after Power Up Command
      delay(10);
      // Snapshot the current Millis count for later timeout
      enterTxMillis = millis();
      #ifdef STATEDECODE
      Serial.println("Consumer Loop >> CDSETUP going to CARRIERCHECK");
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
        Serial.println("Consumer Loop >> CARRIERCHECK carrier detected, waiting");
        #endif
        if((millis() - enterTxMillis) >= TXFAILUREDELAY){
          // Power Down
          power_down();
          // Wait for 1.5ms after Power Down Command
          delay(2);
          #ifdef STATEDECODE
          Serial.println("Consumer Loop >> CARRIERCHECK going to CARRIERFAILURE");
          #endif
          state = CARRIERFAILURE;
        }
      }else{
        enterTxMillis = millis();
        #ifdef STATEDECODE
        Serial.println("Consumer Loop >> CARRIERCHECK going to DELAYTOTX");
        #endif
        state = DELAYTOTX;
      }
      break;
    case DELAYTOTX:
      // Due to Async timing, delay a bit to make sure the other device is in
      // Rx Mode before trying to send
      if((millis() - enterTxMillis) >= DELAYTXMILLIS){
        enterTxMillis = millis();
        #ifdef STATEDECODE
        Serial.println("Consumer Loop >> DELAYTOTX going to TXCOMMAND");
        #endif
        state = TXCOMMAND;
      }
      break;
    case TXCOMMAND:
      if(nrfResults == 1){
        nrfResults = 0;
      }
      // Power Up in Tx
      power_up_tx();
      // Wait for 1.5ms after Power Up Command
      delay(2);
      // Clear FIFOs
      clear_fifos();
      // Update Current Timestamp
      currentBootTime = millis();
      // Write Data to the Payload FIFO
      digitalWrite(NRFCSn, LOW);
      SPI.transfer(W_TX_PAYLOAD); // Write Reg (001x_xxxx)
      SPI.transfer(0x10);         // "Command" Word (Request Sample)
      SPI.transfer(((currentTime+currentBootTime) >>  0) & 0xff);
      SPI.transfer(((currentTime+currentBootTime) >>  8) & 0xff);
      SPI.transfer(((currentTime+currentBootTime) >> 16) & 0xff);
      SPI.transfer(((currentTime+currentBootTime) >> 24) & 0xff);
      digitalWrite(NRFCSn, HIGH);
      // Set CE Hi (>10us) to Enable Tx
      digitalWrite(NRFCE, HIGH);
      delay(10);
      digitalWrite(NRFCE, LOW);
      #ifdef STATEDECODE
      Serial.println("Consumer Loop >> TXCOMMAND going to WAITFORSTATUS");
      #endif
      totalHandshakes++;
      state = WAITFORSTATUS;
      break;
    case WAITFORSTATUS:
      if(nrfResults == 1){
        nrfResults = 0;
        // Check Status Register for Results
        resultStatus = get_status();
        // Clear STATUS Flags
        clear_flags();
        #ifdef STATEDECODE
        Serial.println("Consumer Loop >> WAITFORSTATUS going to DECODESTATUS");
        #endif
        state = DECODESTATUS;
      }
      break;
    case DECODESTATUS:
      // Clear FIFOs
      clear_fifos();
      // Tx Failed
      if((resultStatus & 0x10) == 0x10){
        if(retryCount >= 5){
          retryCount = 0;
          #ifdef STATEDECODE
          Serial.println("Consumer Loop >> sensor response hard failure. Stopping..."); 
          #endif
          // Increment Failed Handshake Counter
          failedHandshakes++;
          // Power Down
          power_down();
          state = RETRYFAILURE;
        }else{
          retryCount += 1;
          #ifdef STATEDECODE
          Serial.println("Consumer Loop >> sensor response failed. Retrying...");
          #endif
          // Power Down
          power_down();
          state = DELAYTOTX;
        }
      }
      // Tx of Command Succeeded
      if((resultStatus & 0x20) == 0x20){
        retryCount = 0;
        #ifdef STATEDECODE
        Serial.println("Consumer Loop >> DECODESTATUS going to SETUPRX");
        #endif
        state = SETUPRX;
      }
      // Rx Data Present
      if((resultStatus & 0x40) == 0x40){
        #ifdef DATAFLOW
        Serial.println("Consumer Loop >> Why am I receiving data?!?");
        Serial.println("Consumer Loop >> I'm trying to Tx a command!");
        #endif
      }
      break;
    case SETUPRX:
      digitalWrite(NRFCE, LOW);
      // Power Rx
      power_up_rx();
      // Wait for 1.5ms after Power Up Command
      delay(2);
      // Setup Rx, CE set HI until data received
      digitalWrite(NRFCE, HIGH);
      #ifdef STATEDECODE
      Serial.println("Consumer Loop >> SETUPRX going to WAITFORRX");
      #endif
      state = WAITFORRX;
      break;
    case WAITFORRX:
      if(nrfResults == 1){
        nrfResults = 0;
        #ifdef STATEDECODE
        Serial.println("Consumer Loop >> WAITFORRX going to RXDATA");
        #endif
        state = RXDATA;
      }
      break;
    case RXDATA:
      // Clear STATUS Flags
      clear_flags();
      // Disable Rx
      digitalWrite(NRFCE, LOW);
      // Get number of bytes received
      payloadByteCountTop = get_top_rx_fifo_count();
      #ifdef DATAFLOW
      Serial.print("Bytes reported in top FIFO entry is: ");
      Serial.println(payloadByteCountTop, DEC);
      #endif
      // Temp Storage
      byte payloadBytes[payloadByteCountTop];
      // Read out Received Bytes
      digitalWrite(NRFCSn, LOW);
      SPI.transfer(R_RX_PAYLOAD);
      for(int i=0; i<payloadByteCountTop; i++){
        payloadBytes[i] = SPI.transfer(NOP);
      }
      digitalWrite(NRFCSn, HIGH);
      // The DHT11 currently returns 1 Byte for the 
      // integral part and 1 Byte for the franctional
      // part of each Humidity and Temp, however the
      // DHT11 doesn't actually return those parts so
      // they are omitted in the DHT11 Library, 
      // returning only a single byte.
      #ifdef CONSUMERDATA
        Serial.println("--- Sensor Data ---");
        Serial.print("Time of last update [HH:MM]: ");
        if(((currentTime+currentBootTime)/1000/60/60)%24 < 10){
          Serial.print("0");
        }
        Serial.print(((currentTime+currentBootTime)/1000/60/60)%24, DEC);
        Serial.print(":");
        if((((currentTime+currentBootTime)/1000/60)%60) < 10){
          Serial.print("0");
        }
        Serial.println((((currentTime+currentBootTime)/1000/60)%60), DEC);
        #ifdef HTU21D
          humidity = payloadBytes[0] + payloadBytes[1]*256;
          temperature = payloadBytes[2] + payloadBytes[3]*256;
          Serial.print("Humidity Value [%RH]: ");
          Serial.println(((float)(-6.0) + (float)(125.0) * ((float)(humidity) / (float)(65536))), 2);
          Serial.print("Internal Temp [F]: ");
          Serial.println((((float)(-46.85) + (float)(175.72) * ((float)(temperature) / (float)(65536)))*9/5 + 32), 2);
        #else
          Serial.print("Humidity (%): ");
          Serial.println(payloadBytes[0]);
          Serial.print("Temperature (F): ");
          Serial.println((payloadBytes[1]*1.8 + 32));
        #endif
        Serial.println("--- Comm Stats ---");
        Serial.print("Total Comms: ");
        Serial.println(totalHandshakes, DEC);
        Serial.print("Failed Comms: ");
        Serial.println(failedHandshakes, DEC);
        Serial.println(" ");
      #endif
      // Clear FIFOs
      clear_fifos();
      // Power down the nRF
      power_down();
      // Clear the Retry Counter for Later
      retryCount = 0;
      #ifdef STATEDECODE
      Serial.println("Consumer Loop >> RXDATA going to IDLE");
      Serial.println(" ");
      #endif
      state = IDLE;
      break;
    case CARRIERFAILURE:
      // Dead State, the Carrier detection state failed hard, never
      // allowing this device to Tx sensor data
      #ifdef STATEDECODE
      Serial.println("Consumer Loop >> recovering from carrier failure...");
      Serial.println(" ");
      #endif
      state = IDLE;
      break;
    case RETRYFAILURE:
      // Dead State, the Retry of Sensor Data attempts exceeded the allowed
      // number of retries
      #ifdef STATEDECODE
      Serial.println("Consumer Loop >> recovering from sensor response failure...");
      Serial.println(" ");
      #endif
      state = IDLE;
      break;
    default:
      timerExpired = 0;
      retryCount = 0;
      nrfResults = 0;
      enterTxMillis = millis();
      power_down();
      state = IDLE;
      break;
  }
}

void getUserInput(){
  // Get User Input
  if(Serial.available() > 0){
    // Decode User Input, Convert to Time
    switch(Serial.read()){
      case 48 :             // "0"
        if(timeChar == 0){
          currentTime = 0*10*60*60*1000;
          timeChar++;
        }else if(timeChar == 1){
          currentTime = currentTime + 0*1*60*60*1000;
          timeChar++;
        }else if(timeChar == 2){
          currentTime = currentTime + 0*10*60*1000;
          timeChar++;
        }else if(timeChar == 3){
          currentTime = currentTime + 0*1*60*1000;
          timeChar = 0;
        }
        break;
      case 49 :             // "1"
        if(timeChar == 0){
          currentTime = 1*10*60*60*1000;
          timeChar++;
        }else if(timeChar == 1){
          currentTime = currentTime + 1*1*60*60*1000;
          timeChar++;
        }else if(timeChar == 2){
          currentTime = currentTime + 1*10*60*1000;
          timeChar++;
        }else if(timeChar == 3){
          currentTime = currentTime + 1*1*60*1000;
          timeChar = 0;
        }
        break;
      case 50 :             // "2"
        if(timeChar == 0){
          currentTime = 2*10*60*60*1000;
          timeChar++;
        }else if(timeChar == 1){
          currentTime = currentTime + 2*1*60*60*1000;
          timeChar++;
        }else if(timeChar == 2){
          currentTime = currentTime + 2*10*60*1000;
          timeChar++;
        }else if(timeChar == 3){
          currentTime = currentTime + 2*1*60*1000;
          timeChar = 0;
        }
        break;
      case 51 :             // "3"
        if(timeChar == 0){
          currentTime = 2*10*60*60*1000;
          timeChar++;
        }else if(timeChar == 1){
          currentTime = currentTime + 3*1*60*60*1000;
          timeChar++;
        }else if(timeChar == 2){
          currentTime = currentTime + 3*10*60*1000;
          timeChar++;
        }else if(timeChar == 3){
          currentTime = currentTime + 3*1*60*1000;
          timeChar = 0;
        }
        break;
      case 52 :             // "4"
        if(timeChar == 0){
          currentTime = 2*10*60*60*1000;
          timeChar++;
        }else if(timeChar == 1){
          currentTime = currentTime + 4*1*60*60*1000;
          timeChar++;
        }else if(timeChar == 2){
          currentTime = currentTime + 4*10*60*1000;
          timeChar++;
        }else if(timeChar == 3){
          currentTime = currentTime + 4*1*60*1000;
          timeChar = 0;
        }
        break;
      case 53 :             // "5"
        if(timeChar == 0){
          currentTime = 2*10*60*60*1000;
          timeChar++;
        }else if(timeChar == 1){
          currentTime = currentTime + 5*1*60*60*1000;
          timeChar++;
        }else if(timeChar == 2){
          currentTime = currentTime + 5*10*60*1000;
          timeChar++;
        }else if(timeChar == 3){
          currentTime = currentTime + 5*1*60*1000;
          timeChar = 0;
        }
        break;
      case 54 :             // "6"
        if(timeChar == 0){
          currentTime = 2*10*60*60*1000;
          timeChar++;
        }else if(timeChar == 1){
          currentTime = currentTime + 6*1*60*60*1000;
          timeChar++;
        }else if(timeChar == 2){
          currentTime = currentTime + 5*10*60*1000;
          timeChar++;
        }else if(timeChar == 3){
          currentTime = currentTime + 6*1*60*1000;
          timeChar = 0;
        }
        break;
      case 55 :             // "7"
        if(timeChar == 0){
          currentTime = 2*10*60*60*1000;
          timeChar++;
        }else if(timeChar == 1){
          currentTime = currentTime + 7*1*60*60*1000;
          timeChar++;
        }else if(timeChar == 2){
          currentTime = currentTime + 5*10*60*1000;
          timeChar++;
        }else if(timeChar == 3){
          currentTime = currentTime + 7*1*60*1000;
          timeChar = 0;
        }
        break;
      case 56 :             // "8"
        if(timeChar == 0){
          currentTime = 2*10*60*60*1000;
          timeChar++;
        }else if(timeChar == 1){
          currentTime = currentTime + 8*1*60*60*1000;
          timeChar++;
        }else if(timeChar == 2){
          currentTime = currentTime + 5*10*60*1000;
          timeChar++;
        }else if(timeChar == 3){
          currentTime = currentTime + 8*1*60*1000;
          timeChar = 0;
        }
        break;
      case 57 :             // "9"
        if(timeChar == 0){
          currentTime = 2*10*60*60*1000;
          timeChar++;
        }else if(timeChar == 1){
          currentTime = currentTime + 9*1*60*60*1000;
          timeChar++;
        }else if(timeChar == 2){
          currentTime = currentTime + 5*10*60*1000;
          timeChar++;
        }else if(timeChar == 3){
          currentTime = currentTime + 9*1*60*1000;
          timeChar = 0;
        }
        break;
      case 67 :             // "C"
        Serial.println("Cleared stats!");
        totalHandshakes = 0;
        failedHandshakes = 0;
        break;
      case 99 :             // "c"
        Serial.println("Cleared stats!");
        totalHandshakes = 0;
        failedHandshakes = 0;
        break;
      case 84 :             // "T"
        Serial.println("Cleared time entry!");
        currentTime = 0;
        timeChar = 0;
        break;
      case 116 :             // "t"
        Serial.println("Cleared time entry!");
        currentTime = 0;
        timeChar = 0;
        break;
    }
  }
}

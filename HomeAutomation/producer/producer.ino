// We're using the higher-precision HTU21D
#define HTU21D

// Used for debugging
#define DATAFLOW
#define STATEDECODE
#define PRODUCERDATA

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

// Producer Globals
#ifdef HTU21D
  #include <Wire.h>
  word humidity;
  word temperature;
#else
  #include <dht11.h>
  #define DHT11PIN    2
  byte humidity;
  byte temperature;
#endif

#define NRFCSn      10
#define NRFIRQn     3
#define NRFCE       4
volatile int newAddress[5] = {0x01,0x23,0x45,0x67,0x89};
#endif
enum producerStates {IDLE,WAITFORRX,RXCOMMAND,SAMPLEDATA,CDSETUP,CARRIERCHECK,
  DELAYTOTX,TXDATA,WAITFORSTATUS,DECODESTATUS,CARRIERFAILURE,RETRYFAILURE};
producerStates state = IDLE;
volatile unsigned long enterTxMillis;
volatile unsigned int retryCount;
volatile char resultStatus = 0x20;
#define TXFAILUREDELAY 100
#define DELAYTXMILLIS 20

unsigned long currentBootTime = 0;

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

  Serial.println("Core Setup Complete!");
}

/*********************************/
/*********** Main Loop ***********/
/*********************************/
void loop(){
  // Loop Variables
  char rdCnt = 0;

  // Loop Decode
  switch(state){
    case IDLE:
      digitalWrite(NRFCE, LOW);
      // Power Up in Rx Mode
      power_up_rx();
      // Wait for 1.5ms after Power Up Command
      delay(2);
      // Setup Rx, CE set HI until data received
      digitalWrite(NRFCE, HIGH);
      #ifdef STATEDECODE
      Serial.println("Producer Loop >> IDLE going to WAITFORRX");
      #endif
      state = WAITFORRX;
      break;
    case WAITFORRX:
      if(nrfResults == 1){
        nrfResults = 0;
        #ifdef STATEDECODE
        Serial.println("Consumer Loop >> WAITFORRX going to RXCOMMAND");
        #endif
        state = RXCOMMAND;
      }
      break;
    case RXCOMMAND:
      // Get number of bytes received
      char payloadByteCountTop = get_top_rx_fifo_count();
      #ifdef DATAFLOW
      Serial.print("Bytes in top Rx FIFO: ");
      Serial.println(payloadByteCountTop, DEC);
      #endif
      // Temporary Storage
      char payloadBytes0[payloadByteCountTop];
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
      retryCount = 0;
      #ifdef STATEDECODE
      Serial.println("Producer Loop >> RXCOMMAND going to SAMPLEDATA");
      #endif
      state = SAMPLEDATA;
      break;
    case SAMPLEDATA:
      #ifdef HTU21D
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
            Serial.println("Warning > Invalid humidity sample.");
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
          Serial.println("Warning > Invalid temperature sample.");
        }
        rdCnt = 0;
      #else
        static Dht11 sensor(DHT11PIN);
        if(nrfResults == 1){
          nrfResults = 0;
        }
        switch (sensor.read()) {
          case Dht11::OK:
            humidity = sensor.getHumidity();
            temperature = sensor.getTemperature();
            break;
          default:
            break;
        }
      #endif
      #ifdef STATEDECODE
      Serial.println("Producer Loop >> SAMPLEDATA going to DELAYTOTX");
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
      Serial.println("Producer Loop >> CDSETUP going to CARRIERCHECK");
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
          // Failed CD, power down and go to fail state
          power_down();
          // Wait for 1.5ms after Power Down Command
          delay(2);
          #ifdef STATEDECODE
          Serial.println("Producer Loop >> CARRIERCHECK going to CARRIERFAILURE");
          #endif
          state = CARRIERFAILURE;
        }
      }else{
        enterTxMillis = millis();
        #ifdef STATEDECODE
        Serial.println("Producer Loop >> CARRIERCHECK going to DELAYTOTX");
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
        Serial.println("Producer Loop >> DELAYTOTX going to TXDATA");
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
        #ifdef HTU21D
          Serial.print("Humidity Value [%RH]: ");
          Serial.println(((float)(-6.0) + (float)(125.0) * ((float)(humidity) / (float)(65536))), 2);
          Serial.print("Internal Temp [C]: ");
          Serial.println(((float)(-46.85) + (float)(175.72) * ((float)(temperature) / (float)(65536))), 2);
        #else
          Serial.print("Humidity (%): ");
          Serial.println(humidity);
          Serial.print("Temperature (F): ");
          Serial.println((temperature*1.8 + 32));
        #endif
      #endif
      // Update Current Timestamp
      currentBootTime = millis();
      // Write Data to the Payload FIFO
      digitalWrite(NRFCSn, LOW);
      SPI.transfer(W_TX_PAYLOAD); // Write Reg (001x_xxxx)
      #ifdef HTU21D
        // The HTU21D has 12-bits for Humidity (word type) and 14-bits for the
        // temperature (word type)
        SPI.transfer((char)(humidity&0xFF));
        SPI.transfer((char)((humidity&0xFF00)>>8));
        SPI.transfer((char)(temperature&0xFF));
        SPI.transfer((char)((temperature&0xFF00)>>8));
      #else
        // The DHT11 only has one byte per value (char type)
        SPI.transfer(humidity);
        SPI.transfer(temperature);
      #endif
      SPI.transfer((currentBootTime >>  0) & 0xff);
      SPI.transfer((currentBootTime >>  8) & 0xff);
      SPI.transfer((currentBootTime >> 16) & 0xff);
      SPI.transfer((currentBootTime >> 24) & 0xff);
      digitalWrite(NRFCSn, HIGH);
      // Set CE Hi (>10us) to Enable Tx
      digitalWrite(NRFCE, HIGH);
      delay(1);
      digitalWrite(NRFCE, LOW);
      #ifdef STATEDECODE
      Serial.println("Producer Loop >> TXDATA going to WAITFORSTATUS");
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
        Serial.println("Producer Loop >> WAITFORSTATUS going to DECODESTATUS");
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
        if(retryCount >= 5){
          retryCount = 0;
          #ifdef STATEDECODE
          Serial.println("Producer Loop >> sensor response hard failure. Stopping..."); 
          #endif
          state = RETRYFAILURE;
        }else{
          retryCount += 1;
          #ifdef STATEDECODE
          Serial.println("Producer Loop >> Tx sensor data failed. Retrying...");
          #endif
          state = CDSETUP;
        }
      }
      // Tx Succeeded
      if((resultStatus & 0x20) == 0x20){
        retryCount = 0;
        #ifdef STATEDECODE
        Serial.println("Producer Loop >> DECODESTATUS going to IDLE");
        Serial.println(" ");
        #endif
        state = IDLE;
      }
      // Rx Data Present
      if((resultStatus & 0x40) == 0x40){
        Serial.println("Producer Loop >> Why am I receiving data?!?");
        Serial.println("Producer Loop >> I'm trying to Tx my sensor data!");
      }
      break;
    case CARRIERFAILURE:
      // Dead State, the Carrier detection state failed hard, never
      // allowing this device to Tx sensor data
      #ifdef STATEDECODE
      Serial.println("Producer Loop >> recovering from carrier failure...");
      Serial.println(" ");
      #endif
      state = IDLE;
      break;
    case RETRYFAILURE:
      // Dead State, the Retry of Sensor Data attempts exceeded the allowed
      // number of retries
      #ifdef STATEDECODE
      Serial.println("Producer Loop >> recovering from sensor response failure...");
      Serial.println(" ");
      #endif
      state = IDLE;
      break;
    default:
      retryCount = 0;
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
  sleep(5);
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

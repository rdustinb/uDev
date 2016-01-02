/*
 This sketch is used to begin __DEBUGging communications with
 the nRf24L01.

 The Pro Mini is using the following pins for comms to the nRF24L01:

 CSn  - 10
 MOSI - 11
 MISO - 12
 SCK  - 13
 CE   - 4
 IRQn - 3

 The Pro Mini is using the following pins for comms with the DHT11 Sensor:
 
 DATA - 2

 The Teensy 3.2 is using the following pins for comms to the nRF24L01:

 CSn  - 10
 MOSI - 11
 MISO - 12
 SCK  - 13
 IRQn - 6
 CE   - 4

 The Teensy 3.2 is using the following pins for comms to the :

 The Teensy 3.2 is using the following pins for comms to the HTU21D:

 SDA  - 18
 SCL  - 19

 */

/***********************************
  Configure this particular Device
***********************************/
#ifndef __DEBUG
  #define __DEBUG
#endif

#ifndef __CONSUMER
  #define __CONSUMER
#endif

/*#ifndef __PRODUCER0*/
  /*#define __PRODUCER0*/
/*#endif*/

//#ifndef __PRODUCER1
//  #define __PRODUCER1
//#endif

//#ifndef __PRODUCER2
//  #define __PRODUCER2
//#endif

// Consumer Address, Teensy Device
#ifdef __CONSUMER
  #include "Timer.h"
  Timer t;
  static long TIMEOUTDELAY = 5000;
  int THISADDR[5] = {0xdb,0x00,0x00,0x00,0x00};
  int THATADDR0[5] = {0xdb,0x10,0x00,0x00,0x00};
  int THATADDR1[5] = {0xdb,0x10,0x00,0x00,0x01};
  int THATADDR2[5] = {0xdb,0x10,0x00,0x00,0x02};
  #ifndef __TEENSY
    #define __TEENSY
  #endif
#endif

// Producer Addresses, Arduino Pro Mini Devices
#ifdef __PRODUCER0
  int THISADDR[5] = {0xdb,0x10,0x00,0x00,0x00};
  volatile int RXCOMMAND = 0;
  #ifndef __ARDUINOPRO
    #define __ARDUINOPRO
  #endif
#endif
#ifdef __PRODUCER1
  char THISADDR[5] = {0xdb,0x10,0x00,0x00,0x01};
  volatile int RXCOMMAND = 0;
  #ifndef __ARDUINOPRO
    #define __ARDUINOPRO
  #endif
#endif
#ifdef __PRODUCER2
  char THISADDR[5] = {0xdb,0x10,0x00,0x00,0x02};
  volatile int RXCOMMAND = 0;
  #ifndef __ARDUINOPRO
    #define __ARDUINOPRO
  #endif
#endif

/***********************************
***********************************/

#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>

// HTU21D Device Parameters
#define TEMPDATALEN         3
#define HTU21D              0x40
#define STARTTEMPSAMPLE     0xE3
#define STARTHUMSAMPLE      0xE5
#define WRITEREG            0xE6
#define READREG             0xE7
#define STARTTEMPSAMPLENOH  0xF3
#define STARTHUMSAMPLENOH   0xF5
#define SOFTRESET           0xFE

// nRF Device Parameters
#define CONFIG        0x00
#define EN_AA         0x01
#define EN_RXADDR     0x02
#define SETUP_AW      0x03
#define SETUP_RETR    0x04
#define RF_CH         0x05
#define RF_SETUP      0x06
#define STATUS        0x07
#define OBSERVE_TX    0x08
#define RPD           0x09
#define RX_ADDR_P0    0x0a
#define RX_ADDR_P1    0x0b
#define RX_ADDR_P2    0x0c
#define RX_ADDR_P3    0x0d
#define RX_ADDR_P4    0x0e
#define RX_ADDR_P5    0x0f
#define TX_ADDR       0x10
#define RX_PW_P0      0x11
#define RX_PW_P1      0x12
#define RX_PW_P2      0x13
#define RX_PW_P3      0x14
#define RX_PW_P4      0x15
#define RX_PW_P5      0x16
#define FIFO_STATUS   0x17
#define DYNPLD        0x1c
/* CONFIG Register Sets of Vals */
#define TXMODE        0x0a
#define RXMODE        0x0b
#define PWRDWN        0x08

/* STATUS Register Interrupts */
#define MAX_RT        0x10
#define TX_DS         0x20
#define RX_DR         0x40
#define RX_P_NO       0x0E

#define NOP           0xFF
#ifdef __TEENSY
  #define NRFCSn      10
  #define NRFIRQn     6
  #define NRFCE       4
  #define HTU21DADDR  0x40
#else
#ifdef __ARDUINOPRO
  #include <dht11.h>
  #define NRFCSn      5
  #define NRFIRQn     3
  #define NRFCE       4
  #define DHT11PIN    2
#endif
#endif

// Stored Address
#define STORESTATUS   0x10
#define ADDRSTORED    0x01
#define STOREADDRESS  0x11

int pipeSel = 0;

/*********************************/
/**** Hum/Temp Sensor, HTU21D ****/
/*********************************/
float read_sensor_value(int select){
  // Capture the Data, branched based on selection
  char sensor_data[3];
  Wire.beginTransmission(HTU21D);
  switch(select){
    case 0 :
      Wire.write(STARTTEMPSAMPLE);
      break;
    case 1 :
      Wire.write(STARTHUMSAMPLE);
      break;
  }
  Wire.endTransmission(0);
  // Request a read
  Wire.requestFrom(HTU21D, 3);
  // Wait for data present
  while(3 > Wire.available());
  // Capture the data
  for(int i=0; i<3; i++){
    if(Wire.available()) sensor_data[i] = Wire.read();
  }

  // Properly Decode Read Data based on configured resolution
  // Modify the passed pointer for data storage
  float value = (float)(sensor_data[0]<<8) + (float)(sensor_data[1]&0xFC);
  // Branch if Temperature (0) or Humidity (1)
  switch(select){
    case 0 :
      value = (value / 65536) * 175.72 - 46.85;
      break;
    case 1 : 
      value = (value / 65536) * 125.0 - 6.0;
      break;
  }

  return value;
}

/*********************************/
/*********** nRF24L01 ************/
/*********************************/
char read_status(){
  // Drive NRFCSn LOW
  digitalWrite(NRFCSn, LOW);
  // Read the Register
  SPI.transfer(STATUS);
  // Get the response
  char result = SPI.transfer(NOP);
  // Deassert NRFCSn
  digitalWrite(NRFCSn, HIGH);

  // Return STATUS
  return result;
}

char read_rfsetup(){
  // Drive NRFCSn LOW
  digitalWrite(NRFCSn, LOW);
  // Read the Register
  SPI.transfer(RF_SETUP);
  // Get the response
  char result = SPI.transfer(NOP);
  // Deassert NRFCSn
  digitalWrite(NRFCSn, HIGH);

  // Return STATUS
  return result;
}

void write_rxAddr(int rxPipe, int newAddr[5]){
  // Decode Rx Pipe Register Address
#ifdef __DEBUG
  Serial.print("Rx Pipe selected to write: ");
  Serial.println(rxPipe, DEC);
#endif
  int address = RX_ADDR_P0;
  int length = 5;
  switch(rxPipe){
    case 0:
      address = RX_ADDR_P0;
      length = 5;
      break;
    case 1:
      address = RX_ADDR_P1;
      length = 5;
      break;
    case 2:
      address = RX_ADDR_P2;
      length = 1;
      break;
    case 3:
      address = RX_ADDR_P3;
      length = 1;
      break;
    case 4:
      address = RX_ADDR_P4;
      length = 1;
      break;
    case 5:
      address = RX_ADDR_P5;
      length = 1;
      break;
  };

  // Read Register has a command of 1a_aaaa
  int command = address + 0x20;
  // Drive NRFCSn LOW
  digitalWrite(NRFCSn, LOW);
  // Write the Register
  SPI.transfer(command);
  for(int i=0; i<length; i++){
    SPI.transfer(newAddr[i]);
  }
  // Deassert NRFCSn
  digitalWrite(NRFCSn, HIGH);
}

void read_rxAddr(int rxPipe){
  // Decode Rx Pipe Register Address
#ifdef __DEBUG
  Serial.print("Rx Pipe selected to read: ");
  Serial.println(rxPipe, DEC);
#endif
  int address = RX_ADDR_P0;
  int length = 5;
  int result[5];
  switch(rxPipe){
    case 0:
      address = RX_ADDR_P0;
      length = 5;
      break;
    case 1:
      address = RX_ADDR_P1;
      length = 5;
      break;
    case 2:
      address = RX_ADDR_P2;
      length = 1;
      break;
    case 3:
      address = RX_ADDR_P3;
      length = 1;
      break;
    case 4:
      address = RX_ADDR_P4;
      length = 1;
      break;
    case 5:
      address = RX_ADDR_P5;
      length = 1;
      break;
  };

  // Drive NRFCSn LOW
  digitalWrite(NRFCSn, LOW);
  // Read the Register
  SPI.transfer(address);
  // Get the response
  for(int i=0; i<length; i++){
    result[i] = SPI.transfer(NOP);
  }
  // Deassert NRFCSn
  digitalWrite(NRFCSn, HIGH);

#ifdef __DEBUG
  // Check that the address has been changed
  Serial.print("Address length in bytes is: ");
  Serial.println(length, DEC);
  Serial.println("Read Rx Pipe address is:");
  Serial.print("0x");
  for(int i=0; i<length; i++){
    if(int(result[i]) < 0x10){
      Serial.print("0");
    }
    Serial.print(result[i], HEX);
  }
  Serial.println(" ");
#endif
}

/*********************************/
/******** Timer Routine **********/
/*********************************/
void sendReq(){
  // Print out the milliseconds, is the timer library fairly accurate?
  #ifdef __DEBUG
    Serial.print("60 second tick: millis()=");
    Serial.println(millis(), DEC);
  #endif
  // The Consumer simply waits until the hardware expires and then sends
  // a request to each enumerated producer device in it's device list.
  // For now when the timer expires the consumer writes and reads back a
  // new self-address and samples its own HTU21D.
  #ifdef __DEBUG
    write_rxAddr(pipeSel, THISADDR);
    read_rxAddr(pipeSel);
    pipeSel++;
    if(pipeSel > 5){
      pipeSel = 0;
    }
  #endif

  float temperature = read_sensor_value(0);
  float humidity = read_sensor_value(1);
  #ifdef __DEBUG
    Serial.print("Sensor temperature is: ");
    Serial.println(temperature);
    Serial.print("Sensor humidity is: ");
    Serial.println(humidity);

    // Read the STATUS Register
    char nrf_data = read_status();
    Serial.print("STATUS register value is: 0x");
    Serial.println(nrf_data, HEX);
    nrf_data = read_rfsetup();
    Serial.print("RF_SETUP register value is: 0x");
    Serial.println(nrf_data, HEX);
    Serial.println(" ");
    Serial.println(" ");
  #endif
}

/*********************************/
/********** ISR Routine **********/
/*********************************/
#ifndef __CONSUMER
void nrfIsr(){
  RXCOMMAND = 1;
}
#endif

/*********************************/
/********** Main Setup ***********/
/*********************************/
void setup() {
  // Setup Serial Monitor
  Serial.begin(115200);
  // Join the I2C Bus as Master
  Wire.begin();
  // Setup the nRF Pins
  pinMode(NRFCSn, OUTPUT);
  //pinMode(NRFIRQn, INPUT);
  pinMode(NRFCE, OUTPUT);
  // Begin SPI for the nRF Device
  SPI.begin();
  #ifdef __CONSUMER
    // Create the timer, calls a function every 60 seconds
    t.every(10000, sendReq);
  #endif
  #ifndef __CONSUMER
    // Attach the interrupt for producers, waits for incoming packet
    attachInterrupt(digitalPinToInterrupt(NRFIRQn), nrfIsr, FALLING);
  #endif
}

/*********************************/
/*********** Main Loop ***********/
/*********************************/
void loop() {
#ifdef __CONSUMER
  t.update();
#endif
#ifndef __CONSUMER
  if(RXCOMMAND == 1){
    // nRF has thrown an interrupt, now process it

    // Read the STATUS Register
    char nrf_data = read_status();
    Serial.print("STATUS register value is: 0x");
    Serial.println(nrf_data, HEX);
    nrf_data = read_rfsetup();
    Serial.print("RF_SETUP register value is: 0x");
    Serial.println(nrf_data, HEX);

    // Is this a request to drive a controller?

    // Is this a request for new data?
    static Dht11 sensor(DHT11PIN);

    switch (sensor.read()) {
      case Dht11::OK:
        Serial.print("Humidity (%): ");
        Serial.println(sensor.getHumidity());
        Serial.print("Temperature (F): ");
        Serial.println((sensor.getTemperature()*1.8 + 32));
        break;

      case Dht11::ERROR_CHECKSUM:
        Serial.println("Checksum error");
        break;

      case Dht11::ERROR_TIMEOUT:
        Serial.println("Timeout error");
        break;

      default:
        Serial.println("Unknown error");
        break;
    }
    Serial.println(" ");
    Serial.println(" ");

    // Clear the flag for sending new data as requested
    RXCOMMAND = 0;
  }
#endif
}

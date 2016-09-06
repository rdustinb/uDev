int blueLed = 6;
int greenLed = 5;
int redLed = 3;

// Enable (1) or disable (0) debug statements
int debug = 1;

// To Limit the maximum pulse width output, change this value
// full pulse width output is a value of 1, half pulse width
// is a value of 2, 33% pulse width is a value of 3, etc
int scalefactor = 1;

// Use this for capping the max value
int capValR = 255;
int capValG = 200;
int capValB = 245;

// Update Delay in ms
int updateDelay = 100;

void setup() {
  Serial.begin(115200);

  // Set LED Drivers to Outputs
  pinMode(blueLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(redLed, OUTPUT);
}

void loop() {
  // Read The Whiper Values
  int potVal = analogRead(0);
  int valRed;
  int valGreen;
  int valBlue;

  // Pot Value 0 ---- 128 ---- 256 ---- 384 ---- 512 ---- 640 ---- 768 ---- 896 ---- 1024
  // Red       255 --------------------- 0                          0 -------------- 128
  // Green     0 ---------------------- 255 ----------------------- 0
  // Blue                                0 ----------------------- 255 ------------- 128

  // Red
  if((potVal >= 0) && (potVal < 384)){
    valRed = 255 - (potVal*0.67);
  }else if(potVal >= 768){
    valRed = (potVal - 768)*0.67;
  }else{
    valRed = 0;
  }

  // Green
  if((potVal >= 0) && (potVal < 384)){
    valGreen = potVal*0.67;
  }else if((potVal >= 384) && (potVal < 768)){
    valGreen = (768 - potVal)*0.67;
  }else{
    valGreen = 0;
  }

  // Blue
  if((potVal > 384) && (potVal < 768)){
    valBlue = (potVal - 384)*0.67;
  }else if((potVal >= 768) && (potVal <= 1024)){
    valBlue = (1152 - potVal)*0.67;
  }else{
    valBlue = 0;
  }

  // Writing 256 to a PWM blows it up
  if(valRed > capValR){
    valRed = capValR;
  }
  if(valGreen > capValG){
    valGreen = capValG;
  }
  if(valBlue > capValB){
    valBlue = capValB;
  }

  // Write the PWM Value to the individual LED Drivers
  analogWrite(redLed, valRed);
  analogWrite(greenLed, valGreen);
  analogWrite(blueLed, valBlue);
  
  // Now Print to Check Functionality
  if(debug){
    Serial.print("Pot Value is: ");
    Serial.println(potVal);
    Serial.print("Red Value is: ");
    Serial.println(valRed);
    Serial.print("Green Value is: ");
    Serial.println(valGreen);
    Serial.print("Blue Value is: ");
    Serial.println(valBlue);
  }

  // Delay so the print doesn't blow up
  delay(updateDelay);
}

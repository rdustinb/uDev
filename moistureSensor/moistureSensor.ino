
#define WET3 7
#define WET2 6
#define WET1 5
#define WET0 4
#define DRY0 3
#define DRY1 2
#define DRY2 1
#define DRY3 0

// Sample once per hour
//#define TIMEOUT_MS 750000
#define TIMEOUT_MS 5000
#define NUMDATAPOINTS 128
#define MIN 868
#define MAX 880

int moisturePin = A1;
int moistureValue = 0;
int moistureHistory[NUMDATAPOINTS];
uint16_t moistureHistoryIndex = 0;

void setup() {
  Serial.begin(115200);
}

void loop() {
  moistureHistory[moistureHistoryIndex] = analogRead(moisturePin);
  boundValue();
  Serial.println(moistureHistory[moistureHistoryIndex]);
  Serial.println(F("What the plant is saying: "));
  getDescription(moistureHistory[moistureHistoryIndex]);
  Serial.println(F(" "));
  asciiChart();
  moistureHistoryIndex++;
  if(moistureHistoryIndex == NUMDATAPOINTS){
    moistureHistoryIndex = NUMDATAPOINTS-1;
    shiftData();
  }
  Serial.println(F(" "));
  Serial.println(F(" "));
  delay(TIMEOUT_MS);
}

void getDescription(int value){
  int rounded = (value - MIN)*7/(MAX-MIN);
  switch(rounded){
    case WET3:
      Serial.println("\tI'm drowning!");
    break;
    case WET2:
      Serial.println("\tI'm drenched!");
    break;
    case WET1:
      Serial.println("\tWet enough!");
    break;
    case WET0:
      Serial.println("\tKind of wet.");
    break;
    case DRY0:
      Serial.println("\tKind of dry.");
    break;
    case DRY1:
      Serial.println("\tI'm thirsty.");
    break;
    case DRY2:
      Serial.println("\tI'm parched.");
    break;
    case DRY3:
      Serial.println("\tBone dry!");
    break;
    default:
      Serial.println("\tBone dry!");
    break;
  }
}

void asciiChart(){
  for(int i=9; i>-1; i--){
    Serial.print(F("  |"));
    for(int j=0; j<NUMDATAPOINTS; j++){
      // Print Empty Columns
      if(j > moistureHistoryIndex){
        Serial.print(F(" "));
      }else{
        // Normalize the value
        int normalizedValue = (moistureHistory[j] - MIN)*10/(MAX-MIN);
        if(normalizedValue > i){
          Serial.print(F("*"));
        }else{
          Serial.print(F(" "));
        }
      }
    }
    Serial.println(F(" "));
  }

  Serial.print(F("  |"));

  for(int i=0; i<NUMDATAPOINTS; i++){
    Serial.print(F("-"));
  }
  Serial.println(F(" "));
}

void shiftData(){
  for(int i=0; i<(NUMDATAPOINTS-1); i++){
    moistureHistory[i] = moistureHistory[i+1];
  }
}

void boundValue(){
  if(moistureHistory[moistureHistoryIndex] > MAX){
    moistureHistory[moistureHistoryIndex] = MAX;
  }else if(moistureHistory[moistureHistoryIndex] < MIN){
    moistureHistory[moistureHistoryIndex] = MIN;
  }
}


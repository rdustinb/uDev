const int keypad0 = A0;
const int keypad1 = A1;
const int keypad2 = A2;

int sensor0 = 0;
int sensor1 = 0;
int sensor2 = 0;

int debounceDelayms = 1;

int keyAmbientLevel = 75;
int keyConversionError = 25;

int topRowVal = 1023;
int topMidRowVal = 415;
int botMidRowVal = 240;
int botRowVal = 150;

/**************************************************************
                           Keypad Call
**************************************************************/
char scanKeypad(){
    /*
    Due to the analog resistor division for each keypad column
    the following values apply:
        ~1023 = Top Row
        ~415 = Middle Top Row
        ~250 = Middle Bottom Row
        ~150 = Bottom Row
    */
    sensor0 = analogRead(keypad0);
    sensor1 = analogRead(keypad1);
    sensor2 = analogRead(keypad2);
    
    // If a button is pressed, the analog line will be pulled up toat least ~1V
    // then delay, and reread the key again (switch debounce)
    // Then decode the analog level.
    if(sensor0 > keyAmbientLevel){
        delay(debounceDelayms);
        sensor0 = analogRead(keypad0);
        // Decode pressed key
        if((sensor0 >= (topRowVal-keyConversionError)) && (sensor0 <= (topRowVal+keyConversionError))){
            return '3';
        }else if((sensor0 >= (topMidRowVal-keyConversionError)) && (sensor0 <= (topMidRowVal+keyConversionError))){
            return '6';
        }else if((sensor0 >= (botMidRowVal-keyConversionError)) && (sensor0 <= (botMidRowVal+keyConversionError))){
            return '9';
        }else if((sensor0 >= (botRowVal-keyConversionError)) && (sensor0 <= (botRowVal+keyConversionError))){
            return '#';
        }
    }
    else if(sensor1 > keyAmbientLevel){
        delay(debounceDelayms);
        sensor1 = analogRead(keypad1);
        // Decode pressed key
        if((sensor1 >= (topRowVal-keyConversionError)) && (sensor1 <= (topRowVal+keyConversionError))){
            return '2';
        }else if((sensor1 >= (topMidRowVal-keyConversionError)) && (sensor1 <= (topMidRowVal+keyConversionError))){
            return '5';
        }else if((sensor1 >= (botMidRowVal-keyConversionError)) && (sensor1 <= (botMidRowVal+keyConversionError))){
            return '8';
        }else if((sensor1 >= (botRowVal-keyConversionError)) && (sensor1 <= (botRowVal+keyConversionError))){
            return '0';
        }
    }
    else if(sensor2 > keyAmbientLevel){
        delay(debounceDelayms);
        sensor2 = analogRead(keypad2);
        // Decode pressed key
        if((sensor2 >= (topRowVal-keyConversionError)) && (sensor2 <= (topRowVal+keyConversionError))){
            return '1';
        }else if((sensor2 >= (topMidRowVal-keyConversionError)) && (sensor2 <= (topMidRowVal+keyConversionError))){
            return '4';
        }else if((sensor2 >= (botMidRowVal-keyConversionError)) && (sensor2 <= (botMidRowVal+keyConversionError))){
            return '7';
        }else if((sensor2 >= (botRowVal-keyConversionError)) && (sensor2 <= (botRowVal+keyConversionError))){
            return '*';
        }
    }
    return 'x';
}

char scanKeypadBasic(){
    sensor0 = analogRead(keypad0);
    sensor1 = analogRead(keypad1);
    sensor2 = analogRead(keypad2);
    
    // Return a basic value indicating if any of the keys are
    // pressed at the moment the function was called or not.
    if(sensor0 > keyAmbientLevel){
        return 'y';
    }else if(sensor1 > keyAmbientLevel){
        return 'y';
    }else if(sensor2 > keyAmbientLevel){
        return 'y';
    }
    // No keys pressed, return 'n'
    return 'n';
}

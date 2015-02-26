// Globals
int keyStillPressed = 0;
int ignoreKeypadDelay = 0;
char animate_sel = '+';
int curr_row = 0;
int curr_col = 0;
int update_row = 0;
int update_col = 0;
int boot_flag = 0;

void setup()
{
    Serial.begin(115200);
    /*************************************
        Init all 7Segment Characters
    *************************************/
    init_7seg_array();
    
    /*************************************
         Dot Matrix LCD Playground
    *************************************/
    init_lcd();
}

// Loop turns the display into a local serial monitor echo.
// Type to the Arduino from the serial monitor, and it'll echo
// what you type on the display. Type ~ to clear the display.
void loop()
{
    // If a key is successfully registered, ignore the keypad for
    // about a second so that the debounce doesn't re-register
    if(keyStillPressed == 1){
        // Block another keypad scan until the key is released
        if(scanKeypadBasic() == 'n'){
            keyStillPressed = 0;
            ignoreKeypadDelay = 3;
        }
    // Now delay a bit to account for key release debounce
    }else if(ignoreKeypadDelay != 0){
        ignoreKeypadDelay--;
    }else{
        char val = scanKeypad();
        if(val != 'x'){
            keyStillPressed = 1;
            Serial.println(val);
            if((val == '*') || (val == '#')){
                if(val == '*'){
                    boot_flag = 0;
                    curr_col = 0;
                    curr_row = 0;
                    clear_chars();
                }
                if(val == '#'){
                    if(animate_sel == '-'){
                        animate_sel = '+';
                    }else if(animate_sel == '+'){
                        animate_sel = '-';
                    }
                }
            }else{
                Serial.print("Column: ");
                Serial.println(curr_col);
                Serial.print("Row: ");
                Serial.println(curr_row);
                // If we just booted, then clear all character displays
                if(boot_flag == 0){
                    boot_flag = 1;
                    off_all_7seg_array();
                }
                // Write to the current character location
                update_chars(curr_row, curr_col, val);
                // Determine when to move to the next row, reset column
                if(curr_col == 3){
                    curr_col = 0;
                    // Determine when to loop back to the first row
                    if(curr_row == 2){
                        curr_row = 0;
                    }else{
                        curr_row++;
                    }
                }else{
                    curr_col++;
                }
            }
        }
    }
    // Bubble Display
    animate_lcd(animate_sel);
    // Startup 7 Segment LED Blips, Cycle is every 5ms due to Keypad scanning
    if(boot_flag == 0){
        loop_blips_7seg_array();
        delay(12);
    }else{
        for(int i=0; i<12; i++){
            update_char_7seg_array(update_row, update_col);
            // Determine when to move to the next row, reset column
            if(update_col == 3){
                update_col = 0;
                // Determine when to loop back to the first row
                if(update_row == 2){
                    update_row = 0;
                }else{
                    update_row++;
                }
            }else{
                update_col++;
            }
            delay(1);
        }
        // This is necessary to prevent the final character in the 12-char array
        // to "flash" due to the updating of the LCD which follows in the next 
        // loop iteration.
        off_all_7seg_array();
    }
}

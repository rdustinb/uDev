// 7Seg LED Characters
// http://en.wikipedia.org/wiki/Seven-segment_display
const int ROWS = 3;
const int CHARS = 4;

/********** Character Cathodes ***********/
// Line 0, Character 0 -- 18
// Line 0, Character 1 -- 5
// Line 0, Character 2 -- 4
// Line 0, Character 3 -- 19
// Line 1, Character 0 -- 2
// Line 1, Character 1 -- 21
// Line 1, Character 2 -- 3
// Line 1, Character 3 -- 20
// Line 2, Character 0 -- 0
// Line 2, Character 1 -- 1
// Line 2, Character 2 -- 22
// Line 2, Character 3 -- 23
const int char_array[ROWS][CHARS] = {{18, 5, 4, 19}, {2, 21, 3, 20}, {0, 1, 22, 23}};
char characters[ROWS][CHARS] = {{'x','x','x','x'},{'x','x','x','x'},{'x','x','x','x'}};

/********** Distributed Anodes ***********/
// Segment A -- 29
// Segment B -- 31
// Segment C -- 27
// Segment D -- 24
// Segment E -- 32
// Segment F -- 30
// Segment G -- 25
// Segment DP -- 28
const int SEGA = 29;
const int SEGB = 31;
const int SEGC = 27;
const int SEGD = 24;
const int SEGE = 32;
const int SEGF = 30;
const int SEGG = 25;
const int SEGDP = 28;

// Globals
const int BLIPPERIOD = 4;
int blips_loop_cnt = 1;
int blips_seg_drive = 0;

/**************************************************************
                       7 Segment Array Calls
**************************************************************/
// High End Calls
void loop_blips_7seg_array(){
    // Make sure all characters are on
    on_all_7seg_array();
    // Now update state
    if(blips_loop_cnt == BLIPPERIOD){
        blips_loop_cnt = 1;
        // If a period has lapsed, change segment
        if(blips_seg_drive == 5){
            blips_seg_drive = 0;
        }else{
            blips_seg_drive++;
        }
        // Decode Character to Drive
        switch(blips_seg_drive){
            case 0:
                write_all_7seg_array('a');
                break;
            case 1:
                write_all_7seg_array('b');
                break;
            case 2:
                write_all_7seg_array('c');
                break;
            case 3:
                write_all_7seg_array('d');
                break;
            case 4:
                write_all_7seg_array('e');
                break;
            case 5:
                write_all_7seg_array('f');
                break;
        }
    }else{
        blips_loop_cnt++;
    }
}

// Low End Calls
void init_7seg_array(){
    // Test 7Seg Characters
    // Character Cathodes
    for(int i=0; i<ROWS; i++){
        for(int j=0; j<CHARS; j++){
            pinMode(char_array[i][j], OUTPUT);
        }
    }
    // Segment Anodes
    pinMode(SEGA, OUTPUT);
    pinMode(SEGB, OUTPUT);
    pinMode(SEGC, OUTPUT);
    pinMode(SEGD, OUTPUT);
    pinMode(SEGE, OUTPUT);
    pinMode(SEGF, OUTPUT);
    pinMode(SEGG, OUTPUT);
    pinMode(SEGDP, OUTPUT);
}

void off_all_7seg_array(){
    // Disable Cathodes
    for(int i=0; i<ROWS; i++){
        for(int j=0; j<CHARS; j++){
            digitalWrite(char_array[i][j], LOW);
        }
    }
}

void on_all_7seg_array(){
    // Enable Cathods
    for(int i=0; i<ROWS; i++){
        for(int j=0; j<CHARS; j++){
            digitalWrite(char_array[i][j], HIGH);
        }
    }
}

void write_char_7seg_array(int row, int column, char val){
    // Disable All Segments
    off_all_7seg_array();
    // Decode What segments to turn on
    decode_segments(val);
    // Turn On Correct Character
    digitalWrite(char_array[row][column], HIGH);
}

void update_char_7seg_array(int row, int column){
    // Disable All Segments
    off_all_7seg_array();
    // Decode What segments to turn on
    decode_segments(characters[row][column]);
    // Turn On Correct Character
    digitalWrite(char_array[row][column], HIGH);
}

void update_all_chars(){
    for(int i=0; i<ROWS; i++){
        for(int j=0; j<CHARS; j++){
            update_char_7seg_array(i, j);
        }
    }
}

void update_chars(int row, int col, char val){
    characters[row][col] = val;
}

void clear_chars(){
    // Enable Cathods
    for(int i=0; i<ROWS; i++){
        for(int j=0; j<CHARS; j++){
            characters[i][j] = 'x';
        }
    }
}

void write_all_7seg_array(char val){
    // Disable All Segments
    off_all_7seg_array();
    // Decode What Segments to Drive
    decode_segments(val);
    // Turn On All Segments
    on_all_7seg_array();
}

void decode_segments(char val){
    // Decode What Segments to Drive
    switch(val){
        case '0':
            digitalWrite(SEGA, HIGH);
            digitalWrite(SEGB, HIGH);
            digitalWrite(SEGC, HIGH);
            digitalWrite(SEGD, HIGH);
            digitalWrite(SEGE, HIGH);
            digitalWrite(SEGF, HIGH);
            digitalWrite(SEGG, LOW);
            digitalWrite(SEGDP, LOW);
            break;
        case '1':
            digitalWrite(SEGA, LOW);
            digitalWrite(SEGB, HIGH);
            digitalWrite(SEGC, HIGH);
            digitalWrite(SEGD, LOW);
            digitalWrite(SEGE, LOW);
            digitalWrite(SEGF, LOW);
            digitalWrite(SEGG, LOW);
            digitalWrite(SEGDP, LOW);
            break;
        case '2':
            digitalWrite(SEGA, HIGH);
            digitalWrite(SEGB, HIGH);
            digitalWrite(SEGC, LOW);
            digitalWrite(SEGD, HIGH);
            digitalWrite(SEGE, HIGH);
            digitalWrite(SEGF, LOW);
            digitalWrite(SEGG, HIGH);
            digitalWrite(SEGDP, LOW);
            break;
        case '3':
            digitalWrite(SEGA, HIGH);
            digitalWrite(SEGB, HIGH);
            digitalWrite(SEGC, HIGH);
            digitalWrite(SEGD, HIGH);
            digitalWrite(SEGE, LOW);
            digitalWrite(SEGF, LOW);
            digitalWrite(SEGG, HIGH);
            digitalWrite(SEGDP, LOW);
            break;
        case '4':
            digitalWrite(SEGA, LOW);
            digitalWrite(SEGB, HIGH);
            digitalWrite(SEGC, HIGH);
            digitalWrite(SEGD, LOW);
            digitalWrite(SEGE, LOW);
            digitalWrite(SEGF, HIGH);
            digitalWrite(SEGG, HIGH);
            digitalWrite(SEGDP, LOW);
            break;
        case '5':
            digitalWrite(SEGA, HIGH);
            digitalWrite(SEGB, LOW);
            digitalWrite(SEGC, HIGH);
            digitalWrite(SEGD, HIGH);
            digitalWrite(SEGE, LOW);
            digitalWrite(SEGF, HIGH);
            digitalWrite(SEGG, HIGH);
            digitalWrite(SEGDP, LOW);
            break;
        case '6':
            digitalWrite(SEGA, HIGH);
            digitalWrite(SEGB, LOW);
            digitalWrite(SEGC, HIGH);
            digitalWrite(SEGD, HIGH);
            digitalWrite(SEGE, HIGH);
            digitalWrite(SEGF, HIGH);
            digitalWrite(SEGG, HIGH);
            digitalWrite(SEGDP, LOW);
            break;
        case '7':
            digitalWrite(SEGA, HIGH);
            digitalWrite(SEGB, HIGH);
            digitalWrite(SEGC, HIGH);
            digitalWrite(SEGD, LOW);
            digitalWrite(SEGE, LOW);
            digitalWrite(SEGF, LOW);
            digitalWrite(SEGG, LOW);
            digitalWrite(SEGDP, LOW);
            break;
        case '8':
            digitalWrite(SEGA, HIGH);
            digitalWrite(SEGB, HIGH);
            digitalWrite(SEGC, HIGH);
            digitalWrite(SEGD, HIGH);
            digitalWrite(SEGE, HIGH);
            digitalWrite(SEGF, HIGH);
            digitalWrite(SEGG, HIGH);
            digitalWrite(SEGDP, LOW);
            break;
        case '9':
            digitalWrite(SEGA, HIGH);
            digitalWrite(SEGB, HIGH);
            digitalWrite(SEGC, HIGH);
            digitalWrite(SEGD, HIGH);
            digitalWrite(SEGE, LOW);
            digitalWrite(SEGF, HIGH);
            digitalWrite(SEGG, HIGH);
            digitalWrite(SEGDP, LOW);
            break;
        case 'a':
            digitalWrite(SEGA, HIGH);
            digitalWrite(SEGB, LOW);
            digitalWrite(SEGC, LOW);
            digitalWrite(SEGD, LOW);
            digitalWrite(SEGE, LOW);
            digitalWrite(SEGF, LOW);
            digitalWrite(SEGG, LOW);
            digitalWrite(SEGDP, LOW);
            break;
        case 'b':
            digitalWrite(SEGA, LOW);
            digitalWrite(SEGB, HIGH);
            digitalWrite(SEGC, LOW);
            digitalWrite(SEGD, LOW);
            digitalWrite(SEGE, LOW);
            digitalWrite(SEGF, LOW);
            digitalWrite(SEGG, LOW);
            digitalWrite(SEGDP, LOW);
            break;
        case 'c':
            digitalWrite(SEGA, LOW);
            digitalWrite(SEGB, LOW);
            digitalWrite(SEGC, HIGH);
            digitalWrite(SEGD, LOW);
            digitalWrite(SEGE, LOW);
            digitalWrite(SEGF, LOW);
            digitalWrite(SEGG, LOW);
            digitalWrite(SEGDP, LOW);
            break;
        case 'd':
            digitalWrite(SEGA, LOW);
            digitalWrite(SEGB, LOW);
            digitalWrite(SEGC, LOW);
            digitalWrite(SEGD, HIGH);
            digitalWrite(SEGE, LOW);
            digitalWrite(SEGF, LOW);
            digitalWrite(SEGG, LOW);
            digitalWrite(SEGDP, LOW);
            break;
        case 'e':
            digitalWrite(SEGA, LOW);
            digitalWrite(SEGB, LOW);
            digitalWrite(SEGC, LOW);
            digitalWrite(SEGD, LOW);
            digitalWrite(SEGE, HIGH);
            digitalWrite(SEGF, LOW);
            digitalWrite(SEGG, LOW);
            digitalWrite(SEGDP, LOW);
            break;
        case 'f':
            digitalWrite(SEGA, LOW);
            digitalWrite(SEGB, LOW);
            digitalWrite(SEGC, LOW);
            digitalWrite(SEGD, LOW);
            digitalWrite(SEGE, LOW);
            digitalWrite(SEGF, HIGH);
            digitalWrite(SEGG, LOW);
            digitalWrite(SEGDP, LOW);
            break;
        case 'g':
            digitalWrite(SEGA, LOW);
            digitalWrite(SEGB, LOW);
            digitalWrite(SEGC, LOW);
            digitalWrite(SEGD, LOW);
            digitalWrite(SEGE, LOW);
            digitalWrite(SEGF, LOW);
            digitalWrite(SEGG, HIGH);
            digitalWrite(SEGDP, LOW);
            break;
        case 'p':
            digitalWrite(SEGA, LOW);
            digitalWrite(SEGB, LOW);
            digitalWrite(SEGC, LOW);
            digitalWrite(SEGD, LOW);
            digitalWrite(SEGE, LOW);
            digitalWrite(SEGF, LOW);
            digitalWrite(SEGG, LOW);
            digitalWrite(SEGDP, HIGH);
            break;
        case 'x':
            digitalWrite(SEGA, LOW);
            digitalWrite(SEGB, LOW);
            digitalWrite(SEGC, LOW);
            digitalWrite(SEGD, LOW);
            digitalWrite(SEGE, LOW);
            digitalWrite(SEGF, LOW);
            digitalWrite(SEGG, LOW);
            digitalWrite(SEGDP, LOW);
            break;
    }
}


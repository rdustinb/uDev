#include <SPI.h> // We'll use SPI to transfer data. Faster!

/* PCD8544-specific defines: */
#define LCD_COMMAND  0
#define LCD_DATA     1

/* 84x48 LCD Defines: */
#define LCD_WIDTH   84 // Note: x-coordinates go wide
#define LCD_HEIGHT  48 // Note: y-coordinates go high
#define WHITE       0  // For drawing pixels. A 0 draws white.
#define BLACK       1  // A 1 draws black.

/* Pin definitions:
Most of these pins can be moved to any digital or analog pin.
DN(MOSI)and SCLK should be left where they are (SPI pins). The
LED (backlight) pin should remain on a PWM-capable pin. */
const int scePin = 6;   // SCE - Chip select, pin 3 on LCD.
const int rstPin = 7;   // RST - Reset, pin 4 on LCD.
const int dcPin = 8;    // DC - Data/Command, pin 5 on LCD.
const int sdinPin = 11;  // DN(MOSI) - Serial data, pin 6 on LCD.
const int sclkPin = 13;  // SCLK - Serial clock, pin 7 on LCD.
const int blPin = 9;    // LED - Backlight LED, pin 8 on LCD.

/* Font table:
This table contains the hex values that represent pixels for a
font that is 5 pixels wide and 8 pixels high. Each byte in a row
represents one, 8-pixel, vertical column of a character. 5 bytes
per character. */
static const byte ASCII[][5] = {
    // First 32 characters (0x00-0x19) are ignored. These are
    // non-displayable, control characters.
     {0x00, 0x00, 0x00, 0x00, 0x00} // 0x20
    ,{0x00, 0x00, 0x5f, 0x00, 0x00} // 0x21 !
    ,{0x00, 0x07, 0x00, 0x07, 0x00} // 0x22 "
    ,{0x14, 0x7f, 0x14, 0x7f, 0x14} // 0x23 #
    ,{0x24, 0x2a, 0x7f, 0x2a, 0x12} // 0x24 $
    ,{0x23, 0x13, 0x08, 0x64, 0x62} // 0x25 %
    ,{0x36, 0x49, 0x55, 0x22, 0x50} // 0x26 &
    ,{0x00, 0x05, 0x03, 0x00, 0x00} // 0x27 '
    ,{0x00, 0x1c, 0x22, 0x41, 0x00} // 0x28 (
    ,{0x00, 0x41, 0x22, 0x1c, 0x00} // 0x29 )
    ,{0x14, 0x08, 0x3e, 0x08, 0x14} // 0x2a *
    ,{0x08, 0x08, 0x3e, 0x08, 0x08} // 0x2b +
    ,{0x00, 0x50, 0x30, 0x00, 0x00} // 0x2c ,
    ,{0x08, 0x08, 0x08, 0x08, 0x08} // 0x2d -
    ,{0x00, 0x60, 0x60, 0x00, 0x00} // 0x2e .
    ,{0x20, 0x10, 0x08, 0x04, 0x02} // 0x2f /
    ,{0x3e, 0x51, 0x49, 0x45, 0x3e} // 0x30 0
    ,{0x00, 0x42, 0x7f, 0x40, 0x00} // 0x31 1
    ,{0x42, 0x61, 0x51, 0x49, 0x46} // 0x32 2
    ,{0x21, 0x41, 0x45, 0x4b, 0x31} // 0x33 3
    ,{0x18, 0x14, 0x12, 0x7f, 0x10} // 0x34 4
    ,{0x27, 0x45, 0x45, 0x45, 0x39} // 0x35 5
    ,{0x3c, 0x4a, 0x49, 0x49, 0x30} // 0x36 6
    ,{0x01, 0x71, 0x09, 0x05, 0x03} // 0x37 7
    ,{0x36, 0x49, 0x49, 0x49, 0x36} // 0x38 8
    ,{0x06, 0x49, 0x49, 0x29, 0x1e} // 0x39 9
    ,{0x00, 0x36, 0x36, 0x00, 0x00} // 0x3a :
    ,{0x00, 0x56, 0x36, 0x00, 0x00} // 0x3b ;
    ,{0x08, 0x14, 0x22, 0x41, 0x00} // 0x3c <
    ,{0x14, 0x14, 0x14, 0x14, 0x14} // 0x3d =
    ,{0x00, 0x41, 0x22, 0x14, 0x08} // 0x3e >
    ,{0x02, 0x01, 0x51, 0x09, 0x06} // 0x3f ?
    ,{0x32, 0x49, 0x79, 0x41, 0x3e} // 0x40 @
    ,{0x7e, 0x11, 0x11, 0x11, 0x7e} // 0x41 A
    ,{0x7f, 0x49, 0x49, 0x49, 0x36} // 0x42 B
    ,{0x3e, 0x41, 0x41, 0x41, 0x22} // 0x43 C
    ,{0x7f, 0x41, 0x41, 0x22, 0x1c} // 0x44 D
    ,{0x7f, 0x49, 0x49, 0x49, 0x41} // 0x45 E
    ,{0x7f, 0x09, 0x09, 0x09, 0x01} // 0x46 F
    ,{0x3e, 0x41, 0x49, 0x49, 0x7a} // 0x47 G
    ,{0x7f, 0x08, 0x08, 0x08, 0x7f} // 0x48 H
    ,{0x00, 0x41, 0x7f, 0x41, 0x00} // 0x49 I
    ,{0x20, 0x40, 0x41, 0x3f, 0x01} // 0x4a J
    ,{0x7f, 0x08, 0x14, 0x22, 0x41} // 0x4b K
    ,{0x7f, 0x40, 0x40, 0x40, 0x40} // 0x4c L
    ,{0x7f, 0x02, 0x0c, 0x02, 0x7f} // 0x4d M
    ,{0x7f, 0x04, 0x08, 0x10, 0x7f} // 0x4e N
    ,{0x3e, 0x41, 0x41, 0x41, 0x3e} // 0x4f O
    ,{0x7f, 0x09, 0x09, 0x09, 0x06} // 0x50 P
    ,{0x3e, 0x41, 0x51, 0x21, 0x5e} // 0x51 Q
    ,{0x7f, 0x09, 0x19, 0x29, 0x46} // 0x52 R
    ,{0x46, 0x49, 0x49, 0x49, 0x31} // 0x53 S
    ,{0x01, 0x01, 0x7f, 0x01, 0x01} // 0x54 T
    ,{0x3f, 0x40, 0x40, 0x40, 0x3f} // 0x55 U
    ,{0x1f, 0x20, 0x40, 0x20, 0x1f} // 0x56 V
    ,{0x3f, 0x40, 0x38, 0x40, 0x3f} // 0x57 W
    ,{0x63, 0x14, 0x08, 0x14, 0x63} // 0x58 X
    ,{0x07, 0x08, 0x70, 0x08, 0x07} // 0x59 Y
    ,{0x61, 0x51, 0x49, 0x45, 0x43} // 0x5a Z
    ,{0x00, 0x7f, 0x41, 0x41, 0x00} // 0x5b [
    ,{0x02, 0x04, 0x08, 0x10, 0x20} // 0x5c \
    ,{0x00, 0x41, 0x41, 0x7f, 0x00} // 0x5d ]
    ,{0x00, 0x41, 0x41, 0x7f, 0x00} // 0x5d ]
    ,{0x04, 0x02, 0x01, 0x02, 0x04} // 0x5e ^
    ,{0x40, 0x40, 0x40, 0x40, 0x40} // 0x5f _
    ,{0x00, 0x01, 0x02, 0x04, 0x00} // 0x60 `
    ,{0x20, 0x54, 0x54, 0x54, 0x78} // 0x61 a
    ,{0x7f, 0x48, 0x44, 0x44, 0x38} // 0x62 b
    ,{0x38, 0x44, 0x44, 0x44, 0x20} // 0x63 c
    ,{0x38, 0x44, 0x44, 0x48, 0x7f} // 0x64 d
    ,{0x38, 0x54, 0x54, 0x54, 0x18} // 0x65 e
    ,{0x08, 0x7e, 0x09, 0x01, 0x02} // 0x66 f
    ,{0x0c, 0x52, 0x52, 0x52, 0x3e} // 0x67 g
    ,{0x7f, 0x08, 0x04, 0x04, 0x78} // 0x68 h
    ,{0x00, 0x44, 0x7d, 0x40, 0x00} // 0x69 i
    ,{0x20, 0x40, 0x44, 0x3d, 0x00} // 0x6a j
    ,{0x7f, 0x10, 0x28, 0x44, 0x00} // 0x6b k
    ,{0x00, 0x41, 0x7f, 0x40, 0x00} // 0x6c l
    ,{0x7c, 0x04, 0x18, 0x04, 0x78} // 0x6d m
    ,{0x7c, 0x08, 0x04, 0x04, 0x78} // 0x6e n
    ,{0x38, 0x44, 0x44, 0x44, 0x38} // 0x6f o
    ,{0x7c, 0x14, 0x14, 0x14, 0x08} // 0x70 p
    ,{0x08, 0x14, 0x14, 0x18, 0x7c} // 0x71 q
    ,{0x7c, 0x08, 0x04, 0x04, 0x08} // 0x72 r
    ,{0x48, 0x54, 0x54, 0x54, 0x20} // 0x73 s
    ,{0x04, 0x3f, 0x44, 0x40, 0x20} // 0x74 t
    ,{0x3c, 0x40, 0x40, 0x20, 0x7c} // 0x75 u
    ,{0x1c, 0x20, 0x40, 0x20, 0x1c} // 0x76 v
    ,{0x3c, 0x40, 0x30, 0x40, 0x3c} // 0x77 w
    ,{0x44, 0x28, 0x10, 0x28, 0x44} // 0x78 x
    ,{0x0c, 0x50, 0x50, 0x50, 0x3c} // 0x79 y
    ,{0x44, 0x64, 0x54, 0x4c, 0x44} // 0x7a z
    ,{0x00, 0x08, 0x36, 0x41, 0x00} // 0x7b {
    ,{0x00, 0x00, 0x7f, 0x00, 0x00} // 0x7c |
    ,{0x00, 0x41, 0x36, 0x08, 0x00} // 0x7d }
    ,{0x10, 0x08, 0x08, 0x10, 0x08} // 0x7e ~
    ,{0x78, 0x46, 0x41, 0x46, 0x78} // 0x7f DEL
};

/* The displayMap variable stores a buffer representation of the
pixels on our display. There are 504 total bits in this array,
same as how many pixels there are on a 84 x 48 display.

Each byte in this array covers a 8-pixel vertical block on the
display. Each successive byte covers the next 8-pixel column over
until you reach the right-edge of the display and step down 8 rows.

To update the display, we first have to write to this array, then
call the updateDisplay() function, which sends this whole array
to the PCD8544.

Because the PCD8544 won't let us write individual pixels at a
time, this is how we can make targeted changes to the display. */
byte displayMap[LCD_WIDTH * LCD_HEIGHT / 8] = {};

// Bubble Animation Arrays
const int BUBBLEDLY = 8;
int bubble_cnt = BUBBLEDLY;
const int BUBBLE_LAYERS = 3;
const int BUBBLERADIUS[BUBBLE_LAYERS] = {3,2,1};
const int BUBBLEMOVE[BUBBLE_LAYERS] = {3,2,1};
const int BUBBLECNT[BUBBLE_LAYERS] = {6,10,20};
int bubble_x[BUBBLE_LAYERS][20] = {};
int bubble_y[BUBBLE_LAYERS][20] = {};

// Raindrop Animation Arrays
const int RAINDLY = 8;
int rain_cnt = RAINDLY;
const int RAIN_LAYERS = 3;
const int RAINRADIUS[RAIN_LAYERS] = {3,2,1};
const int RAINMOVE[RAIN_LAYERS] = {3,2,1};
const int RAINCNT[RAIN_LAYERS] = {6,10,20};
int rain_x[RAIN_LAYERS][20] = {};
int rain_y[RAIN_LAYERS][20] = {};

/**************************************************************
                           LCD Calls
**************************************************************/
// High End Calls
void init_lcd(){
    // Setup the LCD
    lcdBegin();
    // Set Contrast of LCD
    setContrast(65);
    // Clear the displayMap
    clearDisplay(WHITE);
    // Write new displayMap to the LCD
    updateDisplay();
    // Setup Random Bubble Pixels
    for(int i=0; i<BUBBLE_LAYERS; i++){
        for(int j=0; j<BUBBLECNT[i]; j++){
            bubble_x[i][j] = random(-BUBBLERADIUS[i],LCD_WIDTH+BUBBLERADIUS[i]);
        }
        for(int j=0; j<BUBBLECNT[i]; j++){
            bubble_y[i][j] = random(-BUBBLERADIUS[i],LCD_HEIGHT+BUBBLERADIUS[i]);
        }
    }
    // Setup Random Rain Pixels
    for(int i=0; i<RAIN_LAYERS; i++){
        for(int j=0; j<RAINCNT[i]; j++){
            rain_x[i][j] = random(-RAINRADIUS[i],LCD_WIDTH+RAINRADIUS[i]);
        }
        for(int j=0; j<RAINCNT[i]; j++){
            rain_y[i][j] = random(-RAINRADIUS[i],LCD_HEIGHT+RAINRADIUS[i]);
        }
    }
}

void animate_lcd(char selection){
    if(selection == '+'){
        // All Planes of Bubbles updated simultaneously
        if(bubble_cnt == BUBBLEDLY){
            // Clear bubble_cnt
            bubble_cnt = 0;
            // First Clear the Display
            clearDisplay(WHITE);

            // Plane 1
            // Update "Bubble" Pixels
            for(int i=0; i<BUBBLE_LAYERS; i++){
                for(int j=0; j<BUBBLECNT[i]; j++){
                    // Draw each bubble!
                    setCircle(bubble_x[i][j], bubble_y[i][j], BUBBLERADIUS[i], BLACK, 1);
                    // If Bubble has "floated" out of the screen, reset to bottom
                    // and change x position. This could get more advanced to create
                    // the illusion of random bubble placement but for now, this is
                    // good.
                    if(bubble_y[i][j] <= -BUBBLERADIUS[i]){
                        bubble_y[i][j] = (LCD_HEIGHT+BUBBLERADIUS[i]);
                        bubble_x[i][j] = random(-BUBBLERADIUS[i],LCD_WIDTH+BUBBLERADIUS[i]);
                    }else{
                        bubble_y[i][j] -= BUBBLEMOVE[i];
                    }
                }
            }

            // Write new displayMap to the LCD
            updateDisplay();
        }else{
            bubble_cnt++;
        }
    }else if(selection == '-'){
        // All Planes of Bubbles updated simultaneously
        if(bubble_cnt == BUBBLEDLY){
            // Clear bubble_cnt
            bubble_cnt = 0;
            // First Clear the Display
            clearDisplay(WHITE);

            // Plane 1
            // Update "Bubble" Pixels
            for(int i=0; i<BUBBLE_LAYERS; i++){
                for(int j=0; j<BUBBLECNT[i]; j++){
                    // Draw each bubble!
                    setCircle(bubble_x[i][j], bubble_y[i][j], BUBBLERADIUS[i], BLACK, 1);
                    // If Bubble has "floated" out of the screen, reset to bottom
                    // and change x position. This could get more advanced to create
                    // the illusion of random bubble placement but for now, this is
                    // good.
                    if(bubble_y[i][j] >= LCD_HEIGHT+BUBBLERADIUS[i]){
                        bubble_y[i][j] = (-BUBBLERADIUS[i]);
                        bubble_x[i][j] = random(-BUBBLERADIUS[i],LCD_WIDTH+BUBBLERADIUS[i]);
                    }else{
                        bubble_y[i][j] += BUBBLEMOVE[i];
                    }
                }
            }

            // Write new displayMap to the LCD
            updateDisplay();
        }else{
            bubble_cnt++;
        }
    }
}

// Low End Calls
// Because I keep forgetting to put bw variable in when setting...
void setPixel(int x, int y)
{
    setPixel(x, y, BLACK); // Call setPixel with bw set to Black
}

void clearPixel(int x, int y)
{
    setPixel(x, y, WHITE); // call setPixel with bw set to white
}

// This function sets a pixel on displayMap to your preferred
// color. 1=Black, 0= white.
void setPixel(int x, int y, boolean bw)
{
    // First, double check that the coordinate is in range.
    if ((x >= 0) && (x < LCD_WIDTH) && (y >= 0) && (y < LCD_HEIGHT))
    {
        byte shift = y % 8;

        if (bw) // If black, set the bit.
            displayMap[x + (y/8)*LCD_WIDTH] |= 1<<shift;
        else   // If white clear the bit.
            displayMap[x + (y/8)*LCD_WIDTH] &= ~(1<<shift);
    }
}

// setLine draws a line from x0,y0 to x1,y1 with the set color.
// This function was grabbed from the SparkFun ColorLCDShield
// library.
void setLine(int x0, int y0, int x1, int y1, boolean bw)
{
    int dy = y1 - y0; // Difference between y0 and y1
    int dx = x1 - x0; // Difference between x0 and x1
    int stepx, stepy;

    if (dy < 0)
    {
        dy = -dy;
        stepy = -1;
    }
    else
        stepy = 1;

    if (dx < 0)
    {
        dx = -dx;
        stepx = -1;
    }
    else
        stepx = 1;

    dy <<= 1; // dy is now 2*dy
    dx <<= 1; // dx is now 2*dx
    setPixel(x0, y0, bw); // Draw the first pixel.

    if (dx > dy)
    {
        int fraction = dy - (dx >> 1);
        while (x0 != x1)
        {
            if (fraction >= 0)
            {
                y0 += stepy;
                fraction -= dx;
            }
            x0 += stepx;
            fraction += dy;
            setPixel(x0, y0, bw);
        }
    }
    else
    {
        int fraction = dx - (dy >> 1);
        while (y0 != y1)
        {
            if (fraction >= 0)
            {
                x0 += stepx;
                fraction -= dy;
            }
            y0 += stepy;
            fraction += dx;
            setPixel(x0, y0, bw);
        }
    }
}

// setRect will draw a rectangle from x0,y0 top-left corner to
// a x1,y1 bottom-right corner. Can be filled with the fill
// parameter, and colored with bw.
// This function was grabbed from the SparkFun ColorLCDShield
// library.
void setRect(int x0, int y0, int x1, int y1, boolean fill, boolean bw)
{
    // check if the rectangle is to be filled
    if (fill == 1)
    {
        int xDiff;

        if(x0 > x1)
            xDiff = x0 - x1; //Find the difference between the x vars
        else
            xDiff = x1 - x0;

        while(xDiff > 0)
        {
            setLine(x0, y0, x0, y1, bw);

            if(x0 > x1)
                x0--;
            else
                x0++;

            xDiff--;
        }
    }
    else
    {
        // best way to draw an unfilled rectangle is to draw four lines
        setLine(x0, y0, x1, y0, bw);
        setLine(x0, y1, x1, y1, bw);
        setLine(x0, y0, x0, y1, bw);
        setLine(x1, y0, x1, y1, bw);
    }
}

// setCircle draws a circle centered around x0,y0 with a defined
// radius. The circle can be black or white. And have a line
// thickness ranging from 1 to the radius of the circle.
// This function was grabbed from the SparkFun ColorLCDShield
// library.
void setCircle (int x0, int y0, int radius, boolean bw, int lineThickness)
{
    for(int r = 0; r < lineThickness; r++)
    {
        int f = 1 - radius;
        int ddF_x = 0;
        int ddF_y = -2 * radius;
        int x = 0;
        int y = radius;

        setPixel(x0, y0 + radius, bw);
        setPixel(x0, y0 - radius, bw);
        setPixel(x0 + radius, y0, bw);
        setPixel(x0 - radius, y0, bw);

        while(x < y)
        {
            if(f >= 0)
            {
                y--;
                ddF_y += 2;
                f += ddF_y;
            }
            x++;
            ddF_x += 2;
            f += ddF_x + 1;

            setPixel(x0 + x, y0 + y, bw);
            setPixel(x0 - x, y0 + y, bw);
            setPixel(x0 + x, y0 - y, bw);
            setPixel(x0 - x, y0 - y, bw);
            setPixel(x0 + y, y0 + x, bw);
            setPixel(x0 - y, y0 + x, bw);
            setPixel(x0 + y, y0 - x, bw);
            setPixel(x0 - y, y0 - x, bw);
        }
        radius--;
    }
}

// This function will draw a char (defined in the ASCII table
// near the beginning of this sketch) at a defined x and y).
// The color can be either black (1) or white (0).
void setChar(char character, int x, int y, boolean bw)
{
    byte column; // temp byte to store character's column bitmap
    for (int i=0; i<5; i++) // 5 columns (x) per character
    {
        column = ASCII[character - 0x20][i];
        for (int j=0; j<8; j++) // 8 rows (y) per character
        {
            if (column & (0x01 << j)) // test bits to set pixels
                setPixel(x+i, y+j, bw);
            else
                setPixel(x+i, y+j, !bw);
        }
    }
}

// setStr draws a string of characters, calling setChar with
// progressive coordinates until it's done.
// This function was grabbed from the SparkFun ColorLCDShield
// library.
void setStr(char * dString, int x, int y, boolean bw)
{
    while (*dString != 0x00) // loop until null terminator
    {
        setChar(*dString++, x, y, bw);
        x+=5;
        for (int i=y; i<y+8; i++)
        {
            setPixel(x, i, !bw);
        }
        x++;
        if (x > (LCD_WIDTH - 5)) // Enables wrap around
        {
            x = 0;
            y += 8;
        }
    }
}

// This function will draw an array over the screen. (For now) the
// array must be the same size as the screen, covering the entirety
// of the display.
void setBitmap(char * bitArray)
{
    for (int i=0; i<(LCD_WIDTH * LCD_HEIGHT / 8); i++)
        displayMap[i] = bitArray[i];
}

// This function clears the entire display either white (0) or
// black (1).
// The screen won't actually clear until you call updateDisplay()!
void clearDisplay(boolean bw)
{
    for (int i=0; i<(LCD_WIDTH * LCD_HEIGHT / 8); i++)
    {
        if (bw)
            displayMap[i] = 0xFF;
        else
            displayMap[i] = 0;
    }
}

// Helpful function to directly command the LCD to go to a
// specific x,y coordinate.
void gotoXY(int x, int y)
{
    LCDWrite(0, 0x80 | x);  // Column.
    LCDWrite(0, 0x40 | y);  // Row.  ?
}

// This will actually draw on the display, whatever is currently
// in the displayMap array.
void updateDisplay()
{
    gotoXY(0, 0);
    for (int i=0; i < (LCD_WIDTH * LCD_HEIGHT / 8); i++)
    {
        LCDWrite(LCD_DATA, displayMap[i]);
    }
}

// Set contrast can set the LCD Vop to a value between 0 and 127.
// 40-60 is usually a pretty good range.
void setContrast(byte contrast)
{
    LCDWrite(LCD_COMMAND, 0x21); //Tell LCD that extended commands follow
    LCDWrite(LCD_COMMAND, 0x80 | contrast); //Set LCD Vop (Contrast): Try 0xB1(good @ 3.3V) or 0xBF if your display is too dark
    LCDWrite(LCD_COMMAND, 0x20); //Set display mode
}

/* There are two ways to do this. Either through direct commands
to the display, or by swapping each bit in the displayMap array.
We'll leave both methods here, comment one or the other out if
you please. */
void invertDisplay()
{
    /* Direct LCD Command option
    LCDWrite(LCD_COMMAND, 0x20); //Tell LCD that extended commands follow
    LCDWrite(LCD_COMMAND, 0x08 | 0x05); //Set LCD Vop (Contrast): Try 0xB1(good @ 3.3V) or 0xBF if your display is too dark
    LCDWrite(LCD_COMMAND, 0x20); //Set display mode  */

    /* Indirect, swap bits in displayMap option: */
    for (int i=0; i < (LCD_WIDTH * LCD_HEIGHT / 8); i++)
    {
        displayMap[i] = ~displayMap[i] & 0xFF;
    }
    updateDisplay();
}

// There are two memory banks in the LCD, data/RAM and commands.
// This function sets the DC pin high or low depending, and then
// sends the data byte
void LCDWrite(byte data_or_command, byte data)
{
    //Tell the LCD that we are writing either to data or a command
    digitalWrite(dcPin, data_or_command);

    //Send the data
    digitalWrite(scePin, LOW);
    SPI.transfer(data); //shiftOut(sdinPin, sclkPin, MSBFIRST, data);
    digitalWrite(scePin, HIGH);
}

//This sends the magical commands to the PCD8544
void lcdBegin(void)
{
    //Configure control pins
    pinMode(scePin, OUTPUT);
    pinMode(rstPin, OUTPUT);
    pinMode(dcPin, OUTPUT);
    pinMode(sdinPin, OUTPUT);
    pinMode(sclkPin, OUTPUT);
    pinMode(blPin, OUTPUT);
    analogWrite(blPin, 180);

    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV4);
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);

    //Reset the LCD to a known state
    digitalWrite(rstPin, LOW);
    digitalWrite(rstPin, HIGH);

    LCDWrite(LCD_COMMAND, 0x21); //Tell LCD extended commands follow
    LCDWrite(LCD_COMMAND, 0xB0); //Set LCD Vop (Contrast)
    LCDWrite(LCD_COMMAND, 0x04); //Set Temp coefficent
    LCDWrite(LCD_COMMAND, 0x14); //LCD bias mode 1:48 (try 0x13)
    //We must send 0x20 before modifying the display control mode
    LCDWrite(LCD_COMMAND, 0x20);
    LCDWrite(LCD_COMMAND, 0x0C); //Set display control, normal mode.
}

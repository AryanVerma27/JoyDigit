/*
 * JoyDigit — Joystick to Seven-Segment Display
 *
 * Board : MSP-EXP432P401R / MSP-EXP432P4111 LaunchPad + Educational BoosterPack MKII
 * IDE   : Energia (board: "RED LaunchPad w/ msp432p401r EMT (48MHz)")
 *
 * Reads the BoosterPack MKII analog joystick (12-bit ADC) and shows a two-digit
 * number (00-79) on a "seven-segment" display drawn on the HX8353E LCD:
 *   - X-axis -> tens digit (0-7)
 *   - Y-axis -> ones digit (0-9)
 *
 * Seven-segment drawing helpers are adapted from the MSP432 seven-segment
 * LCD framework by Qilin Si & Max Steele.
 */

// Include application, user and local libraries
#include <SPI.h>
#include <LCD_screen.h>
#include <LCD_screen_font.h>
#include <LCD_utilities.h>
#include <Screen_HX8353E.h>
#include <Terminal12e.h>
#include <Terminal6e.h>
#include <Terminal8e.h>

// Declare header function with default values
void headerFooter(String header_text = "JoyDigit",
                  String footer_text = "7-Seg Display",
                  uint16_t color = blueColour);
void clearHeaderFooter();
void infoScreen();
void setOnes(int value);
void setTens(int value);

// Joystick pins on the BoosterPack MKII
const int joystickXPin = 2;   // J1.2  -> X axis
const int joystickYPin = 26;  // J3.26 -> Y axis

int x;
int y;

// Define screen
Screen_HX8353E myScreen;

// ---------------------------------------------------------------------------
// setup(): runs once
// ---------------------------------------------------------------------------
void setup()
{
    Serial.begin(9600);          // serial debug output

    // MSP432 analogRead() defaults to 10 bits; this sketch uses 12 bits (0-4095)
    analogReadResolution(12);

    // Init screen
    myScreen.begin();
    myScreen.setPenSolid(true);

    // Print info screen
    infoScreen();
    delay(2000);

    // Clear screen and put header and footer on screen
    myScreen.clear();
    headerFooter();
}

// ---------------------------------------------------------------------------
// loop(): runs continuously
// ---------------------------------------------------------------------------
void loop()
{
    // Scale raw ADC readings (~0-3500 usable range) to single digits
    y = map(analogRead(joystickYPin), 0, 3500, 0, 9);   // ones place 0-9
    x = map(analogRead(joystickXPin), 0, 3500, 0, 7);   // tens place 0-7  -> max 79

    // Clamp to valid range in case the ADC exceeds 3500
    y = constrain(y, 0, 9);
    x = constrain(x, 0, 7);

    Serial.print("X: ");
    Serial.println(x);
    Serial.print("Y: ");
    Serial.println(y);

    setOnes(y);
    setTens(x);
}

/* ---------------------------------------------------------------------------
 * Seven-segment display helpers
 * ------------------------------------------------------------------------- */

// Possible colors: white, red, green, blue, yellow, cyan, orange, magenta, violet, gray, darkGray
const uint16_t digit_color  = greenColour;
const uint16_t header_color = blueColour;
const uint16_t error_color  = redColour;

// Error variables
bool error_flag = true;
bool one_error  = false;
bool ten_error  = false;
uint16_t last_x = 0;

// Segment on/off definitions
//      _1_
//   2 |   | 6
//     |_3_|
//   4 |   | 7
//     |_5_|
// Segment mask (index 10 = 'E' for error)
bool num_seg[11][7] = {
    {1,1,0,1,1,1,1},  // 0
    {0,0,0,0,0,1,1},  // 1
    {1,0,1,1,1,1,0},  // 2
    {1,0,1,0,1,1,1},  // 3
    {0,1,1,0,0,1,1},  // 4
    {1,1,1,0,1,0,1},  // 5
    {0,1,1,1,1,0,1},  // 6
    {1,0,0,0,0,1,1},  // 7
    {1,1,1,1,1,1,1},  // 8
    {1,1,1,0,0,1,1},  // 9
    {1,1,1,1,1,0,0},  // E
};

// Digit pixel map {x, y, width, height} for each segment
uint16_t sev_seg0[7][4] = {
    { 3, 0,21, 3},
    { 0, 3, 3,32},
    { 3,35,21, 3},
    { 0,38, 3,29},
    { 3,67,21, 3},
    {24, 3, 3,32},
    {24,38, 3,29}
};

// Draw a number at a given (x, y) position with (0,0) at top left
void drawNumber(int value, uint16_t x_offset, uint16_t y_offset, uint16_t color)
{
    if (one_error | ten_error) {
        error_flag = true;
        headerFooter(">>>>>> ERROR <<<<<<", "Check serial monitor!", error_color);
    } else if (error_flag) {
        error_flag = false;
        clearHeaderFooter();
        headerFooter();
    }

    for (int i = 0; i < 7; i++) {
        myScreen.dRectangle(sev_seg0[i][0] + x_offset,
                            sev_seg0[i][1] + y_offset,
                            sev_seg0[i][2],
                            sev_seg0[i][3],
                            num_seg[value][i] ? color : blackColour);
    }
}

// Output the tens digit to the seven segment display
void setTens(int value)
{
    uint16_t x_offset = 24;
    uint16_t y_offset = 29;
    if (value > 9) {
        ten_error = true;
        Serial.print("[ERROR]: Tens digit value of ");
        Serial.print(value);
        Serial.println(" is outside of expected range (0-9)!");
        drawNumber(10, x_offset, y_offset, error_color);
    } else {
        ten_error = false;
        drawNumber(value, x_offset, y_offset, digit_color);
    }
}

// Output the ones digit to the seven segment display
void setOnes(int value)
{
    uint16_t x_offset = 72;
    uint16_t y_offset = 29;
    if (value > 9) {
        one_error = true;
        Serial.print("[ERROR]: Ones digit value of ");
        Serial.print(value);
        Serial.println(" is outside of expected range (0-9)!");
        drawNumber(10, x_offset, y_offset, error_color);
    } else {
        one_error = false;
        drawNumber(value, x_offset, y_offset, digit_color);
    }
}

void headerFooter(String header_text, String footer_text, uint16_t color)
{
    uint16_t header_x = (myScreen.screenSizeX() - myScreen.fontSizeX() * header_text.length()) / 2;
    uint16_t footer_x = (myScreen.screenSizeX() - myScreen.fontSizeX() * footer_text.length()) / 2;

    myScreen.gText(header_x, 0, header_text, color);
    myScreen.gText(footer_x, myScreen.screenSizeY() - myScreen.fontSizeY() - 1, footer_text, color);
    myScreen.dLine(0, myScreen.fontSizeY() + 2, myScreen.screenSizeX(), 1, color);
    myScreen.dLine(0, myScreen.screenSizeY() - myScreen.fontSizeY() - 3, myScreen.screenSizeX(), 1, color);
}

// Clears the header and footer area (only needed when going from error to regular)
void clearHeaderFooter()
{
    myScreen.dRectangle(0, 0, myScreen.screenSizeX(), myScreen.fontSizeY(), blackColour);
    myScreen.dRectangle(0, myScreen.screenSizeY() - myScreen.fontSizeY() - 1,
                        myScreen.screenSizeX(), myScreen.fontSizeY(), blackColour);
}

void infoScreen()
{
    myScreen.clear(cyanColour);

    String line1 = " JoyDigit ";
    String line2 = " 7-Segment Demo ";
    uint16_t t1_width = myScreen.fontSizeX() * line1.length();
    uint16_t t2_width = myScreen.fontSizeX() * line2.length();
    myScreen.dLine((myScreen.screenSizeX() - t1_width) / 2, 4, t1_width, 1, blueColour);
    myScreen.gText((myScreen.screenSizeX() - t1_width) / 2, 5, line1, whiteColour, blueColour);
    myScreen.gText((myScreen.screenSizeX() - t2_width) / 2, 5 + myScreen.fontSizeY() * 1, line2, whiteColour, blueColour);

    String line4_bot = " Created by: ";
    String line3_bot = " Qilin Si ";
    String line2_bot = " & ";
    String line1_bot = " Max Steele ";
    uint16_t b1_width = myScreen.fontSizeX() * line1_bot.length();
    uint16_t b2_width = myScreen.fontSizeX() * line2_bot.length();
    uint16_t b3_width = myScreen.fontSizeX() * line3_bot.length();
    uint16_t b4_width = myScreen.fontSizeX() * line4_bot.length();
    myScreen.gText((myScreen.screenSizeX() - b1_width) / 2, myScreen.screenSizeY() - (5 + myScreen.fontSizeY() * 1), line1_bot, whiteColour, blueColour);
    myScreen.gText((myScreen.screenSizeX() - b2_width) / 2, myScreen.screenSizeY() - (5 + myScreen.fontSizeY() * 2), line2_bot, whiteColour, blueColour);
    myScreen.gText((myScreen.screenSizeX() - b3_width) / 2, myScreen.screenSizeY() - (5 + myScreen.fontSizeY() * 3), line3_bot, whiteColour, blueColour);
    myScreen.gText((myScreen.screenSizeX() - b4_width) / 2, myScreen.screenSizeY() - (5 + myScreen.fontSizeY() * 4), line4_bot, whiteColour, blueColour);
    myScreen.dLine((myScreen.screenSizeX() - b4_width) / 2, myScreen.screenSizeY() - (5 + myScreen.fontSizeY() * 4) - 1, b4_width, 1, blueColour);
}

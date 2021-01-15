// File for writing text to the OLED screen, borrowed from https://os.mbed.com/users/star297/code/ssd1331//file/4385fd242db0/ssd1331.cpp/
#include <stdint.h>

// Screen Settings
#define width   96-1        // Max X axial direction in screen
#define height  64-1        // Max Y axial direction in screen
#define Set_Column_Address  0x15
#define Set_Row_Address     0x75
#define contrastA           0x81
#define contrastB           0x82
#define contrastC           0x83
#define display_on          0xAF
#define display_off         0xAE

// Internal Font Size Settings
#define NORMAL  0
#define WIDE    1
#define HIGH    2
#define WH      3
#define WHx36   4
#define X_width 6 
#define Y_height 8 

// GAC hardware acceleration commands
#define GAC_DRAW_LINE           0x21    // Draw Line
#define GAC_DRAW_RECTANGLE      0x22    // Rectangle
#define GAC_COPY_AREA           0x23    // Copy Area
#define GAC_DIM_WINDOW          0x24    // Dim Window
#define GAC_CLEAR_WINDOW        0x25    // Clear Window
#define GAC_FILL_ENABLE_DISABLE 0x26    // Enable Fill
#define SCROLL_SETUP            0x27    // Setup scroll
#define SCROLL_STOP             0x2E    // Scroll Stop
#define SCROLL_START            0x2F    // Scroll Start

// Basic RGB color definitions         RED GREEN BLUE values  
#define Black           0x0000      //   0,   0,   0 
#define LightGrey       0xC618      // 192, 192, 192 
#define DarkGrey        0x7BEF      // 128, 128, 128 
#define Red             0xF800      // 255,   0,   0 
#define Green           0x07E0      //   0, 255,   0 
#define Cyan            0x07FF      //   0, 255, 255 
#define Blue            0x001F      //   0,   0, 255 
#define Magenta         0xF81F      // 255,   0, 255 
#define Yellow          0xFFE0      // 255, 255,   0 
#define White           0xFFFF      // 255, 255, 255 

void pixel(uint8_t x,uint8_t y, uint16_t color); // place a pixel x,y coordinates, color
void locate(uint8_t column, uint8_t row); // text start position x,y
void foreground(uint16_t color); // set text color
void background(uint16_t color); // set background color
void SetFontSize(uint8_t); // set internal font size NORMAL, HIGH, WIDE, WH (high and wide), WHx36 (large 36x36 pixel size)
uint16_t toRGB(uint16_t R,uint16_t G,uint16_t B);   // get color from RGB values 00~FF(0~255)

// Stream implementation functions
void FontSizeConvert(int *lpx, int *lpy);
void PutChar(uint8_t x,uint8_t y,int a);
uint16_t Char_Color;    // text color
uint16_t BGround_Color; // background color

// Window location

uint8_t		char_x;
uint8_t		char_y;
uint8_t		chr_size;

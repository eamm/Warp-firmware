#include <stdint.h>

#include "fsl_spi_master_driver.h"
#include "fsl_port_hal.h"

//#include "devTextSSD1331.h"

#include "SEGGER_RTT.h"
#include "gpio_pins.h"
#include "warp.h"
#include "devSSD1331.h"


volatile uint8_t	inBuffer[1];
volatile uint8_t	payloadBytes[1];


/*
 *	Override Warp firmware's use of these pins and define new aliases.
 */
enum
{
	kSSD1331PinMOSI		= GPIO_MAKE_PIN(HW_GPIOA, 8),
	kSSD1331PinSCK		= GPIO_MAKE_PIN(HW_GPIOA, 9),
	kSSD1331PinCSn		= GPIO_MAKE_PIN(HW_GPIOB, 13),
	kSSD1331PinDC		= GPIO_MAKE_PIN(HW_GPIOA, 12),
	kSSD1331PinRST		= GPIO_MAKE_PIN(HW_GPIOB, 0),
};

int
writeCommand(uint8_t commandByte)
{
	spi_status_t status;

	/*
	 *	Drive /CS low.
	 *
	 *	Make sure there is a high-to-low transition by first driving high, delay, then drive low.
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinCSn);
	OSA_TimeDelay(10);
	GPIO_DRV_ClearPinOutput(kSSD1331PinCSn);

	/*
	 *	Drive DC low (command).
	 */
	GPIO_DRV_ClearPinOutput(kSSD1331PinDC);

	payloadBytes[0] = commandByte;
	status = SPI_DRV_MasterTransferBlocking(0	/* master instance */,
					NULL		/* spi_master_user_config_t */,
					(const uint8_t * restrict)&payloadBytes[0],
					(uint8_t * restrict)&inBuffer[0],
					1		/* transfer size */,
					1000		/* timeout in microseconds (unlike I2C which is ms) */);

	/*
	 *	Drive /CS high
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinCSn);

	return status;
}

/*void displayText(char c)
{
	//Clear Screen
	writeCommand(kSSD1331CommandCLEAR);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x5F);
	writeCommand(0x3F);

	uint8_t char_x = 0;
	uint8_t char_y = 0;

	foreground(toRGB(0,255,0));
	background(toRGB(255,0,0));
	PutChar(char_x, char_y, c);
}*/

// Fill the screen with a red colour

void
redFill(void)
{
	// Clear Screen
	writeCommand(kSSD1331CommandCLEAR);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x5F);
	writeCommand(0x3F);

	// Fill with Red
	writeCommand(0x22);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x5F);
	writeCommand(0x3F);
	writeCommand(0x1C);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x1C);
	writeCommand(0x00);
	writeCommand(0x00);
}

// Function that draws lines on the OLED display

void 
drawLine(int colStart, int rowStart, int colEnd, int rowEnd)
{
	writeCommand(0x21);
	writeCommand(colStart);
	writeCommand(rowStart);
	writeCommand(colEnd);
	writeCommand(rowEnd);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x1C);
}


// Function for drawing characters to the OLED screen

void
drawChar(int startCol, int startRow, char chr)
{
	switch (chr) {
		case 'm':
			drawLine(startCol, startRow+0x07, startCol+0x06, startRow+0x07);
			drawLine(startCol, startRow+0x08, startCol, startRow + 0x0D);
			drawLine(startCol+0x03, startRow+0x08, startCol+0x03, startRow+0x0D);
			drawLine(startCol+0x06, startRow+0x08, startCol+0x06, startRow+0x0D);
			break;
		case 'e':
			drawLine(startCol, startRow+0x07, startCol+0x06, startRow+0x07);
			drawLine(startCol+0x06, startRow+0x08, startCol+0x06, startRow+0x0A);
			drawLine(startCol+0x05, startRow+0x0A, startCol+0x01, startRow+0x0A);
			drawLine(startCol, startRow+0x08, startCol, startRow+0x0D);
			drawLine(startCol+0x01, startRow+0x0D, startCol+0x06, startRow+0x0D);
			break;
		case 'T':
			drawLine(startCol, startRow, startCol+0x06, startRow);
			drawLine(startCol+0x03, startRow+0x01, startCol+0x03, startRow+0x0D);
			break;
		case 'p':
			drawLine(startCol, startRow+0x07, startCol, startRow+0x11);
			drawLine(startCol+0x01, startRow+0x07, startCol+0x06, startRow+0x07);
			drawLine(startCol+0x06, startRow+0x08, startCol+0x06, startRow+0x0D);
			drawLine(startCol+0x05, startRow+0x0D, startCol+0x01, startRow+0x0D);
			break;
		case 'r':
			drawLine(startCol, startRow+0x07, startCol + 0x06, startRow+0x07);
			drawLine(startCol, startRow+0x08, startCol, startRow+0x0D);
			break;
		case 'a':
			drawLine(startCol+0x05, startRow+0x0A, startCol, startRow+0x0A);
			drawLine(startCol, startRow+0x0A, startCol, startRow+0x0D);
			drawLine(startCol, startRow+0x0D, startCol+0x06, startRow+0x0D);
			drawLine(startCol+0x06, startRow+0x0D, startCol+0x06, startRow+0x07);
			drawLine(startCol+0x05, startRow+0x07, startCol, startRow+0x07);
			break;
		case 't':
			drawLine(startCol+0x03, startRow+0x04, startCol+0x03, startRow+0x0D);
			drawLine(startCol+0x01, startRow+0x07, startCol+0x05, startRow+0x07);
			break;
		case 'u':
			drawLine(startCol, startRow+0x07, startCol, startRow+0x0D);
			drawLine(startCol+0x01, startRow+0x0D, startCol+0x06, startRow+0x0D);
			drawLine(startCol+0x06, startRow+0x0C, startCol+0x06, startRow+0x07);
			break;
		case '1':
			drawLine(startCol+0x06, startRow, startCol+0x06, startRow+0x0D);
			break;
		case '2':
			drawLine(startCol, startRow, startCol+0x06, startRow);
			drawLine(startCol+0x06, startRow+0x01, startCol+0x06, startRow+0x06);
			drawLine(startCol+0x05, startRow+0x06, startCol, startRow+0x06);
			drawLine(startCol, startRow+0x07, startCol, startRow+0x0D);
			drawLine(startCol+0x01, startRow+0x0D, startCol+0x06, startRow+0x0D);
			break;
		case '3':
			drawLine(startCol+0x06, startRow, startCol+0x06, startRow+0x0D);
			drawLine(startCol+0x05, startRow, startCol, startRow);
			drawLine(startCol, startRow+0x06, startCol+0x05, startRow+0x06);
			drawLine(startCol, startRow+0x0D, startCol+0x05, startRow+0x0D);
			break;
		case '4':
			drawLine(startCol+0x06, startRow, startCol+0x06, startRow+0x0D);
			drawLine(startCol+0x05, startRow+0x06, startCol, startRow+0x06);
			drawLine(startCol, startRow, startCol, startRow+0x06);
			break;
		case '5':
			drawLine(startCol, startRow, startCol+0x06, startRow);
			drawLine(startCol, startRow+0x06, startCol+0x06, startRow+0x06);
			drawLine(startCol, startRow+0x0D, startCol+0x06, startRow+0x0D);
			drawLine(startCol+0x06, startRow+0x07, startCol+0x06, startRow+0x0D);
			drawLine(startCol, startRow+0x01, startCol, startRow+0x05);
			break;
		case '6':
			drawLine(startCol, startRow, startCol, startRow+0x0D);
			drawLine(startCol+0x06, startRow+0x07, startCol+0x06, startRow+0x0C);
			drawLine(startCol+0x01, startRow+0x06, startCol+0x06, startRow+0x06);
			drawLine(startCol+0x01, startRow, startCol+0x06, startRow);
			drawLine(startCol+0x01, startRow+0x0D, startCol+0x06, startRow+0x0D);
			break;
		case '7':
			drawLine(startCol, startRow, startCol+0x06, startRow);
			drawLine(startCol+0x06, startRow+0x01, startCol+0x06, startRow+0x0D);
			break;
		case'8':
			drawLine(startCol+0x01, startRow, startCol+0x05, startRow);
			drawLine(startCol+0x01, startRow+0x06, startCol+0x05, startRow+0x06);
			drawLine(startCol+0x01, startRow+0x0D, startCol+0x05, startRow+0x0D);
			drawLine(startCol, startRow, startCol, startRow+0x0D);
			drawLine(startCol+0x06, startRow, startCol+0x06, startRow+0x0D);
			break;
		case '9':
			drawLine(startCol+0x01, startRow, startCol+0x05, startRow);
			drawLine(startCol+0x01, startRow+0x06, startCol+0x05, startRow+0x06);
			drawLine(startCol+0x01, startRow+0x0D, startCol+0x05, startRow+0x0D);
			drawLine(startCol, startRow, startCol, startRow+0x06);
			drawLine(startCol+0x06, startRow, startCol+0x06, startRow+0x0D);
			break;
		case '0':
			drawLine(startCol+0x01, startRow, startCol+0x05, startRow);
			drawLine(startCol+0x01, startRow+0x0D, startCol+0x05, startRow+0x0D);
			drawLine(startCol, startRow, startCol, startRow+0x0D);
			drawLine(startCol+0x06, startRow, startCol+0x06, startRow+0x0D);
			break;
		case ':':
			drawLine(startCol, startRow+0x07, startCol, startRow+0x07);
			drawLine(startCol, startRow+0x0D, startCol, startRow+0x0D);
			break;
		case 'C':
			drawLine(startCol, startRow, startCol, startRow+0x0D);
			drawLine(startCol+0x01, startRow, startCol+0x06, startRow);
			drawLine(startCol+0x01, startRow+0x0D, startCol+0x06, startRow+0x0D);
			break;
		case '^':
			drawLine(startCol+0x04, startRow, startCol+0x06, startRow);
			drawLine(startCol+0x04, startRow+0x02, startCol+0x06, startRow+0x02);
			drawLine(startCol+0x04, startRow, startCol+0x04, startRow+0x02);
			drawLine(startCol+0x06, startRow, startCol+0x06, startRow+0x02);
			break;
		case 'H':
			drawLine(startCol, startRow, startCol, startRow+0x0D);
			drawLine(startCol+0x06, startRow, startCol+0x06, startRow+0x0D);
			drawLine(startCol, startRow+0x06, startCol+0x06, startRow+0x06);
			break;
		case 'i':
			drawLine(startCol+0x03, startRow+0x07, startCol+0x03, startRow+0x0D);
			drawLine(startCol+0x03, startRow+0x05, startCol+0x03, startRow+0x05);
			break;
		case 'd':
			drawLine(startCol+0x06, startRow, startCol+0x06, startRow+0x0D);
			drawLine(startCol+0x06, startRow+0x07, startCol, startRow+0x07);
			drawLine(startCol, startRow+0x07, startCol, startRow+0x0D);
			drawLine(startCol, startRow+0x0D, startCol+0x06, startRow+0x0D);
			break;
		case 'y':
			drawLine(startCol, startRow+0x07, startCol, startRow+0x0D);
			drawLine(startCol, startRow+0x0D, startCol+0x06, startRow+0x0D);
			drawLine(startCol+0x06, startRow+0x07, startCol+0x06, startRow+0x11);
			drawLine(startCol+0x06, startRow+0x11, startCol, startRow+0x11);
			break;
		case '.':
			drawLine(startCol, startRow+0x0C, startCol, startRow+0x0D);
			drawLine(startCol+0x01, startRow+0x0C, startCol+0x01, startRow+0x0D);
			break;
		case '%':
			drawLine(startCol, startRow, startCol+0x02, startRow);
			drawLine(startCol, startRow+0x02, startCol+0x02, startRow+0x02);
			drawLine(startCol, startRow, startCol, startRow+0x02);
			drawLine(startCol+0x02, startRow, startCol+0x02, startRow+0x02);

			drawLine(startCol+0x04, startRow+0x0B, startCol+0x06, startRow+0x0B);
			drawLine(startCol+0x04, startRow+0x0D, startCol+0x06, startRow+0x0D);
			drawLine(startCol+0x04, startRow+0x0B, startCol+0x04, startRow+0x0D);
			drawLine(startCol+0x06, startRow+0x0B, startCol+0x06, startRow+0x0D);

			drawLine(startCol, startRow+0x0D, startCol+0x06, startRow);
			break;
		case ' ':
			break;
		case '-':
			drawLine(startCol, startRow+0x07, startCol+0x06, startRow+0x07);
			break;
		case 'G':
			drawLine(startCol, startRow, startCol+0x06, startRow);
			drawLine(startCol, startRow+0x01, startCol, startRow+0x0D);
			drawLine(startCol+0x01, startRow+0x0D, startCol+0x06, startRow+0x0D);
			drawLine(startCol+0x06, startRow+0x0D, startCol+0x06, startRow+0x07);
			drawLine(startCol+0x06, startRow+0x07, startCol+0x03, startRow+0x07);
			break;
		case 's':
			drawLine(startCol, startRow+0x07, startCol+0x06, startRow+0x07);
			drawLine(startCol, startRow+0x08, startCol, startRow+0x0A);
			drawLine(startCol, startRow+0x0A, startCol+0x06, startRow+0x0A);
		        drawLine(startCol+0x06, startRow+0x0A, startCol+0x06, startRow+0x0D);
			drawLine(startCol+0x06, startRow+0x0D, startCol, startRow+0x0D);
			break;
		case 'P':
			drawLine(startCol, startRow, startCol, startRow+0x0D);
			drawLine(startCol, startRow, startCol+0x06, startRow);
			drawLine(startCol+0x06, startRow, startCol+0x06, startRow+0x06);
			drawLine(startCol+0x06, startRow+0x06, startCol, startRow+0x06);
			break;
		case 'k':
			drawLine(startCol, startRow, startCol, startRow+0x0D);
			drawLine(startCol, startRow+0x0A, startCol+0x06, startRow+0x07);
			drawLine(startCol, startRow+0x0A, startCol+0x06, startRow+0x0D);
			break;
	}
}

// Rectangle to delete old values displayed on the OLED screen so that they can be replaced with new values

void
resetRect(int startCol,int startRow, int endCol, int endRow, int blue, int green, int red)
{
	writeCommand(0x22);
	writeCommand(startCol);
	writeCommand(startRow);
	writeCommand(endCol);
	writeCommand(endRow);
	writeCommand(blue);
	writeCommand(green);
	writeCommand(red);
	writeCommand(blue);
	writeCommand(green);
	writeCommand(red);
}

// Initialise the display with the appropriate characters and values

void
displayInit(void)
{
	int startCol = 0x01;
	int startRow = 0x01;
	char tempArray[] = "Temp:";
	for (int i=0;i<sizeof tempArray;i++)
	{
		drawChar(startCol, startRow, tempArray[i]);
		startCol+=0x08;
	}
	char valueArray[] = "00^C";
	startCol = 0x24;
	for (int i=0;i<sizeof valueArray;i++)
	{
		drawChar(startCol, startRow, valueArray[i]);
		startCol+=0x08;
	}
	startCol = 0x01;
	startRow = 0x11;
	char humArray[] = "Hum:";
	for (int i=0;i<sizeof humArray;i++)
	{
		drawChar(startCol, startRow, humArray[i]);
		startCol+=0x08;
	}
	char humValue[] = "00%";
	startCol = 0x24;
	for (int i=0;i<sizeof humValue;i++)
	{
		drawChar(startCol, startRow, humValue[i]);
		startCol+=0x08;
	}
	startCol = 0x01;
	startRow = 0x21;
	char gasArray[] = "Gas:";
	for (int i=0;i<sizeof gasArray;i++)
	{
		drawChar(startCol, startRow, gasArray[i]);
		startCol+=0x08;
	}
	char gasValue[] = "000";
       	startCol = 0x24;
	for (int i=0;i<sizeof gasValue;i++)
	{
		drawChar(startCol, startRow, gasValue[i]);
		startCol+=0x08;
	}
	startCol = 0x01;
	startRow = 0x31;
	char presArray[] = "Pres:";
	for (int i=0;i<sizeof presArray;i++)
	{
		drawChar(startCol, startRow, presArray[i]);
		startCol+=0x08;
	}
	startCol = 0x24;
	char presValue[] = "000kPa";
	for (int i=0;i<sizeof presValue;i++)
	{
		drawChar(startCol, startRow, presValue[i]);
		startCol+=0x08;
	}
}

// Fill the screen with a green colour

void
greenFill(void)
{
	// Clear Screen
	writeCommand(kSSD1331CommandCLEAR);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x5F);
	writeCommand(0x3F);

	// Fill screen with Green
	writeCommand(0x22);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x5F);
	writeCommand(0x3F);
	writeCommand(0x00);
	writeCommand(0x1C);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x1C);
	writeCommand(0x00);
}

int
devSSD1331init(void)
{
	/*
	 *	Override Warp firmware's use of these pins.
	 *
	 *	Re-configure SPI to be on PTA8 and PTA9 for MOSI and SCK respectively.
	 */
	PORT_HAL_SetMuxMode(PORTA_BASE, 8u, kPortMuxAlt3);
	PORT_HAL_SetMuxMode(PORTA_BASE, 9u, kPortMuxAlt3);

	enableSPIpins();

	/*
	 *	Override Warp firmware's use of these pins.
	 *
	 *	Reconfigure to use as GPIO.
	 */
	PORT_HAL_SetMuxMode(PORTB_BASE, 13u, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTA_BASE, 12u, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTB_BASE, 0u, kPortMuxAsGpio);


	/*
	 *	RST high->low->high.
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);
	GPIO_DRV_ClearPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);
	GPIO_DRV_SetPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);

	/*
	 *	Initialization sequence, borrowed from https://github.com/adafruit/Adafruit-SSD1331-OLED-Driver-Library-for-Arduino
	 */
	writeCommand(kSSD1331CommandDISPLAYOFF);	// 0xAE
	writeCommand(kSSD1331CommandSETREMAP);		// 0xA0
	writeCommand(0x72);				// RGB Color
	writeCommand(kSSD1331CommandSTARTLINE);		// 0xA1
	writeCommand(0x0);
	writeCommand(kSSD1331CommandDISPLAYOFFSET);	// 0xA2
	writeCommand(0x0);
	writeCommand(kSSD1331CommandNORMALDISPLAY);	// 0xA4
	writeCommand(kSSD1331CommandSETMULTIPLEX);	// 0xA8
	writeCommand(0x3F);				// 0x3F 1/64 duty
	writeCommand(kSSD1331CommandSETMASTER);		// 0xAD
	writeCommand(0x8E);
	writeCommand(kSSD1331CommandPOWERMODE);		// 0xB0
	writeCommand(0x0B);
	writeCommand(kSSD1331CommandPRECHARGE);		// 0xB1
	writeCommand(0x31);
	writeCommand(kSSD1331CommandCLOCKDIV);		// 0xB3
	writeCommand(0xF0);				// 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
	writeCommand(kSSD1331CommandPRECHARGEA);	// 0x8A
	writeCommand(0x64);
	writeCommand(kSSD1331CommandPRECHARGEB);	// 0x8B
	writeCommand(0x78);
	writeCommand(kSSD1331CommandPRECHARGEA);	// 0x8C
	writeCommand(0x64);
	writeCommand(kSSD1331CommandPRECHARGELEVEL);	// 0xBB
	writeCommand(0x3A);
	writeCommand(kSSD1331CommandVCOMH);		// 0xBE
	writeCommand(0x3E);
	writeCommand(kSSD1331CommandMASTERCURRENT);	// 0x87
	writeCommand(0x06);
	writeCommand(kSSD1331CommandCONTRASTA);		// 0x81
	writeCommand(0x91);
	writeCommand(kSSD1331CommandCONTRASTB);		// 0x82
	writeCommand(0xFF);
	writeCommand(kSSD1331CommandCONTRASTC);		// 0x83
	writeCommand(0x7D);
	writeCommand(kSSD1331CommandDISPLAYON);		// Turn on oled panel

	/*
	 *	To use fill commands, you will have to issue a command to the display to enable them. See the manual.
	 */
	writeCommand(kSSD1331CommandFILL);
	writeCommand(0x01);
	/*
	 *	Clear Screen
	 */
	writeCommand(kSSD1331CommandCLEAR);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x5F);
	writeCommand(0x3F);



	/*
	 *	Any post-initialization drawing commands go here.
	 */
	/*writeCommand(0x22);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x5F);
	writeCommand(0x3F);
	writeCommand(0x00);
	writeCommand(0x1C);
	writeCommand(0x00);
	writeCommand(0x00);
	writeCommand(0x1C);
	writeCommand(0x00);*/

	return 0;
}

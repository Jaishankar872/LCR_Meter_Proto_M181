#include "m181_display_softwire.h"

// Private Includes
#include "stdio.h"
#include "ssd1306.h"

// Private Variable Declaration
uint16_t counter_display = 0;
char buffer_display[20];

// Function Definition
void ssd1306_display_sofwire_Init(void)
{
    // Initialize I2C
    DWT_Delay_Init();
    I2C_init();

    // Initialize SSD1306
    ssd1306_Init();
    // Clear the display
    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();

    // Write data to local screenbuffer
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("M181 LCR", Font_11x18, White);

    ssd1306_SetCursor(0, 42);
    ssd1306_WriteString("JAI", Font_11x18, White);

    // Draw rectangle on screen
    for (uint8_t i = 0; i < 28; i++)
    {
        for (uint8_t j = 0; j < 64; j++)
        {
            ssd1306_DrawPixel(100 + i, 0 + j, White);
        }
    }

    // Copy all data from local screenbuffer to the screen
    ssd1306_UpdateScreen();

    // Update the display
    ssd1306_UpdateScreen();
}

void print_home_screen(int _frequency)
{
    ssd1306_SetCursor(20, 23);
    if (_frequency >= 1000)
        sprintf(buffer_display, "Freq->%d", _frequency);
    else
        sprintf(buffer_display, "Freq->%d ", _frequency);
    ssd1306_WriteString(buffer_display, Font_7x10, White);
    // Update the display
    ssd1306_UpdateScreen();
}

// End of File
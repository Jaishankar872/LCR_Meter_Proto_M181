/**
 * @file m181_display_softwire.c
 * @brief SSD1306 display driver using software I2C
 * @details This is a SSD1306 display driver using software I2C
 * @date 2021-07-15
 *
 * @author Jaishankar M
 */

#include "m181_display_softwire.h"

// Private Includes
#include "stdio.h"
#include "ssd1306.h"

// Private Variable Declaration
#define I2C_SDA_Pin GPIO_PIN_3
#define I2C_SDA_GPIO_Port GPIOA
#define I2C_SCL_Pin GPIO_PIN_4
#define I2C_SCL_GPIO_Port GPIOA

uint16_t counter_display = 0;
char buffer_display[20];
uint8_t _screen1_print_one_time = 0;

// Private Function Declaration
void software_I2C_GPIO_init();

// Function Definition
void ssd1306_display_sofwire_Init(void)
{
    // Initialize Software I2C
    software_I2C_GPIO_init();
    DWT_Delay_Init();
    I2C_init();

    // Initialize SSD1306
    ssd1306_Init();
    // Clear the display
    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();

    // Setting the one time print flag
    _screen1_print_one_time = 1;
}

void screen1_home_print(system_data _display)
{
    // Line number variable declaration
    uint8_t _line_spacing_y = 11;
    uint8_t _line1_position_y = 8 + _line_spacing_y;
    uint8_t _line2_position_y = _line1_position_y + _line_spacing_y;
    uint8_t _line3_position_y = _line2_position_y + _line_spacing_y;
    uint8_t _line4_position_y = _line3_position_y + _line_spacing_y;

    uint8_t _offset_position_x = 5;
    uint8_t _value_position_x = 48;
    uint8_t _unit_position_x = 85;

    if (_screen1_print_one_time)
    {
        _screen1_print_one_time = 0; // Clear the flag
                                     // Write data to local screenbuffer
        ssd1306_SetCursor(_offset_position_x, 0);
        ssd1306_WriteString("LCR Ser", Font_11x18, White);

        ssd1306_SetCursor(92, 0);
        sprintf(buffer_display, "V%.2f", fw_version);
        ssd1306_WriteString(buffer_display, Font_7x10, White);

        ssd1306_SetCursor(_offset_position_x, _line1_position_y);
        ssd1306_WriteString("Freq :", Font_7x10, White);
        ssd1306_SetCursor(_unit_position_x, _line1_position_y);
        ssd1306_WriteString("Hz", Font_7x10, White);

        ssd1306_SetCursor(_offset_position_x, _line2_position_y);
        ssd1306_WriteString("UART :", Font_7x10, White);
        ssd1306_SetCursor(_unit_position_x, _line2_position_y);
        ssd1306_WriteString("**", Font_7x10, White);

        ssd1306_SetCursor(_offset_position_x, _line3_position_y);
        ssd1306_WriteString("Volt :", Font_7x10, White);
        ssd1306_SetCursor(_unit_position_x + 2, _line3_position_y);
        ssd1306_WriteString("V", Font_7x10, White);

        ssd1306_SetCursor(_offset_position_x, _line4_position_y);
        ssd1306_WriteString(" Amp :", Font_7x10, White);
        ssd1306_SetCursor(_unit_position_x + 2, _line4_position_y);
        ssd1306_WriteString("V", Font_7x10, White);
        // Update the display
        ssd1306_UpdateScreen();
    }
    // Write Frequency value
    ssd1306_SetCursor(_value_position_x, _line1_position_y);
    if (_display.set_freq >= 1000)
        sprintf(buffer_display, "%d", _display.set_freq);
    else
        sprintf(buffer_display, "%d ", _display.set_freq);
    ssd1306_WriteString(buffer_display, Font_7x10, White);

    // Write UART Mode
    ssd1306_SetCursor(_value_position_x, _line2_position_y);
    if (_display.uart_all_print_DSO)
        sprintf(buffer_display, "ON ");
    else
        sprintf(buffer_display, "OFF");
    ssd1306_WriteString(buffer_display, Font_7x10, White);

    // Write Volt
    ssd1306_SetCursor(_value_position_x, _line3_position_y);
    sprintf(buffer_display, "%.3f", _display.pk_pk_voltage);
    ssd1306_WriteString(buffer_display, Font_7x10, White);

    // Write Amp
    ssd1306_SetCursor(_value_position_x, _line4_position_y);
    sprintf(buffer_display, "%.3f", _display.pk_pk_current);
    ssd1306_WriteString(buffer_display, Font_7x10, White);

    // Update the display
    ssd1306_UpdateScreen();
}

void software_I2C_GPIO_init()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /*Configure Software I2C pins : I2C_SDA_Pin I2C_SCL_Pin */
    GPIO_InitStruct.Pin = I2C_SDA_Pin | I2C_SCL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOA, I2C_SDA_Pin | I2C_SCL_Pin, GPIO_PIN_RESET);
}

// End of File
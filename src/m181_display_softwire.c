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

// Available Font Size
// Font_7x10, Font_11x18, Font_16x26
// Font_11x18 - Big
// Font_7x10  - Small General

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
    /*
     * Display Screen Template
     * 1. <Mode> <Frequency> <FW Version>
     * 2. Z = <Impeadance> <Phase>
     * 3. <Voltage> <Current> <Status>
     * 4. <ESR> <Tan Delta> <>
     */

    // Define positions (constants) for readability
    const uint8_t offset_x = 1, line_spacing_y = 13;

    const uint8_t line1_y = 0;
    const uint8_t val1_x = offset_x, val2_x = 36, val3_x = 59, val4_x = 92;

    const uint8_t line2_y = line_spacing_y;
    const uint8_t val21_x = offset_x, val22_x = 22, val23_x = 84, val24_x = 92;

    const uint8_t line3_y = 8 + line2_y + line_spacing_y;
    const uint8_t val31_x = offset_x, val32_x = 16, val33_x = 58, val34_x = 70, val35_x = 110;

    const uint8_t line4_y = line3_y + line_spacing_y;
    const uint8_t val41_x = offset_x, val42_x = 20, val43_x = 62, val44_x = 75, val45_x = 116;

    if (_screen1_print_one_time)
    {
        _screen1_print_one_time = 0; // Clear one-time flag
        // Line 1
        ssd1306_SetCursor(val1_x, line1_y);
        ssd1306_WriteString("Ser", Font_7x10, White);
        ssd1306_SetCursor(val3_x, line1_y);
        ssd1306_WriteString("kHz", Font_7x10, White);
        ssd1306_SetCursor(val4_x, line1_y);
        sprintf(buffer_display, "V%.2f", fw_version);
        ssd1306_WriteString(buffer_display, Font_7x10, White);
        // Line 2
        ssd1306_SetCursor(val21_x, line2_y);
        ssd1306_WriteString("Z ", Font_11x18, White);
        ssd1306_SetCursor(val23_x, line2_y + 4);
        ssd1306_WriteString("<", Font_7x10, White);
        // Line 3
        ssd1306_SetCursor(val31_x, line3_y);
        ssd1306_WriteString("V:", Font_7x10, White);
        ssd1306_SetCursor(val33_x, line3_y);
        ssd1306_WriteString("A:", Font_7x10, White);
        // Line 4
        ssd1306_SetCursor(val41_x, line4_y);
        ssd1306_WriteString("ER:", Font_7x10, White);
        ssd1306_SetCursor(val43_x, line4_y);
        ssd1306_WriteString("D:", Font_7x10, White);
        ssd1306_UpdateScreen();
    }

    if (_display.adc_measure_status == 4)
    {
        // Line 1: Frequency display
        ssd1306_SetCursor(val2_x, line1_y);
        if (_display.set_freq == 1000)
            sprintf(buffer_display, "1.0");
        else
            sprintf(buffer_display, "%0.1f", (float)(_display.set_freq) / 1000);
        ssd1306_WriteString(buffer_display, Font_7x10, White);

        // Line 2: Impedance and Phase
        ssd1306_SetCursor(val22_x, line2_y);
        float impedance = _display.rms_voltage / _display.rms_current;
        if (impedance < 10.0)
            sprintf(buffer_display, "%.3f", impedance); // 1.234
        else
            sprintf(buffer_display, "%.2f", impedance); // 12.34
        ssd1306_WriteString(buffer_display, Font_11x18, White);
        ssd1306_SetCursor(val24_x, line2_y + 4);
        if (_display.VI_phase < 10.0)
            sprintf(buffer_display, "0%.1f ", _display.VI_phase); // adds space for 01.2 format
        else
            sprintf(buffer_display, "%.1f ", _display.VI_phase); // 12.3
        ssd1306_WriteString(buffer_display, Font_7x10, White);

        // Line 3: Voltage and Current reading
        ssd1306_SetCursor(val32_x, line3_y);
        sprintf(buffer_display, "%.3f", (_display.rms_voltage >= 0) ? _display.rms_voltage : 0.000);
        ssd1306_WriteString(buffer_display, Font_7x10, White);
        ssd1306_SetCursor(val34_x, line3_y);
        sprintf(buffer_display, "%.3f", (_display.rms_current >= 0) ? _display.rms_current : 0.000);
        ssd1306_WriteString(buffer_display, Font_7x10, White);

        // Line 4: ESR and Tan Delta
        ssd1306_SetCursor(val42_x, line4_y);
        sprintf(buffer_display, "%.3f", (_display.esr >= 0) ? _display.esr : 0.000);
        ssd1306_WriteString(buffer_display, Font_7x10, White);
        ssd1306_SetCursor(val44_x, line4_y);
        sprintf(buffer_display, "%.3f", (_display.tan_delta >= 0) ? _display.QF : 0.000);
        ssd1306_WriteString(buffer_display, Font_7x10, White);
        ssd1306_SetCursor(val45_x, line4_y);
        if (_display.uart_all_print_DSO)
            sprintf(buffer_display, "N");
        else
            sprintf(buffer_display, "F");
        ssd1306_WriteString(buffer_display, Font_7x10, White);
    }

    // Line 3: Status
    ssd1306_SetCursor(val35_x, line3_y);
    sprintf(buffer_display, "*%d", _display.adc_measure_status);
    ssd1306_WriteString(buffer_display, Font_7x10, White);
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
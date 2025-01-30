/**
 * @file m181_display_softwire.h
 * @brief SSD1306 display driver using software I2C
 * @details This is a SSD1306 display driver using software I2C
 * @date 2021-07-15
 * 
 * @author Jaishankar M
 */
#ifndef __M181_DISPLAY_SOFTWIRE_H
#define __M181_DISPLAY_SOFTWIRE_H

// Public Variable Declaration for Software I2C
#define SW_I2C_SDA_GPIO_Port GPIOA
#define SW_I2C_SDA_Pin GPIO_PIN_3
#define SW_I2C_SCL_GPIO_Port GPIOA
#define SW_I2C_SCL_Pin GPIO_PIN_4

// Public Function Declaration
void ssd1306_display_sofwire_Init(void);
void print_home_screen(int _frequency);

#endif // __M181_DISPLAY_SOFTWIRE_H
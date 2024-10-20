/*
Code is for SSD1306 128x64 display
*/
#include <SoftWire.h>
#include "ACROBOTIC_SSD1306.h"

SoftWire softWire(PA3, PA4); // your choice of SDA / SCL pins
char ibuffer[60];
bool _run1_one_time_print = 0;
system_data previous_data_display;

void setup_display(float _fw_ver);
void bootup_display();
void data_display(system_data display);

void setup_display(float _fw_ver)
{
    softWire.setRxBuffer(ibuffer, 60);
    softWire.setTxBuffer(ibuffer, 60);
    softWire.begin();
    oled.init(softWire);  // Initialze SSD1306 OLED display
    oled.clearDisplay();  // Clear screen
    oled.setTextXY(0, 0); // Set cursor position, start of line 0
    oled.putString("Bevlux");
    oled.setTextXY(0, 9);
    oled.putString("V");
    oled.putFloat(_fw_ver);

    _run1_one_time_print = 1; // Set to print one time loop
}

void bootup_display()
{
    /*Will be added later*/
}

void data_display(system_data display)
{
    if (_run1_one_time_print)
    {
        int _line_no1 = 1;
        oled.setTextXY(_line_no1, 1); // Set cursor position, start of line 0
        oled.putString("F=");
        oled.setTextXY(_line_no1, 9);
        oled.putString(" Hz");
        oled.setTextXY(_line_no1 + 1, 1); // Set cursor position, start of line 0
        oled.putString("LED ");
        oled.setTextXY(_line_no1 + 2, 1); // Set cursor position, start of line 0
        oled.putString("Mode ");
    }

    if (previous_data_display.set_freq != display.set_freq || _run1_one_time_print)
    {
        if (display.set_freq >= 1000)
            oled.setTextXY(1, 5);
        else
        {
            oled.setTextXY(1, 5);
            oled.putString(" ");
            oled.setTextXY(1, 6);
        }
        oled.putNumber(display.set_freq);
    }

    if (previous_data_display.led_state != display.led_state || _run1_one_time_print)
    {
        oled.setTextXY(2, 6);
        if (display.led_state)
            oled.putString(" ON/10");
        else
            oled.putString("OFF/1 ");
    }
    if (previous_data_display.VI_measure_mode != display.VI_measure_mode || _run1_one_time_print)
    {
        oled.setTextXY(3, 6);
        if (display.VI_measure_mode)
            oled.putString("Volt/10");
        else
            oled.putString("Amp/1    ");
    }
    _run1_one_time_print = 0;        // Clear the one run va
    previous_data_display = display; // Recording the previous data
}
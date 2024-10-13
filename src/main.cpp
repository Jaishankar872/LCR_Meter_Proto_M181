#include <Arduino.h>
#include "system_setup.h"

// Experiment Section After moved to system_setup.h
#include <Wire.h>

#include "DAC_sine_wave_gen.h"

// I2C
TwoWire Wire2(PA3, PA4);  // SDA = PA3, SCL = PA4
#define SCREEN_WIDTH1 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32  // OLED display height, in pixels

#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
//--

void setup()
{
  int_system_setup();

  // Experiment Section After moved to system_setup.h
  Wire2.begin();
  //----

  // Check the I2C Line Manual
  byte error1 = 0;
  Serial_debug.print("Screen Add 0x");
  Serial_debug.println(SCREEN_ADDRESS, HEX);
  for (int i = 1; i <= 10; i++)
  {
    Serial_debug.print("Try Count:");
    Serial_debug.println(i);
    Serial_debug.println("-----");
    Serial_debug.println("First Try");
    Wire2.beginTransmission(SCREEN_ADDRESS);
    Wire2.write(0x00); // Send a basic command (like command mode)
    Wire2.write(0xAE); // Turn OFF display (SSD1306 basic command)
    error1 = Wire2.endTransmission();
    if (error1 == 0)
      Serial_debug.println("1 Slave found at 0x3C!");
    else
    {
      Serial_debug.print("First transmission failed with error code: ");
      Serial_debug.println(error1);
    }

    Serial_debug.println("Second Try");
    Wire2.beginTransmission(SCREEN_ADDRESS);
    Wire2.write(0xAF); // Turn ON displa
    byte error2 = Wire2.endTransmission();
    if (error2 == 0)
      Serial_debug.println("2 Slave found at 0x3C!");
    else
    {
      Serial_debug.print("Second transmission failed with error code: ");
      Serial_debug.println(error2);
    }
  }
  Serial_debug.println("End!!");

  delay(5000);
  Serial_debug.println("I2C Scan Manual");
  for (uint8_t address = 0; address <= 127; address++)
  {
    Wire2.beginTransmission(address);
    bool error = Wire2.endTransmission();
    if (error == 0)
    {
      Serial_debug.print("I2C device found at address 0x");
      Serial_debug.println(address, HEX);
    }
  }
  //----
}

void loop()
{
  regular_task_loop();
}

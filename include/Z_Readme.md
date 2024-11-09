# Program File structure
project_root/
├── include/
│   ├── system_setup.h
│   └── DAC_sine_wave_gen.h
├── src/
│   ├── main.cpp
│   ├── m181_display.cpp
│   └── DAC_sine_wave_gen.cpp
└── platformio.ini

## Pin Function Mode
### VI Pin
*  LOW - Voltage Measurement Mode
* HIGH - Current Measurement Mode
### GS Pin
*  LOW - 1:1 Output 
* HIGH - 1:10 Output (Signal Reduced by 10 time) 
### Truth Table
| Function | VI   | GS   |
|----------|------|------|
| Volt     | LOW  | HIGH |
| Current  | HIGH | LOW  |

### AFC Pin
* HIGH - In M181 always same

# Signal Processing Method

RAW ADC signal processing
 1. Check any mistake in captured ADC data. If yes return as a Error.
 2. Low pass filtering.
 3. Removing the offset voltage from the data.
 4. Measure the frequency and amplitude of signal.Convert ADC value into voltage value.
 5. Put the scaling factor to convert into actual voltage and current value. 
----




# PlatformIO community Implementation reference
1. ADC timer Implentation [link](https://community.platformio.org/t/in-stm32f103-how-to-create-two-timer-interrupt-running-without-disturb-each-other/43870/2)
2. Display with non standard I2C pin [link](https://community.platformio.org/t/ssd1306-display-is-not-working-on-bluepill-board-stm32f103/43752/10)
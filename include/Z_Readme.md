# Program File structure
project_root/
├── include/
│   ├── DAC_sine_wave_gen.h
│   ├── DSP_ADC_data.h
│   ├── m181_display.cpp
│   └── system_setup.h
├── src/
│   └── main.cpp
└── platformio.ini

## Pin Function Mode
### VI Pin
*  LOW - Voltage Measurement Mode
* HIGH - Current Measurement Mode
### GS Pin
*  LOW - Discharge bus 
* HIGH - Measuring Mode
### VI Pin
| Function | VI   |
|----------|------|
| Volt     | LOW  |
| Current  | HIGH |

### AFC Pin
* INPUT Pin - As per the testing JYETech FW.
* This NOT is a output pin with neither LOW nor HIGH.

## Interrupt Scheme
### Timer 1 Interrupt
  Generator Sine Wave via digital pin PB0 to PB7.
  Timer Specs 
   - Prescaler: 2
   - Interval: (1/frequency)*(1/100)
### Timer 2 Interrupt
  VI Measurement toggle switch.
  Timer Specs 
   - Prescaler: 72000
   - Interval: 250ms
### Timer 3 Interrupt
  To trigger ADC start measurement via TGRO in ADC. 
  Timer Specs 
   - Prescaler: 8
   - Interval: (1/frequency)*(1/40)
### ADC conversion Interrupt
  To store the ADC measured data respective variable array.
  Controlled via Timer 3 Interrupt.

# Signal Processing Method

RAW ADC signal processing
 1. Copy & check any mistake in captured ADC data & Return error.
 2. Calculate Amplitude of signal, remove offset.
 3. 
 4. 
 5. 
----




# PlatformIO community Implementation reference
1. ADC timer Implentation [link](https://community.platformio.org/t/in-stm32f103-how-to-create-two-timer-interrupt-running-without-disturb-each-other/43870/2)
2. Display with non standard I2C pin [link](https://community.platformio.org/t/ssd1306-display-is-not-working-on-bluepill-board-stm32f103/43752/10)
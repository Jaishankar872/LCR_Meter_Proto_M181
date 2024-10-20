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

# Pin Function Mode
## VI Pin
*  LOW - Voltage Measurement Mode
* HIGH - Current Measurement Mode
## GS Pin
*  LOW - 1:1 Output 
* HIGH - 1:10 Output (Signal Reduced by 10 time) 
## Truth Table
| Function | VI   | GS   |
|----------|------|------|
| Volt     | LOW  | HIGH |
| Current  | HIGH | LOW  |
*------------------------*
## AFC Pin
* HIGH - In M181 always same
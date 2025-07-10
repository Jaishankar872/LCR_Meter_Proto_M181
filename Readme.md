# Goal
   To Develop only the Firmware for M181 LCR Meter made by JyeTech. Hope to provide understanding in basics of LCR Meter.

## Hardware - M181 LCR Meter 
- Official product page link [here](https://jyetech.com/m181-lcr-meter/)
- I have purchased from Banggood, buying link is [here](https://www.banggood.in/Jyetech-M181-LCR-Meter-18101K-DIY-Kit-100Hz-1KHz-Test-Frequency-High-precision-Small-Value-Inductance-Resistance-and-Capacitance-Measurement-Module-reviews-p2017117.html)

**Note**: Still this firmware under development.
# Tasks to be completed
- [x] Zero Padding function (ADC offset)
- [ ] Auto calibration option
- [ ] Improve the phase difference calculation
- [ ] Fix issue in missing to capture the button press input
- [ ] Correct the current waveform crop issue in auto gain selection. 
- [ ] Add option for Parallel calculation ($C_p$,$L_p$) 


# System Overview
## MCU programming
- Programmer → Raspberry pi debug probe (**modified)
- Interface  → SWD
- IDE        → PlatformIO
- Framework  → STM32Cube

**Programming setup image**

<img src="docs/LCR_Meter_Program_Setup.jpg" alt="image" style="width:400px;height:auto;">

## Output parameter
<img src="docs/output_fw-v0_20.jpg" alt="image" style="width:200px;height:auto;">

* Ser → Calculation mode Series or Parallel
* 1.0kHz → Frequency signal used for measurement
* V0.20 → Firmware Version
* C → Capacitance **Mode Change
* V → RMS Voltage across the DUT
* A → RMS Current flowing via DUT
* ER → Equivalent Series Resistor(ESR)
* D → Dissipation factor (or) Tan Delta

## Analysis Original JYETech Firmware
### 1. With 10nF Film as a load
 Connected 10nF Capacitor as a load then capture waveform and UART serial output

<img src="docs/measure_with_10nF_film_cap.png" alt="image" style="width:200px;height:auto;">

> Note:
> <img src="docs/uart_frame_out1.png" alt="image" style="width:150px;height:auto;">

### 2. VI/GS Pin handling

<img src="docs/ADC_scan_mode1.png" alt="image" style="width:auto;height:auto;">


***
 **Further details will be added in [Wiki page](https://docs.jaidb.in/m181_lcr/Home/)**

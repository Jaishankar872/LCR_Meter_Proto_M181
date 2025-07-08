// system_data.h
#ifndef SYSTEM_DATA_H
#define SYSTEM_DATA_H

#include <stdint.h> // For uint8_t, uint16_t, etc.

// Firmware Version Global Variable
#define fw_version 0.25

// Define the struct
typedef struct system_data
{
	uint8_t  hold_btn;
	uint8_t  sp_btn;
	uint8_t  rcl_btn;
	uint16_t set_freq;
	uint8_t  led_state;
	int8_t   VI_measure_mode;
	float    rms_voltage;
	float    rms_AFC_volt;
	float    rms_current;
	float    rms_AFC_current;
	float    voltage_phase;
	float    current_phase;
	float    VI_phase;
	float    capacitance;
	float    inductance;
	float    resistance;
	int      unit_capacitance;
	int      unit_inductance;
	int      unit_resistance;
	int      unit_esr;
	float    impedance;
	float    esr;
	float    tan_delta;
	float    QF;
	uint8_t  uart_all_print_DSO;
	uint8_t  LCR_Mode;
	uint8_t  adc_measure_status;    // 1 -> Start Voltage, 2 -> Start Current, 3 -> Both are Ready
} system_data;

// UART Config
// #define MATLAB_serial_enable // Comment to Enable Windows GUI

//#define UART_BAUDRATE              115200
#define UART_BAUDRATE                230400
#define PACKET_MARKER                0x19621996

// ADC Config
extern system_data process_data;
#define DMA_ADC_data_length 128 // n=6; Selected Length is 2^n
#define ADC_SAMPLE_RATE 64      // 32 samples per cycle

/*
 * adc_raw_data -> 2-D Array
 * [Volt, AFC, Current, AFC] with Gain A [1- 4]
 * [Volt, AFC, Current, AFC] with Gain B [5- 8]
 */
#define pos_volt_a 0
#define pos_volt_AFC_a 1
#define pos_amp_a 2
#define pos_amp_AFC_a 3

#define pos_volt_b 4
#define pos_volt_AFC_b 5
#define pos_amp_b 6
#define pos_amp_AFC_b 7

// Gain Selector Variable
// Gain A - 0
// Gain B - 1
int8_t volt_gain_sel, amp_gain_sel;

float adc_raw_data[8][DMA_ADC_data_length];
float zero_pad_adc_PA[2];

#endif // SYSTEM_DATA_H

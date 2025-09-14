// system_data.h
#ifndef SYSTEM_DATA_H
#define SYSTEM_DATA_H

#include <stdint.h> // For uint8_t, uint16_t, etc.

// Firmware Version Global Variable
#define fw_version 0.26

// Define the struct
typedef struct system_data
{
	uint8_t hold_btn;
	uint8_t sp_btn;
	uint8_t rcl_btn;
	uint16_t set_freq;
	uint8_t led_state;
	int8_t VI_measure_mode;
	float rms_voltage;
	float rms_AFC_volt;
	float rms_current;
	float rms_AFC_current;
	float voltage_phase;
	float current_phase;
	float VI_phase;
	float capacitance;
	float inductance;
	float resistance;
	int unit_capacitance;
	int unit_inductance;
	int unit_resistance;
	int unit_esr;
	float impedance;
	float esr;
	float tan_delta;
	float QF;
	uint8_t uart_all_print_DSO;
	uint8_t LCR_Mode;
	uint8_t adc_measure_status; // 1 -> Start Voltage, 2 -> Start Current, 3 -> Both are Ready
} system_data;

typedef struct
{
    int16_t adc;
    int16_t afc;
} t_adc_dma_data_16;

typedef struct
{
    int32_t adc;
    int32_t afc;
} t_adc_dma_data_32;

// UART Config
#define MATLAB_SERIAL // Comment to Enable Windows GUI

// #define UART_BAUDRATE              115200
// #define UART_BAUDRATE                230400
#define UART_BAUDRATE 115200
#define PACKET_MARKER 0x19621996

// ADC Config
extern system_data process_data;
uint8_t VI_measure_index;
#define DMA_ADC_DATA_LENGTH 128 // n=6; Selected Length is 2^n
#define no_of_sine_wave_cycle_per_data 2

#define ADC_Block_skip_count 32
#define ADC_Block_avg_count 32
 // 1 = just one block = no averaging


// extern uint8_t DAC_sine_table[64]; // matched to the ADC sampling
// #define ADC_SAMPLE_RATE 64      // 32 samples per cycle

// Amp Settings
// +--------------------------+-------------------------+
// | 0 - GS - LOW - High Gain | 0 - VI - LOW - Voltage  | - Start - 0(00) to ...
// | 1 - GS - HIGH - Low Gain | 1 - VI - HIGH - Current | - Stop  - 3(11)
// +--------------------------+-------------------------+
// +------2nd bit is GS-------+-------1st bit is VI-----+ 
// +---------------------([GS] [VI])--------------------+

#define voltage_high_gain_mode 0 // 0b0000 0000
#define current_high_gain_mode 1 // 0b0000 0001
#define voltage_low_gain_mode 2  // 0b0000 0010
#define current_low_gain_mode 3	 // 0b0000 0011

#define VI_data_ready 4	// All the process complete
#define Stop_Storing_ADC_data 5

uint8_t VI_measure_status; // Status

// Gain Selector Variable
int8_t volt_gain_sel, amp_gain_sel;

int16_t adc_raw_data[8][DMA_ADC_DATA_LENGTH];
float adc_data[8][DMA_ADC_DATA_LENGTH];
int16_t zero_pad_adc_PA[2];

#endif // SYSTEM_DATA_H

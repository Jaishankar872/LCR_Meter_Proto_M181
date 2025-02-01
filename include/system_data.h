// system_data.h
#ifndef SYSTEM_DATA_H
#define SYSTEM_DATA_H

#include <stdint.h> // For uint8_t, uint16_t, etc.

// Define the struct
typedef struct system_data
{
    uint8_t hold_btn, sp_btn, rcl_btn;
    uint16_t set_freq;
    uint8_t led_state;
    int8_t VI_measure_mode;
    float pk_pk_voltage, pk_pk_AFC_volt, pk_pk_current, pk_pk_AFC_current;
} system_data;

// Declare the global variable
extern system_data process_data;

#endif // SYSTEM_DATA_H

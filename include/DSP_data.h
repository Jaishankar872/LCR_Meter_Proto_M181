/**
 * @file DSP_data.h
 * @brief Process the data from ADC
 * This file contains the function definition for Process the data from ADC
 *
 * @author Jaishankar M
 */

#ifndef DSP_DATA_H
#define DSP_DATA_H

// Include Header Files
#include "system_data.h"


// Pubilc Variable Declaration


// Pubilc Function Declaration
void setup_DSP_parameter();
void process_data_via_DSP(system_data *_adc_data);


#endif // End of DSP_DATA_H
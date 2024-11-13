#include <Arduino.h>
#include "system_setup.h"
#include "m181_display.h" //Display section are moved here

#define fw_version 0.26

// Experiment Section After moved to system_setup.h
// Create a HardwareTimer instance for TIM2
#include "stm32f1xx_hal.h"

//------------------------------------------------------------------------

void setup()
{
  int_system_setup(fw_version);
  setup_display(fw_version); // To setup the Display

  // Experiment Section After moved to system_setup.h

  //----
}

void loop()
{
  regular_task_loop();
  data_display(back_end_data); // calling the display function with data
  // Experiment Section After moved to system_setup.h

  //----
}

#include <Arduino.h>
#include "system_setup.h"

#define fw_version 0.1

// Experiment Section After moved to system_setup.h

//--

void setup()
{
  int_system_setup(fw_version);

  // Experiment Section After moved to system_setup.h
  
  //----
}

void loop()
{
  regular_task_loop();
}

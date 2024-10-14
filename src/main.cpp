#include <Arduino.h>
#include "system_setup.h"

#define _fw_version 0.1

// Experiment Section After moved to system_setup.h

//--

void setup()
{
  int_system_setup();

  // Experiment Section After moved to system_setup.h
  
  //----
}

void loop()
{
  regular_task_loop();
}

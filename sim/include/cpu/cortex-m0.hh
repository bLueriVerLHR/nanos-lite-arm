#pragma once

#include "gcpu.hh"
#include "../bus/sysbus.hh"

class Cortex_M0 : public Gcpu {
public:
  Cortex_M0(SystemBus *bus);
  void Step(unsigned in);
};
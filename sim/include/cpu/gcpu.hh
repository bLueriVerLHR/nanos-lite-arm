#pragma once

class Gcpu {

public:
  Gcpu() = default;
  virtual ~Gcpu() = default;
  virtual void Step(unsigned in) = 0;
};
#pragma once

#include <deque>
#include <string>

class Debugger32 {
private:
  std::deque<std::string> memque;
  std::deque<std::string> instque;
  std::deque<std::string> jmpque;
  std::deque<std::string> regque;

  uint32_t maxlen = 50;
  uint32_t curaddr = 0;

public:
  Debugger32();
  ~Debugger32();
  void pushjmp(uint32_t addr);
  void pushreg(std::string msg);
  void pushinst(uint32_t inst);
  void pushmem(uint32_t addr, uint32_t data, bool memi);
  void setaddr(uint32_t addr);
  void setqlen(uint32_t len);
  void getopt();
  void pallmsg();
};
#include "debug/debugger.hh"
#include "common.hh"

Debugger32::Debugger32() {}

Debugger32::~Debugger32() {
  pallmsg();
}

void Debugger32::pushreg(std::string msg) {
  regque.push_back(msg);
  if (regque.size() > 4)
    regque.pop_front();
}

void Debugger32::pushjmp(uint32_t addr) {
  char buf[128] = {0};
  sprintf(buf, "[%08x] b <%08x>", curaddr, addr);
  jmpque.push_back(buf);
  if (jmpque.size() > maxlen)
    jmpque.pop_front();
}

void Debugger32::pushinst(uint32_t inst) {
  char buf[128] = {0};

  sprintf(buf, "[%08x] %08x", curaddr, inst);
  instque.push_back(buf);
  if (instque.size() > maxlen)
    instque.pop_front();
}

void Debugger32::pushmem(uint32_t addr, uint32_t data, bool memi) {
  char buf[128] = {0};
  sprintf(buf, "[%08x] %c @%08x %08x", curaddr, (memi ? 'r' : 'w'), addr, data);
  memque.push_back(buf);
  if (memque.size() > maxlen)
    memque.pop_front();
}

void Debugger32::setaddr(uint32_t addr) { curaddr = addr; }

void Debugger32::setqlen(uint32_t len) { maxlen = len; }

void Debugger32::getopt() {
  static uint32_t pass = 0;
  if (pass) {
    pass -= 1;
    return;
  }

  char buf[128];

  do {
    printf("(dbg) ");
    fgets(buf, 128, stdin);

    if (*buf == 'q')
      exit(EXIT_SUCCESS);

    if (*buf == 'p') {
      pallmsg();
      continue;
    }

    else if (*buf == 's') {
      uint32_t skip;
      char _;
      sscanf(buf, "%c %u", &_, &skip);
      pass = skip;
      break;
    }
  } while (false);
}

void Debugger32::pallmsg() {
  std::cout << "Register Trace Info:" << std::endl;
  for (auto &&msg : regque) {
    std::cout << std::endl;
    std::cout << msg << std::endl;
  }
  std::cout << std::endl;
  std::cout << "Instruction Trace Info:" << std::endl;
  for (auto &&msg : instque) {
    std::cout << "\t" << msg << std::endl;
  }
  std::cout << std::endl;
  std::cout << "Memory Trace Info:" << std::endl;
  for (auto &&msg : memque) {
    std::cout << "\t" << msg << std::endl;
  }
  std::cout << std::endl;
  std::cout << "JMP Trace Info:" << std::endl;
  for (auto &&msg : jmpque) {
    std::cout << "\t" << msg << std::endl;
  }
}
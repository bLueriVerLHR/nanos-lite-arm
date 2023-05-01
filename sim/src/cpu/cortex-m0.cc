#include <common.hh>

#include "common.hh"
#include "cpu/cortex-m0.hh"
#include "bus/sysbus.hh"

namespace {

enum CondType {
  ZF    = 0b000,
  CF    = 0b001,
  NF    = 0b010,
  VF    = 0b011,
  CZF   = 0b100,
  NVF   = 0b101,
  NVZF  = 0b110,
  ALF   = 0b111,
};

enum REGType {
  // GPR
  R0,
  R1,
  R2,
  R3,
  R4,
  R5,
  R6,
  R7,
  R8,
  R9,
  R10,
  R11,
  R12,

  // SPR
  SP = 13,
  LR = 14,
  PC = 15
};

enum SRType {
  SR_LSL,
  SR_LSR,
  SR_ASR,
  SR_ROR,
};

enum EXCEPTIONS {
  HardFault
};

enum Mode {
  Mode_Thread,
  Mode_Handler,
} mstatus;

struct PRIMASK {
  bool PRIMASK;
} PMASK;

struct CONTROL {
  bool SPSEL;
} CTRL;

struct PSR {
  // uint32_t psr;

  // apsr
  bool N; // neg
  bool Z; // zero
  bool C; // carry
  bool V; // overflow

  // ipsr
  bool T; // thumb

  // epsr
  uint32_t ISR_idx; // interrupt service
} xPSR;

struct Reg {
  uint32_t regs[13] = {0};
  uint32_t _LR = 0;
  uint32_t _PC = 0;
  uint32_t sp_process = 0;
  uint32_t sp_main = 0;

  uint32_t get(uint32_t idx) {
    panicifnot(idx >= 0 && idx <= 15);
    if (idx == 15)
      return _PC + 4;

    if (idx == 14)
      return _LR;

    if (idx == 13) {
      if (CTRL.SPSEL == 1)
        return sp_process;
      else
        return sp_main;
    }

    return regs[idx];
  }

  void set(uint32_t idx, uint32_t value) {
    panicifnot(idx >= 0 && idx <= 15);
    if (idx == 15) {
      _PC = value;
      return;
    }

    if (idx == 14) {
      _LR = value;
      return;
    }

    if (idx == 13) {
      if (CTRL.SPSEL == 1)
        sp_process = Mask32<31, 2> & value;
      else
        sp_main = Mask32<31, 2> & value;
      return;
    }

    regs[idx] = value;
  }

  void pc_inc(uint32_t value) { _PC += value; }
  uint32_t inst_addr() { return _PC; }
} R;


class Trie {
  using exectype = void (*)(uint32_t);

  struct node {
    exectype exec;
    bool is_terminal = false;
    node *next[3] = {nullptr};
  };

  node *root;

  exectype search_from(node *nwroot, uint32_t inst, uint32_t start) {
    node *walker = nwroot;
    if (!nwroot) return nullptr;
    if (!start) {
      if (nwroot->is_terminal) {
        return nwroot->exec;
      } else {
        return nullptr;
      }
    }
    uint32_t mask = 1 << (start - 1);
    exectype func = nullptr;
    while (mask) {
      size_t nxt = !!(mask & inst);
      if (walker->next[nxt] == nullptr) {
        if (walker->next[2] == nullptr) {
          return nullptr;
        }
        walker = walker->next[2];
        mask = mask >> 1;
        start -= 1;
        continue;
      }

      func = search_from(walker->next[nxt], inst, start - 1);
      if (func)
        return func;

      func = search_from(walker->next[2], inst, start - 1);
      if (func)
        return func;

      break;
    }
    if (walker->is_terminal)
      return walker->exec;

    return nullptr;
  }

public:
  Trie() : root(new node) {}

  void insert(const char *word, exectype exec) {
    node *walker = root;
    const char *p = word;
    while (p && *p) {
      if (*p == ' ' || *p == '\'') {
        p++;
        continue;
      }
      size_t nxt = *p == 'x' ? 2 : *p == '1' ? 1 : *p == '0' ? 0 : -1;
      panicifnot(nxt != -1);
      if (walker->next[nxt] == nullptr) {
        walker->next[nxt] = new node;
      }
      walker = walker->next[nxt];
      p++;
    }
    if (walker->is_terminal)
      panic("?? what fxxk with this isa?");
    walker->is_terminal = true;
    walker->exec = exec;
  }

  exectype search(uint32_t inst, uint32_t start) {
    node *walker = root;
    uint32_t mask = 1 << (start - 1);
    exectype func = nullptr;
    while (mask) {
      size_t nxt = !!(mask & inst);
      if (walker->next[nxt] == nullptr) {
        if (walker->next[2] == nullptr) {
          return nullptr;
        }
        walker = walker->next[2];
        mask = mask >> 1;
        start -= 1;
        continue;
      }

      func = search_from(walker->next[nxt], inst, start - 1);
      if (func)
        break;

      func = search_from(walker->next[2], inst, start - 1);
      break;
    }
    return func;
  }

  void clear_node(node *walker) {
    for (size_t i = 0; i < 3; ++i) {
      if (walker->next[i]) {
        clear_node(walker->next[i]);
        delete walker->next[i];
        walker->next[i] = nullptr;
      }
    }
    walker->exec = nullptr;
    walker->is_terminal = false;
  }

  ~Trie() {
    clear_node(root);
    delete root;
  }
} dict;

using std::pair;

static SystemBus *sysbus;

static Debugger32 dbgr;

} // namespace

//
// ----- ----- Help ----- -----
//

static bool isinst16 = false, nojmp = true;

#define DINST(inst, hi, lo) (((inst) & Mask32<(hi), (lo)>) >> (lo))
#define SEXT32(x, width) ((int32_t((x) << (32 - (width)))) >>  (32 - (width)))
#define ALIGN(x, y) ((y) * ((x) / (y)))


static inline bool is_zero(uint32_t x) { return x == 0; }

static inline bool is_ones(uint32_t x) { return (~x) == 0; }

static inline uint32_t big_endian_reverse(uint32_t x) {
  x = ((x & 0X0000FFFF) << 16) | ((x & 0XFFFF0000) >> 16);
  x = ((x & 0X00FF00FF) <<  8) | ((x & 0XFF00FF00) >>  8);
  x = ((x & 0X0F0F0F0F) <<  4) | ((x & 0XF0F0F0F0) >>  4);
  x = ((x & 0X33333333) <<  2) | ((x & 0XCCCCCCCC) >>  2);
  x = ((x & 0X55555555) <<  1) | ((x & 0XAAAAAAAA) >>  1);
  return x;
}

static inline uint32_t bit_count(uint32_t x) {
  return __builtin_popcount(x);
}

//
// ----- ----- Exceptions ----- -----
//

static void blx_write_pc(uint32_t addr);

static void exception_return(uint32_t address) {
  panicifnot(mstatus == Mode_Handler);
}

static void bkpt_instr_debug_event() {}

static void call_supervisor() {
  uint32_t svc_service = 0;
  sysbus->read32(svc_service, IMG_ADDR + 0x2c);
  blx_write_pc(svc_service);
}

static bool current_mode_is_privileged() {
  return mstatus == Mode_Handler;
}

static void exception_taken(uint32_t id) {}

static bool event_registered() { return false; }

static void clear_event_register() {}

static void wait_for_interrupt() {}

static void wait_for_event() {}

static void hint_send_event() {}

static void hint_yield() {
  Log("hit yield");
  exit(EXIT_SUCCESS);
}

//
// ----- ----- MEM MISC ----- -----
//

static void data_memory_barrier(uint32_t option) {}

static void data_synchronization_barrier(uint32_t option) {}

static void instruction_synchronization_barrier(uint32_t option) {}

static uint32_t mem_access_aligned(uint32_t address, uint32_t size) {
  if (ALIGN(address, size) != address) {
    exception_taken(HardFault);
  }

  if (size == 4) {
    uint32_t recv;
    sysbus->read32(recv, address);

#ifdef DEBUG_MODE
  dbgr.pushmem(address, recv, true);
#endif
    return recv;
  } else if (size == 2) {
    uint16_t recv;
    sysbus->read16(recv, address);

#ifdef DEBUG_MODE
  dbgr.pushmem(address, recv, true);
#endif
    return (uint32_t)recv;
  } else if (size == 1) {
    uint8_t recv;
    sysbus->read8(recv, address);

#ifdef DEBUG_MODE
  dbgr.pushmem(address, recv, true);
#endif
    return (uint32_t)recv;
  }

  panic("unreachable");
  return 0;
}

static void mem_modify_aligned(uint32_t data, uint32_t address, uint32_t size) {
  if (ALIGN(address, size) != address) {
    exception_taken(HardFault);
  }

#ifdef DEBUG_MODE
  dbgr.pushmem(address, data, false);
#endif

  if (size == 4) {
    uint32_t recv = data;
    sysbus->write32(recv, address);
    return;
  } else if (size == 2) {
    uint16_t recv = data;
    sysbus->write16(recv, address);
    return;
  } else if (size == 1) {
    uint8_t recv = data;
    sysbus->write8(recv, address);
    return;
  }

  panic("unreachable");
}

//
// ----- ----- ALU ----- -----
//

static pair<uint32_t, bool> lsl_c(uint32_t x, uint32_t shamt) {
  panicifnot(shamt > 0);
  uint32_t ext_x = x << shamt;
  bool carry = !!(x & (0x1 << (32 - shamt)));
  return {ext_x, carry};
}

static uint32_t lsl(uint32_t x, uint32_t shamt) {
  panicifnot(shamt >= 0);
  if (shamt == 0)
    return x;

  auto &&[ext_x, carry] = lsl_c(x, shamt);
  return ext_x;
}

static pair<uint32_t, bool> lsr_c(uint32_t x, uint32_t shamt) {
  panicifnot(shamt > 0);
  uint32_t ext_x = x >> shamt;
  bool carry = !!(x & (0x1 << (shamt - 1)));
  return {ext_x, carry};
}

static uint32_t lsr(uint32_t x, uint32_t shamt) {
  panicifnot(shamt >= 0);
  if (shamt == 0)
    return x;

  auto &&[ext_x, carry] = lsr_c(x, shamt);
  return ext_x;
}

static pair<uint32_t, bool> asr_c(uint32_t x, uint32_t shamt) {
  panicifnot(shamt > 0);
  uint32_t ext_x = ((int32_t)x >> shamt);
  bool carry = shamt > 31 ? ((int32_t)x < 0) : !!(x & (0x1 << (shamt - 1)));
  return {ext_x, carry};
}

static uint32_t asr(uint32_t x, uint32_t shamt) {
  panicifnot(shamt >= 0);
  if (shamt == 0)
    return x;

  auto &&[ext_x, carry] = asr_c(x, shamt);
  return ext_x;
}

static pair<uint32_t, bool> ror_c(uint32_t x, uint32_t shamt) {
  panicifnot(shamt > 0);
  uint32_t m = shamt % 32;
  uint32_t ext_x = lsr(x, m) | lsl(x, 32 - m);
  bool carry = ((int32_t)ext_x < 0);
  return {ext_x, carry};
}

static uint32_t ror(uint32_t x, uint32_t shamt) {
  panicifnot(shamt >= 0);
  if (shamt == 0)
    return x;

  auto &&[ext_x, carry] = ror_c(x, shamt);
  return ext_x;
}

static pair<uint32_t, pair<bool, bool>> add_with_carry(uint32_t x, uint32_t y,
                                                       bool carry_in) {
  uint64_t usum = (uint64_t)x + (uint64_t)y + carry_in;
  int64_t ssum = ((int64_t)(int32_t)x + (int64_t)(int32_t)y + carry_in);
  uint32_t sum = Mask64<31, 0> & usum;
  bool carry = (sum != usum);
  bool overflow = ((int64_t)(int32_t)sum != ssum);
  return {sum, {carry, overflow}};
}


//
// ----- ----- Core op ----- -----
//

static void branch_to(uint32_t address) {
#ifdef DEBUG_MODE
  dbgr.pushjmp(address);
#endif

  R.set(PC, address);
  nojmp = false;
}

static void branch_write_pc(uint32_t address) {
  branch_to(Mask32<31, 1> & address);
}

static void bx_write_pc(uint32_t address) {
  if (mstatus == Mode_Handler && ((address & Mask32<31, 28>) >> 28) == 0b1111) {
    exception_return(address & Mask32<27, 0>);
    return;
  }
  xPSR.T = address & Mask32<0, 0>;
  branch_to(address & Mask32<31, 1>);
}

static void blx_write_pc(uint32_t address) {
  xPSR.T = address & Mask32<0, 0>;
  branch_to(address & Mask32<31, 1>);
}

static void load_write_pc(uint32_t address) { bx_write_pc(address); }

static void alu_write_pc(uint32_t address) { branch_write_pc(address); }

//
// ----- ----- Condition ----- -----
//

static bool condition_passed(uint32_t cond) {
  bool result = false;

  switch ((cond & Mask32<3, 1>) >> 1) {
  case 0b000:
    result = (xPSR.Z == 1);
    break;
  case 0b001:
    result = (xPSR.C == 1);
    break;
  case 0b010:
    result = (xPSR.N == 1);
    break;
  case 0b011:
    result = (xPSR.V == 1);
    break;
  case 0b100:
    result = (xPSR.C == 1) && (xPSR.Z == 0);
    break;
  case 0b101:
    result = (xPSR.N == xPSR.V);
    break;
  case 0b110:
    result = (xPSR.N == xPSR.V) && (xPSR.Z == 0);
    break;
  case 0b111:
    result = true;
    break;
  };

  if ((cond & Mask32<0, 0>) == 0b1 && cond != 0b1111)
    result = !result;

  return result;
}

//
// ----- ----- Shift ----- -----
//

static pair<SRType, uint32_t> decode_imm_shift(uint32_t type, uint32_t imm5) {
  SRType shift_t;
  uint32_t shift_n;

  switch (type) {
  case 0b00:
    shift_t = SR_LSL;
    shift_n = imm5;
    break;
  case 0b01:
    shift_t = SR_LSR;
    shift_n = imm5 == 0 ? 32 : imm5;
    break;
  case 0b10:
    shift_t = SR_ASR;
    shift_n = imm5 == 0 ? 32 : imm5;
    break;
  case 0b11:
    shift_t = SR_ROR;
    shift_n = imm5;
    break;
  default:
    panic("unreachable");
  };

  return {shift_t, shift_n};
}

static SRType decode_reg_shift(uint32_t type) {
  SRType shift_t;

  switch (type) {
  case 0b00:
    shift_t = SR_LSL;
    break;
  case 0b01:
    shift_t = SR_LSR;
    break;
  case 0b10:
    shift_t = SR_ASR;
    break;
  case 0b11:
    shift_t = SR_ROR;
    break;
  default:
    panic("unreachable");
  };

  return shift_t;
}

static pair<uint32_t, bool> shift_c(uint32_t value, SRType type,
                                    uint32_t amount, bool carry) {
  if (amount == 0) {
    return {value, carry};
  } else {
    switch (type) {
    case SR_LSL:
      return lsl_c(value, amount);
    case SR_LSR:
      return lsr_c(value, amount);
    case SR_ASR:
      return asr_c(value, amount);
    case SR_ROR:
      return ror_c(value, amount);
    default:
      panic("unreachable");
    }
  }
  return {};
}

static uint32_t shift(uint32_t value, SRType type, uint32_t amount,
                      bool carry) {
  auto &&[result, _] = shift_c(value, type, amount, carry);
  return result;
}

//
// ----- ----- exec ----- -----
//

static void exec_adc_reg_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);

  auto &&[result, bols] = add_with_carry(R.get(n), R.get(m), xPSR.C);
  auto &&[carry, overflow] = bols;

  R.set(d, result);

  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
  xPSR.C = carry;
  xPSR.V = overflow;
}

static void exec_add_imm_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 5, 3);
  uint32_t imm3 = DINST(inst, 8, 6);
  uint32_t imm32 = imm3;

  auto &&[result, bols] = add_with_carry(R.get(n), imm32, false);
  auto &&[carry, overflow] = bols;

  R.set(d, result);
  
  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
  xPSR.C = carry;
  xPSR.V = overflow;
}

static void exec_add_imm_t2(uint32_t inst) {
  uint32_t d = DINST(inst, 10, 8);
  uint32_t n = DINST(inst, 10, 8);
  uint32_t imm8 = DINST(inst, 7, 0);
  uint32_t imm32 = imm8;

  auto &&[result, bols] = add_with_carry(R.get(n), imm32, false);
  auto &&[carry, overflow] = bols;
  
  R.set(d, result);

  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
  xPSR.C = carry;
  xPSR.V = overflow;
}

static void exec_add_reg_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 5, 3);
  uint32_t m = DINST(inst, 8, 6);

  auto &&[result, bols] = add_with_carry(R.get(n), R.get(m), false);
  auto &&[carry, overflow] = bols;

  R.set(d, result);

  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
  xPSR.C = carry;
  xPSR.V = overflow;
}

static void exec_add_sp_reg_t1(uint32_t inst);
static void exec_add_sp_reg_t2(uint32_t inst);

static void exec_add_reg_t2(uint32_t inst) {
  uint32_t Rdn = DINST(inst, 2, 0);
  uint32_t Rm = DINST(inst, 6, 3);
  uint32_t DN = DINST(inst, 7, 7);

  if (Rm == 0b1101) {
    exec_add_sp_reg_t1(inst);
    return;
  } else if ((((DN << 3) | Rdn) == 0b1101)) {
    exec_add_sp_reg_t2(inst);
    return;
  }

  uint32_t d = ((DN << 3) | Rdn);
  uint32_t n = ((DN << 3) | Rdn);
  uint32_t m = Rm;

  if (n == 15 && m == 15)
    panic("unpredictable");

  auto &&[result, bols] = add_with_carry(R.get(n), R.get(m), false);
  auto &&[carry, overflow] = bols;

  if (d == 15) {
    alu_write_pc(result);
  } else {
    R.set(d, result);
  }
}

static void exec_add_sp_imm_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 10, 8);
  uint32_t imm8 = DINST(inst, 7, 0);
  uint32_t imm32 = imm8 << 2;

  auto &&[result, _] = add_with_carry(R.get(SP), imm32, false);
  
  R.set(d, result);
}

static void exec_add_sp_imm_t2(uint32_t inst) {
  uint32_t d = 13;
  uint32_t imm7 = DINST(inst, 6, 0);
  uint32_t imm32 = imm7 << 2;

  auto &&[result, _] = add_with_carry(R.get(SP), imm32, false);
  
  R.set(d, result);
}

static void exec_add_sp_reg_t1(uint32_t inst) {
  uint32_t Rdm = DINST(inst, 2, 0);
  uint32_t DM = DINST(inst, 7, 7);

  uint32_t d = ((DM << 3) | Rdm);
  uint32_t m = d;

  auto &&[result, _] = add_with_carry(R.get(SP), R.get(m), false);
  
  if (d == 15) {
    alu_write_pc(result);
  } else {
    R.set(d, result);
  }

}

static void exec_add_sp_reg_t2(uint32_t inst) {
  uint32_t Rm = DINST(inst, 6, 3);

  uint32_t d = 13;
  uint32_t m = Rm;

  auto &&[result, _] = add_with_carry(R.get(SP), R.get(m), false);
  
  R.set(d, result);
}

static void exec_adr_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 10, 8);
  uint32_t imm8 = DINST(inst, 7, 0);
  uint32_t imm32 = imm8 << 2;

  uint32_t result = ALIGN(R.get(PC), 4) + imm32;

  R.set(d, result);
}

static void exec_and_reg_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);

  uint32_t result = R.get(n) & R.get(m);

  R.set(d, result);

  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);

  // assuming C V wiil not change
}

static void exec_asr_imm_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);
  uint32_t imm5 = DINST(inst, 10, 6);
  auto &&[_, shift_n] = decode_imm_shift(0b10, imm5);

  auto &&[result, carry] = shift_c(R.get(m), SR_ASR, shift_n, xPSR.C);

  R.set(d, result);

  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
  xPSR.C = carry;
}

static void exec_asr_reg_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);

  uint32_t shift_n = R.get(m) & Mask32<7, 0>;
  
  auto &&[result, carry] = shift_c(R.get(n), SR_ASR, shift_n, xPSR.C);
  
  R.set(d, result);
  
  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
  xPSR.C = carry;
}

static void exec_svc_t1(uint32_t inst);
static void exec_udf_t1(uint32_t inst);

static void exec_b_t1(uint32_t inst) {
  uint32_t cond = DINST(inst, 11, 8);
  if (cond == 0b1110) {
    exec_udf_t1(inst);
    return;
  }
  if (cond == 0b1111) {
    exec_svc_t1(inst);
    return;
  }
  uint32_t imm8 = DINST(inst, 7, 0);
  uint32_t imm32 = SEXT32(imm8 << 1, 9);

  if (condition_passed(cond)) {
    branch_write_pc(R.get(PC) + imm32);
  }
}

static void exec_b_t2(uint32_t inst) {
  uint32_t imm11 = DINST(inst, 10, 0);
  uint32_t imm32 = SEXT32(imm11 << 1, 12);

  branch_write_pc(R.get(PC) + imm32);
}

static void exec_bic_reg_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);

  uint32_t result = R.get(n) & (~R.get(m));
  
  R.set(d, result);

  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
}

static void exec_bkpt_t1(uint32_t inst) {
  uint32_t imm8 = DINST(inst, 7, 0);
  uint32_t imm32 = imm8;

  bkpt_instr_debug_event();
}

static void exec_bl_t1(uint32_t inst) {
  uint32_t S = DINST(inst, 10 + 16, 10 + 16);
  uint32_t imm10 = DINST(inst, 9 + 16, 0 + 16);
  uint32_t J1 = DINST(inst, 13, 13);
  uint32_t J2 = DINST(inst, 11, 11);
  uint32_t imm11 = DINST(inst, 10, 0);
  uint32_t I1 = !(J1 ^ S);
  uint32_t I2 = !(J2 ^ S);
  uint32_t _imm_ = (S << 24) | (I2 << 23) | (I2 << 22) | (imm10 << 12) | (imm11 << 1);
  uint32_t imm32 = SEXT32(_imm_, 25);

  uint32_t next_instr_addr = R.get(PC);
  R.set(LR, next_instr_addr | 0x1);
  branch_write_pc(R.get(PC) + imm32);
}

static void exec_blx_reg_t1(uint32_t inst) {
  uint32_t m = DINST(inst, 6, 3);
  
  if (m == 15)
    panic("unpredictable");
  
  uint32_t target = R.get(m);
  uint32_t next_instr_addr = R.get(PC) - 2;
  R.set(LR, next_instr_addr | 0x1);
  blx_write_pc(target);
}

static void exec_bx_t1(uint32_t inst) {
  uint32_t m = DINST(inst, 6, 3);
  
  if (m == 15)
    panic("unpredictable");
  
  bx_write_pc(R.get(m));
}

static void exec_cmn_reg_t1(uint32_t inst) {
  uint32_t n = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);

  auto &&[result, bols] = add_with_carry(R.get(n), R.get(m), false);
  auto &&[carry, overflow] = bols;

  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
  xPSR.C = carry;
  xPSR.V = overflow;
}

static void exec_cmp_imm_t1(uint32_t inst) {
  uint32_t n = DINST(inst, 10, 8);
  uint32_t imm8 = DINST(inst, 7, 0);
  uint32_t imm32 = imm8;

  auto &&[result, bols] = add_with_carry(R.get(n), ~imm32, true);
  auto &&[carry, overflow] = bols;

  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
  xPSR.C = carry;
  xPSR.V = overflow;
}

static void exec_cmp_reg_t1(uint32_t inst) {
  uint32_t n = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);

  auto &&[result, bols] = add_with_carry(R.get(n), ~R.get(m), true);
  auto &&[carry, overflow] = bols;

  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
  xPSR.C = carry;
  xPSR.V = overflow;
}

static void exec_cmp_reg_t2(uint32_t inst) {
  uint32_t Rn = DINST(inst, 2, 0);
  uint32_t N = DINST(inst, 7, 7);
  uint32_t n = (N << 3) | Rn;
  uint32_t m = DINST(inst, 6, 3);

  if (n < 8 && m < 8)
    panic("unpredictable");

  if (n == 15 || m == 15)
    panic("unpredictable");

  auto &&[result, bols] = add_with_carry(R.get(n), ~R.get(m), true);
  auto &&[carry, overflow] = bols;

  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
  xPSR.C = carry;
  xPSR.V = overflow;
}

static void exec_cps_t1(uint32_t inst) {
  uint32_t im = DINST(inst, 4, 4);

  if (current_mode_is_privileged()) {
    PMASK.PRIMASK = im;
  }
}

static void exec_dmb_t1(uint32_t inst) {
  uint32_t option = DINST(inst, 3, 0);

  data_memory_barrier(option);
}

static void exec_dsb_t1(uint32_t inst) {
  uint32_t option = DINST(inst, 3, 0);

  data_synchronization_barrier(option);
}

static void exec_eor_reg_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);

  uint32_t result = R.get(n) ^ R.get(m);
  
  R.set(d, result);

  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
}

static void exec_isb_t1(uint32_t inst) {
  uint32_t option = DINST(inst, 3, 0);

  instruction_synchronization_barrier(option);
}

static void exec_ldm_t1(uint32_t inst) {
  uint32_t n = DINST(inst, 10, 8);
  uint32_t regs = DINST(inst, 7, 0);
  bool wback = !!((regs & (1 << n)) == 0);
  
  if (regs == 0)
    panic("unpredictable");

  uint32_t address = R.get(n);

  for (int i = 0; i < 8; ++i) {
    if (!!(regs & (1 << i))) {
      R.set(i, mem_access_aligned(address, 4));
      address += 4;
    }
  }

  if (wback)
    R.set(n, R.get(n) + 4 * bit_count(regs));
}

static void exec_ldr_imm_t1(uint32_t inst) {
  uint32_t t = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 5, 3);
  uint32_t imm5 = DINST(inst, 10, 6);
  uint32_t imm32 = imm5 << 2;

  uint32_t address = R.get(n) + imm32;
  R.set(t, mem_access_aligned(address, 4));
}

static void exec_ldr_imm_t2(uint32_t inst) {
  uint32_t t = DINST(inst, 10, 8);
  uint32_t n = 13;
  uint32_t imm8 = DINST(inst, 7, 0);
  uint32_t imm32 = imm8 << 2;

  uint32_t address = R.get(n) + imm32;
  R.set(t, mem_access_aligned(address, 4));
}

static void exec_ldr_lit_t1(uint32_t inst) {
  uint32_t t = DINST(inst, 10, 8);
  uint32_t imm8 = DINST(inst, 7, 0);
  uint32_t imm32 = imm8 << 2;

  uint32_t base = ALIGN(R.get(PC), 4);
  uint32_t address = base + imm32;
  R.set(t, mem_access_aligned(address, 4));
}

static void exec_ldr_reg_t1(uint32_t inst) {
  uint32_t t = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 5, 3);
  uint32_t m = DINST(inst, 8, 6);

  uint32_t address = R.get(n) + R.get(m);
  R.set(t, mem_access_aligned(address, 4));
}

static void exec_ldrb_imm_t1(uint32_t inst) {
  uint32_t t = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 5, 3);
  uint32_t imm5 = DINST(inst, 10, 6);
  uint32_t imm32 = imm5;

  uint32_t address = R.get(n) + imm32;
  R.set(t, mem_access_aligned(address, 1));
}

static void exec_ldrb_reg_t1(uint32_t inst) {
  uint32_t t = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 5, 3);
  uint32_t m = DINST(inst, 8, 6);

  uint32_t address = R.get(n) + R.get(m);
  R.set(t, mem_access_aligned(address, 1));
}

static void exec_ldrh_imm_t1(uint32_t inst) {
  uint32_t t = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 5, 3);
  uint32_t imm5 = DINST(inst, 10, 6);
  uint32_t imm32 = imm5 << 1;

  uint32_t address = R.get(n) + imm32;
  R.set(t, mem_access_aligned(address, 2));
}

static void exec_ldrh_reg_t1(uint32_t inst) {
  uint32_t t = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 5, 3);
  uint32_t m = DINST(inst, 8, 6);

  uint32_t address = R.get(n) + R.get(m);
  R.set(t, mem_access_aligned(address, 2));
}

static void exec_ldrsb_reg_t1(uint32_t inst) {
  uint32_t t = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 5, 3);
  uint32_t m = DINST(inst, 8, 6);

  uint32_t address = R.get(n) + R.get(m);
  R.set(t, SEXT32(mem_access_aligned(address, 1), 8));
}

static void exec_ldrsh_reg_t1(uint32_t inst) {
  uint32_t t = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 5, 3);
  uint32_t m = DINST(inst, 8, 6);

  uint32_t address = R.get(n) + R.get(m);
  R.set(t, SEXT32(mem_access_aligned(address, 2), 16));
}

static void exec_lsl_imm_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);
  uint32_t imm5 = DINST(inst, 10, 6);
  auto &&[_, shift_n] = decode_imm_shift(0b00, imm5);

  auto &&[result, carry] = shift_c(R.get(m), SR_LSL, shift_n, xPSR.C);

  R.set(d, result);

  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
  xPSR.C = carry;
}

static void exec_lsl_reg_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);

  uint32_t shift_n = R.get(m) & Mask32<7, 0>;
  
  auto &&[result, carry] = shift_c(R.get(n), SR_LSL, shift_n, xPSR.C);
  
  R.set(d, result);
  
  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
  xPSR.C = carry;
}

static void exec_lsr_imm_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);
  uint32_t imm5 = DINST(inst, 10, 6);
  auto &&[_, shift_n] = decode_imm_shift(0b01, imm5);

  auto &&[result, carry] = shift_c(R.get(m), SR_LSR, shift_n, xPSR.C);

  R.set(d, result);

  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
  xPSR.C = carry;
}

static void exec_lsr_reg_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);

  uint32_t shift_n = R.get(m) & Mask32<7, 0>;
  
  auto &&[result, carry] = shift_c(R.get(n), SR_LSR, shift_n, xPSR.C);
  
  R.set(d, result);
  
  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
  xPSR.C = carry;
}

static void exec_mov_imm_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 10, 8);
  uint32_t imm8 = DINST(inst, 7, 0);
  uint32_t imm32 = imm8;

  uint32_t result = imm32;
  R.set(d, result);

  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
}

static void exec_mov_reg_t1(uint32_t inst) {
  uint32_t D = DINST(inst, 7, 7);
  uint32_t Rm = DINST(inst, 6, 3);
  uint32_t Rd = DINST(inst, 2, 0);
  uint32_t d = ((D << 3) | Rd);
  uint32_t m = Rm;

  uint32_t result = R.get(m);
  if (d == 15) {
    alu_write_pc(result);
  } else {
    R.set(d, result);
  }
}

static void exec_mov_reg_t2(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);

  uint32_t result = R.get(m);
  R.set(d, result);
  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
}

static void exec_mrs_t1(uint32_t inst) { panic("not implemented"); }

static void exec_msr_reg_t1(uint32_t inst) { panic("not implemented"); }

static void exec_mul_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 5, 3);
  uint32_t m = DINST(inst, 2, 0);

  uint64_t op1 = R.get(n); 
  uint64_t op2 = R.get(m);

  uint64_t result = op1 * op2;
  R.set(d, result);
  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
}

static void exec_mvn_reg_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);

  uint32_t result = ~R.get(m);
  R.set(d, result);
  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
}

static void exec_nop_t1(uint32_t inst) {}

static void exec_orr_reg_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);

  uint32_t result = R.get(n) | R.get(m);
  R.set(d, result);
  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
}

static void exec_pop_t1(uint32_t inst) {
  uint32_t reglist = DINST(inst, 7, 0);
  uint32_t P = DINST(inst, 8, 8);
  uint32_t regs = (P << 15) | reglist;

  if (regs == 0)
    panic("unpredictable");

  uint32_t address = R.get(SP);
  for (int i = 0; i < 8; ++i) {
    if (!!(regs & (1 << i))) {
      R.set(i, mem_access_aligned(address, 4));
      address += 4;
    }
  }
  if (P)
    load_write_pc(mem_access_aligned(address, 4));

  R.set(SP, R.get(SP) + 4 * bit_count(regs));
}

static void exec_push_t1(uint32_t inst) {
  uint32_t reglist = DINST(inst, 7, 0);
  uint32_t M = DINST(inst, 8, 8);
  uint32_t regs = (M << 14) | reglist;

  if (regs == 0)
    panic("unpredictable");

  uint32_t address = R.get(SP) - 4 * bit_count(regs);
  for (int i = 0; i < 15; ++i) {
    if (!!(regs & (1 << i))) {
      mem_modify_aligned(R.get(i), address, 4);
      address += 4;
    }
  }

  R.set(SP, R.get(SP) - 4 * bit_count(regs));
}

static void exec_rev_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);

  uint32_t word = R.get(m);
  uint32_t result = ((word & Mask32< 7,  0>) << 24) |
                    ((word & Mask32<15,  8>) <<  8) |
                    ((word & Mask32<23, 16>) >>  8) |
                    ((word & Mask32<31, 24>) >> 24);
  R.set(d, result);
}

static void exec_rev16_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);

  uint32_t word = R.get(m);
  uint32_t result = ((word & Mask32<23, 16>) <<  8) |
                    ((word & Mask32<31, 24>) >>  8) |
                    ((word & Mask32< 7,  0>) <<  8) |
                    ((word & Mask32<15,  8>) >>  8);
  R.set(d, result);
}

static void exec_revsh_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);

  uint32_t word = R.get(m);
  uint32_t result = ((SEXT32((word & Mask32<7, 0>), 8) & Mask32<23, 0>) << 8) |
                    ((word & Mask32<15,  8>) >>  8);
  R.set(d, result);
}

static void exec_ror_reg_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);

  uint32_t shift_n = R.get(m) & Mask32<7, 0>;
  
  auto &&[result, carry] = shift_c(R.get(n), SR_ROR, shift_n, xPSR.C);
  
  R.set(d, result);
  
  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
  xPSR.C = carry;
}

static void exec_rsb_imm_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 5, 3);

  auto &&[result, bols] = add_with_carry(~R.get(n), 0, true);
  auto &&[carry, overflow] = bols;
  R.set(d, result);

  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
  xPSR.C = carry;
  xPSR.V = overflow;
}

static void exec_sbc_reg_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);

  auto &&[result, bols] = add_with_carry(R.get(n), ~R.get(m), xPSR.C);
  auto &&[carry, overflow] = bols;
  R.set(d, result);

  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
  xPSR.C = carry;
  xPSR.V = overflow;
}

static void exec_sev_t1(uint32_t inst) { hint_send_event(); }

static void exec_stm_t1(uint32_t inst) {
  uint32_t n = DINST(inst, 10, 8);
  uint32_t regs = DINST(inst, 7, 0);
  
  if (regs == 0)
    panic("unpredictable");

  uint32_t address = R.get(n);

  for (int i = 0; i < 15; ++i) {
    if (!!(regs & (1 << i))) {
      mem_modify_aligned(R.get(i), address, 4);
      address += 4;
    }
  }

  R.set(n, R.get(n) + 4 * bit_count(regs));
}

static void exec_str_imm_t1(uint32_t inst) {
  uint32_t t = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 5, 3);
  uint32_t imm5 = DINST(inst, 10, 6);
  uint32_t imm32 = imm5 << 2;

  uint32_t address = R.get(n) + imm32;
  mem_modify_aligned(R.get(t), address, 4);
}

static void exec_str_imm_t2(uint32_t inst) {
  uint32_t t = DINST(inst, 10, 8);
  uint32_t n = 13;
  uint32_t imm8 = DINST(inst, 7, 0);
  uint32_t imm32 = imm8 << 2;

  uint32_t address = R.get(n) + imm32;
  mem_modify_aligned(R.get(t), address, 4);
}

static void exec_str_reg_t1(uint32_t inst) {
  uint32_t t = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 5, 3);
  uint32_t m = DINST(inst, 8, 6);

  uint32_t address = R.get(n) + R.get(m);
  mem_modify_aligned(R.get(t), address, 4);
}

static void exec_strb_imm_t1(uint32_t inst) {
  uint32_t t = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 5, 3);
  uint32_t imm5 = DINST(inst, 10, 6);
  uint32_t imm32 = imm5;

  uint32_t address = R.get(n) + imm32;
  mem_modify_aligned(R.get(t), address, 1);
}

static void exec_strb_reg_t1(uint32_t inst) {
  uint32_t t = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 5, 3);
  uint32_t m = DINST(inst, 8, 6);

  uint32_t address = R.get(n) + R.get(m);
  mem_modify_aligned(R.get(t), address, 1);
}

static void exec_strh_imm_t1(uint32_t inst) {
  uint32_t t = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 5, 3);
  uint32_t imm5 = DINST(inst, 10, 6);
  uint32_t imm32 = imm5;

  uint32_t address = R.get(n) + imm32;
  mem_modify_aligned(R.get(t), address, 2);
}

static void exec_strh_reg_t1(uint32_t inst) {
  uint32_t t = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 5, 3);
  uint32_t m = DINST(inst, 8, 6);

  uint32_t address = R.get(n) + R.get(m);
  mem_modify_aligned(R.get(t), address, 2);
}

static void exec_sub_imm_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 5, 3);
  uint32_t imm3 = DINST(inst, 8, 6);
  uint32_t imm32 = imm3;

  auto &&[result, bols] = add_with_carry(R.get(n), ~imm32, true);
  auto &&[carry, overflow] = bols;

  R.set(d, result);
  
  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
  xPSR.C = carry;
  xPSR.V = overflow;
}

static void exec_sub_imm_t2(uint32_t inst) {
  uint32_t d = DINST(inst, 10, 8);
  uint32_t n = DINST(inst, 10, 8);
  uint32_t imm8 = DINST(inst, 7, 0);
  uint32_t imm32 = imm8;

  auto &&[result, bols] = add_with_carry(R.get(n), ~imm32, true);
  auto &&[carry, overflow] = bols;
  
  R.set(d, result);

  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
  xPSR.C = carry;
  xPSR.V = overflow;
}

static void exec_sub_reg_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t n = DINST(inst, 5, 3);
  uint32_t m = DINST(inst, 8, 6);

  auto &&[result, bols] = add_with_carry(R.get(n), ~R.get(m), true);
  auto &&[carry, overflow] = bols;

  R.set(d, result);

  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
  xPSR.C = carry;
  xPSR.V = overflow;
}

static void exec_sub_sp_imm_t1(uint32_t inst) {
  uint32_t d = 13;
  uint32_t imm7 = DINST(inst, 6, 0);
  uint32_t imm32 = imm7 << 2;

  auto &&[result, _] = add_with_carry(R.get(SP), ~imm32, true);
  
  R.set(d, result);
}

static void exec_svc_t1(uint32_t inst) {
  uint32_t imm8 = DINST(inst, 7, 0);
  uint32_t imm32 = imm8;

  R.set(LR, (R.inst_addr() + (isinst16 ? 2 : 4)) | 0x1);
  call_supervisor();
}

static void exec_sxtb_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);

  R.set(d, SEXT32((R.get(m) & Mask32<7, 0>), 8));
}

static void exec_sxth_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);

  R.set(d, SEXT32((R.get(m) & Mask32<15, 0>), 16));
}

static void exec_tst_reg_t1(uint32_t inst) {
  uint32_t n = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);

  uint32_t result = R.get(n) & R.get(m);

  xPSR.N = !!(result & Mask32<31, 31>);
  xPSR.Z = is_zero(result);
}

static void exec_udf_t1(uint32_t inst) {
  panic("undefined");
}

static void exec_udf_t2(uint32_t inst) {
  panic("undefined");
}

static void exec_uxtb_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);

  R.set(d, R.get(m) & Mask32<7, 0>);
}

static void exec_uxth_t1(uint32_t inst) {
  uint32_t d = DINST(inst, 2, 0);
  uint32_t m = DINST(inst, 5, 3);

  R.set(d, R.get(m) & Mask32<15, 0>);
}

static void exec_wfe_t1(uint32_t inst) {
  if (event_registered()) {
    clear_event_register();
  } else {
    wait_for_event();
  }
}

static void exec_wfi_t1(uint32_t inst) { wait_for_interrupt(); }

static void exec_yield_t1(uint32_t inst) { hint_yield(); }


//
// ----- ----- Decoder ----- -----
//

static int cm0 = 0;

Cortex_M0::Cortex_M0(SystemBus *bus) {
  panicifnot(cm0 == 0);
  cm0 += 1;

  panicifnot(bus);
  sysbus = bus;

  dict.insert("000'00'00000'xxx'xxx",   exec_mov_reg_t2);
  dict.insert("000'00'xxxxx'xxx'xxx",   exec_lsl_imm_t1);
  dict.insert("000'01'xxxxx'xxx'xxx",   exec_lsr_imm_t1);
  dict.insert("000'10'xxxxx'xxx'xxx",   exec_asr_imm_t1);
  dict.insert("000'11'0'0'xxx'xxx'xxx", exec_add_reg_t1);
  dict.insert("000'11'0'1'xxx'xxx'xxx", exec_sub_reg_t1);
  dict.insert("000'11'1'0'xxx'xxx'xxx", exec_add_imm_t1);
  dict.insert("000'11'1'1'xxx'xxx'xxx", exec_sub_imm_t1);
  
  dict.insert("001'00'xxx'xxxxxxxx",    exec_mov_imm_t1);
  dict.insert("001'01'xxx'xxxxxxxx",    exec_cmp_imm_t1);
  dict.insert("001'10'xxx'xxxxxxxx",    exec_add_imm_t2);
  dict.insert("001'11'xxx'xxxxxxxx",    exec_sub_imm_t2);

  dict.insert("01001'xxx'xxxxxxxx",     exec_ldr_lit_t1);
  dict.insert("011'0'0'xxxxx'xxx'xxx",  exec_str_imm_t1);
  dict.insert("011'0'1'xxxxx'xxx'xxx",  exec_ldr_imm_t1);
  dict.insert("011'1'0'xxxxx'xxx'xxx",  exec_strb_imm_t1);
  dict.insert("011'1'1'xxxxx'xxx'xxx",  exec_ldrb_imm_t1);
  
  dict.insert("010000'0000'xxx'xxx",    exec_and_reg_t1);
  dict.insert("010000'0001'xxx'xxx",    exec_eor_reg_t1);
  dict.insert("010000'0010'xxx'xxx",    exec_lsl_reg_t1);
  dict.insert("010000'0011'xxx'xxx",    exec_lsr_reg_t1);
  dict.insert("010000'0100'xxx'xxx",    exec_asr_reg_t1);
  dict.insert("010000'0101'xxx'xxx",    exec_adc_reg_t1);
  dict.insert("010000'0110'xxx'xxx",    exec_sbc_reg_t1);
  dict.insert("010000'0111'xxx'xxx",    exec_ror_reg_t1);
  dict.insert("010000'1000'xxx'xxx",    exec_tst_reg_t1);
  dict.insert("010000'1001'xxx'xxx",    exec_rsb_imm_t1);
  dict.insert("010000'1010'xxx'xxx",    exec_cmp_reg_t1);
  dict.insert("010000'1011'xxx'xxx",    exec_cmn_reg_t1);
  dict.insert("010000'1100'xxx'xxx",    exec_orr_reg_t1);
  dict.insert("010000'1101'xxx'xxx",    exec_mul_t1);
  dict.insert("010000'1110'xxx'xxx",    exec_bic_reg_t1);
  dict.insert("010000'1111'xxx'xxx",    exec_mvn_reg_t1);

//dict.insert("010001 00'x'1101'xxx",   exec_add_sp_reg_t1);
//dict.insert("010001 00'1'xxxx'101",   exec_add_sp_reg_t2);
  dict.insert("010001'00'x'xxxx'xxx",   exec_add_reg_t2);
  dict.insert("010001'01'x'xxxx'xxx",   exec_cmp_reg_t2);
  dict.insert("010001'10'x'xxxx'xxx",   exec_mov_reg_t1);
  dict.insert("010001'11'1'xxxx'000",   exec_blx_reg_t1);
  dict.insert("010001'11'0'xxxx'000",   exec_bx_t1);
  
  dict.insert("0101'000'xxx'xxx'xxx",   exec_str_reg_t1);
  dict.insert("0101'001'xxx'xxx'xxx",   exec_strh_reg_t1);
  dict.insert("0101'010'xxx'xxx'xxx",   exec_strb_reg_t1);
  dict.insert("0101'011'xxx'xxx'xxx",   exec_ldrsb_reg_t1);
  dict.insert("0101'100'xxx'xxx'xxx",   exec_ldr_reg_t1);
  dict.insert("0101'101'xxx'xxx'xxx",   exec_ldrh_reg_t1);
  dict.insert("0101'110'xxx'xxx'xxx",   exec_ldrb_reg_t1);
  dict.insert("0101'111'xxx'xxx'xxx",   exec_ldrsh_reg_t1);
  
  dict.insert("1000'0'xxxxx'xxx'xxx",   exec_strh_imm_t1);
  dict.insert("1000'1'xxxxx'xxx'xxx",   exec_ldrh_imm_t1);
  dict.insert("1001'0'xxx'xxxxxxxx",    exec_str_imm_t2);
  dict.insert("1001'1'xxx'xxxxxxxx",    exec_ldr_imm_t2);
  dict.insert("1010'0'xxx'xxxxxxxx",    exec_adr_t1);
  dict.insert("1010'1'xxx'xxxxxxxx",    exec_add_sp_imm_t1);

  dict.insert("1011'0'10'x'xxxx xxxx",    exec_push_t1);
  dict.insert("1011'0 11 0'011'x'0'0'1'0",exec_cps_t1);
  dict.insert("1011'0 00 0'1'xxxxxxx",    exec_sub_sp_imm_t1);
  dict.insert("1011'0 00 0'0'xxxxxxx",    exec_add_sp_imm_t2);
  dict.insert("1011'0 01 0'01'xxx'xxx",   exec_sxtb_t1);
  dict.insert("1011'0 01 0'00'xxx'xxx",   exec_sxth_t1);
  dict.insert("1011'0 01 0'11'xxx'xxx",   exec_uxtb_t1);
  dict.insert("1011'0 01 0'10'xxx'xxx",   exec_uxth_t1);
  dict.insert("1011'1 01 0'00'xxx'xxx",   exec_rev_t1);
  dict.insert("1011'1 01 0'01'xxx'xxx",   exec_rev16_t1);
  dict.insert("1011'1 01 0'11'xxx'xxx",   exec_revsh_t1);
  dict.insert("1011'1'10'x'xxxx xxxx",    exec_pop_t1);
  dict.insert("1011'1 11 0'xxxxxxxx",     exec_bkpt_t1);
  dict.insert("1011'1 11 1'0000'0000",    exec_nop_t1);
  dict.insert("1011'1 11 1'0100'0000",    exec_sev_t1);
  dict.insert("1011'1 11 1'0010'0000",    exec_wfe_t1);
  dict.insert("1011'1 11 1'0011'0000",    exec_wfi_t1);
  dict.insert("1011'1 11 1'0001'0000",    exec_yield_t1);

  dict.insert("1100'0'xxx'xxxxxxxx",  exec_stm_t1);
  dict.insert("1100'1'xxx'xxxxxxxx",  exec_ldm_t1);
  dict.insert("1101'xxxx'xxxxxxxx",   exec_b_t1);
//dict.insert("1101'1111'xxxxxxxx",   exec_svc_t1);
//dict.insert("1101'1110'xxxxxxxx",   exec_udf_t1);
  dict.insert("1110 0'xxxxxxxxxxx",   exec_b_t2);
  
  dict.insert("111 10'0'111 0'0'0'xxxx'1 0'0'0'1'0'0'0'xxxx xxxx",  exec_msr_reg_t1);
  dict.insert("111 10'0'111'0 1'1'1111'1 0'0'0'1 1 1 1'0101'xxxx",  exec_dmb_t1);
  dict.insert("111 10'0'111'0 1'1'1111'1 0'0'0'1 1 1 1'0100'xxxx",  exec_dsb_t1);
  dict.insert("111 10'0'111'0 1'1'1111'1 0'0'0'1 1 1 1'0110'xxxx",  exec_isb_t1);
  dict.insert("111 10'0'111 1'1'0'1111'1 0'0'0'x x x x'xxxx xxxx",  exec_mrs_t1);
  dict.insert("111'10'1 111 1 1 1'xxxx'1'0 1 0'x x x x xxxx xxxx",  exec_udf_t2);
  dict.insert("111 10'x'xxx x x x xxxx'1 1'x'1'x'x x x xxxx xxxx",  exec_bl_t1);

  uint32_t interp_msp;
  uint32_t interp_rst;

  sysbus->read32(interp_msp, IMG_ADDR + 0x0);
  sysbus->read32(interp_rst, IMG_ADDR + 0x4);

  mstatus = Mode_Handler;
  CTRL.SPSEL = 0;
  R.set(SP, interp_msp);
  branch_write_pc(interp_rst);
  nojmp = true;

  dbgr.setqlen(100);
}

static void decode_and_exec(uint32_t inst) {
  isinst16 = (DINST(inst, 31, 27) == 0b11110) ? false : true;
  inst = isinst16 ? inst >> 16 : inst;
  auto exec = dict.search(inst, isinst16 ? 16 : 32);
  panicifnot(exec);

#ifdef DEBUG_MODE
  dbgr.getopt();
  dbgr.pushinst(inst);
  auto before = R;
#endif

  exec(inst);

#ifdef DEBUG_MODE
  char buf[16][256];
  for (int i = 0; i < 13; ++i)
    sprintf(buf[i], "R%02d: %08x => %08x\n", i, before.get(i), R.get(i));
  sprintf(buf[SP], "SP : %08x => %08x\n", before.get(SP), R.get(SP));
  sprintf(buf[LR], "LR : %08x => %08x\n", before.get(LR), R.get(LR));
  sprintf(buf[PC], "PC : %08x => %08x", before.inst_addr(), R.inst_addr());
  std::stringstream ss;
  for (int i = 0; i < 16; ++i)
    ss << buf[i];
  dbgr.pushreg(ss.str());
#endif

  if (nojmp)
    R.pc_inc(isinst16 ? 2 : 4);
  nojmp = true;
}

void Cortex_M0::Step(unsigned in) {
  uint32_t inst = 0;
  uint16_t loinst = 0;
  uint16_t hiinst = 0;
  uint32_t curaddr = R.inst_addr();
  dbgr.setaddr(curaddr);
  sysbus->read16(loinst, curaddr);
  sysbus->read16(hiinst, curaddr + 2);
  decode_and_exec((loinst << 16) | hiinst);
}

#include "arm.h"
#include "decoder.h"
#include "alu.h"
#include <iostream>
#include <iomanip>

static int inc = 0;

CortexM0::CortexM0(Clock& clock, SystemBus *sysbus)
    : alu(m_regs, &m_c, &m_o, &m_n, &m_z, &m_addr, &m_data)
    , debug(m_regs, &m_c, &m_o, &m_n, &m_z, sysbus)
{
  clock.add_clocked(this);
  m_sysbus = sysbus;

  for (auto i = 0; i < CortexM0::MAX_REGS; i++)
    m_regs[i] = 0;

  m_c = 0;
  m_o = 0;
  m_n = 0;
  m_z = 0;
}

uint16_t CortexM0::fetch_inst()
{
  return m_sysbus->read32(m_regs[15] + inc) & 0xFFFF;
}

static inline bool IS_32BIT(uint32_t inst) 
{
  return (inst & 0xF800) == 0xF000;
}

void CortexM0::init()
{
  auto &&msp = m_sysbus->read32(0x0000'0000);
  auto &&rst = m_sysbus->read32(0x0000'0004) & (~0x1);
  m_regs[15] = rst;
  printf("msp: 0x%08x, rst: 0x%08x\n", msp, rst);
}

void CortexM0::execute()
{
  DecodedInst di;
  uint32_t inst_lo16;
  uint32_t inst_hi16;
  uint32_t inst32;

  inst_lo16 = fetch_inst();
  inc = 2;
  inst_hi16 = fetch_inst();
  inst32 = (inst_lo16 << 16) | inst_hi16;
  if (!IS_32BIT(inst_lo16)) {
    inst32 = (inst32 >> 16) & 0xFFFF;
    di = decoder.decode(inst_lo16, 0);
  } else {
    inc += 2;
    di = decoder.decode(inst32, 1);
  }

#ifdef DEBUG_MODE
  debug.pause(inst32, di);
#endif

  m_regs[15] += inc;
  inc = 0;

  if (di.op == BKPT) {
    m_halted = true;
    return;
  } else {
    alu.execute(&di);
  }
}

bool CortexM0::is_halted()
{
  return m_halted;
}

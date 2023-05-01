#pragma once

#include <bits/stdc++.h>

#include "debug/log.hh"
#include "debug/debugger.hh"

#include "bus/sysbus.hh"
#include "bus/memory.hh"
#include "bus/serial.hh"

#define RAM_ADDR 0x0000'0000
#define IMG_ADDR 0x0000'8000
#define STK_ADDR 0x2000'0000

#define DEVICE_BASE 0xa0000000
#define MMIO_BASE 0xa0000000

#define SERIAL_PORT     (DEVICE_BASE + 0x00003f8)
#define KBD_ADDR        (DEVICE_BASE + 0x0000060)
#define RTC_ADDR        (DEVICE_BASE + 0x0000048)
#define VGACTL_ADDR     (DEVICE_BASE + 0x0000100)
#define AUDIO_ADDR      (DEVICE_BASE + 0x0000200)
#define DISK_ADDR       (DEVICE_BASE + 0x0000300)
#define FB_ADDR         (MMIO_BASE   + 0x1000000)
#define AUDIO_SBUF_ADDR (MMIO_BASE   + 0x1200000)

#define panic(x) do {   \
  Log(x);               \
  exit(EXIT_FAILURE);   \
} while (0)

#define panicifnot(cond) do {   \
    if (!(cond)) {                 \
        Log(#cond " fail");     \
        exit(EXIT_FAILURE);     \
    }                           \
} while (0)

template <uint8_t width>
constexpr uint32_t Lo32Mask = width > 31 ? -1 : ((uint32_t)1 << width) - (uint32_t)1;

template <uint8_t width>
constexpr uint32_t Hi32Mask = ~Lo32Mask<width>;

template <uint8_t hi, uint8_t lo>
constexpr uint32_t Mask32 = Lo32Mask<hi + 1> & Hi32Mask<lo>;

template <uint8_t width>
constexpr uint64_t Lo64Mask = width > 63 ? -1 : ((uint64_t)1 << width) - (uint64_t)1;

template <uint8_t width>
constexpr uint64_t Hi64Mask = ~Lo64Mask<width>;

template <uint8_t hi, uint8_t lo>
constexpr uint32_t Mask64 = Lo64Mask<hi + 1> & Hi64Mask<lo>;


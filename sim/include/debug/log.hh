#pragma once

#include <cstdio>
#include <ctime>

#ifdef DEBUG_MODE
#define Log(fmt, ...) printf("[%s@%d %ld] " fmt "\n", __FILE__, __LINE__, clock(), ## __VA_ARGS__)
#else
#define Log(...)
#endif

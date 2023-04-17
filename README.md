# NANOS-LITE For ARM

Target Platform: Cortex-m0
Simulator: [usim](https://github.com/bravegnu/usim)

## Change in USIM

- remove all test files
- change boot stage to
    1. load msp at 0x0000'0000
    2. load rst at 0x0000'0004 and jump to rst

## Change in NANOS-LITE

- add arm syscall using svc
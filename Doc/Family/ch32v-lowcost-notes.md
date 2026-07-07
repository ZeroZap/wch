# CH32V Low-Cost And CH32L103 Family Notes

This document extracts CH32V003, CH32V006/007/M007, and CH32L103 guidance from `Doc/Ref/wch-dev-skill` into repository-specific normalization notes.

Official EVT examples, RM, DS, startup files, linker scripts, board schematics, and current repository source remain the final authority.

## Source Files

- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/recipes/new_project.md`
- `Doc/Family/family-routing.md`
- `Doc/Family/family-normalization-notes.md`

## Supported Chips And EVT Roots

| Source group | Repository EVT roots | Source-note memory | Key constraints |
|---|---|---|---|
| CH32V003 | Verify `CH32V003EVT/` availability | 16 KB Flash, 2 KB RAM | Very small RAM, 64-byte Flash pages, SDI printf often useful. |
| CH32V006/007/M007 | Verify `CH32V006EVT/` availability | Up to 62 KB Flash, 8 KB RAM | More analog/touch/motor examples than V003. |
| CH32L103 | `CH32L103EVT/` | 64 KB Flash, 20 KB RAM | USBFS, USB-PD, CAN, RTOS, low-power and RTC examples in source notes. |

## Architecture, Toolchain, Startup, Linker

- Architecture: low-cost WCH RISC-V StdPeriph-style family.
- Toolchain: MounRiver `.project` / `.wvproj` with `SRC/Core`, `SRC/Debug`, `SRC/Ld`, `SRC/Peripheral`, and `SRC/Startup` linked resources.
- Header names differ: CH32V003 uses `ch32v00x.h`, CH32V006 source notes mention `ch32v00X.h`, and CH32L103 uses `ch32l103.h`.
- Small chips require explicit stack/heap budgeting and careful local buffer use.

## Memory And Boot Layout

- Source notes list CH32V003 16 KB Flash / 2 KB RAM with 256-byte default stack in linker examples.
- CH32V006/007/M007 source notes list up to 62 KB Flash / 8 KB RAM.
- CH32L103 source notes list 64 KB Flash / 20 KB RAM.
- IAP source notes reserve 4 KB boot area and use app origin `0x1000`.
- Flash erase size differs: CH32V003 uses 64-byte pages; CH32V006/L103 use 1024-byte pages in the source notes.

## Peripheral And Example Coverage

CH32V003 examples cover ADC, DMA, EXTI, Flash, GPIO, I2C, IAP, interrupts, watchdogs, OPA, PWR, RCC, SDI printf, SPI, SysTick, Timer/PWM, USART, USART IAP, and WWDG. CH32V006 adds more OPA/CMP, touch, motor, idle receive, and SLI/SLTIM-style examples in source notes. CH32L103 adds CAN, CRC, RTOS, USB, USB-PD, RTC, LPTIM, PMP, richer OPA/CMP/PGA, and low-power examples.

## Topic Cross-References

- Common peripherals: `Doc/HAL/wch-hal-normalization.md`.
- USB-PD: `Doc/USBPD/wch-usbpd-notes.md`.
- USB: `Doc/USB/wch-usb-notes.md`.
- System/analog/power: `Doc/System/wch-system-analog-power-notes.md`.
- Templates: `Doc/Templates/wch-project-template-notes.md`.
- RTOS: `Doc/RTOS/wch-rtos-notes.md`.

## Known Family Pitfalls

- Enable peripheral and GPIO clocks before initialization.
- Use the correct low-cost header; do not include CH57x or unrelated CH32 headers.
- Flash page size differs by chip; do not assume 1 KB pages on CH32V003.
- GPIO alternate-function, analog input, and I2C open-drain modes must match the peripheral.
- RISC-V interrupt handlers need the WCH fast interrupt attribute where used by EVT examples.
- Call `SystemCoreClockUpdate()` and `Delay_Init()` before baud-rate or timing-dependent code.
- TIM1 PWM needs main output enable.
- EXTI lines need explicit GPIO-to-EXTI mapping.
- CH32V003 RAM is too small for large local buffers, RTOS, or broad generated stacks.

## Verification Checklist

- Verify exact EVT roots and chip coverage present in this repository.
- Verify header names, startup files, linker RAM/Flash sizes, stack size, and debug method per chip.
- Verify Flash page size, IAP layout, SDI printf availability, interrupt attributes, and vector-in-RAM examples.
- Verify CH32L103-only features such as USB, USB-PD, CAN, RTOS, RTC, LPTIM, and richer analog blocks.

## Verification Status

- Extracted from `Doc/Ref/wch-dev-skill` Markdown sources and repository topic notes listed above.
- Not fully verified against all low-cost CH32V/CH32L EVT trees, RM, DS, project files, startup files, linker scripts, packages, or board schematics in this pass.
- Treat chip support, memory size, page size, and feature availability as preliminary until checked against exact official material.

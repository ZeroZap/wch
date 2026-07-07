# CH32X And CH6xx USB-PD Family Notes

This document extracts CH32X035, CH32X315, CH641, CH643, and related CH6xx USB-PD guidance from `Doc/Ref/wch-dev-skill` into repository-specific normalization notes.

Official EVT examples, RM, DS, startup files, linker scripts, board schematics, USB-PD/Type-C specifications, and current repository source remain the final authority.

## Source Files

- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/recipes/new_project.md`
- `Doc/Family/family-routing.md`
- `Doc/Family/family-normalization-notes.md`

## Supported Chips And EVT Roots

| Chip/source group | Repository EVT root | Source-note memory | Key constraints |
|---|---|---|---|
| CH32X035 | `CH32X035EVT/` | 62 KB Flash, 20 KB RAM | USB-PD, USB, PIOC, touch/key, small Flash layout. |
| CH32X315 | `CH32X315EVT/` | 192 KB Flash, 64 KB RAM | USBHS/USBSS examples in source notes, CRC/RTC and richer peripherals. |
| CH643 | Verify EVT root before use | 192 KB Flash, 64 KB RAM | USB-PD, PIOC, OPA, RTOS examples in source notes. |
| CH641 | Verify EVT root before use | 16 KB Flash, 2 KB RAM | Very small memory; USB-PD and basic peripherals only. |
| CH634 | Source notes say no EXAM directory | N/A | Use CH32X035/CH643 examples as reference only after hardware verification. |

## Architecture, Toolchain, Startup, Linker

- Architecture: WCH RISC-V StdPeriph-style family.
- Toolchain: MounRiver `.project` / `.wvproj` with shared `SRC/Core`, `SRC/Debug`, `SRC/Ld`, `SRC/Peripheral`, and `SRC/Startup` in source templates.
- Linker scripts must be selected by exact memory density. CH641 uses a much smaller stack default in source notes.
- Source memory notes state no `.highcode` section for these chips unlike CH57x.

## Memory And Boot Layout

- Standard app origin is `0x00000000`.
- IAP source layout reserves a 4 KB bootloader at Flash start and uses app origin `0x00001000`.
- Flash page erase/program unit in source notes is 256-byte page erase and 4-byte word program.
- CH641 has only 2 KB RAM; stack, heap, USB-PD state, and buffers must be minimized.

## Peripheral And Example Coverage

Source examples include ADC, DMA, EXTI, Flash, GPIO, I2C, IAP, watchdogs, PWR, RCC, SPI, SysTick, TIM, USART, USB, USBPD, SDI printf, and chip-dependent RTOS/PIOC/OPA/PMP/TOUCHKEY/USBHS/USBSS/CRC/RTC examples.

## Topic Cross-References

- USB-PD and PIOC: `Doc/USBPD/wch-usbpd-notes.md`.
- USB device/host: `Doc/USB/wch-usb-notes.md`.
- Common peripherals: `Doc/HAL/wch-hal-normalization.md`.
- IAP: `Doc/IAP/wch-iap-ota-notes.md`.
- RTOS: `Doc/RTOS/wch-rtos-notes.md`.
- Project templates: `Doc/Templates/wch-project-template-notes.md`.

## Known Family Pitfalls

- Enable RCC bus clocks before peripheral access.
- Configure GPIO alternate-function mode before peripheral output.
- Unlock Flash and use page-aligned erase before programming.
- ADC must be calibrated before accurate conversions.
- TIM1 PWM needs main output enable (`MOE`) in source notes.
- USB-PD requires CC pull settings plus comparator thresholds.
- `USBPD_PHY_V33` must match actual VDD; wrong setting can damage or break the PHY.
- AFIO clock must be enabled before pin remap.
- NVIC priority grouping should be configured before individual interrupt priorities.

## Verification Checklist

- Verify exact EVT roots for CH32X035, CH32X315, CH641, CH643, and whether CH634 has usable examples in this repository.
- Verify headers, startup files, linker scripts, memory densities, stack sizes, and `debug.c/h` per chip.
- Verify USB-PD CC pins, CC thresholds, `USBPD_PHY_V33`, VBUS sensing, and board Type-C pull resistor design.
- Verify PIOC SRAM load/remap sequence and microcode examples.
- Verify USBHS/USBSS availability on CH32X315-class examples against active EVT material.

## Verification Status

- Extracted from `Doc/Ref/wch-dev-skill` Markdown sources and repository topic notes listed above.
- Not fully verified against all CH32X/CH6xx EVT trees, RM, DS, USB-PD headers, project files, startup files, linker scripts, packages, or board schematics in this pass.
- Treat chip support, USB controller availability, USB-PD constants, and memory values as preliminary until checked against exact official material.

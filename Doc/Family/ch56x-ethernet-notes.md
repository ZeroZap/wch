# CH56x Ethernet Family Notes

This document extracts CH569 / CH56x guidance from `Doc/Ref/wch-dev-skill` into repository-specific normalization notes for Ethernet, USB3, eMMC, HSPI, ECDC, DVP, storage, and high-speed template work.

Official EVT examples, RM, DS, startup files, linker scripts, board schematics, PHY/storage/camera datasheets, and current repository source remain the final authority.

## Source Files

- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/new_project.md`
- `Doc/Family/family-routing.md`
- `Doc/Family/family-normalization-notes.md`

## Supported Chips And EVT Roots

| Family | Repository EVT root | Architecture | Notes |
|---|---|---|---|
| CH569 / CH56x | `CH569EVT/` | RISC-V, MounRiver | Source family covers CH569 high-speed peripherals. CH561/CH563 are ARM7TDMI and must use `Doc/Family/ch561-ch563` material in a future pass. |

## Architecture, Toolchain, Startup, Linker

- Toolchain: MounRiver Studio project files with `CH56x_common.h` and shared `EXAM/SRC` resources.
- Shared source roots from source notes: `SRC/Peripheral`, `SRC/Startup`, `SRC/Ld`, `SRC/RVMSIS`, and `SRC/Flash_Lib`.
- Clock rule: source pitfalls require `SystemInit(CLK_SOURCE_PLL_120MHz)` before delay and peripheral init for correct UART/timer timing.
- Debug rule: UART0 is the common printf path in source templates; debug UART selection may be controlled by `CH56x_common.h` defines.

## Memory And DMA Layout

The defining CH56x constraint is RAM vs RAMX.

| Region | Source-note address | Size | Use |
|---|---:|---:|---|
| Flash | `0x00000000` | 448 KB code plus last ~64 KB data area | Program, constants, bootloader/app/data partitions. |
| RAM | `0x20000000` | 16 KB | General data, stack, `.data`, `.bss`; not DMA-accessible in source notes. |
| RAMX | `0x20020000` | 32 KB | `.dmadata`, `.highcode`, DMA buffers, fast ISR code, ECDC/ETH/eMMC buffers. |

Rules:

- Place Ethernet, eMMC, SPI/HSPI, and other DMA buffers in `.dmadata` in RAMX.
- Keep time-critical ISRs or fast code in `.highcode` when latency matters.
- Avoid large local buffers because the default stack is small and regular RAM is only 16 KB.
- Flash erase granularity is 256 bytes and write granularity is 4 bytes according to source notes.
- IAP source layout uses a 4 KB bootloader, app at `0x1000`, download buffer around `0x40000`, and data storage near `0x70000`; verify exact EVT linker before use.

## Peripheral And Example Coverage

Source examples include:

- Ethernet: WCHNET TCP client/server, UDP client/server, DHCP, DNS, raw IP ping, MAC raw, and ETH source examples.
- USB3: USBSS device and host examples.
- HSPI: normal mode, burst mode, dynamic up/down role switch, double-DMA mode, and HSPI with ECDC.
- ECDC: AES/SM4 hardware encryption/decryption.
- DVP: OV2640-style camera capture.
- Storage: SD, eMMC, and eMMC with AES-related source notes.
- Common peripherals: GPIO, UART0-3, SPI0/1, TMR0-2, PWMX, Flash, IAP, BUS8, option bytes, low power.

## Topic Cross-References

- Ethernet/WCHNET: `Doc/ETH/wch-ethernet-notes.md`.
- USB3/USB HS: `Doc/USB/wch-usb-notes.md`.
- Storage/eMMC/HSPI: `Doc/Storage/wch-storage-notes.md`.
- ECDC/security: `Doc/Security/wch-security-crypto-notes.md`.
- DVP camera and streaming IO: `Doc/IO/wch-io-media-notes.md`.
- BUS8/PWMX/HMI-specialty: `Doc/HMI/wch-hmi-specialty-notes.md`.
- Project templates: `Doc/Templates/wch-project-template-notes.md`.
- IAP and Flash layout: `Doc/IAP/wch-iap-ota-notes.md`.

## Known Family Pitfalls

- DMA buffers in normal RAM cause Ethernet/eMMC/SPI DMA hangs or corruption; use RAMX `.dmadata`.
- Peripheral clocks are gated by default; enable `BIT_SLP_CLK_*` for ETH, UART, SPI, TMR, EMMC, USBSS, ECDC, etc.
- CH561/CH563 are ARM7TDMI and cannot compile CH569 RISC-V code or use CH569 APIs directly.
- Flash sector erase destroys the whole 256-byte sector; partial writes require read-modify-write.
- Interrupt handlers may need `.highcode`/RAMX placement for latency.
- Ethernet PHY failures require checking PHY address, MDC/MDIO remap, power, reset, and 25 MHz clock.
- ECDC key registers must be initialized before use.
- eMMC buffers need at least word alignment and DMA-accessible placement.
- GPIO pin remap conflicts must be resolved before mixing GPIO and peripheral signals.

## Verification Checklist

- Verify `CH569EVT/EVT/EXAM` categories and exact example paths for ETH, USBSS, HSPI, ECDC, DVP, SD, EMMC, IAP, and common peripherals.
- Verify `Link.ld` RAM/RAMX sections, `.dmadata`, `.highcode`, stack size, and Flash partition layout.
- Verify WCHNET library requirements, periodic task behavior, PHY interface, DMA descriptor placement, and board PHY address.
- Verify USBSS/USBHS controller instance, endpoint/buffer constraints, and host/device examples.
- Verify ECDC test vectors, key/counter handling, RAMX length units, and HSPI-linked DMA mode.
- Verify eMMC/SD block size, alignment, bus width, and encrypted-transfer examples.

## Verification Status

- Extracted from `Doc/Ref/wch-dev-skill` Markdown sources and repository topic notes listed above.
- Not fully verified against CH569 EVT source trees, RM, DS, project files, startup files, linker scripts, board schematics, PHY datasheets, storage datasheets, or camera datasheets in this pass.
- Treat memory layout, DMA constraints, clocks, and feature availability as preliminary until checked against official CH569 material and active headers.

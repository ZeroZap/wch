# WCH Project Template Notes

This document extracts new-project setup guidance from `Doc/Ref/wch-dev-skill` into repository-specific rules for future project templates, CubeX generation, and example migration.

Official EVT projects, toolchain project files, startup files, linker/scatter files, RM, DS, and current repository source remain the final authority.

## Source Files

- `Doc/Ref/wch-dev-skill/AGENTS.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/recipes/new_project.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/recipes/new_project.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/recipes/new_project.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/new_project.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/recipes/new_project.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/recipes/new_project.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/new_project.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/new_project.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/recipes/new_project.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/new_project.md`

## Template Strategy

Use the closest official EVT example as the starting point. Do not generate a project from isolated snippets unless the startup file, linker/scatter file, system clock code, debug output, interrupt file, and toolchain project metadata are already known for the exact chip family.

Rules:

- Select project templates by architecture, chip family, toolchain, and feature profile.
- Keep vendor startup, system, linker, scatter, debug, and StdPeriphDriver files from the closest EVT example until verified.
- Treat BLE, USB, Ethernet, RTOS, IAP, and high-speed interface examples as feature profiles, not small additions to a bare GPIO template.
- Generated templates should preserve source traceability back to the exact EVT example and `Doc/Ref/wch-dev-skill` source recipe.

## Family Template Matrix

| Family | Toolchain style | Base project rule | Must preserve |
|---|---|---|---|
| CH57x BLE/RISC-V | MounRiver `.project` / `.wvproj` | Link or copy `Ld`, `RVMSIS`, `Startup`, `StdPeriphDriver`; choose BLE or non-BLE flow early | `config.h` for BLE, `Main_Circulation()` for BLE, CH57x linker RAM layout. |
| CH58x/CH59x BLE/RISC-V | MounRiver `.project` / `.wvproj` | Copy closest BLE, USB, UART, LCD, or GPIO example from `EXAM` | Correct `CH58x_common.h` or `CH59x_common.h`, BLE heap, linked resources. |
| CH32V general | MounRiver `.project` / `.wvproj` | Copy closest CH32V103/V20x/V307/V407 EVT example | Correct `ch32v*_conf.h`, `system_ch32v*.c`, startup, `Ld/Link.ld`, debug UART. |
| CH32V low-cost / CH32L103 | MounRiver `.project` / `.wvproj` | Copy closest CH32V003/CH32V006/CH32L103 example | Small Flash/RAM linker values, debug mode such as USART or SDI printf, low-resource settings. |
| CH32X/CH64x USB-PD | MounRiver `.project` / `.wvproj` | Copy closest GPIO, USART, USBPD, USB, PIOC, DMA, PWR, or IAP example | Correct `ch32x035.h`, `ch32x3x5.h`, `ch643.h`, or `ch641.h`; startup and memory density. |
| CH32H417 | MounRiver `.project` / `.wvproj` | Copy closest CH32H417 example and select core/linker profile | `Link_v5f.ld` or `Link_v3f.ld`, bootloader offset at `0x10000`, ITCM/DTCM layout. |
| CH569 / CH56x | MounRiver `.project` / `.wvproj` | Copy closest CH569 GPIO/UART/high-speed example | `CH56x_common.h`, `Flash_Lib`, startup, RAM/DMA-accessible memory rules. |
| CH561/CH563 ARM7TDMI | Keil MDK `.uvproj` plus scatter file | Copy closest Keil MDK example | `CH561SFR.H` or `CH563SFR.H`, `SYSFREQ.H/C`, `STARTUP.S`, `*.SCF`, `ISPXT56X.O`. |
| CH32F / CH32M ARM Cortex-M | Keil MDK or MounRiver | Copy closest CH32F10x/F20x/M030 example | Correct `ch32f10x` or `ch32f20x` headers, `*_conf.h`, interrupt file, system file, scatter/linker file. |
| CH5xx 8051 | Keil C51 / SDCC-style project | Copy EVT C51 project structure or create `Public`, driver, and app groups | `CH5xx.H`, `Debug.C/H`, C51 memory model, code ROM limit when ISP bootloader is reserved. |

## Common Project Shape

Generated templates should keep these layers separate:

```text
Application code       # User main, application modules, profiles
Board support          # LED, debug UART, buttons, sensor/codec/transceiver pins
Feature profile        # BLE, USB, ETH, RTOS, IAP, CAN, storage, display
Vendor driver source   # StdPeriphDriver, register headers, BLE stack, USB stack
Core/startup/linker    # startup assembly, system clock, vector table, Link.ld/.sct
Toolchain metadata     # .project/.wvproj, .uvproj, include paths, defines
```

Rules:

- Keep board pins and application logic out of vendor driver directories.
- Keep generated user files small and replaceable; avoid editing vendor headers directly.
- Store all required include paths, linked resources, preprocessor defines, linker scripts, and startup files in template metadata.
- Use the official example's directory conventions unless there is a deliberate repository-level migration plan.

## Main Loop Rules

Bare-metal StdPeriph-style templates:

1. Configure interrupt priority grouping when the family uses NVIC/PFIC grouping.
2. Update `SystemCoreClock` and initialize delay/debug output.
3. Print clock and chip ID when debug output is available.
4. Initialize GPIO and feature peripherals after their clocks and pins are configured.
5. Keep the main loop explicit; do not hide blocking protocol loops behind unrelated helpers.

BLE templates:

- Initialize clock, optional DCDC, debug UART, BLE stack, HAL, GAP role, services, and application state in the order used by the official BLE example.
- Call `Main_Circulation()` and treat it as non-returning.
- Size `BLE_MEMHEAP_SIZE`, buffer length/count, sleep policy, TX power, and connection limits in `config.h`.

8051 templates:

- Use safe-mode sequences for protected register writes.
- Keep `data` and `xdata` memory use explicit.
- Enable global interrupts only after clock, debug UART, and peripheral state are configured.
- Keep ROM/code-size limits below any reserved ISP bootloader region.

## Linker And Memory Rules

Linker/scatter files are template-critical assets.

Rules:

- Do not share linker scripts across chips with different Flash/RAM size, bootloader offset, ITCM/DTCM, BLE RAM reservation, or IAP layout.
- For IAP applications, derive Flash origin and length from `Doc/IAP/wch-iap-ota-notes.md` rather than hard-coding `0x1000` everywhere.
- For CH32H417, preserve V5F/V3F linker distinctions and bootloader offset rules.
- For CH57x/CH58x/CH59x BLE projects, verify RAM origin/length against BLE stack and chip memory layout before changing heap sizes.
- For CH561/CH563, use Keil scatter files and ARM7 startup rules instead of CH32 RISC-V linker assumptions.
- For CH5xx, enforce Keil C51 memory model, code banking, and ISP bootloader reservation in project options.

## Toolchain Metadata Rules

MounRiver/Eclipse-style projects:

- Preserve `.project`, `.cproject` where present, `.wvproj`, linked resource paths, include paths, build defines, and linker script selection.
- Linked resources such as `Ld`, `RVMSIS`, `Startup`, and `StdPeriphDriver` must point to the correct EVT `SRC` tree for the selected family.
- Missing linked resources commonly cause undefined references or missing headers.

Keil MDK/C51 projects:

- Preserve `.uvproj` or `.uvprojx`, target device, include paths, scatter file, memory model, and HEX output settings.
- For C51, set memory model and code banking deliberately; generated code should not silently switch between `data`, `idata`, `xdata`, and code banking assumptions.

## Debug Output Rules

- Pick one debug output path per template: UART, SDI printf, semihosting-like debug, or none.
- Store debug UART instance, pins, baud rate, and required GPIO mode in board metadata.
- Do not rely on debug output for production timing or low-power behavior.
- If debug pins conflict with a feature such as USB-PD, PIOC, KEYSCAN, or SWD/SWDIO remap, make the conflict explicit.

## Common Pitfalls

| Pitfall | Consequence | Rule |
|---|---|---|
| Starting from snippets instead of EVT project | Missing startup, linker, system, or project metadata | Copy closest official example first. |
| Wrong chip header or `*_conf.h` | Build succeeds with wrong peripheral definitions or fails at link time | Select header and driver prefix by exact family. |
| Missing linked resources | Undefined references or missing include files | Preserve `.project` linked resource paths. |
| Reusing linker script across memory variants | Flash/RAM overlap, bootloader overwrite, or BLE heap corruption | Derive memory from exact chip and image role. |
| Omitting BLE `Main_Circulation()` | BLE stack stops processing | Preserve BLE main-loop pattern. |
| Copying CH32V template to CH561/CH563 | Wrong architecture and startup model | Use ARM7 register-level Keil template. |
| Copying CH32 template to CH5xx | Wrong architecture, memory model, and interrupt syntax | Use Keil C51/8051-specific structure. |
| Debug UART pins not configured | No `printf` output | Model debug UART pins and clocks. |

## Template Metadata Checklist

Future CubeX/template metadata should represent these fields explicitly:

- Target chip, package, architecture, family, exact EVT example source, and feature profile.
- Toolchain type, project file names, linked resources, include paths, defines, startup file, system file, linker/scatter file.
- Memory layout: Flash/RAM origin and length, bootloader/app offset, BLE heap, stack/heap, ITCM/DTCM or DMA-accessible regions.
- Debug path: UART/SDI/none, pins, baud rate, clock, and conflict notes.
- Main-loop profile: bare-metal, BLE TMOS, RTOS, IAP bootloader, USB stack, Ethernet stack, or feature-specific scheduler.
- Board hooks: LED, button, reset, BOOT pin, transceiver/codec/sensor pins, oscillator, and power rails.
- Verification source: exact EVT example, RM/DS chapter, source recipe, and successful build command/toolchain version.

## Verification Status

- Extracted from `Doc/Ref/wch-dev-skill` Markdown sources listed above.
- Not verified against every EVT project file, compiler version, startup assembly, linker/scatter file, memory density, debug probe, or board variant in this pass.
- Treat directory structures, project file names, include paths, linker values, debug pin choices, and main-loop snippets as preliminary until checked against the exact target EVT example and toolchain.

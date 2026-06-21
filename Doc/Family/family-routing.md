# WCH Chip Family Routing

This document is the repository-specific entry point for routing WCH chips to their source references, EVT directories, and future HAL family notes.

Sources:

- `Doc/Ref/wch-dev-skill/SKILL.md`
- `Doc/Ref/wch-dev-skill/AGENTS.md`
- Repository EVT directories at the workspace root
- Official references under `Doc/DS/` and `Doc/RM/`

## Routing Table

| Chip or family | Skill source family | Repository EVT/source directories | Architecture | Primary focus |
|---|---|---|---|---|
| CH57x, CH572, CH573, CH579 | `Doc/Ref/wch-dev-skill/chips/ch57x/` | `CH572EVT/`, `CH573EVT/` | RISC-V | BLE 5.x, RF, USB, LCD/NET variants |
| CH58x, CH59x, CH583, CH585, CH592, CH595 | `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/` | `CH583EVT/`, `CH585EVT/`, `CH592EVT/`, `CH595EVT/` | RISC-V | BLE, USB, LCD, NFCA, encoder/keyscan variants |
| CH32V103, CH32V20x, CH32V307, CH32V407 | `Doc/Ref/wch-dev-skill/chips/ch32v-general/` | `CH32V103EVT/`, `CH32V20xEVT/`, `CH32V307EVT/`, `CH32V407EVT/` | RISC-V | StdPeriphDriver, USB, ETH, CAN, FSMC/LTDC, high-speed variants |
| CH32V003, CH32V006, CH32L103 | `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/` | Low-cost material in `Doc/DS/`, `Doc/RM/`, and related EVT sources | RISC-V | Small-resource HAL subset, OPA/CMP, low-cost Flash/RAM constraints |
| CH32F103, CH32F20x, CH32M030 | `Doc/Ref/wch-dev-skill/chips/ch32f-arm/` | `CH32M030EVT/` plus related official docs | ARM Cortex-M3 | ARM StdPeriphDriver compatibility, USB, Ethernet, USB-PD references |
| CH32X035, CH32X315, CH641, CH643 | `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/` | `CH32X035EVT/`, `CH32X315EVT/` | RISC-V | USB-PD, PIOC, USB, touch/key peripheral variants |
| CH569 | `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/` | `CH569EVT/` | RISC-V | Ethernet, USB3, eMMC, ECDC, HSPI, DVP |
| CH32H417 | `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/` | High-performance references in `Doc/DS/`, `Doc/RM/`, and future EVT imports | RISC-V | USB3, USB-PD, LTDC, SerDes, high-performance peripherals |
| CH561, CH563 | `Doc/Ref/wch-dev-skill/chips/ch561-ch563/` | Official docs and any imported CH561/CH563 EVT material | ARM7TDMI | Register-level Ethernet/USB HS, ARM7-specific startup and interrupts |
| CH543 through CH559 | `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/` | CH5xx-related official docs and any imported EVT material | 8051 | USB, TouchKey, Type-C, 8051 memory qualifiers and safe mode |

## Repository Rules

- Use this table only for routing and source discovery.
- Treat official `Doc/DS/`, `Doc/RM/`, and EVT source code as the final authority.
- Treat `Doc/Ref/wch-dev-skill` as extraction input, not as final HAL specification.
- Do not mix headers, startup files, linker scripts, or interrupt attributes across routed families.

## Extraction Targets

| Family | First extraction target | Later targets |
|---|---|---|
| `ch57x` | `Doc/BLE/` | `Doc/Core/`, `Doc/USB/`, `Doc/HAL/` |
| `ch58x-ch59x` | `Doc/BLE/` | `Doc/USB/`, `Doc/HAL/`, `Doc/Family/` |
| `ch32v-general` | `Doc/HAL/` | `Doc/Core/`, `Doc/ETH/`, `Doc/USB/`, `Doc/Family/` |
| `ch32v-lowcost` | `Doc/HAL/` | `Doc/Core/`, `Doc/USBPD/`, `Doc/Family/` |
| `ch32f-arm` | `Doc/HAL/` | `Doc/Core/`, `Doc/ETH/`, `Doc/USB/`, `Doc/Family/` |
| `ch32x-usbpd` | `Doc/USBPD/` | `Doc/USB/`, `Doc/HAL/`, `Doc/Family/` |
| `ch56x-ethernet` | `Doc/ETH/` | `Doc/USB/`, `Doc/Core/`, `Doc/Family/` |
| `ch32h-highperf` | `Doc/HAL/` | `Doc/USB/`, `Doc/ETH/`, `Doc/Core/`, `Doc/Family/` |
| `ch561-ch563` | `Doc/ETH/` | `Doc/Core/`, `Doc/Family/` |
| `ch5xx-8051` | `Doc/Core/` | `Doc/USB/`, `Doc/Family/` |

## Open Follow-Ups

- Verify exact EVT coverage for CH32V003, CH32V006, CH32L103, CH32F103, CH32F20x, CH561, CH563, and CH5xx in this repository.
- Create per-family notes only when a family is actively normalized.
- Cross-link this document from future topic documents when they extract family-specific behavior.

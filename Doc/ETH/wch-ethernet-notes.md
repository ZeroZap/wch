# WCH Ethernet Notes

This document extracts Ethernet guidance from `Doc/Ref/wch-dev-skill` into repository-specific notes for future HAL, driver, and template work.

Scope:

- CH569 / CH56x Ethernet and USB3-capable RISC-V family.
- CH561/CH563 ARM7TDMI register-level Ethernet family.
- CH32V307/CH32V407 StdPeriph-style Ethernet family.
- CH32F20x WCHNET Ethernet family.
- CH32H417 high-performance Ethernet family.

Official EVT examples, board schematics, PHY datasheets, RM, DS, and headers remain the final source of truth.

## Source Files

- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/eth_comm.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/recipes/eth_comm.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/eth_comm.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/ethernet.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/eth_comm.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/resources/pitfalls.md`

## Family Coverage

| Family | Ethernet block | PHY model | Software style | Notes |
|---|---|---|---|---|
| CH569 / CH56x | Integrated 10/100M MAC | External PHY over MII/RMII | RISC-V peripheral library | DMA buffers must be in RAMX / `.dmadata` in source notes. |
| CH561/CH563 | Integrated 10/100M MAC | External PHY over MII/RMII | ARM7 register-level | No StdPeriph; uses `R32_ETH_*` registers and `__irq`. |
| CH32V307/CH32V407 | Integrated MAC | CH32V307 source notes mention internal 10BASE-T PHY; 100M needs external PHY | StdPeriph ETH driver | Requires RCC clocks, GPIO remap, DMA descriptors. |
| CH32F20x | Ethernet MAC with WCHNET | Board/variant dependent | WCHNET TCP/IP socket API | `WCHNET_MainTask()` must run periodically. |
| CH32H417 | Integrated 10/100M MAC | Source notes state internal PHY; supports MII/RMII and PTP | StdPeriph-like `ch32h417_eth.h` | Has PTP and high-performance DMA concerns. |

## Hardware Model

Common Ethernet hardware stack:

```text
MCU MAC <-> MII/RMII/RGMII pins <-> PHY <-> magnetics/RJ45
        <-> MDC/MDIO management bus
        <-> DMA descriptor rings and frame buffers
```

HAL metadata should model:

- MAC instance and register base.
- PHY address and management bus pins.
- MII/RMII/RGMII mode and pin mapping.
- PHY reset GPIO and reset timing.
- Reference clock source and PHY clock requirement.
- DMA descriptor count, buffer size, memory region, and alignment.
- MAC address source and uniqueness policy.
- Link status, speed, duplex, auto-negotiation, and checksum offload.

## Initialization Sequence

Generic Ethernet initialization sequence:

1. Initialize system clock and delay/timer base.
2. Enable Ethernet MAC/DMA/clock gates.
3. Configure GPIO pins and remap/AF mode for MDC, MDIO, TX, RX, clock, and control signals.
4. Reset external PHY via GPIO or PHY register when required.
5. Probe PHY address through MDC/MDIO.
6. Configure auto-negotiation or fixed speed/duplex.
7. Software-reset MAC/DMA block.
8. Configure MAC address and frame filter options.
9. Allocate and initialize TX/RX DMA descriptor rings.
10. Enable DMA interrupts or polling path.
11. Start MAC TX/RX and DMA TX/RX.
12. Verify link status before transmitting application frames.

Family variations:

- CH56x uses `PWR_PeriphClkCfg(ENABLE, BIT_SLP_CLK_ETH)` in source notes.
- CH32V/CH32H use `RCC_*PeriphClockCmd(...)` style clock enables.
- CH561/CH563 use register clock gates such as `R8_SLP_CLK_OFF*` bits.
- CH32F20x WCHNET uses a periodic network task and socket-level API rather than direct frame-only examples.

## PHY Access And Link Management

PHY access usually uses MDC/MDIO management registers or helper functions:

- `ETH_ReadPHYRegister(phy_addr, reg)`
- `ETH_WritePHYRegister(phy_addr, reg, value)`
- Register-level CH561/CH563 equivalent uses `R32_ETH_MII_ADDR` and `R32_ETH_MII_DATA`.

PHY handling rules:

- Do not hardcode PHY address without board verification. Scan addresses 0 through 31 when bring-up fails.
- If reads return `0xFFFF` or `0x0000`, check PHY power, reset, MDC/MDIO pin mapping, and reference clock.
- Reset PHY after power-up when the board requires it.
- Wait for auto-negotiation or explicitly configure speed/duplex.
- Re-check link status before sending frames and after PHY change interrupts.

Common registers from source notes:

| Register | Purpose |
|---|---|
| `PHY_BCR` | Basic Control Register, includes reset. |
| `PHY_BSR` | Basic Status Register, includes link and auto-negotiation status. |
| `PHY_PHYIDR1` / `PHY_PHYIDR2` | PHY identity. |
| `PHY_ANAR` | Auto-negotiation advertisement. |

## DMA Descriptors And Buffer Placement

Common descriptor fields:

| Field | Purpose |
|---|---|
| `Status` | Own bit, frame status, first/last segment, length fields. |
| `ControlBufferSize` | Buffer size and descriptor control. |
| `Buffer1Addr` | Frame buffer pointer. |
| `Buffer2NextDescAddr` | Next descriptor pointer or second buffer. |

Rules:

- TX/RX descriptors must be aligned at least to word boundaries.
- Ethernet frames require buffers large enough for full Ethernet frame size, usually around 1518 to 1536 bytes.
- Descriptor rings should wrap the final descriptor to the first descriptor.
- DMA ownership bits must be respected before writing TX descriptors or reading RX descriptors.
- After returning RX descriptors to DMA, resume RX polling/demand if the hardware requires it.

Family-specific buffer placement:

| Family | Buffer placement rule |
|---|---|
| CH569 / CH56x | Source notes require Ethernet DMA buffers and descriptors in RAMX through `.dmadata`; regular RAM is not DMA-accessible for ETH. |
| CH561/CH563 | Source notes place aligned buffers in DATA SRAM, not `.dmadata`. |
| CH32V / CH32H | Source notes require alignment; verify cache/TCM/external RAM constraints in EVT. |
| CH32F20x WCHNET | Socket receive buffers are assigned through WCHNET APIs; check library constraints. |

## Frame-Level API Shape

Low-level Ethernet HAL should distinguish:

- Raw frame send/receive using DMA descriptors or `ETH_HandleTxPkt` / `ETH_HandleRxPkt`.
- Link and PHY management.
- MAC address configuration.
- DMA descriptor ring configuration.
- Higher protocol stack integration such as WCHNET or lwIP-like layers.

Do not merge raw MAC/PHY APIs with socket APIs into one layer. Use separate levels:

```text
Ethernet MAC driver       # descriptors, frames, link, PHY
Network stack adapter     # WCHNET, lwIP, DHCP, DNS, TCP/UDP sockets
Application template      # TCP client/server, UDP, MQTT, etc.
```

## WCHNET Notes

CH32F20x source notes describe WCHNET socket APIs.

Important rules:

- Call `WCHNET_Init(...)` with IP, gateway, mask, and MAC.
- Call `WCHNET_MainTask()` periodically, often from a timer ISR or cyclic task.
- Handle global interrupts such as PHY change, socket events, unreachable, and IP conflict.
- Use per-socket interrupt handling for receive, connect, disconnect, and timeout.
- Configure socket receive buffers explicitly when required.

WCHNET belongs above the raw MAC layer. Future HAL metadata should keep WCHNET as a stack profile, not as a mandatory Ethernet driver API.

## Interrupts And Polling

Interrupt style varies by architecture:

- CH56x RISC-V examples use WCH fast interrupt attributes.
- CH561/CH563 use `__irq` and register-level interrupt flags.
- CH32V/CH32H use StdPeriph/PFIC/NVIC-like APIs depending family.
- WCHNET may depend on timer-driven cyclic task processing even if Ethernet interrupts exist.

Rules:

- Clear DMA RX/TX interrupt flags according to the family API or register write semantics.
- Avoid heavy protocol processing directly inside low-level DMA ISR; defer to task/main loop when practical.
- Always handle fatal bus errors and abnormal interrupt summary flags if the family exposes them.

## Common Pitfalls

| Pitfall | Affected families | Rule |
|---|---|---|
| Ethernet clock not enabled | All | Enable ETH MAC/DMA/peripheral clock before register access. |
| Wrong PHY address | External PHY designs | Scan 0-31 or check schematic/PHY strap pins. |
| MDC/MDIO or MII/RMII pins not configured | All board-level ETH | Configure pin mux/remap/AF before PHY access. |
| PHY not reset after power-up | Many boards | Use reset GPIO or PHY software reset and wait. |
| DMA buffers in inaccessible memory | CH569 / CH56x especially | Place buffers in required RAM region and section. |
| DMA buffers not aligned | All DMA ETH | Align descriptors and frame buffers to at least 4 bytes. |
| Large frame buffers on stack | All | Use static/global buffers; avoid 1500-byte local arrays. |
| Link status ignored | All | Check link before TX/RX and handle PHY change. |
| CH561/CH563 treated as CH569 | CH561/CH563 | Use ARM7 register-level flow, not CH569 RISC-V APIs. |
| CH32V103/CH32V20x assumed to have ETH | CH32V general | Verify exact chip capability; source notes flag CH32V307/V407 for ETH. |
| MAC address duplicated | All networked products | Use WCH-provided MAC, chip UID-derived local MAC, or assigned local admin MAC. |

## Example Routing

| Family | Source example path from skill notes |
|---|---|
| CH569 | `chips/ch56x-ethernet/resources/EXAM/CH569/ETH/` |
| CH561/CH563 | `chips/ch561-ch563/resources/EXAM/CH563/NET/` |
| CH32V307/CH32V407 | `chips/ch32v-general/resources/EXAM/CH32V307/ETH/` |
| CH32F20x | `chips/ch32f-arm/resources/EXAM/CH32F20x/ETH/TcpClient/` |
| CH32H417 | `chips/ch32h-highperf/resources/EXAM/ETH/` |

## Proposed Ethernet Metadata

Future CubeX/HAL metadata should include:

- `has_eth_mac`: true/false.
- `phy_type`: internal, external, or board-defined.
- `phy_interface`: MII, RMII, RGMII, or fixed internal.
- `phy_address`: default, scan, or board-specific.
- `phy_reset_pin`: optional port/pin and timing.
- `mac_address_source`: fixed, UID-derived, WCH-provided, user-provided.
- `dma_buffer_region`: normal RAM, RAMX, DTCM, external RAM, or stack-prohibited.
- `descriptor_alignment`: byte alignment requirement.
- `frame_buffer_size`: default RX/TX buffer size.
- `driver_level`: raw MAC, WCHNET socket stack, lwIP adapter, or board BSP.
- `requires_periodic_task`: true/false for stack profiles such as WCHNET.

## Verification Status

- Extracted from `wch-dev-skill` Markdown only.
- Exact PHY pin mapping, memory section names, DMA accessibility, and stack API must be checked against repository EVT examples and board schematics.
- Next verification pass should inspect imported `ETH` / `NET` examples and active headers for CH569, CH561/CH563, CH32V307/407, CH32F20x, and CH32H417.

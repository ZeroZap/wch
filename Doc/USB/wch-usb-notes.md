# WCH USB Notes

This document extracts USB guidance from `Doc/Ref/wch-dev-skill` into repository-specific notes for future HAL, driver, template, and metadata work.

Scope:

- USB Full-Speed device flows on CH5xx, CH57x, CH58x/CH59x, CH32X/CH64x, CH32V, CH32F, and CH32H families.
- USB host flows on CH57x and CH58x/CH59x.
- USB High-Speed and USB SuperSpeed notes for CH32H417 and CH569-class sources.
- Common endpoint, descriptor, buffer, clock, interrupt, and pitfall rules.

Official EVT examples, RM, DS, board schematics, USB library headers, and USB specifications remain the final source of truth.

## Source Files

- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/recipes/usb_device.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/recipes/usb_device.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/recipes/usb_host.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/recipes/usb_device.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/recipes/usb_host.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/recipes/usb_device.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/usb_device.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/usb_device.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/usb3_device.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/usbfs_device.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/usbhs_device.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/usb3_device.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/resources/pitfalls.md`

## Family Coverage

| Family | USB roles from source notes | Controller tier | Software style | Notes |
|---|---|---|---|---|
| CH543-CH559 | Device; many parts also host | USB FS | 8051 register-level | Endpoint DMA buffers are in `xdata`; addresses must not overlap. |
| CH57x | Device, host, AOA, U disk | USB FS | CH57x USB libraries | USB needs accurate 48 MHz clock; PA11/PA12 are D-/D+. |
| CH58x/CH59x | Device and host | USB FS, some dual-controller parts | CH58x/CH59x USB libraries | CH583/CH585 source notes require `USB2_*` APIs for the second controller. |
| CH32X/CH64x | Device | USB FS | Register-level USBFS macros | Endpoint buffers must be in USB SRAM, not ordinary RAM. |
| CH32V103/V20x/V30x/V40x | Device | USB 2.0 FS in source recipe | StdPeriph USB library / PMA | Requires 48 MHz USB clock and PMA planning. |
| CH32F10x/F20x | Device | USB 2.0 FS device/host controller | WCH register-based API | Not STM32 USB OTG compatible; use WCH EVT USB examples. |
| CH32H417 | Device and host examples by tier | USBFS, USBHS, USBSS | StdPeriph-like plus USB libraries | USBHS is separate from USBFS; USBSS depends on SerDes. |
| CH569-class CH56x source | USBSS and USBHS | USB 3.0 SS plus USB 2.0 HS | CH56x peripheral libraries | Source recipe says USBSS applies to CH569 only; verify exact chip capabilities in RM/DS. |

## USB Tier Model

Keep USB tier, role, and class separate in HAL metadata.

| Tier | Nominal speed | Typical WCH names | Main constraints |
|---|---:|---|---|
| USBFS | 12 Mbps | USB, USBFS | 48 MHz clock, EP0 control, PMA/USB SRAM or endpoint DMA buffers. |
| USBHS | 480 Mbps | USBHS | 480 MHz/high-speed clocking, larger FIFOs, internal or ULPI PHY depending board. |
| USBSS / USB3 | 5 Gbps | USBSS | SerDes/PHY readiness, SuperSpeed descriptors, high-speed PCB layout. |

Do not hide tier differences behind only `usb_device_init()`. A future HAL should expose role, tier, PHY, endpoint memory model, and class stack independently.

## Device Initialization Sequence

Generic USB device sequence:

1. Configure system clock so the selected USB tier receives the required clock.
2. Enable the USB controller clock gate before register access.
3. Configure USB pins or rely on the USB library only when the EVT example does so for that family.
4. Allocate endpoint buffers in the controller-accessible memory region.
5. Reset the USB SIE/controller when required.
6. Initialize EP0 as a control endpoint with a valid 64-byte setup/data buffer for FS.
7. Initialize class endpoints and descriptors.
8. Set the initial device address to zero.
9. Enable pull-up or connect state so the host can enumerate the device.
10. Enable USB interrupts and clear pending flags.
11. Process SETUP, IN, OUT, bus reset, suspend, and resume events.
12. Only start class data transfers after the device reaches configured state.

Family variations:

- CH57x examples explicitly set `SetSysClock(CLK_SOURCE_PLL_48MHz)` for USB.
- CH32V examples use `RCC_USBCLKConfig(...)` and enable the USB peripheral clock.
- CH32X examples enable `RCC_AHBPeriph_USBFS` and use `R8_*` USBFS registers.
- CH32H USBFS examples enable `RCC_HB2Periph_USBFS` and then call `USB_Init()`.
- CH5xx examples configure USB pin function, `USB_CTRL`, DMA addresses, endpoint modes, and 8051 interrupt enable bits directly.
- CH569 USBSS examples enable `BIT_SLP_CLK_USBSS`; USBHS uses a separate clock bit.

## EP0 And Standard Requests

EP0 is mandatory and must handle enumeration before class traffic.

Rules:

- Treat EP0 as a state machine, not as a simple data pipe.
- Always reset device address to zero after bus reset.
- Handle at least `GET_DESCRIPTOR`, `SET_ADDRESS`, and `SET_CONFIGURATION` for successful enumeration.
- Apply `SET_ADDRESS` timing exactly as the EVT stack expects; many controllers commit the address after the status stage.
- Process SETUP packets before returning descriptor or class data.
- Return STALL only for truly unsupported requests; accidental STALL on standard requests breaks enumeration.
- Keep descriptor lengths, total configuration length, endpoint count, and interface count consistent.

Descriptor rules:

- FS control endpoint max packet size is commonly 64 bytes in these recipes.
- CDC usually needs communication and data interfaces, functional descriptors, an interrupt IN endpoint, and bulk IN/OUT endpoints.
- HID needs a HID report descriptor and consistent report lengths in endpoint transfers.
- Composite devices must keep interface numbers, endpoint addresses, class descriptors, and total length synchronized.
- USBSS devices additionally need BOS, USB 2.0 extension, SuperSpeed capability, and SuperSpeed endpoint companion descriptors.

## Endpoint And Buffer Rules

Endpoint buffer memory is controller-specific and should be modeled explicitly.

| Family or tier | Buffer model from source notes | Rule |
|---|---|---|
| CH5xx 8051 | Endpoint DMA buffers in `xdata` | Use even, non-overlapping addresses; set `UEPx_DMA_L/H` to the allocated address. |
| CH32X USBFS | USB SRAM endpoint buffers | Do not place endpoint buffers in ordinary SRAM. |
| CH32V USB library | PMA packet memory | Plan PMA addresses and endpoint sizes; PMA is limited. |
| CH32H USBFS | USB SRAM / PMA | Place endpoint buffers where the USB library/controller can access them. |
| CH32H USBSS | DMA endpoint buffers | Align buffers and use DMA for throughput; check SerDes and USBSS requirements. |
| CH569 USBSS/USBHS | Controller-specific DMA buffers | Verify section, alignment, and accessibility in EVT examples. |

Common endpoint rules:

- Allocate endpoint buffers statically or globally; avoid large USB buffers on the stack.
- Keep IN and OUT buffers separate unless the controller explicitly supports shared EP0 memory.
- Respect double-buffer mode setup; it changes the required memory footprint.
- Re-arm OUT endpoints after consuming data.
- Only mark IN endpoints valid or ready after the buffer and transfer length are set.
- Respect data toggles unless the controller is configured for automatic toggling.
- Map endpoint address, direction, type, max packet size, and buffer address from descriptors into controller configuration.

## Device Transfer Handling

WCH source recipes use two broad styles:

| Style | Families | Typical flow |
|---|---|---|
| Library state machine | CH57x, CH58x/CH59x, CH32V, CH32H USBFS | Initialize USB, call library transfer processing or let ISR dispatch to callbacks. |
| Register-level event handler | CH5xx, CH32X, CH32F-style recipes | Decode interrupt flags, token type, endpoint number, and manually ACK/NAK/STALL. |

Rules:

- Keep class logic outside the lowest-level interrupt parser when possible.
- Do minimal work in the USB ISR: clear flags, move small data, update state, and defer heavier parsing.
- For library-based flows, use the callback names and transfer task expected by that family rather than inventing a new dispatch path.
- For register-level flows, clear interrupt flags using the write semantics defined by the active header or EVT code.
- Do not start CDC, HID, or vendor data movement until the configured state is reached.

## Host Initialization And Enumeration

Generic USB host sequence extracted from CH57x and CH58x/CH59x notes:

1. Provide valid VBUS power and over-current handling at board level.
2. Configure the USB clock and D-/D+ pins.
3. Initialize the host controller with `USB_HostInit()` or family equivalent.
4. Detect root hub connection status.
5. Enable or reset the root hub port.
6. Read the device descriptor.
7. Read the configuration descriptor.
8. Assign an address.
9. Select a configuration.
10. Parse interfaces and endpoints for the target class.
11. Start class transfer flow such as HID polling, AOA, or U disk storage.

Host rules:

- Host mode requires reliable 5 V VBUS power; device detection failures are often hardware power issues.
- Receive buffers must be large enough for descriptors and class responses.
- Endpoint type and max packet size must come from the enumerated descriptor, not hardcoded assumptions.
- U disk examples require the disk/file-system readiness step before file operations.
- AOA examples require the Android device to support the protocol and may force a re-enumeration after accessory identification.

## USBFS Notes

USBFS is the common baseline across many WCH families.

Rules:

- Provide an accurate 48 MHz USB clock.
- Use PA11/PA12 as D-/D+ where the source notes specify that mapping, but verify remap and package pins for the exact part.
- Check whether an internal pull-up is available or an external 1.5 kOhm D+ pull-up is required by the board/controller.
- Keep PMA, USB SRAM, or endpoint DMA memory use explicit in templates.
- Use EVT descriptors and class files as the starting point for CDC, HID, vendor, and composite devices.

## USBHS Notes

USBHS is not just a faster USBFS instance.

Rules:

- Model USBHS as a separate controller and tier with separate interrupt vectors, FIFO sizing, and PHY requirements.
- Configure the high-speed clock tree exactly as the EVT example requires.
- Verify whether the board uses an internal transceiver or an external ULPI PHY.
- Endpoint count and FIFO/buffer sizing differ from USBFS; do not reuse FS PMA assumptions.
- Use USBHS-specific device and host examples for CH32H-class parts.

## USBSS / USB3 Notes

USBSS requires additional hardware, descriptors, and link handling.

Rules:

- Enable the USBSS peripheral clock before USBSS register access.
- Initialize and validate the SerDes/PHY before enabling USBSS traffic.
- Wait for PLL lock and PHY ready on CH32H417-style SerDes flows.
- Verify 90 ohm differential pair layout, SSTX/SSRX routing, and connector wiring before debugging software.
- Provide BOS and SuperSpeed capability descriptors.
- Provide SuperSpeed endpoint companion descriptors for SS endpoints.
- Use DMA and aligned buffers for high-throughput endpoints.
- Handle link state changes and SuperSpeed power states in addition to transfer completion.

CH569/CH56x note:

- The source recipe text is internally inconsistent in its comparison table and notes; treat CH569 USBSS capability as a routing hint and verify exact USBSS/USBHS support against CH569 RM, DS, headers, and EVT examples before designing a common HAL profile.

## Multi-Instance USB

Some CH58x/CH59x parts expose multiple USB controllers.

Rules:

- Model USB instance identity explicitly, for example `usb1` vs `usb2`.
- Use `USB2_*` initialization and transfer processing functions for the second controller where the EVT/source notes require them.
- Do not share endpoint buffers, callback tables, or global device state between instances unless the vendor library explicitly supports it.
- Keep pin mapping, clock gates, interrupt vectors, and descriptors tied to the selected instance.

## Common Pitfalls

| Pitfall | Affected families | Rule |
|---|---|---|
| USB clock not exactly configured | FS device/host flows | Configure the required 48 MHz clock or tier-specific clock before USB init. |
| Peripheral clock gate disabled | CH32X/CH32V/CH32H/CH569 | Enable USBFS/USBHS/USBSS clock before register access. |
| EP0 standard requests incomplete | All device flows | Implement descriptor, address, and configuration handling before class logic. |
| Descriptor length mismatch | All device flows | Keep descriptor array lengths, `wTotalLength`, endpoint count, and interface count consistent. |
| Endpoint buffer in wrong memory | CH5xx, CH32X, CH32V, CH32H, USBSS | Use xdata, USB SRAM, PMA, or DMA-capable memory as required by the controller. |
| Endpoint buffers overlap | Register-level endpoint controllers | Allocate fixed non-overlapping ranges and account for double-buffer mode. |
| OUT endpoint not re-armed | All device flows | Return the OUT endpoint to ACK/valid state after data is consumed. |
| IN transfer started before length/data set | All device flows | Fill buffer, set length, then mark TX valid/ACK/start. |
| Wrong USB instance functions | CH583/CH585 style dual USB | Use `USB2_*` APIs for USB2. |
| Host VBUS missing or weak | Host flows | Validate 5 V supply, current limit, and connector power switch before debugging enumeration. |
| Host descriptor buffer too small | Host flows | Size receive buffers for full device/configuration descriptors. |
| U disk file operations before disk ready | CH57x/CH58x host storage | Call disk readiness/init function before file open/read/write. |
| USBSS SerDes PLL not locked | CH32H417 USB3 | Wait for PLL lock and PHY ready before USBSS traffic. |
| Treating WCH USB as STM32 OTG | CH32F/CH32V-style code | Use WCH USB registers/libraries and EVT examples, not STM32 OTG assumptions. |

## Example Routing

| Family | Source example path from skill notes |
|---|---|
| CH5xx | `chips/ch5xx-8051/resources/EXAM/USB/Device/`, `Host/`, `USB_LIB/` |
| CH57x device | `chips/ch57x/resources/EXAM/USB/Device/COM/`, `HID_CompliantDev/`, `VendorDefinedDev/`, `CompoundDev/` |
| CH57x host | `chips/ch57x/resources/EXAM/USB/Host/HostEnum/`, `HostAOA/`, `U_DISK/` |
| CH58x/CH59x device | `chips/ch58x-ch59x/resources/EXAM/USB/Device/COM/`, `HID/`, `Vendor/` |
| CH58x/CH59x host | `chips/ch58x-ch59x/resources/EXAM/USB/Host/HostEnum/`, `U_DISK/` |
| CH32X | `CH32X035EVT/EVT/EXAM/USB/` |
| CH32V | `chips/ch32v-general/resources/EXAM/` USB examples by exact chip |
| CH32F | `EVT/EXAM/USB/` in the matching CH32F EVT package |
| CH569 USBSS | `CH569EVT/EVT/EXAM/USBSS/USBD/`, `USBH/` |
| CH32H417 USBFS/USBHS/USBSS | `CH32H417EVT/EVT/EXAM/USBFS/`, `USBHS/`, `USBSS/` |

## Proposed USB Metadata

Future CubeX/HAL metadata should include:

- `usb_instances`: list of controllers with instance name and base address.
- `usb_role_support`: device, host, or OTG/dual-role if confirmed.
- `usb_tier`: FS, HS, SS.
- `usb_phy`: internal FS, internal HS, ULPI, SerDes, or board-defined.
- `usb_clock_requirements`: required frequency and clock source constraints.
- `usb_pins`: D-/D+, VBUS, ID, ULPI, SSTX, SSRX, and board power switch pins.
- `endpoint_count`: endpoint numbers and direction capabilities.
- `endpoint_memory_model`: xdata, PMA, USB SRAM, FIFO, DMA buffer, or other.
- `endpoint_buffer_alignment`: byte alignment and section requirements.
- `supports_double_buffer`: per endpoint if known.
- `requires_pullup_control`: internal, external, or board-specific.
- `interrupt_vectors`: low-priority, high-priority, USBHS, USBSS, or instance-specific names.
- `class_profiles`: CDC, HID, vendor, composite, MSC host, AOA host, or other templates.
- `library_style`: vendor USB library, StdPeriph USB library, register-level, or USBSS-specific.

## Verification Status

- Extracted from `wch-dev-skill` Markdown only.
- Exact endpoint counts, endpoint memory sizes, pin remaps, interrupt names, clock tree values, PHY selection, and USBSS capability must be checked against official EVT examples, headers, RM, DS, and board schematics.
- The CH569 USB3 source note contains inconsistent wording/table entries; do not finalize CH56x USB3 capability metadata without official cross-check.
- Next verification pass should inspect imported `USB`, `USBFS`, `USBHS`, and `USBSS` examples for the active families and normalize exact API names from headers.

# WCH USB-PD, Type-C, And PIOC Notes

This document extracts USB-PD, USB Type-C CC detection, and PIOC guidance from `Doc/Ref/wch-dev-skill` into repository-specific rules for future HAL, driver, template, and metadata work.

Scope:

- USB-PD source, sink, and DRP-style flows on CH32X/CH64x, CH32M030, and CH32H417-class sources.
- USB Type-C attach, orientation, Rp/Rd, and current advertisement/detection notes from CH5xx sources.
- PIOC bring-up and usage rules for programmable I/O examples on CH32X035/CH643-class sources.
- Common CC, comparator, PHY voltage, BMC timing, message, interrupt, and pin-remap pitfalls.

Official EVT examples, RM, DS, schematics, USB-PD specifications, Type-C specifications, and current repository source remain the final authority.

## Source Files

- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/recipes/usbpd_config.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/recipes/pio_config.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/usbpd.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/usbpd_config.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/recipes/type_c.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/resources/example_list.md`

## Family Coverage

| Family or source group | USB-PD / Type-C support from source notes | Software style | Rules to preserve |
|---|---|---|---|
| CH32X035, CH32X315, CH641, CH643 | USB-PD examples for source, sink, DRP/state-machine flows; PIOC examples on CH32X035/CH643-class sources | StdPeriph plus USBPD registers; PIOC SFR/microcode | Enable correct USBPD clock domain, configure CC comparators, select active CC line, match BMC timers to clock, and treat PIOC as a separate programmable engine. |
| CH32M030 | Built-in USBPD peripheral; USBPD0/USBPD1 availability varies by package | `PD_Process` example module | Select `PD_SEL` correctly, account for external 5.1K Rd requirements in sink mode, keep 1 ms PD timing, and monitor VBUS. |
| CH32H417 | Integrated USBPD controller with PD 3.0-oriented source notes | StdPeriph-like USBPD registers | Enable the right bus clock, configure `CONFIG`, `CONTROL`, `PORT_CC1`, `PORT_CC2`, and use `USBPD_PHY_V33` according to VDD. |
| CH543-CH559 Type-C sources | Type-C attach, orientation, and current detection through `USB_C_CTRL` and ADC on UCC pins | 8051 SFR/register-level | DFP advertises current with Rp; UFP enables Rd and measures Rp voltage; UCC pins must be floating inputs before ADC reads. |

## Concept Boundaries

Keep USB, Type-C, USB-PD, and PIOC separate in HAL metadata.

| Concept | Purpose | Do not conflate with |
|---|---|---|
| USB D+/D- controller | USB data enumeration and class traffic | USB-PD CC BMC communication. |
| Type-C CC detection | Attach, cable orientation, advertised default/1.5A/3.0A current | Full USB-PD message negotiation. |
| USB-PD controller | BMC signaling, GoodCRC, PDO/RDO negotiation, reset handling | USBFS endpoint or descriptor logic. |
| PIOC | Programmable I/O engine for custom serial protocols | GPIO bit-banging executed by the main CPU. |

Rules:

- USB-PD communicates over CC1/CC2, not D+/D-.
- Type-C current advertisement through Rp/Rd is not the same as PD PDO negotiation.
- USB-PD examples may coexist with USBFS examples, but their clocks, pins, interrupts, and state machines must remain independently modeled.
- PIOC should be modeled as its own peripheral with code SRAM, SFR/data registers, IO mapping, and interrupt behavior.

## Type-C CC Rules

Type-C role decides the CC pull configuration.

| Role | CC pull | Source-note behavior | HAL implication |
|---|---|---|---|
| DFP / Source-facing host | Rp pull-up on both CC pins before attach | CH5xx uses `USB_C_CTRL` PU bits; CH32 USBPD sources use `CC_PU_*` values | Represent advertised current separately from power role. |
| UFP / Sink-facing device | Rd pull-down on both CC pins before attach | CH5xx enables `bUCC1_PD_EN | bUCC2_PD_EN`; CH32M030 sink notes may require external 5.1K Rd | Metadata must record whether Rd is internal or external. |
| DRP | Alternates source/sink behavior | CH32X source notes mention source, sink, and DRP flows | DRP needs explicit policy/state-machine support, not a static role flag. |

Orientation and attach rules:

- Read both CC pins and select exactly one active communication line after attach.
- Treat both-CC-connected or neither-CC-connected results as exceptional states requiring debounce, disconnect handling, or accessory-specific handling.
- On 8051 Type-C examples, set UCC pins to floating input before ADC measurement; output mode causes incorrect or zero readings.
- On CH32 USBPD examples, use `CC_SEL` to select CC1 or CC2 before BMC communication.
- Verify board pin mapping for each instance; CH32M030 USBPD0 and USBPD1 use different CC pins and availability varies by package.

## USB-PD Bring-Up Sequence

Generic USB-PD initialization sequence:

1. Configure system clock and update clock state used by delay/timer code.
2. Enable the USBPD peripheral bus clock for the selected family and instance.
3. Enable GPIO, AFIO, or remap clocks needed for CC pins or instance selection.
4. Configure CC role pulls and comparator thresholds for both CC pins.
5. Configure PHY voltage mode with `USBPD_PHY_V33` according to actual VDD.
6. Configure BMC timing values for the current system clock.
7. Enable PD input filtering and relevant RX/TX/reset interrupts.
8. Detect attach and select the active CC channel.
9. Start the PD state machine in the main loop or dedicated process function.
10. Clear and service interrupt flags without doing heavy policy work in the ISR.

Family variations:

- CH32X source notes use `RCC_AHBPeriphClockCmd(RCC_AHBPeriph_USBPD, ENABLE)` and PC14/PC15 for CH32X035 CC pins.
- CH32H417 source notes use `RCC_APB1PeriphClockCmd(RCC_APB1Periph_USBPD, ENABLE)` in the example.
- CH32M030 source notes use the `PD_Process` module, `PD_SEL`, and a 1 ms TIM1 tick for PD timing.
- USBPD1 on CH32M030 may require AFIO clock and `GPIO_Remap_SDI_Disable` according to the source recipe.

## CC Comparator And PHY Voltage Rules

CC comparator thresholds are required for BMC receive detection.

Rules:

- Do not configure only `CC_PD` or only `CC_PU_*`; pair role pull settings with a suitable comparator threshold.
- Sink examples use pull-down plus a low comparator threshold such as `CC_CMP_22` or `CC_CMP_45` depending family recipe.
- Source examples use pull-up current settings such as `CC_PU_330`, `CC_PU_180`, or `CC_PU_80` plus a comparator threshold such as `CC_CMP_66`.
- Threshold names and exact supported values are header-specific; read the target chip USBPD header before generating constants.

`USBPD_PHY_V33` must match board VDD:

- Set `USBPD_PHY_V33` only for 3.3 V VDD direct mode.
- Clear `USBPD_PHY_V33` when VDD is greater than 4 V so the internal LDO limits the PHY voltage.
- Treat this as a safety rule. A wrong setting can cause no communication or PHY damage.

## BMC Timing And Message Rules

USB-PD protocol handling is timing-sensitive and stateful.

BMC timing rules:

- BMC TX/RX timer values must be derived from the actual system clock.
- Source notes list 48 MHz, 24 MHz, and 12 MHz timer presets; do not reuse 48 MHz values after changing the clock.
- CH32M030 source notes maintain a 1 ms TIM1 tick and call detection/main PD processing from the main loop.

Message rules:

- Keep a real PD state machine for source, sink, DRP, reset, voltage-adjustment, and wait states.
- Handle `GoodCRC` after messages; otherwise the partner can treat the message as lost.
- Increment `Msg_ID` after each successful message sequence as required by the protocol.
- Validate sink requests against available source PDOs before sending `Accept` and changing voltage.
- Send `PS_RDY` only after the requested power supply state is actually ready.
- Hard Reset is PHY-level BMC signaling and cannot be modeled as only a normal PD message.
- PD 2.0/3.0 basic negotiation examples do not imply full PPS, AVS, or extended message support.

PDO/RDO rules:

- Fixed PDO voltage is encoded in 50 mV units and current in 10 mA units in the source recipe.
- PPS/APDO fields use different units and bit positions; do not parse them as fixed supply PDOs.
- Avoid hard-coding unsafe voltage requests. Source notes explicitly warn that incorrect requested voltage can damage connected devices.
- Monitor VBUS for attach/removal and power-supply state, especially in sink examples.

## Interrupt And State Handling

USBPD ISR rules:

- Read status once, handle only set flags, and clear each handled flag in the family-prescribed way.
- Treat receive complete, transmit complete, reset, and buffer error flags separately.
- Use `volatile` flags or a small event queue to defer policy/state-machine work out of the ISR.
- Keep reset handling explicit; receiving `IF_RX_RESET` should move the PD state machine to a reset/recovery state.

Main-loop rules:

- Call PD detection and PD main processing at the cadence expected by the EVT example.
- Protect shared millisecond counters or deltas when updated by timer ISRs.
- Keep power-stage control synchronized with PD policy state, not just message receipt.

## PIOC Rules

PIOC is a small programmable I/O engine with its own code, SFR/data registers, IO pins, and interrupt request path.

Initialization sequence:

1. Enable GPIOC and AFIO clocks needed by the selected PIOC pins.
2. Enable PIOC pin remap with `GPIO_PinRemapConfig(GPIO_Remap_PIOC, ENABLE)` when required.
3. Disable conflicting SWJ/SDI remaps only when the selected board/pins need those pins.
4. Configure TX pins as alternate-function push-pull and RX pins as input pull-up or the protocol-required mode.
5. Enable and prioritize `PIOC_IRQn`.
6. Load precompiled PIOC microcode into `PIOC_SRAM_BASE` with the required alignment.
7. Reset/release PIOC and enable `RB_MST_IO_EN0`, `RB_MST_IO_EN1`, and `RB_MST_CLK_GATE` in the source-prescribed order.
8. Configure protocol parameters in data registers before starting transfers through `R8_CTRL_WR`.

PIOC design rules:

- Treat PIOC microcode as protocol-specific firmware, not as ordinary C logic.
- Keep baud-rate/timing register presets tied to the system clock; source UART presets assume 48 MHz.
- Model data registers as the host/PIOC mailbox. Do not hide their buffer size constraints.
- Keep double-buffered TX/RX ownership explicit when generating helpers.
- Clear the PIOC interrupt request by writing `R8_CTRL_RD` as shown in the source recipe.
- If PIOC does not run, check clock gate, reset release, microcode load, remap, and SWJ pin ownership before debugging protocol logic.

PIOC memory/register notes from sources:

- `PIOC` appears at `0x40006800` on the CH32X035 memory-layout note.
- `PIOC_SRAM_BASE` is used for microcode loading.
- `R8_DATA_REG0` through `R8_DATA_REG31` are general data registers; some recipes use 32-bit aliases for timing configuration.

## Common Pitfalls

| Pitfall | Consequence | Rule |
|---|---|---|
| Missing CC comparator threshold | PD messages cannot be decoded | Always pair CC pull configuration with comparator configuration. |
| Wrong `USBPD_PHY_V33` for VDD | No communication or possible PHY damage | Set for 3.3 V direct mode; clear for VDD greater than 4 V. |
| Reusing BMC timer constants after a clock change | Protocol errors | Derive timer values from the current system clock. |
| Communicating on the wrong CC line | Attach detected but PD traffic fails | Detect orientation and update `CC_SEL`. |
| Treating Type-C current advertisement as PD negotiation | Incorrect power policy | Separate Rp/Rd current levels from PDO/RDO negotiation. |
| Missing external Rd on CH32M030 sink designs | Sink attach/negotiation failure | Add required 5.1K CC pull-downs when the chip/package lacks internal Rd. |
| Not monitoring VBUS | Missed disconnect or unsafe power state | Include VBUS state in sink/source policy. |
| Forgetting AFIO/remap clock | Pin remap has no effect | Enable AFIO before `GPIO_PinRemapConfig`. |
| Leaving SWJ/SDI on PIOC pins | PIOC RX/TX pins do not work | Disable only the conflicting debug/remap function needed by the pinout. |
| Loading PIOC microcode after bad reset/clock order | PIOC not running | Follow reset, SRAM load, IO enable, and clock gate order from EVT examples. |

## HAL Metadata Checklist

Future HAL or template metadata should represent these fields explicitly:

- USBPD instance, register base, IRQ, bus clock domain, and clock enable function.
- CC1/CC2 pins, package availability, alternate/remap requirements, and board connector orientation mapping.
- Role policy: source, sink, DRP, data role, power role, VCONN support if implemented.
- CC pull type, Rp advertised current, internal/external Rd availability, and comparator threshold.
- PHY voltage mode and board VDD assumption.
- BMC timer source clock and generated timer constants.
- PD revision and supported feature level, including whether PPS/APDO or extended messages are implemented.
- PDO/RDO tables, safe voltage/current limits, and VBUS measurement channel.
- ISR/event flags and state-machine entry points.
- PIOC code image, SRAM base, IO pins, remap dependencies, data register usage, and protocol timing clock.

## Verification Status

- Extracted from `Doc/Ref/wch-dev-skill` Markdown sources listed above.
- Not verified against silicon, schematics, RM, DS, EVT source trees, or USB-IF compliance tools in this pass.
- Register names, bit names, bus domains, pin mappings, and threshold constants must be checked against the exact target chip header and board schematic before implementation.

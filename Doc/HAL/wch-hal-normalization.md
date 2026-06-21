# WCH HAL Normalization Notes

This document extracts common peripheral patterns from `Doc/Ref/wch-dev-skill` into repository-specific rules for future WCH HAL unification.

Scope:

- First pass covers GPIO, clock/RCC, UART/USART, ADC, DMA, and Flash storage.
- Families represented: CH57x/CH58x BLE-style APIs, CH32V/CH32F/CH32X/CH32H StdPeriph-style APIs, CH561/CH563 register-level APIs, and CH5xx 8051 APIs.
- This is not a final HAL API specification. It is a normalization guide and checklist.

Official EVT examples, RM, DS, headers, and current repository source remain the final authority.

## Source Files

- `Doc/Ref/wch-dev-skill/SKILL.md`
- `Doc/Ref/wch-dev-skill/AGENTS.md`
- `Doc/Ref/wch-dev-skill/chips/*/recipes/gpio_control.md`
- `Doc/Ref/wch-dev-skill/chips/*/recipes/uart_comm.md`
- `Doc/Ref/wch-dev-skill/chips/*/recipes/adc_reading.md`
- `Doc/Ref/wch-dev-skill/chips/*/recipes/flash_storage.md`
- `Doc/Ref/wch-dev-skill/chips/*/recipes/dma_transfer.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/pitfalls.md`

## API Style Families

| Style | Families | Characteristics | HAL adapter implication |
|---|---|---|---|
| BLE common API | CH57x, CH58x/CH59x | Functions like `GPIOA_ModeCfg`, `GPIOA_SetBits`, `UART1_DefInit`; PFIC interrupts | Needs compact direct helpers and highcode-aware ISR support. |
| StdPeriphDriver | CH32V, CH32F, CH32X, CH32H | `GPIO_InitTypeDef`, `USART_InitTypeDef`, `RCC_*ClockCmd`, `NVIC_Init` | Natural fit for structured HAL config objects. |
| Register-level ARM7 | CH561/CH563 | `R32_PA_DIR`, `R32_PA_OUT`, `R8_*` clock/IRQ registers | HAL adapter must map operations to bitfield writes, not StdPeriph calls. |
| 8051 SFR | CH5xx | `Pn_MOD_OC`, `Pn_DIR_PU`, bit-addressable pins, safe mode, `interrupt` syntax | HAL adapter must model memory qualifiers, SFR bits, and compiler constraints. |

## Universal Peripheral Initialization Order

Across WCH families, peripheral bring-up should follow this order:

1. Select system/peripheral clock source and update system clock state.
2. Enable peripheral bus clock or power gate.
3. Configure GPIO pin mode and alternate function.
4. Configure peripheral registers or init structure.
5. Configure DMA if used.
6. Configure interrupt controller if used.
7. Enable the peripheral.
8. Clear pending flags before entering the main loop.

Do not invert clock/GPIO/peripheral order unless an EVT example explicitly requires it.

## Clock And RCC Normalization

HAL metadata should distinguish these clock operations:

| Operation | StdPeriph-style example | BLE-style / register-style equivalent |
|---|---|---|
| System clock select/update | `SystemCoreClockUpdate()` | `SetSysClock(...)` |
| Peripheral clock enable | `RCC_APB2PeriphClockCmd(...)`, `RCC_AHBPeriphClockCmd(...)` | `PWR_PeriphClkCfg(...)`, register clock-gate bits, or family helper |
| ADC clock prescale | `RCC_ADCCLKConfig(...)` | Family-specific ADC clock config |
| Low-power gate | RCC/PWR gating | Sleep clock disable registers on register-level parts |

Rules:

- HAL descriptions must include bus domain and clock enable dependency for each peripheral instance.
- GPIO and AFIO/remap clocks are separate on many StdPeriph families.
- Register-level families may use inverted clock-gate bits, such as 0 meaning clock enabled.

## GPIO Normalization

Common logical GPIO model:

| HAL concept | StdPeriph mapping | CH57x/CH58x mapping | CH561/CH563 mapping | CH5xx mapping |
|---|---|---|---|---|
| Floating input | `GPIO_Mode_IN_FLOATING` | `GPIO_ModeIN_Floating` | direction input, no PU/PD | `Pn_MOD_OC=0`, `Pn_DIR_PU=0` |
| Pull-up input | `GPIO_Mode_IPU` | `GPIO_ModeIN_PU` | direction input + PU bit | quasi or input with pull mode depending chip |
| Pull-down input | `GPIO_Mode_IPD` | `GPIO_ModeIN_PD` | direction input + PD bit | chip-specific, often limited |
| Push-pull output | `GPIO_Mode_Out_PP` | `GPIO_ModeOut_PP_*mA` | DIR output, optional drive bit | `Pn_MOD_OC=0`, `Pn_DIR_PU=1` |
| Open-drain output | `GPIO_Mode_Out_OD` or `GPIO_Mode_AF_OD` | family-specific | DIR output + open-drain/PD bit | `Pn_MOD_OC=1`, `Pn_DIR_PU=0` |
| Alternate function | `GPIO_Mode_AF_PP` or `GPIO_Mode_AF_OD` plus remap | family pin function setup | register mux where available | `PIN_FUNC` bits |

GPIO HAL requirements:

- Represent port, pin, mode, pull, drive strength, output type, speed, and alternate function separately.
- Track whether a family uses explicit AF mode, remap bits, or dedicated pin-function registers.
- Require peripheral pin ownership checks before enabling a conflicting peripheral.
- For EXTI/GPIO interrupts, include trigger mode, polarity, pending flag clear behavior, and IRQ controller enable.

GPIO pitfalls:

- GPIO writes silently fail when the port clock is not enabled on StdPeriph families.
- EXTI/remap fails without AFIO clock on CH32V-style APIs.
- Floating input is unstable without external or internal pull configuration.
- 8051 quasi-bidirectional mode is not equivalent to push-pull output.

## UART/USART Normalization

Common UART model:

| Concept | Fields |
|---|---|
| Instance | UART/USART index and register base |
| Pins | TX, RX, optional RTS/CTS, remap option |
| Format | baud, data bits, stop bits, parity |
| Mode | TX, RX, half-duplex, synchronous, flow control |
| Transfer | polling, interrupt RX/TX, DMA TX/RX |
| Debug | default printf UART, usually 115200 8N1 |

StdPeriph initialization sequence:

1. Enable GPIO and USART clocks.
2. Configure TX pin as alternate-function push-pull.
3. Configure RX pin as floating input or pull-up input, depending family example.
4. Fill `USART_InitTypeDef`.
5. Call `USART_Init()`.
6. Enable interrupts or DMA if needed.
7. Call `USART_Cmd(..., ENABLE)`.

HAL adapter rules:

- Do not assume TX/RX pins are fixed; remap is family and instance specific.
- Model DMA request channel mapping separately from UART instance.
- Ring buffers are a template concern, not a low-level UART peripheral requirement.
- For BLE-style parts, helpers like `UART1_DefInit()` may configure defaults and hide pins; HAL should expose explicit pin mode when generating reusable code.

## ADC Normalization

Common ADC model:

| Concept | Fields |
|---|---|
| Instance | ADC index and channel count |
| Channel | external pin channel, internal temperature, Vrefint |
| GPIO mode | analog input or family equivalent |
| Clock | ADC clock divider and max frequency limit |
| Sequence | rank, sample time, scan count |
| Conversion mode | single, continuous, triggered, injected |
| Transfer | polling EOC, interrupt, DMA circular buffer |
| Calibration | reset/start/wait before first conversion where required |

ADC rules extracted from the source docs:

- Configure analog GPIO mode before sampling external channels.
- Enable ADC peripheral clock before ADC init.
- Configure ADC clock divider so the ADC clock stays within family limits.
- Run calibration before first conversion on CH32V-style ADCs.
- For multi-channel continuous scanning, DMA circular mode is the usual template.
- Internal temperature and reference channels require explicit enable calls where available.

Pitfalls:

- Skipping ADC calibration leads to inaccurate or invalid readings.
- Using digital input/output pin mode for analog channels corrupts readings.
- Forgetting DMA circular mode in continuous scan loses repeated samples.
- Temperature conversion formulas are family/reference-voltage dependent and must be verified against RM/DS.

## DMA Normalization

Common DMA model:

| Concept | Fields |
|---|---|
| Controller/channel | DMA instance and channel or request line |
| Direction | memory-to-memory, peripheral-to-memory, memory-to-peripheral |
| Addresses | peripheral register address, memory buffer address |
| Increment | peripheral increment, memory increment |
| Width | byte, halfword, word |
| Mode | normal or circular |
| Priority | low/medium/high/very-high |
| Trigger | peripheral request mapping |
| Interrupts | transfer complete, half transfer, error |

DMA rules:

- Enable DMA clock before channel configuration.
- Deinit or disable the channel before changing configuration.
- Peripheral data register addresses must match the family header, such as `USARTx->DATAR` or `ADCx->RDATAR` in CH32V-style examples.
- Request/channel mapping is not portable across families; keep it in metadata.
- Continuous ADC scan should use circular DMA unless the application intentionally uses a finite capture.
- For UART TX DMA, set data counter before enabling the DMA channel for each transfer.

## Flash Storage Normalization

Common Flash model:

| Concept | Fields |
|---|---|
| Region | code Flash, DataFlash, option bytes, SNV/NV area |
| Erase unit | bytes per page/sector/block |
| Program unit | byte, halfword, word, page, family-specific fast write |
| Unlock/lock | required sequence and API |
| Protection | write protection, option bytes, safe mode |
| Safety | read-modify-write, verify, avoid active code area |

Flash rules:

- Always unlock before erase/program when the family requires it.
- Clear status flags before starting a new operation on StdPeriph-style Flash APIs.
- Erase before writing; Flash programming cannot change bits from 0 back to 1.
- Use read-modify-write for partial page updates.
- Lock Flash after programming.
- Verify written data when storing configuration or boot metadata.
- Never use a data storage address without checking linker layout, IAP offset, and OTA/SNV/DataFlash regions.

Known erase units from extracted notes:

| Family | Erase unit |
|---|---|
| CH57x | 256 bytes |
| CH58x/CH59x | 256 bytes |
| CH32V003 | 64 bytes |
| CH32V006/CH32L103 | 1024 bytes |
| CH32V103 | 1024 bytes |
| CH32V20x/CH32V307/CH32V407 | 4096 bytes |
| CH32H417 | 4096 bytes |
| CH561/CH563 | 4096 bytes |
| CH5xx 8051 DataFlash | chip-specific byte-oriented DataFlash with protected-write rules |

## Register-Level And 8051 Adapter Rules

CH561/CH563:

- No `GPIO_InitTypeDef` or StdPeriph GPIO calls in source notes.
- GPIO control maps to direction, pull, drive, clear, output, and interrupt registers.
- IRQ setup uses dedicated enable/status/mode/polarity registers and `__irq` handlers.
- Clock gates may use sleep clock-off registers rather than RCC helpers.

CH5xx 8051:

- GPIO mode is controlled by `Pn_MOD_OC` and `Pn_DIR_PU` pairs.
- Some pin functions are selected through `PIN_FUNC` bits.
- Interrupts use compiler-specific vector declarations such as `interrupt INT_NO_GPIO using 1`.
- Flash/DataFlash writes require safe mode and interrupt masking.
- Large buffers should be placed in `xdata`, not `data`.

## Proposed HAL Metadata Primitives

Use these concepts in future metadata extraction:

- `family`: chip family route from `Doc/Family/family-routing.md`.
- `instance`: peripheral instance name and base address.
- `clock`: clock gate API/register, bus domain, reset dependency.
- `pin`: port, pin, mode, pull, drive, speed, AF/remap, conflict list.
- `irq`: IRQ name/vector, priority model, handler syntax, flag clear rule.
- `dma`: request source, controller, channel, direction, widths, circular support.
- `flash`: erase unit, program unit, safe regions, unlock/lock API.
- `template`: minimal init sequence and required include/header set.

## Cross-Family HAL Checklist

Before promoting any peripheral rule into a shared HAL abstraction, verify:

- Does the target family use StdPeriph, BLE helper APIs, register-level SFR, or 8051 SFR?
- Which clock must be enabled before peripheral access?
- Which GPIO mode and remap/alternate-function path is required?
- Which IRQ controller and handler attribute syntax is valid?
- Are status flags cleared by write-0, write-1, read side effect, or API call?
- Is DMA request/channel mapping fixed, remappable, or absent?
- What Flash erase/program unit applies to persistent storage examples?
- Is there an existing EVT example that matches the intended template?

## Verification Status

- Extracted from `wch-dev-skill` Markdown only.
- Exact API names and request mappings must be checked against repository EVT headers before code generation.
- Next verification pass should inspect representative EVT examples for GPIO, USART, ADC, DMA, and Flash in each imported family.

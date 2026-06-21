# WCH RTOS Integration Notes

This document extracts RTOS-related references from `Doc/Ref/wch-dev-skill` into repository-specific rules for future project templates, HAL integration, and example indexing.

Scope:

- FreeRTOS, RT-Thread, HarmonyOS LiteOS_m, TencentOS Tiny, and uC/OS-II references found in source example indexes.
- RTOS availability by family where the source notes explicitly list examples.
- Common integration constraints around startup, tick, interrupt priority, stack, heap, linker layout, and BLE/TMOS boundaries.
- This pass does not define a new RTOS abstraction API because the source notes mostly index examples rather than document scheduler APIs.

Official EVT examples, RTOS upstream ports, startup files, linker scripts, RM, DS, and current repository source remain the final authority.

## Source Files

- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/resources/example_list.md`
- `Doc/BLE/wch-ble-notes.md`
- `Doc/Core/wch-core-notes.md`

## RTOS Coverage From Source Notes

| Family or source group | RTOS examples explicitly listed | Example roots or naming | Notes |
|---|---|---|---|
| CH58x/CH59x | FreeRTOS, HarmonyOS, RT-Thread | `FreeRTOS/`, `HarmonyOS/`, `RT-Thread/` | BLE families also use TMOS for BLE stack event scheduling; do not treat TMOS as interchangeable with a general RTOS. |
| CH32V low-cost | FreeRTOS, HarmonyOS LiteOS_m, RT-Thread, TencentOS Tiny | `FreeRTOS/FreeRTOS`, `HarmonyOS/LiteOS_m`, `RT-Thread/rt-thread`, `TencentOS/TencentOS` | Source selection guide says RTOS integration on low-cost list is CH32L103-only for FreeRTOS/RT-Thread. Verify chip support before templating. |
| CH32V general | FreeRTOS, RT-Thread, HarmonyOS Lite, TencentOS Tiny on CH32V103/CH32V307/CH32V407; not listed for CH32V20x in the availability table | `EVT/EXAM/FreeRTOS/`, `EVT/EXAM/RT-Thread/`, `EVT/EXAM/HarmonyOS/`, `EVT/EXAM/TencentOS/` | Use family-specific EVT directories rather than one shared CH32V template. |
| CH32X035 / CH643-class sources | FreeRTOS, HarmonyOS, RT-Thread, TencentOS Tiny | `FreeRTOS/`, `HarmonyOS/`, `RT-Thread/`, `TencentOS/` | CH32X315/CH641 sections in the source index do not list the same RTOS set; check exact chip examples. |
| CH32F103 / CH32F20x / CH32M030 | FreeRTOS, RT-Thread, LiteOS_M or HarmonyOS, TencentOS Tiny | `FreeRTOS`, `RT_Thread` or `RT-Thread`, `LiteOS_M`, `TencentOS_Tiny` or `TencentOS` | Source notes mix Keil MDK and MounRiver project formats; keep toolchain metadata explicit. |
| CH32H417 | No RTOS directory listed in the source example list read in this pass | None in source note | Do not claim RTOS example coverage without EVT verification. |
| CH561/CH563 ARM7TDMI | uC/OS-II on CH563 only | `UCOS/` | CH561 table says no RTOS; CH563 table says uC/OS-II. |
| CH5xx 8051 | No RTOS examples listed in indexed source notes | N/A | Treat as bare-metal unless a specific chip EVT proves otherwise. |

## Concept Boundaries

Keep these scheduling models separate.

| Concept | Purpose | Boundary rule |
|---|---|---|
| Bare-metal superloop | Direct main loop with interrupts and polling | Do not add RTOS dependencies to simple peripheral examples by default. |
| BLE TMOS | BLE stack event loop, timers, and message dispatch on CH57x/CH58x/CH59x BLE sources | `Main_Circulation()` and `TMOS_SystemProcess()` are BLE stack scheduling requirements, not a general-purpose RTOS port. |
| FreeRTOS / RT-Thread / LiteOS_m / TencentOS Tiny | General RTOS kernels with tasks, tick, synchronization, and heap/stack settings | Use the vendor EVT port and startup/linker files for the exact chip. |
| uC/OS-II | CH563 RTOS port in the ARM7TDMI source index | Do not apply CH32 RISC-V FreeRTOS assumptions to CH563. |

Rules:

- A HAL template should expose scheduler type as metadata: bare-metal, BLE TMOS, FreeRTOS, RT-Thread, LiteOS_m, TencentOS Tiny, or uC/OS-II.
- BLE TMOS event handlers must return unhandled event bits; returning zero after handling one event drops other pending events.
- BLE projects must call `Main_Circulation()` or equivalent stack loop; sleeping with `__WFI()` alone does not process BLE events.
- General RTOS ports need their own tick, context switch, interrupt priority, heap, and task stack configuration. Do not infer these details from TMOS snippets.

## Project Selection Rules

Use the closest official example as the starting point.

Rules:

- Select the RTOS example from the same chip family and same toolchain whenever possible.
- Prefer exact chip EVT project paths over sibling-family examples when startup, vector, flash, or RAM layout differs.
- Preserve RTOS port files, startup files, linker scripts, and interrupt handlers from the EVT example until verified.
- Keep RTOS kernel configuration files under template control, not hidden in generated application code.
- If an RTOS is not listed for a chip in the source example list, mark support as unknown instead of unsupported unless the RM/EVT confirms absence.

Directory naming is not uniform across families:

- `RT-Thread`, `RT_Thread`, and `rt-thread` all appear in source paths or tables.
- `HarmonyOS`, `HarmonyOS/LiteOS_m`, and `LiteOS_M` appear in source paths or tables.
- `TencentOS`, `TencentOS/TencentOS`, and `TencentOS_Tiny` appear in source paths or tables.
- `FreeRTOS` may appear as a category or nested example directory.

## Startup, Tick, And Interrupt Rules

RTOS bring-up must align with the target startup and interrupt model.

Rules:

- Keep architecture-specific interrupt attributes from `Doc/Core/wch-core-notes.md`, such as WCH fast interrupt attributes where used by the family.
- Configure the RTOS tick source exactly as the EVT port expects. Do not reuse a bare-metal SysTick delay setup without checking the RTOS port.
- Avoid using the same timer for RTOS tick and an application timebase unless the EVT example does so.
- Configure interrupt priority grouping before enabling RTOS-managed interrupts on StdPeriph-style projects.
- Do not call blocking RTOS APIs from ISRs unless the RTOS port documents an ISR-safe variant.
- Keep context-switch and vector-table code from the port. Hand-edit only after checking the exact core and compiler ABI.

Interaction with low power:

- Bare-metal examples often use `__WFI()` or sleep modes directly; RTOS projects need idle hook or tickless-idle policy instead.
- BLE TMOS sleep and BLE stack timing must be verified before combining with a general RTOS scheduler.
- Watchdog refresh belongs in a deterministic task, idle hook, or system monitor, not scattered across unrelated tasks.

## Stack, Heap, And Linker Rules

The strongest RTOS pitfall found in source notes is stack pressure.

Rules:

- Do not place large buffers on task stacks. Use static/global buffers or explicit RTOS heap allocations when appropriate.
- Size each task stack from actual call depth, library use, ISR nesting, and protocol stack requirements.
- Keep RTOS heap, C heap, BLE heap, USB buffers, Ethernet buffers, DMA buffers, and application stacks as separate budget items.
- Verify linker RAM length and section placement before increasing RTOS heap or task count.
- Enable stack overflow checking when the RTOS port supports it.
- Add high-watermark or runtime stack diagnostics in debug builds when available.

Cross-document memory constraints:

- BLE notes define BLE heap and TMOS event-loop requirements.
- USB notes define endpoint buffer and controller-accessible memory requirements.
- Ethernet notes define DMA descriptors and packet buffers.
- IAP notes define bootloader/application offsets that can change available Flash for RTOS images.

## Synchronization And Driver Rules

RTOS integration changes driver ownership and blocking behavior.

Rules:

- Protect shared peripherals with mutexes or a single owner task.
- Use queues, message buffers, or event flags to hand data from ISRs to tasks.
- Keep ISR work minimal: clear flags, move small data, wake a task, and return.
- Replace busy-wait polling with RTOS waits only after verifying timeout and ISR wakeup behavior.
- Keep DMA buffers aligned and alive for the full transfer; do not allocate DMA buffers on short-lived task stacks.
- For UART/USB/Ethernet bridges, use ring buffers and backpressure instead of blocking inside callbacks.

## BLE TMOS Cross-Reference

BLE source notes include TMOS event semantics that are relevant when mixing BLE with any other scheduler.

Rules:

- `Main_Circulation()` must keep BLE stack event processing alive.
- Each BLE role or multi-connection item can need a distinct task ID in TMOS-based examples.
- TMOS `ProcessEvent` handlers should clear only handled bits by returning `events ^ HANDLED_EVT` or equivalent logic.
- GATT/GAP operations often take a TMOS task ID so stack messages return to the correct handler.
- If a general RTOS is added around a BLE stack example, define which context owns BLE stack calls and how TMOS processing is serviced.

See `Doc/BLE/wch-ble-notes.md` for BLE-specific role, heap, and event-loop details.

## Common Pitfalls

| Pitfall | Consequence | Rule |
|---|---|---|
| Treating TMOS as a general RTOS | Wrong scheduler assumptions in BLE projects | Keep TMOS and RTOS metadata separate. |
| Omitting `Main_Circulation()` in BLE examples | BLE events, timers, and messages stop processing | Preserve BLE stack event loop unless porting with a verified integration layer. |
| Returning zero from TMOS handler after one event | Other pending events are lost | Return only the unhandled event bits. |
| Reusing an RTOS example from a different chip | Wrong startup, vectors, tick, RAM, or linker layout | Start from the exact chip EVT example. |
| Large local buffers inside tasks | Stack overflow, hard faults, corrupted variables | Use static/global buffers or increase measured task stack sizes. |
| Sharing peripherals without ownership | Race conditions and corrupted transfers | Use one owner task or RTOS synchronization primitives. |
| Blocking inside ISR or callback | Missed deadlines or deadlock | Defer work to a task with ISR-safe wakeup primitives. |
| Ignoring IAP/app offsets | RTOS image overlaps bootloader or flag pages | Use `Doc/IAP/` layout rules for RTOS application linker files. |

## Template Metadata Checklist

Future RTOS project templates should represent these fields explicitly:

- Target chip, core, toolchain, startup file, linker/scatter file, and vector table mechanism.
- Scheduler type: bare-metal, BLE TMOS, FreeRTOS, RT-Thread, LiteOS_m, TencentOS Tiny, or uC/OS-II.
- RTOS example root and exact EVT source path.
- Tick source, tick rate, interrupt priority rules, and context-switch interrupt/vector requirements.
- RTOS heap size, per-task stack sizes, C heap/stack, BLE heap, USB/Ethernet/DMA buffer reservations.
- ISR-safe API rules and task wakeup mechanism.
- Low-power integration model: idle hook, tickless idle, BLE sleep, or bare-metal sleep.
- Debug diagnostics: stack overflow hook, malloc failure hook, runtime stats, asserts, UART logging.
- Cross-feature constraints: BLE TMOS loop, IAP offset, USB endpoint memory, Ethernet DMA, and watchdog refresh.

## Verification Status

- Extracted from `Doc/Ref/wch-dev-skill` Markdown source indexes and pitfalls listed above.
- No RTOS recipe source file was found in this pass; conclusions are intentionally limited to availability, project selection, and integration rules.
- Not verified against RTOS source trees, EVT project files, startup assembly, linker scripts, or kernel configuration headers in this pass.
- Exact API names, hook names, tick sources, priority rules, heap models, and supported chips must be checked against the selected official EVT RTOS project before implementation.

# WCH IAP, OTA, Bootloader, And Flash Layout Notes

This document extracts IAP, OTA, bootloader, application-offset, and update-flow guidance from `Doc/Ref/wch-dev-skill` into repository-specific rules for future bootloader, linker, HAL, and template work.

Scope:

- IAP bootloader layouts and application linker/scatter offsets across WCH RISC-V, ARM, ARM7TDMI, Ethernet, BLE, USB-PD, high-performance, and 8051 source notes.
- UART, USB, BLE, Ethernet, and application-triggered firmware update flows.
- Single-application, dual-image, backup-upgrade, and built-in ISP bootloader patterns.
- Flash erase/program, validity checks, vector relocation, reset/start-mode, and rollback pitfalls.

Official EVT examples, RM, DS, startup files, linker scripts, schematics, boot ROM behavior, and current repository source remain the final authority.

## Source Files

- `Doc/Ref/wch-dev-skill/chips/ch57x/recipes/iap_ota.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/recipes/iap_ota.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/recipes/ble_ota.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-lowcost/recipes/iap_ota.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/iap_ota.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/recipes/iap_ota.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/iap_ota.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/iap_ota.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/iap_ota.md`
- `Doc/Ref/wch-dev-skill/chips/ch561-ch563/recipes/iap_ota.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/recipes/iap_bootloader.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/example_list.md`

## Address Models

Do not assume one bootloader size or one application offset for all WCH families.

| Family or source group | Boot/application model from source notes | Linker/scatter implication | Main risk |
|---|---|---|---|
| CH57x BLE OTA | 4 KB bootloader at `0x00000000`; Image A, Image B, IAP image, and DataFlash flag regions vary by chip/layout | Separate linker scripts for Image A, Image B, and IAP/jump images | Wrong image flag or offset breaks OTA commit/rollback. |
| CH58x/CH59x | 4 KB bootloader at `0x00000000`; APP starts at `0x00001000` in simple IAP/OTA notes | `FLASH ORIGIN = 0x00001000`; reduce FLASH length by 4 KB and chip-specific flash size | Default linker script overwrites bootloader. |
| CH32V low-cost / CH32X / CH56x / CH561-CH563 | Common simple IAP model uses 4 KB bootloader and APP at `0x00001000` | Modify `Link.ld` or Keil scatter file to start at `0x1000` | Jump succeeds only if vector table, stack, and reset handler match offset. |
| CH32V general | Example dual-image layout uses 16 KB bootloader at `0x00000000`, APP A at `0x00004000`, APP B at `0x00020000`, config flag near top of flash | Application linker origin can be `0x00004000` in the example; image sizes are product-specific | Copying a 4 KB bootloader template would under-allocate bootloader space. |
| CH32F10x / CH32F20x | Flash base is `0x08000000`; CH32F10x example uses APP at `0x08001000`; CH32F20x example uses APP at `0x08002000` | Use absolute `0x0800xxxx` origins, not zero-based RISC-V addresses | Mixing zero-based and `0x08000000` address spaces corrupts flash writes/jumps. |
| CH32H417 | Bootloader occupies `0x00000000` to `0x0000FFFF`; APP starts at `0x00010000` | Application linker origin is `0x10000`; erase/program region starts there | Large bootloader area and option bytes/start mode differ from small CH32V layouts. |
| CH5xx 8051 | Built-in ISP bootloader is at upper Flash; user app stays at lower `0x0000` region with code-size limit | Keil C51 code size/ROM range must stop before bootloader address | Unlike CH32 IAP, user app usually does not move upward; it must avoid upper ISP region. |

Rules:

- Store bootloader start, bootloader size, application start, application max size, flag region, backup region, and data region as target metadata.
- Derive linker `ORIGIN` and `LENGTH` from the selected layout; do not hard-code `0x1000` outside the families/layouts that use it.
- Keep flash write/erase limits inside the application or inactive-image area. The bootloader must not erase itself except in an explicit bootloader-update design.
- Keep flag/config pages outside all images and outside user data regions.

## Bootloader Flow

Generic IAP boot flow:

1. Reset enters the bootloader or built-in ISP entry path.
2. Bootloader initializes only the minimum clock, debug, transport, and flash services required.
3. Bootloader checks an update request from Flash flag, DataFlash flag, backup register, GPIO pin, command byte, or start-mode state.
4. If no update is requested, bootloader validates the application image.
5. If the image is valid, bootloader disables interrupts, relocates vectors when required, sets the stack pointer when required, and jumps to the application reset handler.
6. If update is requested or the application is invalid, bootloader receives firmware through UART, USB, BLE, Ethernet, or another transport.
7. Bootloader validates size, address range, checksum/CRC, and optional signature before programming.
8. Bootloader erases the destination region, programs data using the target flash granularity, verifies written data, updates flags, and resets or jumps.
9. On failure, bootloader stays in recovery mode or rolls back to the previous valid image.

Do not put normal application services into the bootloader unless they are required for update transport. Bootloader size is a first-order constraint on small chips.

## Application Jump Rules

Jumping to an application is architecture-specific.

Common rules:

- Validate the initial stack pointer before jumping. It must fall inside the target RAM range for that chip.
- Validate the reset vector before jumping. It should point into executable flash or the expected image region.
- Disable interrupts before the jump.
- Stop or deinitialize peripherals that can interrupt or DMA after the jump.
- Clear pending interrupt flags when the target startup code does not guarantee a clean state.
- Set the vector table base or family-specific vector table register when the architecture requires it.
- Set the main stack pointer before calling the application reset handler on Cortex-M/RISC-V examples that use this pattern.

Family examples from source notes:

- CH32V low-cost uses `NVIC_SetVectorTable(NVIC_VectTab_FLASH, APP_START_ADDR)` before jumping.
- CH56x source notes use `PFIC_SetVTFAddr(...)` in the jump path.
- CH561/CH563 source notes write the vector table register at `0xE000ED08` and use ARM7/MDK scatter placement.
- CH57x `BackupUpgrade_JumpIAP` can use a startup assembly `j` instruction for fixed-address jump variants.
- CH5xx 8051 jump-to-ISP disables global interrupts with `EA = 0` and calls the chip-specific bootloader address.

When generating reusable helpers, keep the vector-relocation primitive separate from the validity check and jump primitive.

## Linker And Scatter Rules

Application linker files must match the image slot.

Rules:

- Maintain separate linker/scatter files for bootloader, application slot A, application slot B, IAP commit image, and fixed jump image when those images exist.
- Application `FLASH ORIGIN` must equal the image start address.
- Application `FLASH LENGTH` must stop before the next slot, flag page, DataFlash region, persistent storage, or bootloader region.
- RAM origin/length may also differ in BLE OTA examples; do not copy only the Flash origin.
- Startup vector placement must remain first in the image so the first two words are stack pointer and reset vector where the jump code expects them.
- For Keil C51 CH5xx projects, constrain code ROM size/range so generated code cannot overlap the upper built-in ISP bootloader.

Common offsets from source notes:

| Layout | Application start example | Notes |
|---|---:|---|
| 4 KB bootloader | `0x00001000` | CH58x/CH59x simple IAP, CH32V low-cost, CH32X/CH64x, CH56x, CH561/CH563 source notes. |
| 16 KB bootloader | `0x00004000` | CH32V general dual-image example. |
| CH32F10x | `0x08001000` | Absolute Flash base address. |
| CH32F20x | `0x08002000` | Larger bootloader example. |
| CH32H417 | `0x00010000` | 64 KB bootloader region. |
| CH5xx 8051 | App at lower `0x0000`, upper bootloader reserved | Enforce max code size below `0x3800`, `0x7000`, or `0xF000` depending chip. |

## Flash Programming Rules

Flash update code must be built around the target's erase and program granularity.

Rules:

- Unlock Flash only around the erase/program window; lock it again immediately after.
- Erase before programming. Partial updates require read-modify-erase-write when the erase block contains other data.
- Align erase start and size to the target erase granularity.
- Align writes to the target program granularity: byte, half-word, word, page buffer, or ROM helper requirement.
- Reject firmware images larger than the selected slot.
- Reject packets whose destination address falls outside the selected inactive/application slot.
- Verify checksum/CRC before committing flags; verify written flash when possible.
- Do not run code from a Flash page while erasing or programming that page.

Granularity examples from source notes:

- CH32V003 source note mentions 64-byte pages and half-word programming in the low-cost example.
- CH32X source note uses a 256-byte page buffer and word programming for the example.
- CH58x/CH59x pitfall notes mention 256-byte erase granularity in the flash-update context.
- CH56x source note uses `FLASH_ROM_ERASE` and `FLASH_ROM_WRITE` over 256-byte or larger regions depending call site.
- CH561/CH563 source note uses 4 KB erase blocks in the ARM7TDMI example.
- CH5xx source note says CH554 flash is organized in 512-byte blocks and must be erased before byte writes.

## Update Transport Rules

The transport should be replaceable; the flash writer and image policy should not depend on UART/BLE/Ethernet details.

Transport-agnostic packet rules:

- Include command, target address or implicit offset, length, payload, and checksum/CRC in the protocol.
- ACK only after the packet is validated and safely written or staged.
- NAK malformed length, checksum mismatch, out-of-range address, flash failure, and sequence mismatch.
- Include commands for device info, erase, program, verify, end/commit, and optional reset.
- Use monotonic offset or packet number checks so duplicate/out-of-order packets cannot corrupt an image silently.

Transport-specific notes:

- UART IAP examples use simple ACK/NAK flows and must handle timeouts or recovery.
- USB IAP appears in CH58x/CH59x examples as an alternate transport; keep USB endpoint logic separate from flash policy.
- BLE OTA requires enough BLE heap, an OTA service/profile, MTU-aware chunking, and reconnect/error handling.
- Ethernet OTA should download into a staging buffer or inactive image and verify CRC before programming/committing.

## OTA And Dual-Image Rules

Dual-image OTA needs explicit image state management.

Image states from source notes include:

- Current Image A.
- Current or backup Image B.
- IAP/commit image.
- DataFlash or flag page state such as `IMAGE_A_FLAG`, `IMAGE_B_FLAG`, or `IMAGE_IAP_FLAG`.

Rules:

- Program new firmware to an inactive slot or download buffer when rollback is required.
- Do not overwrite the running image until the new image has passed integrity checks.
- Commit by writing a small flag only after programming and verification succeed.
- On next boot, copy/promote the backup image to the running slot only when the commit flag requests it.
- Clear or update flags after a successful promotion; leave enough information to recover from reset during promotion.
- Erase backup only after the active image is known good.
- For simple `OnlyUpdateApp` flows, document that rollback is weaker than `BackupUpgrade`.

BLE OTA rules:

- Keep BLE OTA service/protocol separate from bootloader policy.
- Ensure BLE heap and packet sizes are sufficient for OTA throughput.
- Align OTA data writes to the Flash requirements; source notes mention 256-byte alignment in CH57x pitfalls.
- Preserve connection state and notify progress/errors when the OTA profile supports it.

## Trigger And Boot-Mode Rules

Update entry should be deterministic and recoverable.

Trigger sources:

- GPIO pin sampled at boot.
- UART/USB/BLE/Ethernet command in the application.
- Flash flag or DataFlash flag.
- Backup register or persistent config word.
- Built-in start-mode mechanism such as `SystemReset_StartMode(Start_Mode_BOOT)` or `Start_Mode_USER` where supported.
- CH5xx built-in ISP entry through BOOT pin or a jump to the chip-specific bootloader address.

Rules:

- Debounce or sanity-check hardware trigger pins.
- Write flags with erase/program semantics; never assume a Flash word can change from `0` back to `1` without erase.
- Clear update flags only after successful programming or after intentionally aborting an update.
- Prefer reset into bootloader for complex updates rather than jumping from a fully running application with active interrupts/peripherals.
- When using `SystemReset_StartMode`, unlock/lock Flash or option-byte access exactly as the target example requires.

## Safety And Recovery Rules

Minimum safety checks before accepting firmware:

- Firmware size fits the target slot.
- Destination address range is within the selected slot.
- Initial stack pointer is inside valid RAM for the target.
- Reset vector is inside valid executable image range.
- CRC/checksum matches metadata.
- Optional version, hardware ID, and image type match the target.
- Power supply is stable enough for erase/program duration.

Recovery rules:

- Keep a known-good image when the product requires field-update reliability.
- Keep the bootloader write-protected if the target supports it.
- Add watchdog handling carefully: it can recover from a stuck update, but it can also reset during erase/program if not serviced or disabled intentionally.
- Keep a serial/USB/BOOT-pin recovery path available when possible.
- Never mark a new image active before verifying it.

## CH5xx 8051 ISP Rules

CH5xx IAP differs from CH32-style application offsets.

Rules:

- Built-in ISP bootloader lives in the upper Flash region; application normally starts at `0x0000`.
- Use the chip-specific bootloader address such as `0x3800` for CH554, `0x7000` for CH549, or `0xF000` for CH559 source examples.
- Disable interrupts with `EA = 0` before jumping to the bootloader.
- Keep application code size below the bootloader start address in Keil C51 project settings.
- Do not use full-chip erase when intending to preserve the bootloader or user application.
- Erase before writing Flash data; CH5xx examples use ROM control operations and block erase semantics.

## Common Pitfalls

| Pitfall | Consequence | Rule |
|---|---|---|
| Using default application linker script with IAP | Application overwrites bootloader | Set `FLASH ORIGIN` to the application slot and reduce length. |
| Assuming every WCH IAP app starts at `0x1000` | Wrong jump/write addresses on CH32V general, CH32F, CH32H, CH5xx | Use target-specific layout metadata. |
| Not relocating vector table | Interrupts or exceptions jump to bootloader vectors | Set vector base or startup jump according to architecture. |
| Jumping without disabling interrupts | Crash after application entry | Disable interrupts and clear/deinit active peripherals before jump. |
| Not validating stack pointer/reset vector | Jump to erased or corrupted flash | Check RAM range and executable reset vector range. |
| Writing Flash without erase | Program failure or corrupted image | Erase aligned pages/blocks before programming. |
| Writing outside image slot | Bootloader, flags, or user data corruption | Bounds-check every packet address and total size. |
| Clearing update flag too early | Failed update appears committed | Clear or switch flags only after verify succeeds. |
| No rollback image | Device can brick after interrupted update | Use backup/dual-image for field OTA when reliability matters. |
| CH5xx code overlaps ISP bootloader | ISP entry fails or app behaves unpredictably | Enforce C51 code ROM limit below bootloader address. |

## HAL And Template Metadata Checklist

Future bootloader/HAL/template metadata should represent these fields explicitly:

- Chip/family, architecture, Flash base, Flash size, RAM ranges, and vector-table mechanism.
- Bootloader start, bootloader size, application slot starts, slot sizes, backup/download region, and flag/data region.
- Linker/scatter template per image role.
- Flash erase size, program size, unlock/lock APIs, ROM helper APIs, and write-protection capabilities.
- Transport type, packet size, checksum/CRC algorithm, timeout policy, and ACK/NAK protocol.
- Image metadata: version, hardware ID, image role, length, CRC, signature if used.
- Trigger method: GPIO, command, Flash flag, DataFlash flag, backup register, start-mode, or built-in ISP.
- Jump method: stack pointer setup, reset vector address, vector table relocation, interrupt disable, and reset fallback.
- OTA policy: single app, backup upgrade, dual active images, IAP commit image, rollback rules.
- Verification and recovery policy: CRC, read-back verify, watchdog, write-protect, safe-mode/recovery entry.

## Verification Status

- Extracted from `Doc/Ref/wch-dev-skill` Markdown sources listed above.
- Not verified against silicon, EVT source trees, linker scripts, startup files, RM, DS, option-byte behavior, or boot ROM behavior in this pass.
- All addresses, region sizes, vector relocation functions, Flash granularity, and start-mode APIs must be checked against the exact target chip, package, memory density, and board design before implementation.

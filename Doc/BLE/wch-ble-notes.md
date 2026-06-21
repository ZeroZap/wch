# WCH BLE Notes

This document extracts BLE-specific guidance from `Doc/Ref/wch-dev-skill` into repository-specific notes for future HAL/template work.

Scope:

- CH57x family: CH572, CH573, CH579 style BLE projects.
- CH58x/CH59x family: CH583, CH585, CH592, CH595 style BLE projects.
- Focus: initialization order, `config.h`, GAP/GATT service registration, role templates, pitfalls, and example routing.

Official EVT examples and headers remain the final source of truth. Treat this file as a routing and design note for HAL unification.

## Source Files

- `Doc/Ref/wch-dev-skill/chips/ch57x/resources/config_reference.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/resources/ble_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/recipes/ble_peripheral.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/recipes/ble_hid.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/recipes/iap_ota.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/config_reference.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/recipes/ble_peripheral.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/recipes/ble_central.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/recipes/ble_mesh.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/recipes/ble_usb_combo.md`

## Family Mapping

| Family | Chips | Typical BLE library/header | Repository sources |
|---|---|---|---|
| CH57x | CH572, CH573, CH579 | `CH57xBLE_LIB.h`, `CH57x_common.h` | `CH572EVT/`, `CH573EVT/`, official RM/DS |
| CH58x | CH583, CH585, CH592 | `CH58xBLE_LIB.h`, `CH58x_common.h` | `CH583EVT/`, `CH585EVT/`, `CH592EVT/` |
| CH59x | CH595 | `CH59xBLE_LIB.h`, `CH59x_common.h` | `CH595EVT/` |

## Mandatory Initialization Order

The BLE stack has strict initialization ordering. Do not reorder this sequence without checking the family EVT example.

Common peripheral role sequence:

```c
SetSysClock(CLK_SOURCE_PLL_60MHz);

#if (defined(DCDC_ENABLE)) && (DCDC_ENABLE == TRUE)
PWR_DCDCCfg(ENABLE);
#endif

/* Optional debug UART init here. */

CH57X_BLEInit();      /* CH57x */
/* or */
CH58X_BLEInit();      /* CH58x/CH59x */

HAL_Init();
GAPRole_PeripheralInit();
Peripheral_Init();
Main_Circulation();   /* never returns */
```

Rules:

- BLE stack init must happen before `HAL_Init()`.
- `HAL_Init()` must happen before GAP role/application init.
- `Main_Circulation()` runs the TMOS event loop and must not be replaced by a plain `while(1) { __WFI(); }` loop.
- Central role uses `GAPRole_CentralInit()` and `Central_Init()` in place of the peripheral role calls.

## `config.h` Parameters

Important BLE configuration macros extracted from CH57x and CH58x/CH59x references:

| Parameter | Meaning | Notes |
|---|---|---|
| `DCDC_ENABLE` | Enables DC/DC converter | Usually lower power; verify board support. |
| `HAL_SLEEP` | Enables BLE sleep behavior | Disable during early debug unless low-power behavior is being validated. |
| `BLE_MEMHEAP_SIZE` | BLE stack heap | Minimum often 4KB for simple peripheral; 6KB recommended baseline; 8KB+ for multi-role; 10KB+ for Mesh. |
| `BLE_BUFF_MAX_LEN` | BLE buffer max length | Effective `ATT_MTU = BLE_BUFF_MAX_LEN - 4`. Use `27` for MTU 23, `251` for MTU 247. |
| `BLE_BUFF_NUM` | Number of BLE packet buffers | More buffers are needed for notifications, central, or multi-connection use. |
| `BLE_TX_NUM_EVENT` | TX packets per connection event | Raising improves throughput but increases buffer pressure. |
| `BLE_TX_POWER` | TX power enum | Enum spelling differs between families/chips. Verify against the active BLE library header. |
| `PERIPHERAL_MAX_CONNECTION` | Peripheral role connection count | Packed into `ConnectNumber` with central count. |
| `CENTRAL_MAX_CONNECTION` | Central role connection count | Use `0` for peripheral-only projects. |
| `BLE_SNV_ADDR` | BLE Simple NV storage address | Must not overlap application image, OTA area, or DataFlash layout. |

Connection count packing:

```c
cfg.ConnectNumber = (PERIPHERAL_MAX_CONNECTION & 3) | (CENTRAL_MAX_CONNECTION << 2);
```

CH58x/CH59x details:

- CH585 uses `CH58xBLE_LIB.h`; CH595 uses `CH59xBLE_LIB.h`.
- CH595 has smaller Flash in the source notes; review SNV address carefully before enabling bonding or OTA.
- TX power enum names may differ, for example `LL_TX_POWEER_0_DBM` vs `LL_TX_PWR_0_DBM`; verify with headers.

## GAP/GATT Registration Order

Standard services must be registered before custom services.

Recommended service order:

```c
GGS_AddService(GATT_ALL_SERVICES);          /* GAP service */
GATTServApp_AddService(GATT_ALL_SERVICES);  /* GATT service */
DevInfo_AddService();                       /* Device Information */
MyCustomService_AddService(GATT_ALL_SERVICES);
```

Why this matters:

- GATT attributes are handle-based.
- Standard services occupy expected handle ranges.
- Custom service handles should be allocated after GAP/GATT/Device Information services.

## Advertising Rules

- Legacy advertising data is limited to 31 bytes.
- Scan response data is also constrained; do not pack all metadata into advertising data.
- Use `GAPRole_SetParameter(GAPROLE_ADVERT_DATA, len, data)` only after verifying `len <= 31`.
- Store the connection handle from the connection event before sending notifications.

Example connection handle lifecycle:

```c
static uint16_t connHandle = INVALID_CONNHANDLE;

case GAPROLE_CONNECTED:
    connHandle = pEvent->linkCmpl.connectionHandle;
    break;

case GAPROLE_DISCONNECTED:
    connHandle = INVALID_CONNHANDLE;
    break;
```

## Notifications

Notifications require:

- A valid connection handle.
- A valid characteristic handle.
- Client Characteristic Configuration Descriptor enabled by the peer.
- Payload length aligned with negotiated MTU.

For throughput-oriented designs:

- Use `BLE_BUFF_MAX_LEN = 251` for MTU 247 only when the peer and application can handle DLE.
- Increase heap and buffer counts before increasing TX events per connection event.
- Consider 2M PHY where supported and validated by EVT examples.

## TMOS Event Handling

BLE examples rely on TMOS event processing.

Rules:

- Register tasks with `TMOS_ProcessEventRegister(...)`.
- `Main_Circulation()` must run continuously.
- Event handlers must return unhandled event bits, not blindly return `0` after handling one event.

Pattern:

```c
if (events & MY_EVT) {
    handle_my_evt();
    return (events ^ MY_EVT);
}
return 0;
```

## Interrupt Placement

For BLE families, latency-sensitive interrupt handlers should use WCH fast interrupt attributes and the RAM/highcode section when required by the family EVT startup/linker setup.

```c
__attribute__((interrupt("WCH-Interrupt-fast")))
__attribute__((section(".highcode")))
void UART1_IRQHandler(void) {
    /* ISR body */
}
```

Verify that `.highcode` is present in the active linker script and startup copy logic before relying on it.

## Role Templates

### Peripheral

Primary sources:

- `ch57x/recipes/ble_peripheral.md`
- `ch58x-ch59x/recipes/ble_peripheral.md`

Peripheral flow:

1. Configure `config.h` heap, buffers, TX power, connection counts.
2. Initialize BLE stack, HAL, GAP peripheral role, and application.
3. Register GAP, GATT, Device Info, then custom services.
4. Configure advertising data and scan response data.
5. Start advertising.
6. Track connection state and connection handle.
7. Process read/write callbacks and send notifications only when connected.

Example roots from source notes:

- CH57x: `resources/EXAM/BLE/Peripheral/`
- CH58x/CH59x: `resources/EXAM/BLE/Peripheral/`, `resources/EXAM/BLE/HeartRate/`, `resources/EXAM/BLE/BLE_UART/`

### Central

Primary source:

- `ch58x-ch59x/recipes/ble_central.md`

Central flow:

1. Configure heap and central connection count.
2. Initialize BLE stack and HAL.
3. Call `GAPRole_CentralInit()` and `Central_Init()`.
4. Start scanning.
5. Filter discovered devices and establish link.
6. Run service discovery.
7. Read/write target characteristics.

Example roots:

- `resources/EXAM/BLE/Central/`
- `resources/EXAM/BLE/MultiCentral/`

### HID

Primary source:

- `ch57x/recipes/ble_hid.md`

HID-specific notes:

- HID devices typically enable low-power behavior after debugging.
- Register standard services before HID service.
- Common services include Device Information, Battery, HID, and optionally Scan Parameters.
- HID report descriptors and report IDs are part of the application contract; keep them explicit in generated templates.

Example roots:

- `resources/EXAM/BLE/HID_Keyboard/`
- `resources/EXAM/BLE/HID_Mouse/`
- `resources/EXAM/BLE/HID_Consumer/`
- `resources/EXAM/BLE/HID_Touch/`

### Mesh

Primary source:

- `ch58x-ch59x/recipes/ble_mesh.md`

Mesh-specific notes:

- Mesh requires more heap than a simple peripheral; source notes use 10KB+ as a baseline.
- Mesh init happens after BLE stack and HAL init.
- Provisioning data, OOB data, device UUID, keys, and TTL need dedicated template fields.

Example roots:

- `resources/EXAM/BLE/MESH/adv_ali_light/`
- `resources/EXAM/BLE/MESH/adv_vendor/`
- `resources/EXAM/BLE/MESH/provisioner_vendor/`

### BLE + USB Combo

Primary source:

- `ch58x-ch59x/recipes/ble_usb_combo.md`

Combo-specific notes:

- Budget larger BLE heap because BLE and USB processing share RAM and event latency constraints.
- Initialize BLE/HAL/GAP/application before USB app init according to the source recipe.
- Use ring buffers for USB-to-BLE and BLE-to-USB paths.
- Keep USB instance selection explicit on parts with multiple USB controllers.

Example concepts:

- BLE data writes into a BLE ring buffer, then USB sends it.
- USB OUT data writes into a USB ring buffer, then BLE notification sends it.
- Optional 2M PHY update can improve throughput if supported and validated.

## OTA And IAP Cross-Reference

BLE OTA references belong primarily in `Doc/IAP/` because they are constrained by Flash layout and linker offsets.

BLE-relevant reminders:

- OTA writes must respect Flash erase granularity.
- Active image, backup image, bootloader, IAP image, and SNV/DataFlash must not overlap.
- BLE OTA command handlers usually expose info, erase, program, verify, and end/switch-image commands.

Source:

- `Doc/Ref/wch-dev-skill/chips/ch57x/recipes/iap_ota.md`

## Common BLE Pitfalls

| Pitfall | Impact | Rule |
|---|---|---|
| BLE init order changed | Hard fault or undefined controller behavior | Keep stack init before HAL and GAP role init. |
| `Main_Circulation()` omitted | No BLE events processed | Always enter TMOS event loop. |
| Heap too small | Silent memory corruption or hard fault | Use 6KB baseline; increase for Mesh, multi-role, combo, or high throughput. |
| Wrong MTU assumption | Payload truncation or throughput mismatch | `ATT_MTU = BLE_BUFF_MAX_LEN - 4`. |
| Custom GATT service registered first | Handle/service issues | Register GAP, GATT, Device Info first. |
| ISR not in highcode where required | Timing jitter or Flash wait-state sensitivity | Use fast interrupt and `.highcode` after linker/startup verification. |
| GPIO not configured before peripheral | UART/USB/debug signals fail | Configure pins before peripheral init. |
| Notification uses invalid handle | Notification fails | Store handle from connection event and reset on disconnect. |
| TMOS event handler returns `0` too early | Pending events dropped | Return unhandled event bits. |
| Advertising data exceeds 31 bytes | Truncation or advertising failure | Keep legacy advertising payload <= 31 bytes. |
| UUID byte order wrong | Service discovery mismatch | Store BLE UUIDs little-endian where required by stack examples. |

## Verification Status

- Extracted from `wch-dev-skill` Markdown only.
- API names and exact example paths must still be checked against repository EVT source before generating code or templates.
- Next verification pass should inspect `CH572EVT/`, `CH573EVT/`, `CH583EVT/`, `CH585EVT/`, `CH592EVT/`, and `CH595EVT/` BLE example folders.

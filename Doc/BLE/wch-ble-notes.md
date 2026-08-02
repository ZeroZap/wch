# WCH BLE 笔记

本文从 `Doc/Ref/wch-dev-skill` 提取 BLE 专用指导，整理为适用于本仓库的笔记，供后续 HAL/模板工作使用。

范围：

- CH57x 系列：CH572、CH573、CH579 风格的 BLE 项目。
- CH58x/CH59x 系列：CH583、CH585、CH592、CH595 风格的 BLE 项目。
- 重点：初始化顺序、`config.h`、GAP/GATT 服务注册、角色模板、陷阱及示例索引。

最终应以官方 EVT 示例及头文件为准。本文仅作为 HAL 统一工作的索引及设计笔记。

## 来源文件

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

## 系列映射

| 系列 | 芯片 | 典型 BLE 库/头文件 | 仓库来源 |
|---|---|---|---|
| CH57x | CH572、CH573、CH579 | `CH57xBLE_LIB.h`、`CH57x_common.h` | `CH572EVT/`、`CH573EVT/`、官方 RM/DS |
| CH58x | CH583、CH585 | `CH58xBLE_LIB.h`、`CH58x_common.h` | `CH583EVT/`、`CH585EVT/` |
| CH59x | CH592、CH595 | `CH59xBLE_LIB.h`、`CH59x_common.h` | `CH592EVT/`、`CH595EVT/` |

## 强制初始化顺序

BLE 栈具有严格的初始化顺序。未检查对应系列 EVT 示例前，不得调整此顺序。

常见外围设备角色顺序：

```c
/* 时钟配置必须采用目标芯片 EVT 的值，不能统一写成 60 MHz。 */
SetSysClock(EVT_CLOCK_CONFIG);

#if (defined(DCDC_ENABLE)) && (DCDC_ENABLE == TRUE)
PWR_DCDCCfg(ENABLE);
#endif

/* Optional debug UART init here. */

CH57x_BLEInit();      /* CH57x */
/* or */
CH58x_BLEInit();      /* CH583/CH585 */
/* or */
CH59x_BLEInit();      /* CH592/CH595 */

HAL_Init();
GAPRole_PeripheralInit();
Peripheral_Init();
Main_Circulation();   /* never returns */
```

规则：

- BLE 栈初始化必须在 `HAL_Init()` 之前完成。
- `HAL_Init()` 必须在 GAP 角色/应用初始化之前完成。
- `Main_Circulation()` 运行 TMOS 事件循环，不得替换为普通的 `while(1) { __WFI(); }` 循环。
- 中心设备角色使用 `GAPRole_CentralInit()` 和 `Central_Init()` 替代外围设备角色调用。

时钟必须按目标 EVT 区分：

| 目标 | 当前 BLE EVT 配置 | 规则 |
|---|---|---|
| CH572 | `CLK_SOURCE_HSE_PLL_100MHz` | 使用 100 MHz 配置，不得套用 60 MHz。 |
| CH592 | `CLK_SOURCE_PLL_60MHz` | 使用当前 CH592 BLE EVT 的 60 MHz 配置。 |
| CH585、CH595 | `SYSCLK_FREQ` | 在公共头文件或工程配置中先确定 `SYSCLK_FREQ`，再调用 `SetSysClock(SYSCLK_FREQ)`；不得把宏隐含替换为统一的 60 MHz。 |

其他型号也应直接核对其当前 EVT；上述表不是跨系列的统一时钟策略。

## `config.h` 参数

从 CH57x 及 CH58x/CH59x 参考资料提取的重要 BLE 配置宏：

| 参数 | 含义 | 说明 |
|---|---|---|
| `DCDC_ENABLE` | 使能 DC/DC 转换器 | 通常可降低功耗；应验证开发板是否支持。 |
| `HAL_SLEEP` | 使能 BLE 睡眠行为 | 初期调试时禁用，除非正在验证低功耗行为。 |
| `BLE_MEMHEAP_SIZE` | BLE 栈堆 | 6KB 只能作为保守起始建议，不能写成库的统一最小值。CH572 EVT 默认配置为 3584 字节，当前库初始化检查要求至少 3KB；最终大小应以目标 EVT、角色、连接数和功能实测为准。 |
| `BLE_BUFF_MAX_LEN` | BLE 缓冲区最大长度 | 实际 `ATT_MTU = BLE_BUFF_MAX_LEN - 4`。MTU 23 使用 `27`，MTU 247 使用 `251`。 |
| `BLE_BUFF_NUM` | BLE 数据包缓冲区数量 | 通知、中心设备或多连接用途需要更多缓冲区。 |
| `BLE_TX_NUM_EVENT` | 每个连接事件的 TX 数据包数 | 提高该值可提升吞吐量，但会增加缓冲区压力。 |
| `BLE_TX_POWER` | TX 功率枚举 | 枚举拼写因系列/芯片而异。应对照当前 BLE 库头文件验证。 |
| `PERIPHERAL_MAX_CONNECTION` | 外围设备角色连接数 | 与中心设备连接数一起打包到 `ConnectNumber`。 |
| `CENTRAL_MAX_CONNECTION` | 中心设备角色连接数 | 仅外围设备项目使用 `0`。 |
| `BLE_SNV_ADDR` | BLE Simple NV 存储地址 | 不得与应用镜像、OTA 区域或 DataFlash 布局重叠。 |

连接数打包方式：

```c
cfg.ConnectNumber = (PERIPHERAL_MAX_CONNECTION & 3) | (CENTRAL_MAX_CONNECTION << 2);
```

CH58x/CH59x 细节：

- CH585 使用 `CH58xBLE_LIB.h`；CH595 使用 `CH59xBLE_LIB.h`。
- CH583/CH585 调用 `CH58x_BLEInit()`；CH592/CH595 调用 `CH59x_BLEInit()`。
- 来源笔记中 CH595 的闪存较小；启用绑定或 OTA 前应仔细检查 SNV 地址。
- TX 功率枚举名称可能不同，例如 `LL_TX_POWEER_0_DBM` 与 `LL_TX_PWR_0_DBM`；应通过头文件验证。

## GAP/GATT 注册顺序

标准服务必须在自定义服务之前注册。

建议的服务顺序：

```c
GGS_AddService(GATT_ALL_SERVICES);          /* GAP service */
GATTServApp_AddService(GATT_ALL_SERVICES);  /* GATT service */
DevInfo_AddService();                       /* Device Information */
MyCustomService_AddService(GATT_ALL_SERVICES);
```

此顺序的重要性：

- GATT 属性基于句柄。
- 标准服务占用预期的句柄范围。
- 自定义服务句柄应在 GAP/GATT/设备信息服务之后分配。

## 广播规则

- 传统广播数据限制为 31 字节。
- 扫描响应数据同样受限；不得将所有元数据都塞入广播数据。
- 仅在确认 `len <= 31` 后使用 `GAPRole_SetParameter(GAPROLE_ADVERT_DATA, len, data)`。
- 发送通知前，保存连接事件中的连接句柄。

连接句柄生命周期示例：

```c
static uint16_t connHandle = INVALID_CONNHANDLE;

case GAPROLE_CONNECTED:
    connHandle = pEvent->linkCmpl.connectionHandle;
    break;

case GAPROLE_DISCONNECTED:
    connHandle = INVALID_CONNHANDLE;
    break;
```

## 通知

发送通知需要满足：

- 有效的连接句柄。
- 有效的特征句柄。
- 对端已启用 Client Characteristic Configuration Descriptor。
- 载荷长度与协商的 MTU 一致。

对于面向吞吐量的设计：

- 仅当对端及应用能够处理 DLE 时，才为 MTU 247 使用 `BLE_BUFF_MAX_LEN = 251`。
- 增加每个连接事件的 TX 事件数前，先增大堆及缓冲区数量。
- 在支持且经 EVT 示例验证时考虑使用 2M PHY。

## TMOS 事件处理

BLE 示例依赖 TMOS 事件处理。

规则：

- 使用 `TMOS_ProcessEventRegister(...)` 注册任务。
- `Main_Circulation()` 必须持续运行。
- 事件处理函数必须返回未处理的事件位，不能在处理一个事件后直接返回 `0`。

模式：

```c
if (events & MY_EVT) {
    handle_my_evt();
    return (events ^ MY_EVT);
}
return 0;
```

## 中断放置

对于 BLE 系列，若对应系列 EVT 启动/链接器配置有要求，延迟敏感的中断处理函数应使用 WCH 快速中断属性及 RAM/highcode 段。

```c
__attribute__((interrupt("WCH-Interrupt-fast")))
__attribute__((section(".highcode")))
void UART1_IRQHandler(void) {
    /* ISR body */
}
```

依赖 `.highcode` 前，应确认当前链接器脚本及启动复制逻辑中包含该段。

## 角色模板

### 外围设备

主要来源：

- `ch57x/recipes/ble_peripheral.md`
- `ch58x-ch59x/recipes/ble_peripheral.md`

外围设备流程：

1. 配置 `config.h` 的堆、缓冲区、TX 功率及连接数。
2. 初始化 BLE 栈、HAL、GAP 外围设备角色及应用。
3. 依次注册 GAP、GATT、设备信息和自定义服务。
4. 配置广播数据及扫描响应数据。
5. 启动广播。
6. 跟踪连接状态及连接句柄。
7. 处理读写回调，仅在已连接时发送通知。

来源笔记中的示例根目录：

- CH57x: `resources/EXAM/BLE/Peripheral/`
- CH58x/CH59x: `resources/EXAM/BLE/Peripheral/`, `resources/EXAM/BLE/HeartRate/`, `resources/EXAM/BLE/BLE_UART/`

### 中心设备

主要来源：

- `ch58x-ch59x/recipes/ble_central.md`

中心设备流程：

1. 配置堆及中心设备连接数。
2. 初始化 BLE 栈及 HAL。
3. 调用 `GAPRole_CentralInit()` 和 `Central_Init()`。
4. 启动扫描。
5. 过滤发现的设备并建立链路。
6. 执行服务发现。
7. 读写目标特征。

示例根目录：

- `resources/EXAM/BLE/Central/`
- `resources/EXAM/BLE/MultiCentral/`

### HID

主要来源：

- `ch57x/recipes/ble_hid.md`

HID 专用笔记：

- HID 设备通常在调试完成后启用低功耗行为。
- 在 HID 服务前注册标准服务。
- 常用服务包括设备信息、电池、HID，以及可选的扫描参数。
- HID 报告描述符及报告 ID 属于应用约定；在生成的模板中应显式保留。

示例根目录：

- `resources/EXAM/BLE/HID_Keyboard/`
- `resources/EXAM/BLE/HID_Mouse/`
- `resources/EXAM/BLE/HID_Consumer/`
- `resources/EXAM/BLE/HID_Touch/`

### Mesh

主要来源：

- `ch58x-ch59x/recipes/ble_mesh.md`

Mesh 专用笔记：

- Mesh 比简单外围设备需要更多堆；来源笔记以 10KB+ 为基线。
- Mesh 初始化在 BLE 栈及 HAL 初始化后进行。
- 配网数据、OOB 数据、设备 UUID、密钥及 TTL 需要专用模板字段。

示例根目录：

- `resources/EXAM/BLE/MESH/adv_ali_light/`
- `resources/EXAM/BLE/MESH/adv_vendor/`
- `resources/EXAM/BLE/MESH/provisioner_vendor/`

### BLE + USB 组合

主要来源：

- `ch58x-ch59x/recipes/ble_usb_combo.md`

组合专用笔记：

- 由于 BLE 和 USB 处理共享 RAM 及事件延迟约束，应预留更大的 BLE 堆。
- 按来源方案，在初始化 USB 应用前初始化 BLE/HAL/GAP/应用。
- USB 到 BLE 及 BLE 到 USB 路径使用环形缓冲区。
- 对具有多个 USB 控制器的芯片，显式选择 USB 实例。

示例概念：

- BLE 数据写入 BLE 环形缓冲区，再由 USB 发送。
- USB OUT 数据写入 USB 环形缓冲区，再通过 BLE 通知发送。
- 若支持且经验证，可选择更新至 2M PHY 以提高吞吐量。

## OTA 与 IAP 交叉引用

BLE OTA 参考资料主要归入 `Doc/IAP/`，因为其受闪存布局及链接偏移约束。

BLE 相关提醒：

- OTA 写入必须遵循闪存擦除粒度。
- 活动镜像、备份镜像、引导加载程序、IAP 镜像及 SNV/DataFlash 不得重叠。
- BLE OTA 命令处理函数通常提供信息、擦除、编程、验证及结束/切换镜像命令。

来源：

- `Doc/Ref/wch-dev-skill/chips/ch57x/recipes/iap_ota.md`

## 常见 BLE 陷阱

| 陷阱 | 影响 | 规则 |
|---|---|---|
| 更改 BLE 初始化顺序 | 硬错误或控制器行为未定义 | 保持栈初始化先于 HAL 及 GAP 角色初始化。 |
| 省略 `Main_Circulation()` | 不处理任何 BLE 事件 | 始终进入 TMOS 事件循环。 |
| 堆过小 | 初始化失败、内存损坏或硬错误 | 6KB 仅作保守建议；CH572 当前 EVT 默认 3584 字节且库检查下限为 3KB。按目标 EVT 和功能测量，Mesh、多角色、组合或高吞吐量场景通常需要增大。 |
| MTU 假设错误 | 载荷截断或吞吐量不匹配 | `ATT_MTU = BLE_BUFF_MAX_LEN - 4`。 |
| 首先注册自定义 GATT 服务 | 句柄/服务问题 | 先注册 GAP、GATT、设备信息。 |
| 需要时 ISR 未放入 highcode | 时序抖动或对闪存等待状态敏感 | 验证链接器/启动配置后使用快速中断及 `.highcode`。 |
| 未在外设前配置 GPIO | UART/USB/调试信号失效 | 外设初始化前配置引脚。 |
| 通知使用无效句柄 | 通知失败 | 保存连接事件中的句柄，并在断开时复位。 |
| TMOS 事件处理函数过早返回 `0` | 丢弃待处理事件 | 返回未处理的事件位。 |
| 广播数据超过 31 字节 | 截断或广播失败 | 传统广播载荷保持在 31 字节以内。 |
| UUID 字节序错误 | 服务发现不匹配 | 栈示例有要求时，以小端序存储 BLE UUID。 |

## 验证状态

- 仅提取自 `wch-dev-skill` Markdown。
- 生成代码或模板前，仍须根据仓库 EVT 源码检查 API 名称及确切示例路径。
- 下一轮验证应检查 `CH572EVT/`、`CH573EVT/`、`CH583EVT/`、`CH585EVT/`、`CH592EVT/` 及 `CH595EVT/` 的 BLE 示例目录。

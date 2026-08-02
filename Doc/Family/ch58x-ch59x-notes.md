# CH58x 和 CH59x 系列说明

本文档从 `Doc/Ref/wch-dev-skill` 提取 CH583、CH585、CH592 和 CH595 指导，形成针对 BLE、USB、HMI、RTOS、IAP 和模板工作的仓库专用归一化说明。

官方 EVT 示例、BLE 栈库、RM、DS、启动代码文件、链接脚本、开发板原理图和当前仓库源代码仍是最终依据。

## 源文件

- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/config_reference.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/recipes/new_project.md`
- `Doc/Family/family-routing.md`
- `Doc/Family/family-normalization-notes.md`

## 支持的芯片和 EVT 根目录

| 芯片 | 仓库 EVT 根目录 | 源说明中的内存 | 源说明中的关键附加功能 |
|---|---|---|---|
| CH583 | `CH583EVT/` | 448 KB Flash、32 KB RAM | BLE、源表中的双 USB、通用 CH58x 示例。 |
| CH585 | `CH585EVT/` | 448 KB Flash、128 KB RAM | BLE、NFCA、LED 控制器、LCD、源表中的双 USB。 |
| CH592 | `CH592EVT/` | 448 KB Flash、26 KB RAM | BLE、LCD，RAM 小于 CH583/CH585。 |
| CH595 | `CH595EVT/` | 240 KB Flash、32 KB RAM | BLE、LCD、ENCODER、KEYSCAN、LED 控制器。 |

## 架构、工具链、启动代码、链接器

- 架构：WCH RISC-V BLE MCU 系列。
- 工具链：MounRiver Studio 工程文件，并链接或从 EVT 源复制 `Ld/Link.ld`、`Startup`、`RVMSIS` 和 `StdPeriphDriver`。
- 主头文件：根据所选芯片使用 `CH58x_common.h` 或 `CH59x_common.h`。
- BLE 栈库：源说明在 BLE 示例下列出 `libCH58xBLE.a` / `libCH59xBLE.a`；需按 EVT 树验证确切库路径。
- 模板规则：修改应用代码前，选择最接近的 BLE、UART、USB、LCD 或 GPIO 示例。

## 内存和引导布局

- 源说明中的标准应用从 `0x00000000` 开始。
- 源说明中的 IAP 应用使用 4 KB 引导加载程序，应用起始地址为 `0x00001000`。
- 源说明中的 Flash 擦除粒度为 256 字节，写入粒度为 4 字节。
- BLE 堆和缓冲区在 `config.h` 中配置，并非仅由链接器配置。

规则：

- CH592 的 26 KB RAM 和 CH595 的 240 KB Flash 要求更严格地规划堆、栈和功能预算。
- CH585 较大的 128 KB RAM 可支持更大的 BLE 堆或缓冲，但仍需进行 EVT/链接器验证。
- 未检查内存长度时，不要在 CH583/CH585/CH592/CH595 间复制同一个 `Link.ld`。
- IAP、OTA 和 BackupUpgrade 流程必须使用 EVT 示例中匹配的链接脚本和镜像标志。

## 外设和示例覆盖范围

通用源示例包括：

- BLE 角色：Peripheral、Central、Broadcaster、Observer、CentPeri、MultiCentral、MultiCentPeri、HID、HeartRate、BLE_UART、BLE_USB、DTM、RF_PHY、Mesh、OTA/IAP 变体。
- USB：在提供的情况下，包括设备 COM/HID/厂商自定义和主机枚举/AOA/U_DISK。
- 通用外设：ADC/TouchKey、Flash、I2C、IAP、PM、PWMX、SPI、定时器、UART1、RTOS 示例。
- 芯片专有功能：CH585 NFCA、CH595 ENCODER 和 KEYSCAN、CH585/CH595 LED 控制器，以及源说明所述 CH585/CH592/CH595 上的 LCD。

## 主题交叉引用

- BLE 规则：`Doc/BLE/wch-ble-notes.md`。
- USB 规则：`Doc/USB/wch-usb-notes.md`。
- IAP/OTA 规则：`Doc/IAP/wch-iap-ota-notes.md`。
- RTOS/TMOS 边界：`Doc/RTOS/wch-rtos-notes.md`。
- LCD/TouchKey/KEYSCAN：`Doc/HMI/wch-hmi-specialty-notes.md`。
- 工程模板：`Doc/Templates/wch-project-template-notes.md`。
- 低功耗/系统实用功能：`Doc/System/wch-system-analog-power-notes.md`。

## 已知系列陷阱

- BLE 初始化顺序必须准确：系统时钟、BLE 栈初始化、HAL 初始化、GAP 角色初始化、应用初始化、`Main_Circulation()`。
- `Main_Circulation()` 是 TMOS/BLE 调度器，不得省略，也不得允许其返回。
- `BLE_MEMHEAP_SIZE`、`BLE_BUFF_MAX_LEN`、连接限制和休眠策略必须与角色/配置文件匹配。
- 源说明中的 ATT MTU 为 `BLE_BUFF_MAX_LEN - 4`。
- GATT 服务应先注册 GAP/GATT/DeviceInfo，再注册自定义服务。
- 中断处理程序可能需要 WCH 快速中断属性并放置在 `.highcode` 中。
- CH583/CH585 双 USB 示例需要使用正确的 USB 实例函数。
- Flash 局部更新需要对 256 字节扇区执行读取-修改-写入。
- 根据源说明，CH595 KEYSCAN 休眠唤醒需要系列专用电源准备，例如 `KeyScanPowAdj()`。
- 当开发板使用 NFC_CTR 路径时，CH585 NFCA 需要设置模拟/天线控制引脚。

## 验证清单

- 验证每个 EVT 根目录和所选 `.wvproj/.project/.cproject` 结构。
- 验证确切的芯片头文件、BLE 库、`config.h`、链接器内存长度、启动代码文件和调试 UART 引脚。
- 验证 BLE 角色示例路径、栈初始化顺序、服务注册、堆/缓冲区大小和休眠设置。
- 按芯片验证 USB 控制器实例、端点缓冲区规则和双 USB 可用性。
- 根据开发板原理图和 DS 验证 CH585 NFCA 及 CH595 KEYSCAN/ENCODER/LED 示例。
- 生成更新模板前验证 IAP/OTA 链接器偏移、镜像标志和备份升级流程。

## 验证状态

- 根据 `Doc/Ref/wch-dev-skill` Markdown 源和上述仓库主题说明提取。
- 本轮尚未针对所有 CH583/CH585/CH592/CH595 EVT 树、BLE 库、RM、DS、工程文件、启动代码文件、链接脚本、封装或开发板原理图进行完整验证。
- 在根据确切 EVT 和官方文档完成核查前，应将功能可用性、内存值和芯片专用示例视为初步信息。

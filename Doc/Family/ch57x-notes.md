# CH57x 系列说明

本文档从 `Doc/Ref/wch-dev-skill` 提取 CH57x 指导，形成针对 BLE、USB、RF、IAP、HMI 和模板工作的仓库专用归一化说明。

官方 EVT 示例、BLE 库、RM、DS、启动代码文件、链接器/分散加载文件、开发板原理图和当前仓库源代码仍是最终依据。

## 源文件

- `Doc/Ref/wch-dev-skill/chips/ch57x/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/resources/config_reference.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/resources/ble_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/recipes/new_project.md`
- `Doc/Family/family-routing.md`
- `Doc/Family/family-normalization-notes.md`

## 支持的芯片和 EVT 根目录

| 芯片/源组 | 仓库 EVT 根目录 | 源说明中的架构/工具链 | 关键约束 |
|---|---|---|---|
| CH572 / CH573 风格 CH57x | `CH572EVT/`, `CH573EVT/` | RISC-V、MounRiver、`Link.ld` | BLE 栈、`.highcode`；两款芯片的 Flash/RAM 起点和容量不同。 |
| CH579 旧版源说明 | 源说明提及 CH579EVT | ARM Cortex-M0、Keil `.sct` | 架构、Flash/RAM 映射及 NET/LCD 示例不同。使用前需验证本仓库是否包含确切的 EVT。 |

## 架构、工具链、启动代码、链接器

- CH572 风格源使用 MounRiver，并链接 `Ld`、`RVMSIS`、`Startup` 和 `StdPeriphDriver` 目录。
- CH579 源说明使用 Keil 分散加载文件和 ARM Cortex-M0 启动代码，而非 CH572 RISC-V 流程。
- `.highcode` 从 Flash 加载并在 RAM 中执行，用于中断处理程序、Flash 例程和时序敏感的 BLE 路径。
- BLE 工程需要 `config.h`、BLE 栈库、HAL、角色/配置文件源代码和 `Main_Circulation()`。

## 内存和引导布局

- 当前默认链接脚本中，CH572 为 240 KB Flash，RAM 位于 `0x20000000`、长度 12 KB；CH573 为 448 KB Flash，RAM 位于 `0x20003800`、长度 18 KB。不得交换两者布局。
- CH572 IAP 源说明：起始位置有 4 KB 引导加载程序，应用起始地址为 `0x1000`；BackupUpgrade 布局使用镜像 A/B、IAP 镜像和 DataFlash 镜像标志。
- CH579 源说明：应用从 `0x00000000` 开始，DataFlash 位于 `0x3E800` 附近，引导加载程序位于顶部 `0x3F000` 附近，RAM 分为两个区。
- BLE 堆由 `BLE_MEMHEAP_SIZE` 配置；源说明提到 CH572 默认约 6 KB，CH579 约 8 KB。

## 外设和示例覆盖范围

源示例包括 ADC/TouchKey、CMP、Flash/DataFlash、I2C、基于 USB/UART 的 IAP、KEYSCAN、电源管理、PWMX、RF、SPI、定时器、UART、USB 设备/主机以及 BLE 角色/配置文件。CH579 源说明还提及段式 LCD 和 WCHNET 风格的 NET 示例。

## 主题交叉引用

- BLE：`Doc/BLE/wch-ble-notes.md`。
- USB：`Doc/USB/wch-usb-notes.md`。
- IAP/OTA：`Doc/IAP/wch-iap-ota-notes.md`。
- KEYSCAN/TouchKey/LCD：`Doc/HMI/wch-hmi-specialty-notes.md`。
- 低功耗/系统：`Doc/System/wch-system-analog-power-notes.md`。
- 工程模板：`Doc/Templates/wch-project-template-notes.md`。

## 已知系列陷阱

- BLE 初始化顺序很严格：时钟、BLE 库、HAL、GAP 角色、应用初始化，然后是 `Main_Circulation()`。
- `Main_Circulation()` 必须运行 BLE/TMOS 事件循环；仅使用 `__WFI()` 不会处理 BLE 事件。
- BLE 堆和 `BLE_BUFF_MAX_LEN` 的大小必须适配角色、MTU、服务和连接数。
- 源说明中的 ATT MTU 为 `BLE_BUFF_MAX_LEN - 4`。
- GAP/GATT/DeviceInfo 服务必须在自定义服务之前注册。
- 中断处理程序应在需要时使用 WCH 快速中断属性并放置在 `.highcode` 中。
- Flash 写入需要进行 256 字节擦除/读取-修改-写入处理。
- CH572 风格和 CH579 风格布局的 IAP 应用起始地址不同。

## 验证清单

- 验证确切的 EVT 根目录，以及本仓库是否存在相关 CH579 资料。
- 验证 BLE 库路径、头文件名、`config.h`、堆/缓冲区设置、角色示例和 `Main_Circulation()` 调用位置。
- 验证链接器/分散加载文件、RAM 起始地址、DataFlash 地址、IAP/OTA 镜像标志和启动向量行为。
- 根据当前头文件和开发板原理图验证 USB、RF、KEYSCAN、CMP、TouchKey 和电源示例。

## 验证状态

- 根据 `Doc/Ref/wch-dev-skill` Markdown 源和上述仓库主题说明提取。
- 本轮尚未针对所有 CH57x EVT 树、BLE 库、RM、DS、工程文件、启动代码文件、链接器/分散加载文件、封装或开发板原理图进行完整验证。
- 在与实际仓库 EVT 资料匹配前，尤其应将 CH579 相关细节视为源说明线索。

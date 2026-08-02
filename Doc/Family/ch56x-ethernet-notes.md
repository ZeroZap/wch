# CH56x 以太网系列说明

本文档从 `Doc/Ref/wch-dev-skill` 提取 CH569 / CH56x 指导，形成针对以太网、USB3、eMMC、HSPI、ECDC、DVP、存储和高速模板工作的仓库专用归一化说明。

官方 EVT 示例、RM、DS、启动代码文件、链接脚本、开发板原理图、PHY/存储/摄像头数据手册和当前仓库源代码仍是最终依据。

## 源文件

- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/new_project.md`
- `Doc/Family/family-routing.md`
- `Doc/Family/family-normalization-notes.md`

## 支持的芯片和 EVT 根目录

| 系列 | 仓库 EVT 根目录 | 架构 | 说明 |
|---|---|---|---|
| CH569 / CH56x | `CH569EVT/` | RISC-V、MounRiver | 源系列涵盖 CH569 高速外设。CH561/CH563 是 ARM7TDMI，未来处理时必须使用 `Doc/Family/ch561-ch563` 资料。 |

## 架构、工具链、启动代码、链接器

- 工具链：带 `CH56x_common.h` 和共享 `EXAM/SRC` 资源的 MounRiver Studio 工程文件。
- 源说明中的共享源根目录：`SRC/Peripheral`、`SRC/Startup`、`SRC/Ld`、`SRC/RVMSIS` 和 `SRC/Flash_Lib`。
- 时钟规则：源陷阱要求在延时和外设初始化前调用 `SystemInit(CLK_SOURCE_PLL_120MHz)`，以确保 UART/定时器时序正确。
- 调试规则：UART0 是源模板中的通用 printf 路径；调试 UART 的选择可能由 `CH56x_common.h` 中的定义控制。

## 内存和 DMA 布局

CH56x 的决定性约束是 RAM 与 RAMX 的区别。

| 区域 | 源说明地址 | 大小 | 用途 |
|---|---:|---:|---|
| Flash | `0x00000000` | 448 KB 代码区及末尾约 64 KB 数据区 | 程序、常量、引导加载程序/应用/数据分区。 |
| RAM | `0x20000000` | 16 KB | 通用数据、栈、`.data`、`.bss`；源说明中 DMA 不可访问。 |
| RAMX | `0x20020000` | 32 KB | `.dmadata`、`.highcode`、DMA 缓冲区、快速 ISR 代码、ECDC/ETH/eMMC 缓冲区。 |

规则：

- 将以太网、eMMC、SPI/HSPI 和其他 DMA 缓冲区放入 RAMX 的 `.dmadata`。
- 对延迟敏感时，将时序关键的 ISR 或快速代码放入 `.highcode`。
- 避免使用大型局部缓冲区，因为默认栈较小，常规 RAM 仅有 16 KB。
- 根据源说明，Flash 擦除粒度为 256 字节，写入粒度为 4 字节。
- IAP 源布局使用 4 KB 引导加载程序，应用位于 `0x1000`，下载缓冲区约位于 `0x40000`，数据存储区靠近 `0x70000`；使用前需验证确切的 EVT 链接器配置。

## 外设和示例覆盖范围

源示例包括：

- 以太网：WCHNET TCP 客户端/服务器、UDP 客户端/服务器、DHCP、DNS、原始 IP ping、MAC raw 和 ETH 源示例。
- USB3：USBSS 设备和主机示例。
- HSPI：普通模式、突发模式、动态上下行角色切换、双 DMA 模式以及搭配 ECDC 的 HSPI。
- ECDC：AES/SM4 硬件加密/解密。
- DVP：OV2640 风格的摄像头采集。
- 存储：SD、eMMC 以及源说明中与 AES 相关的 eMMC。
- 通用外设：GPIO、UART0-3、SPI0/1、TMR0-2、PWMX、Flash、IAP、BUS8、选项字节、低功耗。

## 主题交叉引用

- 以太网/WCHNET：`Doc/ETH/wch-ethernet-notes.md`。
- USB3/USB HS：`Doc/USB/wch-usb-notes.md`。
- 存储/eMMC/HSPI：`Doc/Storage/wch-storage-notes.md`。
- ECDC/安全：`Doc/Security/wch-security-crypto-notes.md`。
- DVP 摄像头和流式 IO：`Doc/IO/wch-io-media-notes.md`。
- BUS8/PWMX/HMI 专用功能：`Doc/HMI/wch-hmi-specialty-notes.md`。
- 工程模板：`Doc/Templates/wch-project-template-notes.md`。
- IAP 和 Flash 布局：`Doc/IAP/wch-iap-ota-notes.md`。

## 已知系列陷阱

- 普通 RAM 中的 DMA 缓冲区会导致以太网/eMMC/SPI DMA 挂起或数据损坏；应使用 RAMX `.dmadata`。
- 外设时钟默认关闭；应为 ETH、UART、SPI、TMR、EMMC、USBSS、ECDC 等启用 `BIT_SLP_CLK_*`。
- CH561/CH563 是 ARM7TDMI，无法编译 CH569 RISC-V 代码或直接使用 CH569 API。
- Flash 扇区擦除会破坏整个 256 字节扇区；局部写入需要读取-修改-写入。
- 为降低延迟，中断处理程序可能需要放置在 `.highcode`/RAMX 中。
- 以太网 PHY 故障排查需检查 PHY 地址、MDC/MDIO 重映射、电源、复位和 25 MHz 时钟。
- 使用前必须初始化 ECDC 密钥寄存器。
- eMMC 缓冲区至少需要字对齐，并放置在 DMA 可访问区域。
- 混用 GPIO 和外设信号前，必须解决 GPIO 引脚重映射冲突。

## 验证清单

- 验证 `CH569EVT/EXAM` 分类，以及 ETH、USBSS、HSPI、ECDC、DVP、SD、EMMC、IAP 和通用外设的确切示例路径。
- 验证 `Link.ld` RAM/RAMX 段、`.dmadata`、`.highcode`、栈大小和 Flash 分区布局。
- 验证 WCHNET 库要求、周期任务行为、PHY 接口、DMA 描述符放置位置和开发板 PHY 地址。
- 验证 USBSS/USBHS 控制器实例、端点/缓冲区约束及主机/设备示例。
- 验证 ECDC 测试向量、密钥/计数器处理、RAMX 长度单位和 HSPI 关联的 DMA 模式。
- 验证 eMMC/SD 块大小、对齐、总线宽度和加密传输示例。

## 验证状态

- 根据 `Doc/Ref/wch-dev-skill` Markdown 源和上述仓库主题说明提取。
- 本轮尚未针对 CH569 EVT 源代码树、RM、DS、工程文件、启动代码文件、链接脚本、开发板原理图、PHY 数据手册、存储器件数据手册或摄像头数据手册进行完整验证。
- 在根据官方 CH569 资料和当前头文件完成核查前，应将内存布局、DMA 约束、时钟和功能可用性视为初步信息。

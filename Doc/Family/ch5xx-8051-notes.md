# CH5xx 8051 系列说明

本文档从 `Doc/Ref/wch-dev-skill` 提取 CH543 至 CH559 8051 系列指导，形成针对 C51 模板、USB、Type-C、IAP、TouchKey 和低资源 HAL 工作的仓库专用归一化说明。

官方 EVT 示例、数据手册、寄存器头文件、Keil C51 工程、ISP 引导加载程序行为、开发板原理图和当前仓库源代码仍是最终依据。

## 源文件

- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/resources/memory_layout.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/resources/example_list.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/recipes/new_project.md`
- `Doc/Family/family-routing.md`
- `Doc/Family/family-normalization-notes.md`

## 支持的芯片和 EVT 根目录

源说明涵盖 CH543、CH545、CH547、CH549、CH552/CH551、CH554、CH555、CH557、CH558 和 CH559。部分 EVT 包仅包含头文件，只有在验证寄存器兼容性后才能借用相关芯片的示例。

## 架构、工具链、启动代码、链接器

- 架构：8051 Harvard 架构，具有独立的 code、data/idata、xdata、bit 和 SFR 空间。
- 工具链：Keil C51 以及可能的 SDCC 风格流程；源方案重点使用 Keil C51 `.uvproj` 工程。
- 常见工程根文件包括 `Public/CH5xx.H`、`Debug.C/H`、应用 `Main.C` 和外设 `.C/.H` 文件。
- C51 内存模型、代码分区以及 `data`/`idata`/`xdata` 限定符对模板至关重要。

## 内存和引导布局

- 源说明中各芯片的代码 Flash 约为 10 KB 至 64 KB。
- `data` 为 128 字节直接寻址空间；`idata` 为 256 字节间接寻址空间；xdata 视芯片而定，为 512 字节至 4 KB。
- 多款 CH55x/CH54x 芯片的 DataFlash 位于 `0xC000`，大小因芯片而异。
- 许多 CH5xx 芯片的内置 ISP 引导加载程序位于代码 Flash 高地址区；应用通常从 `0x0000` 开始，且不得进入引导加载程序区域。
- 中断向量占用低位代码地址，从复位向量 `0x0000` 开始。

## 外设和示例覆盖范围

源示例包括 ADC、比较器、DataFlash/FlashRom、GPIO、I2C、部分芯片上的 USB-PD/Type-C、PWM、SPI、定时器、TouchKey、UART、USB 设备/主机、IAP、RGB/LED 控制、加密/芯片 ID 示例，以及视芯片而定的类似大容量存储的 USB 示例。

## 主题交叉引用

- 8051 内核/内存规则：`Doc/Core/wch-core-notes.md`。
- 工程模板：`Doc/Templates/wch-project-template-notes.md`。
- USB：`Doc/USB/wch-usb-notes.md`。
- Type-C：`Doc/USBPD/wch-usbpd-notes.md`。
- TouchKey/HMI：`Doc/HMI/wch-hmi-specialty-notes.md`。
- IAP/ISP：`Doc/IAP/wch-iap-ota-notes.md`。
- 通用外设：`Doc/HAL/wch-hal-normalization.md`。

## 已知系列陷阱

- 不要使用 CH32 或 CH57x 的头文件、启动代码、链接器、中断属性或工程结构。
- 写入受保护的系统寄存器前，应使用受保护寄存器安全模式序列。
- Flash/DataFlash 写入期间应控制 `EA` 中断状态。
- 使用正确的 C51 内存限定符；大缓冲区应放入 `xdata`，而非稀缺的 `data`。
- USB 缓冲区可能要求偶数地址对齐和特定的 xdata 放置方式。
- ISP 引导加载程序占用 ROM 高地址区；Keil 代码大小/ROM 范围必须避免重叠。
- DataFlash 访问需要芯片专用 ROM 地址寄存器及安全模式/中断处理。
- TouchKey 和比较器引脚需要配置为模拟/浮空输入。

## 验证清单

- 验证本仓库中存在的 CH5xx EVT 根目录和头文件。
- 验证芯片专用 `CH5xx.H`、SFR 名称、中断编号、向量布局和 C51 工程设置。
- 按芯片验证 DataFlash 大小/地址、ISP 引导加载程序地址、代码 ROM 限制和 IAP 跳转地址。
- 验证 USB 设备/主机示例、端点缓冲区、Type-C 引脚、TouchKey 焊盘和开发板级约束。
- 验证生成模板所用的 Keil 内存模型、代码分区、xdata 大小和调试 UART 设置。

## 验证状态

- 根据 `Doc/Ref/wch-dev-skill` Markdown 源和上述仓库主题说明提取。
- 本轮尚未针对所有 CH5xx EVT 树、数据手册、Keil 工程、头文件、ISP 引导加载程序行为或开发板原理图进行完整验证。
- 在根据确切芯片资料完成核查前，应将内存映射、引导加载程序地址、DataFlash 行为和示例兼容性视为初步信息。

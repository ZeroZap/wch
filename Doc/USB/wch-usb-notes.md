# WCH USB 笔记

本文从 `Doc/Ref/wch-dev-skill` 提取 USB 相关指导，整理为适用于本仓库的笔记，供后续 HAL、驱动、模板及元数据工作使用。

范围：

- CH5xx、CH57x、CH58x/CH59x、CH32X/CH64x、CH32V、CH32F 及 CH32H 系列的 USB 全速设备流程。
- CH57x 及 CH58x/CH59x 的 USB 主机流程。
- CH32H417 及 CH569 系列来源的 USB 高速和 USB 超高速笔记。
- 通用端点、描述符、缓冲区、时钟、中断及陷阱规则。

最终应以官方 EVT 示例、RM、DS、开发板原理图、USB 库头文件及 USB 规范为准。

## 来源文件

- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/recipes/usb_device.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/recipes/usb_device.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/recipes/usb_host.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/recipes/usb_device.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/recipes/usb_host.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/recipes/usb_device.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/usb_device.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/usb_device.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/usb3_device.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/usbfs_device.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/usbhs_device.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/usb3_device.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/resources/pitfalls.md`

## 当前控制器矩阵

下表以当前仓库 EVT 目录为依据，角色表示仓库中存在对应 Host/Device 示例，不外推到未导入型号。

| 系列/型号 | USBFS | USBHS | USBSS | 当前示例根及说明 |
|---|---|---|---|---|
| CH32V103 | Host、Device | 无当前示例 | 无当前示例 | `CH32V103EVT/EXAM/USB/USBFS/`，含 Host IAP。 |
| CH32V20x | Host、Device | 无当前示例 | 无当前示例 | `CH32V20xEVT/EXAM/USB/USBFS/`；另有旧 `USB/USBD/` 设备库示例。 |
| CH32V307 | Host、Device | Host、Device | 无当前示例 | `CH32V307EVT/EXAM/USB/USBFS/`、`CH32V307EVT/EXAM/USB/USBHS/`。 |
| CH32V407 | 未见独立 USBFS 示例 | Host、Device | 无当前示例 | `CH32V407EVT/EXAM/USBHS/`，含 device/host、双设备和 device-host 示例。 |
| CH32X035 | Host、Device | 无当前示例 | 无当前示例 | `CH32X035EVT/EXAM/USB/USBFS/`，含 Host IAP；USB-PD 另行建模。 |
| CH32X315 | 无当前示例 | Host、Device | Host、Device | `CH32X315EVT/EXAM/USBHS/`、`CH32X315EVT/EXAM/USBSS/`。 |
| CH572 | Host、Device | 无当前示例 | 无当前示例 | `CH572EVT/EXAM/USB/{Host,Device}/`。 |
| CH573 | Host、Device | 无当前示例 | 无当前示例 | 实际路径为 `CH573EVT/EVT/EXAM/USB/{Host,Device}/`。 |
| CH583 | Host、Device，含第二 USB 实例示例 | 无高速控制器证据 | 无当前示例 | `CH583EVT/EXAM/USB/`，含 `CompoundU2Dev`、`HostU2Enum`；第二实例不能据名称推断为 USBHS。 |
| CH585 | Host、Device | Host、Device | 无当前示例 | `CH585EVT/EXAM/USB/USBFS/`、`CH585EVT/EXAM/USB/USBHS/`。 |
| CH592 | Host、Device | 无当前示例 | 无当前示例 | `CH592EVT/EXAM/USB/{Host,Device}/`。 |
| CH595 | Host、Device | 无当前示例 | 无当前示例 | `CH595EVT/EXAM/USB/{Host,Device}/`。 |
| CH32M030 | Host、Device | 无当前示例 | 无当前示例 | `CH32M030EVT/EXAM/USB/USBFS/`。 |
| CH32L103 | Host、Device | 无当前示例 | 无当前示例 | `CH32L103EVT/EXAM/USB/USBFS/`。 |
| CH32H417 | Host、Device | Host、Device | Host、Device | 当前包目录名为 `CH32H47TEVT/EXAM/USBFS/`、`USBHS/`、`USBSS/`。 |
| CH569 | 无当前示例 | 与 USBSS 示例配套的 USB 2.0 路径 | Host、Device | `CH569EVT/EXAM/USBSS/`；具体控制器组合和角色边界仍须对照 RM/DS。 |

CH5xx 8051、未导入的 CH57x/CH58x/CH59x、CH32X/CH64x、CH32V 和 CH32F 型号的能力不能由上述型号矩阵类推，必须查看目标型号 EVT、RM 和 DS。

## USB 层级模型

在 HAL 元数据中将 USB 层级、角色及类分开。

| 层级 | 标称速率 | 典型 WCH 名称 | 主要约束 |
|---|---:|---|---|
| USBFS | 12 Mbps | USB、USBFS | 48 MHz 时钟、EP0 控制及控制器规定的端点 DMA 缓冲区。 |
| USBHS | 480 Mbps | USBHS | 480 MHz/高速时钟、更大的 FIFO，以及取决于开发板的内部或 ULPI PHY。 |
| USBSS / USB3 | 5 Gbps | USBSS | SerDes/PHY 就绪、超高速描述符及高速 PCB 布局。 |

不得仅用 `usb_device_init()` 隐藏层级差异。后续 HAL 应分别暴露角色、层级、PHY、端点内存模型及类栈。

## 设备初始化顺序

通用 USB 设备顺序：

1. 配置系统时钟，使所选 USB 层级获得所需时钟。
2. 访问寄存器前使能 USB 控制器时钟门控。
3. 配置 USB 引脚；仅当对应系列 EVT 示例如此处理时，才依赖 USB 库完成配置。
4. 在控制器可访问的内存区域分配端点缓冲区。
5. 需要时复位 USB SIE/控制器。
6. 将 EP0 初始化为控制端点；对于 FS，应提供有效的 64 字节 SETUP/数据缓冲区。
7. 初始化类端点及描述符。
8. 将初始设备地址设为零。
9. 使能上拉或连接状态，以便主机枚举设备。
10. 使能 USB 中断并清除待处理标志。
11. 处理 SETUP、IN、OUT、总线复位、挂起及恢复事件。
12. 仅在设备进入已配置状态后启动类数据传输。

系列差异：

- CH57x 示例为 USB 显式设置 `SetSysClock(CLK_SOURCE_PLL_48MHz)`。
- CH32V USBFS 示例使用 `RCC_USBCLKConfig(...)` 并使能 USB 外设时钟；其控制器使用端点 DMA，不是 PMA。
- CH32X 示例使能 `RCC_AHBPeriph_USBFS` 并使用 `R8_*` USBFS 寄存器。
- CH32H USBFS 示例使能 `RCC_HB2Periph_USBFS`，然后调用 `USB_Init()`。
- CH5xx 示例直接配置 USB 引脚功能、`USB_CTRL`、DMA 地址、端点模式及 8051 中断使能位。
- CH569 USBSS 示例使能 `BIT_SLP_CLK_USBSS`；USBHS 使用独立的时钟位。

## EP0 与标准请求

EP0 是必需端点，必须先处理枚举，再进行类通信。

规则：

- 将 EP0 视为状态机，而非简单数据管道。
- 总线复位后始终将设备地址复位为零。
- 为成功枚举，至少处理 `GET_DESCRIPTOR`、`SET_ADDRESS` 和 `SET_CONFIGURATION`。
- 严格按照 EVT 栈的要求应用 `SET_ADDRESS` 时序；许多控制器在状态阶段后提交地址。
- 返回描述符或类数据前处理 SETUP 数据包。
- 仅对确实不支持的请求返回 STALL；对标准请求意外返回 STALL 会破坏枚举。
- 保持描述符长度、配置总长度、端点数量及接口数量一致。

描述符规则：

- 这些方案中的 FS 控制端点最大数据包大小通常为 64 字节。
- CDC 通常需要通信接口和数据接口、功能描述符、一个中断 IN 端点，以及批量 IN/OUT 端点。
- HID 需要 HID 报告描述符，且端点传输中的报告长度必须一致。
- 复合设备必须保持接口编号、端点地址、类描述符及总长度同步。
- USBSS 设备还需要 BOS、USB 2.0 扩展、超高速能力及超高速端点伴随描述符。

## 端点与缓冲区规则

端点缓冲区内存取决于控制器，应显式建模。

| 系列或层级 | 来源笔记中的缓冲区模型 | 规则 |
|---|---|---|
| CH5xx 8051 | `xdata` 中的端点 DMA 缓冲区 | 使用偶数且互不重叠的地址；将 `UEPx_DMA_L/H` 设为所分配的地址。 |
| CH32X USBFS | 控制器可访问的端点 DMA 缓冲区 | 按 EVT 的 DMA 地址、对齐及内存区域放置，不得套用 PMA 模型。 |
| CH32V USBFS | 端点 DMA 缓冲区 | 端点寄存器保存 DMA 地址；按 EVT 分配、对齐并避免重叠，不是 STM32 式 PMA。 |
| CH32H USBFS | 控制器专用端点 DMA 缓冲区 | 将端点缓冲区放在 USB 库/控制器可访问的位置并按 EVT 对齐。 |
| CH32H USBSS | DMA 端点缓冲区 | 对齐缓冲区并使用 DMA 提升吞吐量；检查 SerDes 及 USBSS 要求。 |
| CH569 USBSS/USBHS | 控制器专用 DMA 缓冲区 | 在 EVT 示例中验证段、对齐及可访问性。 |

通用端点规则：

- 静态或全局分配端点缓冲区；避免在栈上分配大型 USB 缓冲区。
- 除非控制器明确支持共享 EP0 内存，否则应分开 IN 和 OUT 缓冲区。
- 遵循双缓冲模式设置；该模式会改变所需内存占用。
- 消费数据后重新武装 OUT 端点。
- 仅在设置缓冲区及传输长度后，才将 IN 端点标记为有效或就绪。
- 除非控制器已配置自动切换，否则应正确处理数据切换。
- 将描述符中的端点地址、方向、类型、最大数据包大小及缓冲区地址映射到控制器配置。

## 设备传输处理

WCH 来源方案主要采用两种风格：

| 风格 | 系列 | 典型流程 |
|---|---|---|
| 库状态机 | CH57x、CH58x/CH59x、CH32V、CH32H USBFS | 初始化 USB，调用库传输处理，或由 ISR 分派到回调。 |
| 寄存器级事件处理函数 | CH5xx、CH32X、CH32F 风格方案 | 解码中断标志、令牌类型及端点号，并手动执行 ACK/NAK/STALL。 |

规则：

- 可行时将类逻辑放在最底层中断解析器之外。
- 在 USB ISR 中只执行最少工作：清除标志、移动少量数据、更新状态，并延后较繁重的解析。
- 对基于库的流程，应使用对应系列预期的回调名称及传输任务，而不是另行设计分派路径。
- 对寄存器级流程，按当前头文件或 EVT 代码定义的写入语义清除中断标志。
- 进入已配置状态前，不得启动 CDC、HID 或 Vendor 数据传输。

## 主机初始化与枚举

从 CH57x 及 CH58x/CH59x 笔记提取的通用 USB 主机顺序：

1. 在开发板级提供有效的 VBUS 电源及过流处理。
2. 配置 USB 时钟及 D-/D+ 引脚。
3. 使用 `USB_HostInit()` 或系列等效函数初始化主机控制器。
4. 检测根集线器连接状态。
5. 使能或复位根集线器端口。
6. 读取设备描述符。
7. 读取配置描述符。
8. 分配地址。
9. 选择配置。
10. 解析目标类的接口及端点。
11. 启动 HID 轮询、AOA 或 U 盘存储等类传输流程。

主机规则：

- 主机模式需要可靠的 5 V VBUS 电源；设备检测失败通常源于硬件供电问题。
- 接收缓冲区必须足以容纳描述符及类响应。
- 端点类型及最大数据包大小必须来自枚举得到的描述符，不得硬编码假设。
- U 盘示例要求在文件操作前完成磁盘/文件系统就绪步骤。
- AOA 示例要求安卓设备支持该协议，并可能在附件识别后强制重新枚举。

## USBFS 笔记

USBFS 是许多 WCH 系列的通用基础。

规则：

- 提供精确的 48 MHz USB 时钟。
- 来源笔记指定时使用 PA11/PA12 作为 D-/D+，但应验证具体型号的重映射及封装引脚。
- 检查是否有内部上拉，或开发板/控制器是否需要外部 1.5 kOhm D+ 上拉。
- 在模板中显式描述端点 DMA 的地址、可访问内存和对齐要求；CH32V USBFS 不得标为 PMA。
- 以 EVT 描述符及类文件作为 CDC、HID、Vendor 及 Composite 设备的起点。

## USBHS 笔记

USBHS 并非只是速度更快的 USBFS 实例。

规则：

- 将 USBHS 建模为独立控制器及层级，具有独立的中断向量、FIFO 大小及 PHY 要求。
- 严格按照 EVT 示例要求配置高速时钟树。
- 开发板使用内部高速收发器还是外部 PHY、以及外部接口是否为 ULPI，若尚未由目标 RM/DS、原理图和 EVT 共同确认，必须标为“未验证”，不得据来源笔记定案。
- 端点数量及 FIFO/缓冲区大小与 USBFS 不同；不得复用 FS PMA 假设。
- 对 CH32H 系列芯片使用 USBHS 专用设备和主机示例。

## USBSS / USB3 笔记

USBSS 需要额外硬件、描述符及链路处理。

规则：

- 访问 USBSS 寄存器前使能 USBSS 外设时钟。
- 启用 USBSS 通信前初始化并验证 SerDes/PHY。
- 在 CH32H417 风格的 SerDes 流程中按 EVT 等待 PLL 锁定及 PHY 就绪；具体 PHY/SerDes 能力仍须由 RM/DS 确认。
- 调试软件前，验证 90 ohm 差分对布局、SSTX/SSRX 走线及连接器布线。
- 提供 BOS 及 SuperSpeed 能力描述符。
- 为 SS 端点提供 SuperSpeed 端点伴随描述符。
- 高吞吐量端点使用 DMA 及对齐缓冲区。
- 除传输完成外，还应处理链路状态变化及 SuperSpeed 电源状态。

CH569/CH56x 说明：

- 来源方案的对比表与说明存在内部矛盾；应将 CH569 USBSS 能力仅视为索引提示，并在设计通用 HAL 配置前，根据 CH569 RM、DS、头文件及 EVT 示例验证确切的 USBSS/USBHS 支持。

## 多实例 USB

部分 CH58x/CH59x 型号提供多个 USB 控制器。

规则：

- 显式建模 USB 实例标识，例如 `usb1` 与 `usb2`。
- EVT/来源笔记有要求时，第二个控制器使用 `USB2_*` 初始化及传输处理函数。
- 除非厂商库明确支持，否则不得在实例间共享端点缓冲区、回调表或全局设备状态。
- 将引脚映射、时钟门控、中断向量及描述符与所选实例关联。

## 常见陷阱

| 陷阱 | 受影响系列 | 规则 |
|---|---|---|
| USB 时钟配置不准确 | FS 设备/主机流程 | USB 初始化前配置所需的 48 MHz 时钟或层级专用时钟。 |
| 外设时钟门控关闭 | CH32X/CH32V/CH32H/CH569 | 访问寄存器前使能 USBFS/USBHS/USBSS 时钟。 |
| EP0 标准请求不完整 | 所有设备流程 | 在类逻辑前实现描述符、地址及配置处理。 |
| 描述符长度不匹配 | 所有设备流程 | 保持描述符数组长度、`wTotalLength`、端点数量及接口数量一致。 |
| 端点缓冲区位于错误内存中 | CH5xx、CH32X、CH32V、CH32H、USBSS | 按控制器要求使用 xdata、端点 DMA、FIFO 或其他指定内存；尤其不得把 CH32V USBFS 误建模为 PMA。 |
| 端点缓冲区重叠 | 寄存器级端点控制器 | 分配固定且互不重叠的范围，并计入双缓冲模式。 |
| OUT 端点未重新武装 | 所有设备流程 | 消费数据后将 OUT 端点恢复为 ACK/有效状态。 |
| 设置长度/数据前启动 IN 传输 | 所有设备流程 | 填充缓冲区并设置长度后，再标记 TX 有效/ACK/启动。 |
| 使用错误的 USB 实例函数 | CH583/CH585 风格双 USB | USB2 使用 `USB2_*` API。 |
| 主机 VBUS 缺失或供电不足 | 主机流程 | 调试枚举前验证 5 V 电源、限流及连接器电源开关。 |
| 主机描述符缓冲区过小 | 主机流程 | 按完整设备/配置描述符确定接收缓冲区大小。 |
| 磁盘就绪前执行 U 盘文件操作 | CH57x/CH58x Host 存储 | 打开/读取/写入文件前调用磁盘就绪/初始化函数。 |
| USBSS SerDes PLL 未锁定 | CH32H417 USB3 | USBSS 通信前等待 PLL 锁定及 PHY 就绪。 |
| 将 WCH USB 当作 STM32 OTG | CH32F/CH32V 风格代码 | 使用 WCH USB 寄存器/库及 EVT 示例，而非 STM32 OTG 假设。 |

## 示例索引

| 系列 | 技能笔记中的来源示例路径 |
|---|---|
| CH5xx | `chips/ch5xx-8051/resources/EXAM/USB/Device/`, `Host/`, `USB_LIB/` |
| CH57x 设备 | `chips/ch57x/resources/EXAM/USB/Device/COM/`、`HID_CompliantDev/`、`VendorDefinedDev/`、`CompoundDev/` |
| CH57x 主机 | `chips/ch57x/resources/EXAM/USB/Host/HostEnum/`、`HostAOA/`、`U_DISK/` |
| CH58x/CH59x 设备 | `chips/ch58x-ch59x/resources/EXAM/USB/Device/COM/`、`HID/`、`Vendor/` |
| CH58x/CH59x 主机 | `chips/ch58x-ch59x/resources/EXAM/USB/Host/HostEnum/`、`U_DISK/` |
| CH32X035 | `CH32X035EVT/EXAM/USB/` |
| CH32V | `chips/ch32v-general/resources/EXAM/` 中按具体芯片分类的 USB 示例 |
| CH32F | 对应 CH32F EVT 包中的 `EVT/EXAM/USB/` |
| CH569 USBSS | `CH569EVT/EXAM/USBSS/` |
| CH32H417 USBFS/USBHS/USBSS | `CH32H47TEVT/EXAM/USBFS/`、`CH32H47TEVT/EXAM/USBHS/`、`CH32H47TEVT/EXAM/USBSS/` |

## 建议的 USB 元数据

后续 CubeX/HAL 元数据应包括：

- `usb_instances`：包含实例名称及基地址的控制器列表。
- `usb_role_support`：经确认的设备、主机或 OTG/双角色支持。
- `usb_tier`：FS、HS、SS。
- `usb_phy`：内部 FS、内部 HS、ULPI、SerDes 或由开发板定义。
- `usb_clock_requirements`：所需频率及时钟源约束。
- `usb_pins`：D-/D+、VBUS、ID、ULPI、SSTX、SSRX 及开发板电源开关引脚。
- `endpoint_count`：端点编号及方向能力。
- `endpoint_memory_model`：xdata、PMA、USB SRAM、FIFO、DMA 缓冲区或其他模型。
- `endpoint_buffer_alignment`：字节对齐及段要求。
- `supports_double_buffer`：已知时按端点记录是否支持双缓冲。
- `requires_pullup_control`：内部、外部或开发板专用。
- `interrupt_vectors`：低优先级、高优先级、USBHS、USBSS 或实例专用名称。
- `class_profiles`：CDC、HID、厂商自定义、复合设备、MSC 主机、AOA 主机或其他模板。
- `library_style`：厂商 USB 库、StdPeriph USB 库、寄存器级或 USBSS 专用。

## 验证状态

- 仅提取自 `wch-dev-skill` Markdown。
- 必须根据官方 EVT 示例、头文件、RM、DS 及开发板原理图检查确切的端点数量、端点内存大小、引脚重映射、中断名称、时钟树值、PHY 选择及 USBSS 能力。
- CH569 及各高速控制器的内部/外部 PHY、ULPI、SerDes 等项目，未经目标 RM/DS、原理图和 EVT 交叉核对，不得最终写入能力元数据。
- 下一轮验证应检查当前系列已导入的 `USB`、`USBFS`、`USBHS` 及 `USBSS` 示例，并依据头文件统一确切的 API 名称。

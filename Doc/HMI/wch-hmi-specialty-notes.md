# WCH 显示、HMI 与专用外设笔记

本文从 `Doc/Ref/wch-dev-skill` 提取显示、人机界面、外部存储器及专用外设相关指导，整理为适用于本仓库的规则，供后续 HAL 元数据及模板使用。

范围：

- 段式 LCD、通过 FSMC/FMC/LTDC 驱动的 TFT LCD，以及 GPHA 图形加速。
- TouchKey 和 KEYSCAN 输入外设。
- CH32H 专用外设：DFSDM、I3C、QSPI、FMC 和 SWPMI。
- CH56x 专用外设：PWMX 和 BUS8 外部总线。

最终应以官方 EVT 示例、RM、DS、原理图、显示面板数据手册、存储器数据手册及当前仓库源码为准。

## 来源文件

- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/recipes/lcd_display.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/fsmc_lcd.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/fsmc_lcd.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/lcd_display.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/gpha_graphics.md`
- `Doc/Ref/wch-dev-skill/chips/ch5xx-8051/recipes/touch_key.md`
- `Doc/Ref/wch-dev-skill/chips/ch32x-usbpd/recipes/touchkey_sensing.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/touchkey.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/touchkey.md`
- `Doc/Ref/wch-dev-skill/chips/ch57x/recipes/keyscan.md`
- `Doc/Ref/wch-dev-skill/chips/ch58x-ch59x/resources/pitfalls.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/dfsdm_filter.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/i3c_comm.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/qspi_flash.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/fmc_storage.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/swpmi_comm.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/pwmx_output.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/bus8_interface.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/example_list.md`

## 外设覆盖范围

| 主题 | 来源笔记涉及的系列 | 主要约束 |
|---|---|---|
| 段式 LCD | CH57x CH579 笔记、CH58x/CH59x CH592/CH595 笔记 | LCD 引脚与 GPIO 复用；占空比、偏置、扫描时钟、COM/SEG 映射及模拟电源必须与面板匹配。 |
| FSMC/FMC LCD 或外部存储器 | CH32V307/CH32V407、CH32F 风格方案、CH32H417 FMC 笔记 | 并行总线时序、地址线映射、数据宽度、GPIO AF 模式及存储区基地址必须与硬件匹配。 |
| LTDC RGB 显示 | CH32H417 | 需要面板时序、像素格式、帧缓冲区位置、图层配置及足够的 RAM 带宽。 |
| GPHA/DMA2D | CH32H417 | 适用于填充、复制、像素转换及混合；输出缓冲区在传输完成前必须保持有效。 |
| TouchKey | CH5xx、CH32X/CH64x、CH32V/CH32H 来源方案 | 触摸引脚必须为浮空输入，需要基线校准及迟滞，阈值取决于开发板。 |
| KEYSCAN | CH57x/CH58x/CH59x 来源笔记 | 扫描引脚可能与调试/ISP 或 GPIO 输出冲突；CH595 的低功耗唤醒需要额外电源配置。 |
| DFSDM | CH32H417 | 通道、滤波器、过采样、数据源、DMA 多路复用及数据缩放必须作为一条流水线配置。 |
| I3C | CH32H417 | 需要规划 I3C 时序、FIFO、动态地址分配、CCC 流程、DMA 多路复用及旧式 I2C 兼容性。 |
| QSPI | CH32H417 | 外部闪存命令集、线模式、空操作周期、地址大小、四线使能及内存映射模式均取决于器件。 |
| SWPMI | CH32H417 | 单线智能卡协议需要线路复位、等待就绪标志、激活及帧状态排序。 |
| PWMX | CH56x | 需要考虑独立 PWM 时钟分频器、周期长度、占空宽度、极性及复位引脚复用等引脚冲突。 |
| BUS8 | CH56x | 外部总线将器件映射到 `0x80000000`；地址线、宽度、建立/保持时序及外部芯片协议取决于开发板。 |

## 分类规则

不得将这些外设简化为单一的通用显示或 GPIO API。

规则：

- 段式 LCD、TFT 并行 LCD、RGB LTDC 面板及内存映射外部总线属于不同的显示/存储模型。
- TouchKey 和 KEYSCAN 是具有模拟/扫描时序要求的输入外设，并非简单的 GPIO 按键。
- GPHA 加速内存操作，但不控制显示面板；应与 LTDC 或 LVGL 元数据分离。
- FMC/FSMC 和 BUS8 暴露内存映射外部器件；生成的代码必须对外部器件时序及开发板布线建模。
- I3C、SWPMI、DFSDM 和 QSPI 等专用串行/控制外设应保留协议专用元数据，而不是强行套用 SPI/I2C/ADC 抽象。

## 显示规则

段式 LCD 规则：

- 必须先使能 LCD 控制器及 LCD 电源，才能获得可见输出。
- 配置与段式 LCD 玻璃匹配的占空比及偏置。
- 根据面板及功耗要求配置扫描时钟。
- 维护 COM/SEG 映射；数据寄存器或段映射错误会导致显示乱码。
- 启用 LCD 的段引脚不再是普通 GPIO 引脚。应禁用或避免冲突的 GPIO 用途。

FSMC/FMC LCD 规则：

- 配置总线前，先使能 GPIO/AFIO 及存储控制器时钟。
- 将所有总线引脚配置为复用推挽或 EVT 示例要求的模式。
- 使数据宽度与布线匹配：在支持时使用 8 位、16 位或 32 位。
- 记录连接到 LCD RS/DC 的地址线。寄存器/数据偏移取决于该地址线。
- 控制器及显示器有要求时，应使用独立的读写时序。
- 遵循来源笔记中的 `0x60000000`、`0x64000000`、`0x68000000` 和 `0x6C000000` 等外部存储区基地址。

LTDC 规则：

- 根据面板数据手册配置像素时钟、HSYNC、VSYNC、DE 及有效区/后沿/前沿值。
- 将帧缓冲区以正确的对齐方式放入 LTDC 可访问且带宽充足的 RAM。
- 保持像素格式、行长度、步距及图层尺寸一致。
- 更改需要重新加载的图层属性后，使用 `LTDC_ReloadConfig(...)`。
- LVGL 或 GUI 刷新回调应仅复制无效区域，并在传输后调用 GUI 刷新就绪钩子。

GPHA 规则：

- 配置传输前先使能 GPHA 时钟。
- 显式配置模式：R2M 填充、M2M 复制、M2M 像素格式转换或混合。
- 读取或复用输出缓冲区前等待传输完成标志。
- 在元数据中显式描述前景/背景图层格式及 alpha 设置。

## Touch 与 Keyscan 规则

TouchKey 规则：

- 将支持触摸的引脚配置为浮空输入。上拉、输出或复用功能会破坏测量结果。
- 每次选择一个通道，并在读取数据前等待测量完成标志。
- 启动时或环境变化后校准无触摸基线。
- 使用平均采样值及按下/释放迟滞阈值。
- 阈值取决于开发板，并受焊盘形状、外壳、湿度、接地及采样时间影响。
- 中断模式扫描仍需进行通道排序及基线比较。

KEYSCAN 规则：

- 引脚复用时，应在启用矩阵扫描前解除调试/ISP 或其他复用功能对引脚的占用。
- 来源中的陷阱有要求时，在启用 keyscan 硬件前将扫描引脚配置为上拉输入。
- 按需结合硬件连发/消抖及应用层消抖。
- 如需睡眠唤醒，应在进入低功耗模式前使能按键唤醒源。
- CH595 风格来源笔记要求：使用 keyscan 从睡眠唤醒时，在 `KeyScan_Cfg(...)` 前调用 `KeyScanPowAdj()`；该调用会改变 ULDO，并可能影响 LSI 频率。

## 专用外设规则

DFSDM 规则：

- 将通道、滤波器、常规转换及数据源配置为匹配的流水线。
- 根据所需带宽、分辨率及延迟设置 Sinc 阶数和过采样率。
- 开始连续转换前，为所选滤波器配置 DMA 及 DMA 多路复用值。
- 将 24 位/32 位结果以一致方式缩放或移位后再传递给应用代码。

I3C 规则：

- 启用控制器前配置总线时序、控制器设置、FIFO 阈值及 GPIO AF。
- 将动态地址分配及通用命令码作为总线启动流程的一部分进行建模。
- 将旧式 I2C 器件支持与 DAA、IBI 等原生 I3C 功能分离。
- 检查 CCC 及私有消息的完成和错误标志。

QSPI 规则：

- 外部闪存命令操作码、地址大小、空操作周期及四线使能序列均取决于器件。
- 为每条命令显式配置 1 线、2 线或 4 线指令/地址/数据阶段。
- 使用自动轮询或状态读取安排写使能及写完成顺序。
- 仅在验证闪存协议及缓存/总线约束后进入内存映射模式。

FMC/BUS8 规则：

- 将外部器件视为开发板级依赖项。时序并非仅由 MCU 决定。
- 在元数据中保留建立时间、保持时间、总线周转时间、数据宽度及地址线数量。
- 访问外部器件寄存器时使用 `volatile` 内存映射访问。
- 不得让编译器优化合并或删除具有总线副作用的寄存器访问。

SWPMI 规则：

- 来源方案要求总线复位时，应先配置单线引脚并将其拉低。
- 激活前等待就绪标志。
- 将 ACT、U-frame 及 I-frame 序列视为协议状态，而非简单字节写入。

PWMX 规则：

- 根据 `Fsys / ((div + 1) * cycle_length)` 推导 PWM 频率。
- 显式描述周期模式、占空比范围、极性及通道引脚映射。
- 检查引脚冲突，尤其是 CH56x 来源笔记中 PWM0 与外部复位共用 PB15 的情况。

## 常见陷阱

| 陷阱 | 后果 | 规则 |
|---|---|---|
| LCD 段引脚复用为 GPIO | GPIO 写入无效或 LCD 输出损坏 | 启用 LCD 后保留 LCD COM/SEG 引脚。 |
| LCD 占空比/偏置不匹配 | 显示暗淡或无法辨认 | 使 LCD 控制器占空比/偏置与面板玻璃匹配。 |
| FSMC/FMC RS 地址线错误 | LCD 命令和数据互换或偏移 | 记录外部布线并据此计算寄存器/数据地址。 |
| 帧缓冲区位于不可访问的 RAM 中 | LTDC/GPHA 输出损坏或空白 | 按要求对齐缓冲区并放入控制器可访问的 RAM。 |
| 触摸引脚配置为上拉/输出 | 无触摸响应、触摸不稳定或误触发 | 感测前使用浮空输入。 |
| 未设置 TouchKey 基线/迟滞 | 误触或状态卡死 | 校准基线并使用独立的按下/释放阈值。 |
| KEYSCAN 在睡眠时使用但未调整电源 | 按键唤醒失败 | 执行 `KeyScanPowAdj()` 等系列专用低功耗准备。 |
| 在不同闪存器件间照搬 QSPI 空操作周期 | 读取数据移位或无效 | 以外部闪存数据手册为准。 |
| 盲目照搬外部总线时序 | 间歇性读写失败 | 根据外部器件数据手册及开发板速度推导时序。 |
| GPHA 完成前复用缓冲区 | 图形输出撕裂或损坏 | 等待传输完成标志。 |

## 元数据检查清单

后续 HMI/专用外设模板应显式表示以下字段：

- 外设类别：段式 LCD、LTDC RGB 面板、FSMC/FMC LCD、外部存储器、TouchKey、KEYSCAN、GPHA、DFSDM、I3C、QSPI、SWPMI、PWMX 或 BUS8。
- 目标芯片、封装、引脚映射、复用功能映射及板级冲突。
- 时钟门控、总线域、DMA 通道、DMA 多路复用、中断及低功耗唤醒依赖项。
- 外部器件参数：面板时序、LCD 玻璃 COM/SEG 映射、存储器时序、闪存操作码表、触摸焊盘布局或按键矩阵布局。
- 缓冲区及内存放置：帧缓冲区、DMA 缓冲区、外部存储器基地址、缓存/一致性需求及对齐。
- 校准/调优值：触摸基线、阈值、按键消抖、LCD 扫描时钟、PWM 分频值、DFSDM 过采样率。
- 验证来源：准确的 EVT 示例、RM/DS 章节、外部器件数据手册及开发板原理图。

## 验证状态

- 提取自上列 `Doc/Ref/wch-dev-skill` Markdown 来源。
- 本轮未根据芯片实物、EVT 源码树、RM、DS、开发板原理图、显示/存储/触摸器件数据手册或时序测量进行验证。
- 在对照确切的目标芯片、封装、开发板及官方示例前，应将所有 API 名称、引脚映射、时序值、DMA 多路复用值及功能可用性视为初步信息。

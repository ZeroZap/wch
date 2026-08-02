# WCH IO、音频、CAN 与摄像头接口笔记

本文从 `Doc/Ref/wch-dev-skill` 提取 CAN、I2S、SAI 和 DVP 摄像头相关指导，整理为适用于本仓库的规则，供后续 HAL 元数据、流式传输模板及板级示例索引使用。

最终应以官方 EVT 示例、RM、DS、总线规范、传感器/音频编解码器数据手册、开发板原理图及当前仓库源码为准。

## 来源文件

- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/can_comm.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/can_comm.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/can_comm.md`
- `Doc/Ref/wch-dev-skill/chips/ch32v-general/recipes/i2s_audio.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/i2s_audio.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/i2s_audio.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/sai_audio.md`
- `Doc/Ref/wch-dev-skill/chips/ch56x-ethernet/recipes/dvp_camera.md`
- `Doc/Ref/wch-dev-skill/chips/ch32f-arm/recipes/dvp_camera.md`
- `Doc/Ref/wch-dev-skill/chips/ch32h-highperf/recipes/dvp_camera.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/peripheral_api.md`
- `Doc/Ref/wch-dev-skill/chips/*/resources/example_list.md`
- `CH32V20xEVT/EXAM/SRC/Peripheral/inc/ch32v20x.h`

## 接口覆盖范围

| 接口 | 来源笔记涉及的系列 | 主要约束 |
|---|---|---|
| CAN | CH32V 通用系列、CH32F 风格来源、CH32H417 | 位时序、滤波器组、收发器引脚、终端匹配及总线关闭恢复均由开发板/协议决定。 |
| I2S | CH32V 通用系列、CH32F 风格来源、CH32H417 | 稳定的音频传输需要准确的音频时钟、帧格式、主从角色、DMA 双缓冲及编解码器控制。 |
| SAI | CH32H417 | 多时隙音频成帧、主时钟、FIFO/DMA 及编解码器同步需要显式元数据。 |
| DVP 摄像头 | CH56x、CH32V20x 头文件、CH32F 风格来源、CH32H417 | V20x 当前 EVT 头文件含 DVP 的 8/10/12 位、JPEG、DMA、裁剪、帧率和中断标志定义；传感器时钟/复位/配置总线、同步极性、DMA 缓冲区位置及帧大小仍取决于具体芯片、封装和开发板。 |

## 分类规则

- 将 CAN、I2S、SAI、DVP、USB 音视频及普通 SPI/UART 数据流划分为独立的元数据类别。
- 流式外设需要定义缓冲区所有权、DMA、中断、时序及背压规则；不得将其建模为简单的阻塞式读写调用。
- 必须显式描述 CAN 收发器待机引脚、音频编解码器控制总线、摄像头复位/电源引脚及外部时钟等开发板依赖项。
- 协议层设置位于外设驱动之上：CANopen/J1939、音频文件/编解码器栈及图像处理不属于底层 HAL 的职责。

## CAN 规则

CAN 配置分为 MCU 外设时序和板级物理总线行为两部分。

规则：

- 在元数据中记录 CAN 实例、RX/TX 引脚、重映射、时钟源、IRQ、滤波器组所有权及收发器控制引脚。
- 根据外设时钟和所需比特率推导预分频值、时间段及采样点。
- 调试软件滤波器前，先检查总线终端匹配、收发器电压、待机/静默引脚状态及共地情况。
- 应有针对性地配置接收滤波器；宽松滤波器适合初始调试，但不宜作为最终协议策略。
- 分别处理错误警告、错误被动、总线关闭、仲裁丢失、RX 溢出及 TX 邮箱空事件。
- 明确定义总线关闭恢复策略；自动恢复可能掩盖接线或比特率故障。

## I2S 与 SAI 规则

音频接口需要稳定的时钟和连续的缓冲区。

规则：

- 记录采样率、位深、通道数、帧格式、时钟极性、主从角色、MCLK 使用情况及编解码器要求。
- 使用能满足采样率误差要求的时钟源生成音频时钟；不得盲目复用近似的系统时钟分频值。
- 为连续播放/采集配置 DMA 循环或双缓冲模式。
- DMA 缓冲区应对齐并采用静态存储，其大小需覆盖延迟以及中断/服务抖动。
- 将音频数据传输与编解码器寄存器配置分离，后者可能使用 I2C/SPI/GPIO 控制引脚。
- 对于 SAI，应显式建模时隙数量、时隙大小、帧长度、同步极性、FIFO 阈值及块选择。

## DVP 摄像头规则

DVP 是一种并行摄像头采集接口，其时序取决于具体传感器。

CH32V20x 头文件证据：

- `CH32V20xEVT/EXAM/SRC/Peripheral/inc/ch32v20x.h` 的 DVP 位定义包含 `RB_DVP_D8_MOD`、`RB_DVP_D10_MOD`、`RB_DVP_D12_MOD`、`RB_DVP_JPEG`、`RB_DVP_DMA_EN` 和 `RB_DVP_CROP`。
- 同一段还定义 100%/50%/25% 帧采集率，以及帧开始、行完成、帧完成、FIFO 溢出和帧停止中断/状态位，证明当前 V20x EVT 头文件描述了 DVP 采集能力。
- 当前抽查只确认头文件位定义；该文件未在同处提供可直接引用的 DVP `TypeDef`/实例基址，本轮也未找到 V20x DVP EVT 应用目录。因此生成驱动前仍须按具体 V20x 型号对照 RM/DS、封装引脚和官方示例，不能仅凭公共头文件宣称所有 V20x 型号均可用。

规则：

- 记录数据宽度、像素时钟边沿、HSYNC/VSYNC 极性、JPEG/RGB/YUV 模式、帧尺寸、裁剪/窗口设置及 DMA 缓冲区地址。
- 启用采集前，先配置传感器电源、复位、SCCB/I2C 控制总线及外部时钟。
- 使 DVP 像素格式与传感器输出格式及下游缓冲区解释方式一致。
- 将 DMA 缓冲区放置在 DVP/DMA 引擎可访问的内存中；对 CH56x 系列来源尤其要注意内存区域限制。
- 将帧开始、帧停止、行完成、帧完成及 FIFO 溢出作为不同事件处理。
- 在 JPEG 模式下，将压缩字节计数及缓冲区边界处理与定宽 RGB 帧大小计算分开。

## 常见陷阱

| 陷阱 | 后果 | 规则 |
|---|---|---|
| 在不同的时钟树间照搬 CAN 比特率配置 | 总线无法通信或进入错误被动状态 | 根据外设时钟重新计算时序。 |
| 忽略 CAN 收发器待机引脚 | TX/RX 引脚有跳变，但总线静默 | 对收发器 GPIO 和开发板供电建模。 |
| 音频时钟近似误差过大 | 音调错误、欠载或编解码器锁定失败 | 验证采样率时钟误差。 |
| 音频 DMA 缓冲区位于栈中 | 出现杂音或内存损坏 | 使用对齐的静态/全局缓冲区。 |
| 将编解码器配置混入 I2S 驱动 | 开发板模板无法复用 | 将编解码器控制保留在开发板/设备层。 |
| DVP 同步极性错误 | 帧偏移、空白或撕裂 | 与传感器数据手册及 EVT 设置保持一致。 |
| DVP 缓冲区位于不可访问的内存中 | 采集失败或输出损坏 | 将缓冲区放置在 DMA 可访问的内存中。 |
| 混用 JPEG 和 RGB 帧大小计算 | 缓冲区溢出或图像截断 | 分别跟踪压缩模式和原始模式。 |

## 元数据检查清单

后续 IO/媒体模板应显式表示以下字段：

- 接口类别：CAN、I2S、SAI、DVP 摄像头或相关板级编解码器/传感器配置。
- 目标芯片、实例、时钟源、总线时钟、引脚、重映射/AF、IRQ、DMA 通道及 DMA 可访问的内存区域。
- CAN 字段：比特率、采样点、滤波器、收发器引脚、终端匹配假设、错误处理及恢复策略。
- 音频字段：采样率、位深、通道、帧格式、MCLK、主从角色、DMA 缓冲及编解码器控制总线。
- DVP 字段：传感器型号、控制总线、复位/电源引脚、输入时钟、同步极性、像素格式、帧大小、DMA 缓冲区及溢出策略。
- 验证来源：准确的 EVT 示例、RM/DS 章节、原理图、总线/传感器/编解码器数据手册及实测时序结果。

## 验证状态

- 提取自上列 `Doc/Ref/wch-dev-skill` Markdown 来源。
- 本轮已抽查当前仓库 V20x EVT 公共头文件中的 DVP 位定义，确认其描述的位宽、JPEG、DMA、裁剪、帧率和事件能力；未据此验证具体 V20x 型号和封装的 DVP 实例可用性。
- 尚未根据芯片实物、RM、DS、开发板原理图、CAN/音频/摄像头外部器件数据手册或时序测量验证。除上述头文件定义外，API、位时序、时钟分频、DMA、引脚映射及缓冲区放置规则仍属待核对信息。

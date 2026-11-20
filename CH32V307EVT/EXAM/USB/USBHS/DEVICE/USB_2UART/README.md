# USB_2UART

基于 CH32V307 USBHS 的 USB 转 8 路 UART 工程。工程由 `SimulateCDC` 的 USB CDC
设备代码和 `8_UART_Server` 的 8 路 UART/DMA 引脚方案组合而成。

> 工程目录沿用用户指定名称 `USB_2UART`，实际功能为 USB 转 8 路串口。

## USB 结构

设备枚举为一个复合设备，包含 8 个独立 CDC ACM 虚拟串口：

| USB CDC 端口         | 数据端点   | UART              | TX   | RX   |
| -------------------- | ---------- | ----------------- | ---- | ---- |
| CDC 1 / `USB2UART-A` | EP1 IN/OUT | USART1            | PA9  | PA10 |
| CDC 2 / `USB2UART-B` | EP2 IN/OUT | USART2            | PA2  | PA3  |
| CDC 3 / `USB2UART-C` | EP3 IN/OUT | USART3            | PB10 | PB11 |
| CDC 4 / `USB2UART-D` | EP4 IN/OUT | UART4             | PC10 | PC11 |
| CDC 5 / `USB2UART-E` | EP5 IN/OUT | UART5             | PC12 | PD2  |
| CDC 6 / `USB2UART-F` | EP6 IN/OUT | UART6（全重映射） | PE10 | PE11 |
| CDC 7 / `USB2UART-G` | EP7 IN/OUT | UART7             | PC2  | PC3  |
| CDC 8 / `USB2UART-H` | EP8 IN/OUT | UART8             | PC4  | PC5  |

USB High-Speed Bulk 最大包长为 512 字节，Full-Speed 为 64 字节。由于 USBHS
控制器只有 15 个非零端点，而完整的 8 路 CDC ACM 需要 24 个端点地址，本工程
采用精简 CDC 描述符：每路只使用一对同号 Bulk IN/OUT 端点，不配置 CDC
Notification 端点。数据透传和线路参数控制均已实现，但不同操作系统/驱动对无
Notification 端点的兼容性存在差异，必须在目标系统上验证 8 路是否全部绑定。

每个 CDC 的 IAD、控制接口和数据接口都引用独立字符串描述符
`USB2UART-A`～`USB2UART-H`。Linux 通常可以从接口属性读取这些名称；Windows
是否将其直接显示为 COM 口 FriendlyName 取决于 `usbser` 驱动版本。若 Windows
仍显示“USB Serial Device (COMx)”，可在设备管理器卸载旧设备并勾选删除驱动缓存
后重新插拔；如仍无变化，则需使用按 `MI_00/MI_02/.../MI_0E` 匹配的 INF 文件指定
八路 FriendlyName。

## 串口参数

默认参数为 `115200 8N1`。打开 PC 虚拟串口时，主机下发的 CDC
`SET_LINE_CODING` 会分别修改对应物理 UART 的波特率、停止位和校验方式。
当前支持：

- 数据位：8 位
- 停止位：1、1.5、2 位
- 校验：None、Odd、Even
- 流控：无

## 数据通路

- UART → USB：每路 UART 使用 2048 字节环形 RX DMA；主循环按 USB 最大包长上传。
- USB → UART：每路端口使用 2 个 512 字节接收槽；UART TX 使用 DMA。
- 队列满时对应 USB OUT 端点返回 NAK，UART 发完后自动恢复 ACK。

## 构建

使用 MounRiver Studio 导入当前目录，工程文件为：

```text
USB_2UART.wvproj
```

目标芯片沿用 `SimulateCDC` 的 CH32V307 工程配置。USBHS 使用板载高速 USB PHY。

## SDI 调试日志

工程已参考 `SDI_Printf` 示例，通过编译宏 `SDI_PRINT=1` 将标准 `printf()` 重定向到
CH32V307 的 SDI 调试通道，不初始化 UART，也不占用 8 路桥接串口或额外 GPIO。
当前日志包括：

- 系统启动和系统时钟；
- 8 路 UART 桥接初始化完成；
- USBHS Device 启动；
- USB 配置成功、HS/FS 速度和最大包长；
- USB 断开或总线复位。

查看日志需要 WCH-LinkE，并使用 WCH-LinkUtility 1.8 或更高版本打开 SDI Printf。
SDI 输出沿用官方同步 `_write()` 实现；调试器未持续读取时，执行到 `printf()` 可能
阻塞。因此启用该固件时应保持 SDI 日志窗口连接。量产或脱离调试器运行时，可在
MounRiver Studio 工程 C 编译器的 Defined symbols 中移除 `SDI_PRINT=1`；`main.c`
中的日志宏会自动编译为空，不会回退到 UART1 输出。

SDI 功能不能与芯片读保护同时使用，具体限制以 WCH-LinkUtility 和芯片手册为准。

## 测试

1. 编译并烧录固件。
2. 将板卡 USBHS Device 接口连接电脑。
3. 检查系统是否成功绑定并出现 8 个虚拟 COM 口；若数量不足，请先确认目标
   CDC 驱动是否接受无 Notification 端点的精简描述符。
4. 将任一路 UART 的 TX/RX 与 USB-TTL 交叉连接并共地。
5. 两侧串口工具使用相同参数，分别进行双向发送测试。

Windows 若没有自动绑定系统 `usbser` 驱动，可在设备管理器为各 CDC 接口选择
“USB Serial Device”驱动。要把通用名称改成 `USB2UART-A`～`USB2UART-H`，需安装
`Windows/USB_2UART.inf`；具体签名及安装步骤见 `Windows/README.md`。Linux 可通过
`dmesg` 或 `ls /dev/ttyACM*` 检查枚举结果。

Windows 下可使用 `Tools/usb2uart_ports.py` 按稳定的 `MI_xx` 接口号动态列举、监控
和解析 A～H 对应的 COM 口，无需依赖设备管理器名称，详见 `Tools/README.md`。

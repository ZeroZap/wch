# Windows 端口名称驱动

Windows 自带的 `usbser.sys` 会把多个 CDC ACM 接口统一显示为“USB 串行设备
(COMx)”，通常不会采用固件中的 `iInterface` 或 `iFunction` 字符串。

`USB_2UART.inf` 仍使用系统自带的 `usbser.sys`，只按照复合设备接口号给 8 路
串口设置不同的设备名称：

| 接口硬件 ID 后缀 | 名称 | UART |
| --- | --- | --- |
| `MI_00` | `USB2UART-A` | USART1 |
| `MI_02` | `USB2UART-B` | USART2 |
| `MI_04` | `USB2UART-C` | USART3 |
| `MI_06` | `USB2UART-D` | UART4 |
| `MI_08` | `USB2UART-E` | UART5 |
| `MI_0A` | `USB2UART-F` | UART6 |
| `MI_0C` | `USB2UART-G` | UART7 |
| `MI_0E` | `USB2UART-H` | UART8 |

完整硬件 ID 示例：

```text
USB\VID_1A86&PID_FE18&MI_00
```

## 为什么不能只改 USB 描述符

固件已经为每个 CDC Function 设置了 `USB2UART-A`～`USB2UART-H` 字符串，但
Windows `usbser.sys` 的设备管理器名称通常来自驱动 INF，而不是 CDC 接口字符串。
因此设备端无法强制 Windows 把“USB 串行设备”改成自定义名称。

## 正式安装

64 位 Windows 10/11 默认只接受签名驱动包。正式发布时需要：

1. 使用 Windows Driver Kit 的 `Inf2Cat` 为当前目录生成 `USB_2UART.cat`。
2. 使用受信任的代码签名证书和 `signtool` 对 CAT 文件签名。
3. 将 `USB_2UART.inf` 与签名后的 `USB_2UART.cat` 一起发布。
4. 以管理员终端安装：

   ```powershell
   pnputil /add-driver .\USB_2UART.inf /install
   ```

5. 拔下设备，在设备管理器中卸载原有 8 个“USB 串行设备”；如有“删除此设备的
   驱动程序”选项则勾选，然后重新插入设备。

## 开发测试

开发阶段可以使用测试签名证书签署 CAT，并在测试机器启用测试签名模式。也可以
临时从 Windows 高级启动菜单选择“禁用驱动程序强制签名”，但该方式只适合开发
验证，不适合向用户发布。

未签名 INF 在正常启动的 64 位 Windows 上安装失败属于预期行为。

## 确认接口映射

在设备管理器中打开某个 COM 口：

1. 选择“详细信息”。
2. 属性选择“硬件 ID”。
3. 查看末尾的 `MI_00`、`MI_02`……`MI_0E`。

这组 `MI_xx` 编号由固件接口号固定决定，不依赖 Windows 分配的 COM 号，因此即使
COM 号变化，A～H 与 UART1～UART8 的映射仍保持不变。

## 已经安装但名称没变化

依次检查：

1. 设备硬件 ID 是否为 `VID_1A86&PID_FE18`。
2. 设备“驱动程序详细信息”是否仍只使用系统自动匹配的旧驱动包。
3. `pnputil /enum-drivers` 中是否已经存在自定义 INF。
4. 卸载旧设备后是否重新执行了 `/install`。
5. CAT 签名证书是否被 Windows 信任。

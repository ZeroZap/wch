# USB_2UART 端口发现工具

`usb2uart_ports.py` 根据稳定的 USB 接口号区分 8 个 CDC 串口，不依赖操作系统
分配的端口名或枚举顺序。

支持 Windows 10/11、Linux 和 macOS，要求 Python 3.9 或更高版本，无第三方
Python 依赖。各系统后端如下：

- Windows：通过 PowerShell/CIM 读取 PnP 实例 ID 中的 `MI_xx`。
- Linux：通过 `/sys/class/tty/ttyACM*` 读取 `bInterfaceNumber`。
- macOS：通过系统自带的 `ioreg` 读取 USB 接口号和 `IOCalloutDevice`。

Windows 不需要自定义 INF；Linux 不需要 udev 规则；macOS 不需要安装配置文件。
这些可选配置只能改善显示名称、固定别名或访问权限，不参与 A～H 映射。

## 一次列举

```shell
python usb2uart_ports.py
```

输出示例：

```text
USB_2UART: 8/8 ports connected
NAME        PORT                  UART     USB IF  STATUS
----------  --------------------  -------  ------  ---------
USB2UART-A  COM20                 USART1   MI_00   connected
USB2UART-B  COM21                 USART2   MI_02   connected
...
USB2UART-H  COM27    UART8    MI_0E   connected
```

未连接端口会显示为 `missing`。8 路齐全时退出码为 `0`，不齐时为 `1`，查询错误时
为 `2`。

## 动态监控热插拔

```powershell
python .\usb2uart_ports.py --watch
```

脚本每秒查询一次，只在端口集合变化时输出。可调整轮询间隔：

```powershell
python .\usb2uart_ports.py --watch --interval 0.5
```

## 等待 8 路全部就绪

```powershell
python .\usb2uart_ports.py --wait --timeout 15
```

也可以等待指定数量：

```powershell
python .\usb2uart_ports.py --wait 4
```

## 查询指定逻辑端口

以下选择器等价：

```powershell
python .\usb2uart_ports.py --resolve C
python .\usb2uart_ports.py --resolve USB2UART-C
python .\usb2uart_ports.py --resolve UART3
python .\usb2uart_ports.py --resolve USART3
python .\usb2uart_ports.py --resolve MI_04
```

命令只输出对应的系统串口名，便于脚本调用：Windows 为 `COMx`，Linux 为
`/dev/ttyACM*`，macOS 为 `/dev/cu.*`。

## JSON 输出

```powershell
python .\usb2uart_ports.py --json
python .\usb2uart_ports.py --watch --json
python .\usb2uart_ports.py --resolve A --json
```

## 在后续 8 口 CLI 中复用

```python
from usb2uart_ports import discover_ports, resolve_port, wait_for_ports

ports = wait_for_ports(timeout=15)
port_a = resolve_port("A", ports)
print(port_a.com)       # COM20、/dev/ttyACM0 或 /dev/cu.*
print(port_a.uart)      # USART1
print(port_a.name)      # USB2UART-A
```

若使用 `pyserial`：

```python
import serial
from usb2uart_ports import resolve_port

port = resolve_port("UART3")
with serial.Serial(port.com, 115200, timeout=1) as device:
    device.write(b"hello\r\n")
```

## 系统权限

Linux 用户必须拥有串口设备的读写权限，常见做法是加入 `dialout` 组，或安装适合
产品部署的 udev 规则。macOS 第一次访问设备时可能受系统安全策略影响。这些权限设置
不会改变接口映射。

当前固件采用精简 CDC（无 Notification endpoint），已经在 Windows 验证；Linux 和
macOS 后端具有模拟数据单元测试，但仍需在目标系统上验证固件是否被对应 CDC 驱动
完整枚举。

## 多块板卡限制

当前固件的 USB 序列号固定。脚本会检测同一个 A～H 通道重复并报错，而不会把两块
板卡错误合并。后续若需要同时连接多块设备，应先让每块 MCU 提供唯一 USB 序列号，
再按 PnP 实例中的设备键对端口分组。

## 单元测试

```powershell
python -m unittest -v test_usb2uart_ports.py
```

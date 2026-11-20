# 8_UART_Server 使用与测试说明

本文记录 `CH32V307EVT/EXAM/ETH/8_UART_Server` 示例的用途、默认配置、接线方式和测试验证方法。

相关源码：

- `User/main.c`
- `User/bsp_uart.h`
- `User/bsp_uart.c`
- `User/net_config.h`

## 功能概述

该示例基于 CH32V307 实现 8 路串口与 8 路 TCP socket 的双向透传。

源码中的映射关系：

```text
uart1 <-> socket0
uart2 <-> socket1
...
uart8 <-> socket7
```

数据流：

```text
PC TCP Server -> CH32V307 socketN -> UART(N+1) TX
UART(N+1) RX -> CH32V307 socketN -> PC TCP Server
```

注意：虽然工程名是 `8_UART_Server`，但源码中的网络端实际是 **TCP client 模式**。板子会主动连接电脑端的 TCP server。

## 默认网络配置

默认配置位于 `User/main.c`：

```c
uint8_t IPAddr[4]   = {192, 168, 1, 10};
uint8_t GWIPAddr[4] = {192, 168, 1, 1};
uint8_t IPMask[4]   = {255, 255, 255, 0};
uint8_t DESIP[4]    = {192, 168, 1, 100};
uint16_t desport = 1000;
uint16_t srcport = 1000;
```

| 项目               | 默认值                               |
| ------------------ | ------------------------------------ |
| 板子 IP            | `192.168.1.10`                       |
| 网关               | `192.168.1.1`                        |
| 子网掩码           | `255.255.255.0`                      |
| 电脑/目标服务器 IP | `192.168.1.100`                      |
| 目标 TCP 端口      | `1000`                               |
| 板子源端口         | 从 `1000` 开始递增                   |
| TCP socket 数量    | 8                                    |
| 网络模式           | 板子为 TCP client，电脑为 TCP server |

源码注释说明：由于引脚复用限制，该示例只支持 10M 网络。

```c
note: Due to pin multiplexing, this example only supports 10M networks.
```

## 默认串口配置

串口配置位于 `User/bsp_uart.h`：

```c
#define BAUD_RATE  921600
```

| 项目   | 默认值   |
| ------ | -------- |
| 波特率 | `921600` |
| 数据位 | 8        |
| 停止位 | 1        |
| 校验   | None     |
| 流控   | None     |

如果 USB-TTL、线材或上位机不稳定，可先将 `BAUD_RATE` 改为 `115200` 做基础验证。

## 串口引脚映射

默认引脚位于 `User/bsp_uart.c` 的 `UART_GPIO_Init()` 和 `BSP_Uart_Init()`。

| Socket  | 串口  | TX   | RX   |
| ------- | ----- | ---- | ---- |
| socket0 | UART1 | PA9  | PA10 |
| socket1 | UART2 | PA2  | PA3  |
| socket2 | UART3 | PB10 | PB11 |
| socket3 | UART4 | PC10 | PC11 |
| socket4 | UART5 | PC12 | PD2  |
| socket5 | UART6 | PE10 | PE11 |
| socket6 | UART7 | PC2  | PC3  |
| socket7 | UART8 | PC4  | PC5  |

注意：若定义了 `CH32V307_DEBUG`，UART1 会用于调试打印，源码会跳过 UART1 的透传逻辑。此时建议从 UART2 开始测试。

## 工程与烧录

工程文件：

```text
8_UART_Server.wvproj
```

适用工具：

- MounRiver Studio
- WCH-Link

工程目标芯片配置中显示：

```text
MCU=CH32V307RCT
Target Path=obj\8_UART_Server.hex
```

源码注释提醒：该示例需要 CH32V307 配置为：

```text
RAM:   128KB
Flash: 192KB
```

如 RAM/Flash 分配不正确，可能导致网络库初始化失败、运行异常或内存不足。可使用 WCHISPTool 检查或配置。

## PC 网络准备

建议将电脑网卡手动配置为：

```text
IP:      192.168.1.100
Mask:    255.255.255.0
Gateway: 192.168.1.1
```

连接方式：

```text
PC 网口 <----网线----> CH32V307 ETH
```

或：

```text
PC <----交换机----> CH32V307 ETH
```

如果直连不稳定，建议：

1. 中间加一个支持 10M 的交换机。
2. 将 PC 网卡速率固定为 `10 Mbps Full Duplex` 或 `10 Mbps Half Duplex`。
3. 检查网线、PHY、板卡供电和网络指示灯。

## 电脑端 TCP Server

板子会主动连接：

```text
192.168.1.100:1000
```

因此电脑端必须先启动 TCP server，监听端口 `1000`。

### 工具 1：网络调试助手

推荐使用支持多客户端连接的 TCP 调试工具，例如：

- NetAssist
- SSCOM 网络调试助手
- 野人网络调试助手
- TCP/UDP Debug Tool

配置：

```text
协议: TCP Server
本地 IP: 192.168.1.100
端口: 1000
```

启动监听后复位板子，应能看到来自 `192.168.1.10` 的 TCP 连接。

该示例会创建 8 个 TCP client，均连接到 `192.168.1.100:1000`。因此测试工具最好支持多连接管理。

### 工具 2：Python 多连接 TCP Server

可使用下面脚本测试多连接：

```python
import socket
import threading

HOST = "192.168.1.100"
PORT = 1000

clients = []

def handle_client(conn, addr, idx):
    print(f"[{idx}] connected from {addr}")
    try:
        while True:
            data = conn.recv(4096)
            if not data:
                print(f"[{idx}] disconnected")
                break
            print(f"[{idx}] recv:", data)
            conn.sendall(b"echo:" + data)
    finally:
        conn.close()

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((HOST, PORT))
    s.listen(8)
    print(f"Listening on {HOST}:{PORT}")

    idx = 0
    while True:
        conn, addr = s.accept()
        idx += 1
        clients.append(conn)
        threading.Thread(target=handle_client, args=(conn, addr, idx), daemon=True).start()
```

保存为 `tcp_server.py` 后运行：

```bash
python tcp_server.py
```

### 工具 3：ncat/netcat

简单单连接测试可使用：

```bash
ncat -l 1000
```

但该示例会建立 8 个连接，普通 netcat 通常不适合完整验证 8 路透传。

## Server 如何区分 socket/UART

该示例的 8 个 socket 都连接到同一个 PC server 地址和端口：

```text
192.168.1.100:1000
```

Server 端不应只靠数据内容区分来源，而应靠 TCP 连接本身区分。每条 TCP 连接由四元组唯一标识：

```text
源 IP:源端口 -> 目标 IP:目标端口
```

板子源码中目标端口固定为 `1000`，但每个 socket 使用不同的源端口：

```c
uint16_t srcport = 1000;
...
TmpSocketInf.SourPort = srcport++;
```

因此 PC server 侧通常会看到类似连接：

```text
192.168.1.10:1000 -> 192.168.1.100:1000
192.168.1.10:1001 -> 192.168.1.100:1000
192.168.1.10:1002 -> 192.168.1.100:1000
...
192.168.1.10:1007 -> 192.168.1.100:1000
```

默认对应关系：

| 板子 socket | UART  | 板子源端口 | PC Server 看到的 peer |
| ----------- | ----- | ---------- | --------------------- |
| socket0     | UART1 | `1000`     | `192.168.1.10:1000`   |
| socket1     | UART2 | `1001`     | `192.168.1.10:1001`   |
| socket2     | UART3 | `1002`     | `192.168.1.10:1002`   |
| socket3     | UART4 | `1003`     | `192.168.1.10:1003`   |
| socket4     | UART5 | `1004`     | `192.168.1.10:1004`   |
| socket5     | UART6 | `1005`     | `192.168.1.10:1005`   |
| socket6     | UART7 | `1006`     | `192.168.1.10:1006`   |
| socket7     | UART8 | `1007`     | `192.168.1.10:1007`   |

在 Python server 中，`accept()` 返回的 `addr` 就是对端地址：

```python
conn, addr = s.accept()
```

其中 `addr[0]` 是对端 IP，`addr[1]` 是对端源端口。可按源端口计算 socket 编号：

```python
socket_index = addr[1] - 1000
uart_index = socket_index + 1
```

例如：

```text
addr = ('192.168.1.10', 1001)
```

则表示：

```text
socket1 <-> UART2
```

### 带 socket/UART 标识的 Python Server

下面脚本会根据对端源端口标记数据来自哪个 socket/UART：

```python
import socket
import threading

HOST = "192.168.1.100"
PORT = 1000
BASE_SRC_PORT = 1000

def socket_name_from_addr(addr):
    peer_ip, peer_port = addr
    socket_index = peer_port - BASE_SRC_PORT

    if 0 <= socket_index <= 7:
        return socket_index, socket_index + 1

    return None, None

def handle_client(conn, addr):
    socket_index, uart_num = socket_name_from_addr(addr)

    if socket_index is not None:
        name = f"socket{socket_index}/UART{uart_num}"
    else:
        name = f"unknown:{addr}"

    print(f"[{name}] connected from {addr}")

    try:
        while True:
            data = conn.recv(4096)
            if not data:
                print(f"[{name}] disconnected")
                break

            print(f"[{name}] recv: {data!r}")

            # 回显给同一个 TCP 连接，对应输出到同一个 UART。
            conn.sendall(b"echo from " + name.encode() + b": " + data)
    finally:
        conn.close()

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((HOST, PORT))
    s.listen(8)

    print(f"Listening on {HOST}:{PORT}")

    while True:
        conn, addr = s.accept()
        threading.Thread(
            target=handle_client,
            args=(conn, addr),
            daemon=True,
        ).start()
```

运行后，如果 UART2 发来数据，通常会看到类似输出：

```text
[socket1/UART2] connected from ('192.168.1.10', 1001)
[socket1/UART2] recv: b'hello-uart2'
```

注意：也可以按 TCP 连接建立顺序区分 socket，但网络连接建立顺序不一定绝对可靠，推荐优先按对端源端口区分。

### 源码中的 socket 创建注意点

`WCHNET_CreateTcpSocket()` 当前写法中使用了 `tcp_connect_num`：

```c
WCHNET_SocketCreat(&SocketId[tcp_connect_num], &TmpSocketInf);
WCHNET_SocketConnect(SocketId[tcp_connect_num]);
```

但该函数内部没有看到 `tcp_connect_num++`。而后续代码处理 socket 时直接按 `0~7` 遍历：

```c
for(i = 0; i < SOCKET_NUM; i++)
{
    socketint = WCHNET_GetSocketInt(i);
    ...
}
```

发送时也直接使用：

```c
WCHNET_SocketSend(i, p, &len);
```

因此该例程隐含假设 WCHNET 分配的 socket id 为 `0~7`。如果实测 8 路连接不正常，优先检查这里的 socket id 保存和递增逻辑。

## 串口侧测试工具

可使用：

- SSCOM
- XCOM
- MobaXterm
- PuTTY
- RealTerm
- 逻辑分析仪或示波器

串口参数：

```text
921600 8N1, no flow control
```

以 UART2 为例，接线：

| CH32V307       | USB-TTL |
| -------------- | ------- |
| PA2 / UART2 TX | RX      |
| PA3 / UART2 RX | TX      |
| GND            | GND     |

## 最小验证流程

1. PC 网卡设置为：

   ```text
   192.168.1.100/24
   ```

2. 打开网络调试助手，启动 TCP Server：

   ```text
   IP:   192.168.1.100
   Port: 1000
   ```

3. 编译并烧录 `8_UART_Server`。

4. 复位板子。

5. 使用 `ping` 验证网络连通：

   ```bash
   ping 192.168.1.10
   ```

6. 查看 TCP Server 是否有来自 `192.168.1.10` 的连接。

7. USB-TTL 接 UART2：

   ```text
   PA2 -> USB-TTL RX
   PA3 -> USB-TTL TX
   GND -> GND
   ```

8. 串口工具设置：

   ```text
   921600 8N1
   ```

9. 串口工具发送：

   ```text
   hello-uart2
   ```

   TCP Server 应收到该数据。

10. TCP Server 向对应连接发送：

    ```text
    hello-socket1
    ```

    UART2 应收到该数据。

## 验证点

### 1. Ping 验证

`User/net_config.h` 中默认开启 ping：

```c
#define WCHNET_PING_ENABLE 1
```

可执行：

```bash
ping 192.168.1.10
```

若 ping 不通，优先检查：

- PC IP 是否为 `192.168.1.100`。
- 子网掩码是否为 `255.255.255.0`。
- 网线、交换机和 PHY 链路灯。
- PC 防火墙。
- 是否存在 IP 冲突。
- 网口速率是否为 10M。

### 2. TCP 连接验证

TCP Server 启动后复位板子，应看到板子主动连接。

若无连接：

- 确认 TCP Server 监听的是 `192.168.1.100:1000`。
- 确认 Windows 防火墙允许该程序监听。
- 使用 Wireshark 检查是否有 ARP、TCP SYN 或重传。
- 确认板子 IP 和目标 IP 配置正确。

### 3. 串口到网络验证

串口发送数据后，TCP Server 应收到。

若收不到：

- 检查 USB-TTL TX/RX 是否交叉连接。
- 检查 GND 是否共地。
- 检查波特率是否为 `921600`。
- 尝试将 `BAUD_RATE` 降为 `115200`。
- 确认测试的是正确 socket 对应的串口。

### 4. 网络到串口验证

TCP Server 向 socket 发送数据后，对应 UART 应输出。

若收不到：

- 确认发送给正确连接。
- 检查串口工具是否打开正确 COM 口。
- 检查 TX 引脚和 USB-TTL RX 是否连接正确。
- 检查是否定义了 `CH32V307_DEBUG` 导致 UART1 被占用。

## 常见问题

### 工程名是 Server，为什么电脑要开 TCP Server？

该示例源码中调用了：

```c
WCHNET_SocketConnect(SocketId[tcp_connect_num]);
```

说明板子作为 TCP client 主动连接电脑。因此电脑端必须先启动 TCP Server。

### 为什么只有 10M 网络？

源码注释说明由于引脚复用限制，该示例只支持 10M 网络。实际测试时如果 PC 网卡自动协商失败，可尝试手动固定 PC 网卡速率为 10M。

### 为什么 UART1 没有透传？

如果定义了 `CH32V307_DEBUG`，UART1 会用于调试打印。源码中多处通过：

```c
#ifndef CH32V307_DEBUG
```

排除了 UART1 的透传初始化和 DMA 逻辑。调试模式下建议测试 UART2~UART8。

### 高波特率下丢包怎么办？

默认波特率为 `921600`，对 USB-TTL、线材、PC 驱动和上位机工具要求较高。可先降到 `115200` 验证功能，再逐步提高波特率。

### 8 路连接如何区分？

映射关系固定为：

```text
socket0 <-> UART1
socket1 <-> UART2
socket2 <-> UART3
socket3 <-> UART4
socket4 <-> UART5
socket5 <-> UART6
socket6 <-> UART7
socket7 <-> UART8
```

若 TCP 调试工具显示多个客户端连接，需要选择对应连接发送数据。

## 推荐排查工具

| 工具                         | 用途                         |
| ---------------------------- | ---------------------------- |
| `ping`                       | 验证 IP 层连通性             |
| Wireshark                    | 抓包分析 ARP、TCP 连接和重传 |
| NetAssist/SSCOM 网络调试助手 | TCP Server、多连接收发测试   |
| Python socket server         | 可控多连接测试               |
| 串口调试助手                 | UART 收发验证                |
| 逻辑分析仪/示波器            | 验证 UART 波形、波特率和电平 |

## 修改默认 IP 和端口

如需修改电脑服务器地址或端口，编辑 `User/main.c`：

```c
uint8_t DESIP[4] = {192, 168, 1, 100};
uint16_t desport = 1000;
```

如需修改板子 IP，编辑：

```c
uint8_t IPAddr[4] = {192, 168, 1, 10};
```

如需修改串口波特率，编辑 `User/bsp_uart.h`：

```c
#define BAUD_RATE  921600
```

# IAP_USB_SIGNED — CH32X035 带校验 / 可选加密的 Boot + App 框架

基于 `EXAM/IAP/USB_UART` 改造：**boot 与 app 两端都只走 USB CDC**（不再用 UART），
镜像在启动前做 **SHA-256 + HMAC-SHA256 校验**，传输/文件层面可选 **AES-128-CTR 加密**。

设计范围（刻意保持简单）：

- **单槽，不做版本回滚**，也没有 A/B 分区或试运行计数
- **只做校验**：每次复位都重新校验整片镜像，通过才跳转
- **可选加密**：加密的是 `.img` 文件和 USB 链路，芯片内 flash 是明文（app 原址执行）
- **每个产品一份 boot 二进制**：版本号、产品 ID、USB PID、HMAC/AES key 全部编译期写死
- boot 区**硬性限制 10 KB**，其余（50 KB）全给 app
- 用满 CH32X035 的 flash 特性：1 KB 扇区擦除 + 256 B 页编程

原例程目录未做任何修改，本工程是完整拷贝 + 重写。

```
IAP_USB_SIGNED/
├── shared/img_hdr.h        Flash 布局、镜像头、flag 页、协议命令（唯一真源）
├── boot/                   Bootloader 工程（10 KB 上限）
│   ├── Ld/Link.ld
│   └── User/
│       ├── main.c          上电流程：校验 → 跳转 / 留在升级模式
│       ├── boot_ctrl.c     关外设 + 经软件中断跳转到 App
│       ├── img_install.c   接收 / 解密 / 校验 / 扇区擦除 + 页编程
│       ├── img_flash.c     256 B 页 / 1 KB 扇区擦写封装
│       ├── img_key.c       HMAC / AES 密钥（每产品一份，gen_key.py 生成）
│       ├── boot_info.h     产品 ID / boot 版本 / USB PID（每产品改这里）
│       ├── iap_proto.c     USB 帧协议 + 命令分发
│       ├── usb_cdc.c/.h    CDC 设备层（从 WS2812_Panel 拷贝，已上板验证）
│       ├── usb_desc.c/.h   VID 0x1A86 / PID 来自 boot_info.h
│       ├── sha256.c/.h     SHA-256 + HMAC
│       └── aes.c/.h        AES-128 单块 + CTR
├── app/                    Demo 应用工程（50 KB）
│   ├── Ld/Link.ld          ORIGIN = 0x2800
│   └── User/
│       ├── main.c          CDC + 心跳灯
│       ├── app_usb.c       INFO / PING / UPGRADE
│       ├── app_iap.c       写 UPGRADE flag 并复位
│       └── usb_*           同上，PID 0xFE0E (app)
├── tools/
│   ├── iap_pack.py         打包 + 签名 + 可选加密 + 校验 + 自检
│   ├── mkfactory.py        boot + app + meta -> factory.bin（调试器一次刷完）
│   ├── flash.sh            factory / boot / app 三种烧录入口
│   ├── iap_flash.py        通过 USB CDC 下发镜像（pyserial）
│   ├── gen_key.py          生成随机密钥，写入 img_key.c
│   └── build.sh            命令行编译 boot / app
└── tests/
    ├── test_crypto.c       密码学标准向量
    ├── test_install.c      镜像状态机 + 模拟 flash（含锁定语义）
    ├── stub/               主机侧 flash 模拟
    └── run.sh              全部主机测试（无需硬件）
```

---

## 1. Flash 布局

| 区域         | 偏移          | 大小   | 说明                               |
| ------------ | ------------- | ------ | ---------------------------------- |
| bootloader   | `0x0000_0000` | 10 KB  | 本工程 boot（实测 9840 B，含加密） |
| app slot     | `0x0000_2800` | 50 KB  | 应用镜像，链接地址 `0x2800`        |
| image header | `0x0000_F000` | 256 B  | 128 B 头 + 0xFF 填充               |
| flag page    | `0x0000_F100` | 256 B  | 启动决策记录                       |
| 用户区       | `0x0000_F200` | 1.5 KB | 预留，boot 不碰，app 可直接用      |

> 代码从 `0x0000_0000` 别名执行，Flash 控制器操作使用 `0x0800_0000` 别名，
> 两个别名都定义在 `shared/img_hdr.h` 里。

RAM 统一按 **16 KB** 规划（高 4 KB 可能被 PIOC 占用），栈 2 KB，链接脚本带
`ASSERT(_ebss <= ...)`，溢出直接编译失败。

---

## 2. 镜像格式

```
文件 / 接收流 = [128 B header][img_len 字节 payload]
```

`img_hdr_t`（小端，`__attribute__((packed))`，128 B）：

| 偏移   | 字段                              | 说明                      |
| ------ | --------------------------------- | ------------------------- |
| `0x00` | `magic` = `0x474D4957` (`"WIMG"`) |                           |
| `0x04` | `hdr_ver` = 1                     |                           |
| `0x06` | `flags`                           | bit0 = payload 以密文传输 |
| `0x08` | `load_addr`                       | 必须是 `0x0000_5000`      |
| `0x0C` | `img_len`                         | **明文**长度              |
| `0x10` | `iv[16]`                          | AES-CTR 初始计数器        |
| `0x20` | `img_crc32`                       | 明文 CRC-32（快速比对用） |
| `0x24` | `sha256[32]`                      | 明文摘要                  |
| `0x44` | `hmac[32]`                        | 签名                      |
| `0x64` | `reserved[28]`                    | 必须为 0                  |

**认证范围**（`IMG_HMAC_COVER = 0x20`，全部覆盖**明文**）：

```
hmac   = HMAC-SHA256(hmac_key, header[0x00..0x20) || 明文 payload)
sha256 = SHA-256(明文 payload)
```

前缀正好在 `iv` 之后结束，所以**收第一帧之前**就能把前缀喂给 HMAC，
边收边算，不需要收完再回读一遍；`img_crc32` / `sha256` 是 boot 自己算出来的派生值。

因为签名和摘要都针对明文，boot 每次复位都可以**直接从 flash 重算并校验**
（flash 里就是明文），不需要保留密文。

---

## 3. 安全模型（请先读这段）

| 项               | 做到什么                           | 没做到什么                                                         |
| ---------------- | ---------------------------------- | ------------------------------------------------------------------ |
| SHA-256 + CRC-32 | 检出传输/存储损坏，每次复位校验    | —                                                                  |
| HMAC-SHA256      | 检出篡改、误刷别的固件、换错密钥   | 密钥编在 boot 里，能 dump boot 的人可以伪造（对称 MAC 的固有限制） |
| AES-128-CTR      | `.img` 文件与 USB 链路上看不到明文 | **芯片内 flash 是明文**（app 原址执行，单槽无法存密文）            |

也就是说：**加密保护的是"文件"和"传输"，不是"片内 flash"**。
要做到片内也是密文，需要第二槽或从 RAM 执行，属于本框架明确不做的范围（不做回滚、单槽）。

其它约定：

- IV 每次打包随机（`iap_pack.py` 默认），CTR 复用 IV 等于泄露明文异或值
- `img_key.c` 里是**公开的开发默认密钥**，量产前必须重新生成，且不要提交到公开仓库
  （`python tools/gen_key.py --write`）
- **每产品一份 key**，可在编译期覆盖：`-DIMG_HMAC_KEY/-DIMG_AES_KEY`（ASCII）或
  `-DIMG_KEY_FILE`（文件），见 §6.1
- 主机工具默认解析 `boot/User/img_key.c`；用了 `-D` 覆盖时要给 `iap_pack.py`/
  `iap_flash.py` 传同一套 key（`--key-file` / `--hmac-key` / `--aes-key`）
- 换 key = 换 boot 二进制，走不了 USB 升级通道，必须重烧 boot（见 §6.1）

---

## 4. CH32X035 flash 特性怎么用（手册表 3-11 / 3-12）

| 参数             | 典型          | 最大          |
| ---------------- | ------------- | ------------- |
| 页（256 B）编程  | 1.5 ms        | 2.0 ms        |
| 页（256 B）擦除  | 2.5 ms        | 3.0 ms        |
| 扇区（1 KB）擦除 | 2.7 ms        | 3.3 ms        |
| 擦写寿命         | 300K 次 @25 ℃ | 100K 次 @70 ℃ |
| 数据保存         | 20 年 @25 ℃   | 10 年 @70 ℃   |

据此做的三个决定：

1. **用 1 KB 扇区擦除，而不是 4 次 256 B 页擦除**
   时间基本相同（3.3 ms vs 4×3.0 ms），但寿命消耗只有 1/4。
   `BEGIN` 时一次性擦掉 `ceil(img_len/1KB)` 个扇区，之后只做页编程。
2. **擦除与编程都回读校验**
   擦完读一遍必须全 `0xFF`，写完读一遍必须逐字节相同；对不上就返回
   `IAP_ST_FLASH`，绝不带着坏页跳转。
3. **每帧 ACK 的流控**
   主机每发 56 B 等一个 ACK。boot 的 USB 中断只攒字节，擦写全在主循环，
   一页 1.5 ms 的编程不会卡住端点；也保证命令槽不会被覆盖。

一次完整升级的擦写预算（**满 50 KB 镜像** = 200 扇区 = 200 页）：

| 动作           | 次数        | 最大耗时    | 擦写寿命消耗                                        |
| -------------- | ----------- | ----------- | --------------------------------------------------- |
| 扇区擦除       | 200         | 660 ms      | 200 次                                              |
| 页编程         | 200         | 400 ms      | —                                                   |
| 头页 + flag 页 | 2 擦 + 2 写 | ~10 ms      | 2 次                                                |
| 合计           | —           | **~1.07 s** | 202 次擦除 → 300K/202 ≈ **1485 次满镜像升级** @25 ℃ |

实际 app 远小于满槽，寿命按镜像大小线性折算：

| 镜像大小          | 扇区擦除 | 每次升级的擦除次数 | 25 ℃ 下可升级次数 |
| ----------------- | -------- | ------------------ | ----------------- |
| 5.5 KB（本 demo） | 6        | 8                  | ≈ 37 000          |
| 10 KB             | 10       | 12                 | ≈ 25 000          |
| 20 KB             | 20       | 22                 | ≈ 13 600          |
| 50 KB（满槽）     | 200      | 202                | ≈ 1 485           |

> 只有 `ceil(img_len/1KB)` 个扇区会被擦除，换更小的固件不会额外消耗寿命；
> 头页和 flag 页每次升级各擦一次，所以最小消耗是 2 次/升级。

---

## 5. 实测数据（CH32X035C-R0-1V2 + WCH-LinkE）

`openocd`: `device id = 0x8c80abcd`，`flash size = 62kbytes`。

| 项目             | 结果                                                                        |
| ---------------- | --------------------------------------------------------------------------- |
| boot 尺寸        | text 9792 / data 48 / bss 4040（含 2K 栈），flash 共 **9840 / 10240 B**     |
| app 尺寸         | text 5456 / data 48 / bss 2404，flash 共 5504 / 51200 B                     |
| `INFO`           | 协议 v2：boot 1.0 / product 0x0000 / PID 0xFE0D，load `0x2800`，slot 51200  |
| 明文 5504 B 升级 | DATA ≈ 20.7 kB/s，`END` 16 ms                                               |
| 加密 5504 B 升级 | `BEGIN` 16 ms（6 扇区），DATA ≈ 13.6 kB/s，`END` 16 ms                      |
| 满 40 KB 镜像    | `BEGIN` 156 ms（40 扇区），DATA ≈ 21.5 kB/s（40 KB ≈ 1.9 s），`END` 16 ms   |
| 10K / 50K 分区   | boot 9840 B（占 96%）、app 从 `0x2800` 起、`INFO` 报协议 v2                 |
| 加密镜像落盘     | `.img` payload 是密文；flash `0x2800..` 与 `app.bin` **逐字节相同**（明文） |
| 跳转             | boot 端口 `1A86:FE0D` 消失，app 端口 `1A86:FE0E` 出现，PB0 500 ms 闪烁      |
| app → boot       | `--reboot-to-boot` 写 flag 并复位，重新出现 `FE0D`                          |

> 大数据量升级的时间主要花在"每帧等 ACK"的往返延迟上（56 B/次，≈21 kB/s），
> 不是 flash 速度（满 50 KB 的擦写预算只有约 1.07 s）。要更快就得给 boot 加命令环形
> 队列做窗口化发包，目前刻意保持"一帧一 ACK"，简单且不丢帧。
> 5.5 KB 的 app 全流程不到 1 s，日常使用足够。

### 5.1 boot 体积构成（含加密、签名、CDC 升级）

| 区段                     | 字节             | 说明                                    |
| ------------------------ | ---------------- | --------------------------------------- |
| `.init`（启动跳板）      | 4                |                                         |
| `.vector`（中断向量表）  | 252              |                                         |
| `.text`（代码 + 常量表） | 9536             | 含 AES sbox / SHA K 表                  |
| `.data`（LMA 在 flash）  | 48               |                                         |
| **flash 合计**           | **9840**         | **96.1% of 10 KB**                      |
| `.bss`                   | 1992             | 含 `g_img` 872、`g_pg/g_pt/g_rd` 各 256 |
| `.stack`                 | 2048             |                                         |
| **RAM 合计**             | **4088 / 16384** | **25.0%**                               |

占用最多的符号（gc 之后）：

```
USBFS_IRQHandler   1200   EP0 枚举状态机
usb_cdc_on_packet   402   帧解析
iap_proto_service   384   命令分发
sha256_block        398   + K 表 256
aes128_encrypt      382   + sbox 256
aes128_ctr_xor      218
img_verify_stored   210
img_install_finish  184
memcpy              178   被 sha256/aes/安装器共用
```

### 5.2 为了压进 10 KB 做的事

1. **去掉 `SetSysClock()` 里的 `GPIO_IPD_Unused()`**（−482 B）
   vendor 版本在 `SystemInit` 里把所有 GPIO 设成上拉输入。boot 只用 PC16/PC17，
   引脚状态由 app 重新配置，所以 `boot/User/system_ch32x035.c`（仅 boot 的拷贝）
   省掉了这一步；`app/User` 的拷贝保持 vendor 原样。
2. `-Os -msave-restore -ffunction-sections -fdata-sections` + `--gc-sections`
   已经带走所有没用到的外设库函数（`usb_cdc_log` 等）。
3. 可选 **LTO**：`LTO=1 sh tools/build.sh boot` → 约 9448 B（再省 ~340 B），默认不开。

现在 9840 B，**10 KB 硬上限只剩 400 B**：`boot/Ld/Link.ld` 的 `LENGTH = 10K`
和 `ASSERT(_eitcm <= 0x2800)` 会让超限直接编译失败，不会悄悄压到 app 槽。
想要更多余量就 `LTO=1 sh tools/build.sh boot`（约 9448 B）。

如果哪天还要再腾空间，代价最小的是：`ERASE`/`READ` 调试命令（~0.3 KB）、
`usb_cdc_log`（未链接）、AES（不做加密则 −1.1 KB text、−176 B 密钥调度）、
把 `GPIO_Init` 换成直接写寄存器（~0.4 KB）。

---

## 6. 编译

```sh
cd CH32X035EVT/EXAM/APPLICATION/IAP_USB_SIGNED

sh tools/build.sh boot
sh tools/build.sh app
```

工具链自动查找 MounRiver Studio 的 `riscv-none-embed-gcc`，也可以用
`RISCV_GCC_BIN=/path/to/bin` 指定。产物在 `build/boot/`、`build/app/`。
`LTO=1 sh tools/build.sh boot` 可再省 ~340 B（约 9448 B）。

### 6.1 每个产品一份 boot（版本 / 产品 ID / key / USB PID 全部编译期可配）

不做运行期配 key，产品身份全部编进 boot。**所有可变参数都支持 `-D` 传入**，
一个源树就能出 N 个产品的 boot：

| 宏                                          | 默认                  | 作用                                             |
| ------------------------------------------- | --------------------- | ------------------------------------------------ |
| `BOOT_PRODUCT_ID`                           | `0x0000`              | 产品 ID，`INFO` 回显                             |
| `BOOT_VERSION_MAJOR` / `BOOT_VERSION_MINOR` | `1` / `0`             | boot 版本，`INFO` 回显                           |
| `BOOT_USB_PID`                              | `0xFE0D`              | boot 的 USB PID（多产品插同一台 PC 时用它区分）  |
| `IMG_HMAC_KEY`                              | 开发默认 32 字符      | HMAC-SHA256 密钥，ASCII，必须 32 字符            |
| `IMG_AES_KEY`                               | 开发默认 16 字符      | AES-128 密钥，ASCII，必须 16 字符                |
| `IMG_KEY_FILE`                              | —                     | `#include` 一个密钥文件（随机二进制 key 用这个） |
| `IMG_BOOT_SIZE` / `IMG_SLOT_*` 等           | 见 `shared/img_hdr.h` | 分区布局，改完要同步两个 `Link.ld`               |

前三项在 `boot/User/boot_info.h`（`#ifndef` 包着），key 三项在 `boot/User/img_key.c`。

```sh
# 方式 1：key 写成文件（随机二进制，推荐量产）
python tools/gen_key.py --write -o keys/acme.c
sh tools/build.sh boot -DIMG_KEY_FILE='"../../keys/acme.c"'     -DBOOT_PRODUCT_ID=0x0101 -DBOOT_VERSION_MINOR=3 -DBOOT_USB_PID=0xFE21

# 方式 2：直接内联 ASCII key（CI / 临时验证最方便）
sh tools/build.sh boot     -DIMG_HMAC_KEY='"0123456789abcdef0123456789abcdef"'     -DIMG_AES_KEY='"0123456789abcdef"'     -DBOOT_PRODUCT_ID=0x0101

# 方式 3：什么都不传，用 img_key.c 里的开发默认 key
sh tools/build.sh boot
```

`IMG_HMAC_KEY` / `IMG_AES_KEY` 长度不对会**编译失败**（`img_key.c` 里用
`typedef char ... [(sizeof(...) == LEN+1) ? 1 : -1]` 做静态检查），不会悄悄截断。

> **主机侧必须给同一套 key。** `iap_pack.py` / `iap_flash.py` 默认解析
> `boot/User/img_key.c`，只有"方式 3"对得上；用方式 1/2 时要显式传参：
>
> ```sh
> python tools/iap_pack.py  build/app/app.bin -o build/app.img --key-file keys/acme.c
> python tools/iap_flash.py --port COM61 --image build/app.img --key-file keys/acme.c
> # 或 --hmac-key '32 chars' --aes-key '16 chars'
> ```
>
> 两个工具都会打印 key 指纹（SHA-256 前 16 位），对不上一眼可见：
>
> ```
> keys     : hmac 2f3d6944035b4366 / aes 6c471275adff7426
> ```
>
> key 不匹配时 `BEGIN/DATA` 都会成功，只有 `END` 返回
> `HMAC mismatch (wrong key?)`、state 保持 `NONE` —— 已实测。

`INFO` 会回显 `boot version / product id / usb pid`，产线上用一条命令就能确认板子上
烧的是哪个产品：

```
boot protocol : v2, 56 byte payloads
boot version  : 1.3
product id    : 0x0101
usb pid       : 0xfe21
```

因为 key 在 boot 区，**通过 USB 升级通道无法更换**：换 key 必须重编并重烧 boot，
而重烧 boot 需要调试口（WCH-Link）。也就是说，只要 boot 区不被改写（必要时用选项字节
打开 flash 写保护），普通升级流程只能刷入“被同一把 key 签过”的镜像。

---

## 7. 打包与烧录

### 7.1 打包

```sh
# 明文镜像
python tools/iap_pack.py build/app/app.bin -o build/app.img

# 加密镜像（AES-128-CTR，随机 IV）
python tools/iap_pack.py build/app/app.bin -o build/app_enc.img --encrypt

# 校验已有镜像（结构 / HMAC / SHA-256 / CRC-32）
python tools/iap_pack.py --verify build/app_enc.img

# boot 用 -D 或 IMG_KEY_FILE 换了 key 时，这里要传同一套（否则 END 会报 HMAC 不匹配）
python tools/iap_pack.py build/app/app.bin -o build/app.img --key-file keys/acme.c
python tools/iap_pack.py build/app/app.bin -o build/app.img \
    --hmac-key '0123456789abcdef0123456789abcdef' --aes-key '0123456789abcdef'
```

### 7.2 首次烧录

第一次必须用 WCH-Link 把 **bootloader** 写进去（`0x0000_0000`）：

```sh
OCD=/path/to/OpenOCD/OpenOCD
"$OCD/bin/openocd.exe" -s "$OCD/scripts" -f "$OCD/bin/wch-riscv.cfg" \
    -c "program build/boot/boot.elf verify reset exit"
```

之后 App 通过 USB 安装，不需要再动 WCH-Link：

```sh
python tools/iap_flash.py --port COM61 --info
python tools/iap_flash.py --port COM61 --image build/app_enc.img
```

> **注意**：`openocd program` 会整片擦除，连 `0xF000`/`0xF100` 一起清掉，
> 所以直接用 WCH-Link 写 `app.bin` 到 `0x2800` 是**不能**启动的（没有头和 flag）。
> 这正好是"刷坏了也能救回来"的兜底：boot 会老老实实停在升级模式。

### 7.3 从 App 请求升级

```sh
python tools/iap_flash.py --port COM62 --reboot-to-boot
# 等 FE0E 端口消失、FE0D 端口出现，再用 boot 端口刷
python tools/iap_flash.py --port COM61 --image build/app_enc.img
```

`app_iap_request()` 写 `UPGRADE` flag 到 `0xF100` 再复位，boot 见 flag 即停在升级模式。

---

### 7.4 工厂镜像 `factory.bin`（调试器一次刷完）

WCH 的 OpenOCD flash 驱动在 `program` 之前会**整片擦除**（实测：只写 `0x0` 的 boot，
`0xF000` 的头页和 `0xF100` 的 flag 也会被清掉）。所以调试器侧不存在"只刷 app"，
要刷就是一整片：

```sh
python tools/mkfactory.py --boot build/boot/boot.bin \
                          --app build/app.img -o build/factory.bin
sh tools/flash.sh factory build/factory.bin
```

`factory.bin` 是 61952 B 的稠密镜像（`0x0000..0xF1FF`，空档填 `0xFF`）：

| 偏移     | 内容                                   |
| -------- | -------------------------------------- |
| `0x0000` | boot.bin（不足 10 KB 补 `0xFF`）       |
| `0x2800` | app 明文 payload（加密镜像会先解出来） |
| `0xF000` | 128 B 镜像头 + `0xFF` 填充             |
| `0xF100` | flag 页，`state = VALID`               |

同时输出 `<stem>_payload.bin` 和 `<stem>_meta.bin`（512 B），给需要分区写入的烧录器。
`--hex` 可额外生成 Intel HEX。刷完直接是"能启动"的状态，不需要再走 USB。

三种烧录方式一览：

```sh
sh tools/flash.sh factory build/factory.bin        # 调试器：整片（生产/换机）
sh tools/flash.sh boot    build/boot/boot.bin      # 调试器：只刷 boot，之后靠 USB 装 app
python tools/iap_flash.py --port COM61 --image build/app.img   # USB：只换应用
```

---

## 8. 启动流程

```
复位
 ├─ 读 flag 页（只用于判断"是否被要求升级"）
 ├─ 校验 header + 整片 SHA-256 / CRC-32 / HMAC（按 256 B 页流式，直接读 flash）
 │
 ├─ 校验通过 && flag != UPGRADE
 │    ├─ 开 USB，等主机 BOOT_STAY_WINDOW_MS（默认 1200 ms，可宏改 0）
 │    ├─ 主机已发过 JUMP 或正在传输 → 留在 boot
 │    └─ 否则 → 经软件中断跳到 0x2800
 │
 └─ 其它情况（无镜像 / 头损坏 / 校验失败 / UPGRADE）
      └─ 留在 boot，USB CDC 提供升级协议
```

两点说明：

- **不是回滚**。只有一个镜像槽，校验失败就是"没有可启动的 app"，不会退到旧版本，
  也不存在"启动失败自动重试"的逻辑。
- 1.2 s 窗口只是为了"镜像合法但 app 一启动就崩"时还有机会重刷。
  量产想省掉这点延迟：`sh tools/build.sh boot -DBOOT_STAY_WINDOW_MS=0`，
  此时只能靠 app 自己（或 WCH-Link）回到 boot。

### 跳转为什么必须走软件中断

直接把 `0x2800` 当函数指针调用会在 CH32X035 上翻车：app 的 `_start` 里那句
`csrw 0xbc0, t0`（配置流水线/分支预测）会被判成**非法指令**（`mcause=2`,
`mtval=0xbc029073`），于是卡在 boot 的默认 `j .` 处理里。

所以和厂商 IAP 例程一样，跳转从 `SW_Handler` 里发：

```c
void SW_Handler(void) { __asm("li a6, 0x2800"); __asm("jr a6"); }
...
NVIC_EnableIRQ(Software_IRQn);
NVIC_SetPendingIRQ(Software_IRQn);
__enable_irq();
```

`boot/User/boot_ctrl.c` 里有编译期检查，防止 app 起始地址改了这里没跟着改。

---

## 9. USB 协议

帧格式（双向一致，与 `WS2812_Panel` 的 `wsp_usb` 相同）：

```
A5 5A | CMD | LEN_L LEN_H | PAYLOAD... | CRC8
CRC8: poly 0x07, init 0x00, 覆盖 CMD..PAYLOAD（不含同步字节）
回复: CMD | 0x80，单帧 ≤ 64 B，payload ≤ 56 B
```

| CMD          | 方向 | 请求                                      | 回复                                                                                                                                      |
| ------------ | ---- | ----------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------- |
| `0x01` INFO  | →    | —                                         | ver(v2), max_payload, state, boot_major, product_id(4), boot_minor, flag, usb_pid(2), load_addr(4), slot_size(4), hdr_off(4), flag_off(4) |
| `0x10` BEGIN | →    | `img_len(4) flags(2) load_addr(4) iv(16)` | status（含扇区擦除）                                                                                                                      |
| `0x11` DATA  | →    | ≤ 56 B（密文或明文）                      | status（含页编程）                                                                                                                        |
| `0x12` END   | →    | `hmac(32)`                                | status（写头页 + flag 页）                                                                                                                |
| `0x13` ABORT | →    | —                                         | status                                                                                                                                    |
| `0x14` JUMP  | →    | —                                         | status（先整片校验，通过才跳）                                                                                                            |
| `0x15` ERASE | →    | —                                         | status（擦整个 app slot + 头 + flag）                                                                                                     |
| `0x16` READ  | →    | `addr(4) len(2)`                          | 原始 flash 数据                                                                                                                           |

status：`0` OK、`1` CRC、`2` SHA-256、`3` HMAC、`4` 长度/状态、`5` flash 失败、`6` 无有效 App。

App 侧：`0x01` INFO、`0x02` PING（原样回显）、`0x20` UPGRADE。

实现细节：

- 解析在 USB 中断里只做攒字节（`rx_buf`），提交时再拷到 `cmd_buf`，
  避免擦写期间下一帧把正在用 `cmd_buf` 的 payload 覆盖掉
- 回复若遇到端点忙，最多重试 100 ms，不静默丢帧

---

## 10. 主机测试（无需硬件）

```sh
sh tests/run.sh
```

1. `test_crypto` — SHA-256 / HMAC / AES-128-CTR 标准向量（FIPS 180-4、RFC 4231、SP 800-38A）
2. `test_install` — 真实 `img_install.c` + 模拟 flash（**模拟了 flash 锁定语义**，
   漏掉 unlock 会直接失败）跑完整状态机：奇偶分片、明文/加密、篡改一字节、
   错 HMAC、截断、abort、abort 后重装
3. `iap_pack.py --selfcheck` — AES 向量 + 头文件常量与 `img_hdr.h` 逐项比对
4. **C 与 Python 交叉校验** — C 安装器写出的 flash 内容与 Python 打包结果逐字节相同
   （明文、加密两组）

> 这个套件在上板前抓到过一个真实 bug：`img_install_begin()` 擦完头页后上了锁，
> 后续页写没有重新解锁，导致第 2 页开始报 `flash program/erase failed`。
> 现在 stub 会拒绝带锁的擦写，回归测试覆盖了它。

---

## 11. 上板验证结果

全部已在本机跑通（CH32X035C-R0-1V2 + WCH-LinkE）：

- [x] WCH-Link 写 boot（10 KB 上限内 9840 B），端口出现 `1A86:FE0D`
- [x] `--info` 返回协议 v2 / boot 1.0 / product 0x0000 / slot 51200
- [x] 明文 5504 B 安装 → `BEGIN/DATA/END` 全 OK → 跳转 → `1A86:FE0E`
- [x] 加密 5504 B 安装 → 同上（`BEGIN` 16 ms，`END` 16 ms）
- [x] `--read 0xF000:128` 读回头部，与打包结果逐字节一致
- [x] dump `0x2800..` 与 `app.bin` 逐字节一致（加密镜像也是明文落盘）
- [x] 跳转目标 0x2800 生效（app 向量表在 0x2804）
- [x] 满 40 KB 镜像安装（`--no-jump`），`BEGIN` 156 ms
- [x] `--reboot-to-boot` 让 app 回到 boot，flag 机制有效
- [x] `-DIMG_HMAC_KEY/IMG_AES_KEY` 编出来的 boot：用匹配 key 打包 → 安装成功、跳转正常
- [x] 同一个 boot 用**不匹配** key 打包 → `END` 返回 `HMAC mismatch (wrong key?)`，state 保持 NONE
- [x] `-DIMG_KEY_FILE`（`gen_key.py` 生成的二进制 key 文件）编出来的 boot 正常工作
- [x] 反复升级、加密/明文混用、ABORT

未做：实测启动校验耗时（需要 GPIO 打点或调试器计时），
以及 `BOOT_STAY_WINDOW_MS=0` 的快速启动形态。

---

## 12. 已知取舍 / 后续可做

| 事项         | 现状                         | 说明                                                        |
| ------------ | ---------------------------- | ----------------------------------------------------------- |
| 版本回滚     | **不做**                     | 单槽，校验失败就停在 boot                                   |
| 片内加密     | 不支持                       | app 原址执行，单槽无法存密文；要做得加第二槽                |
| 签名算法     | HMAC-SHA256（对称）          | 需要防"能 dump boot 的人伪造"时应换 Ed25519（头需扩 64 B）  |
| 升级速度     | 56 B/帧 + 逐帧 ACK           | boot 加命令环形队列即可窗口化发包，满 50 KB 可压到 <1 s     |
| 外部 NOR     | 未接入                       | `WS2812_Panel/User/norflash.c` 可直接搬来做镜像缓存         |
| 密钥保护     | 编译期常量，每产品一份 boot  | 换 key 必须重烧 boot；要更强可做 OTP / 外部加密芯片         |
| 用户区       | `0xF200..0xF7FF` 1.5 KB 空闲 | 需要 SN / 校准时可给 app 用，boot 不碰                      |
| 运行期配 key | 不支持（刻意的）             | 若将来要"同一 boot 多产品"，可加 key 记录页，但会削弱抗篡改 |

---

## 13. 作为通用 CH32 + USB bootloader 基线

这个工程刻意做成"够用就好"的一层地基：**10 KB 以内、单槽、只做校验、加密可选**，
不引入任何回滚 / 双槽 / 运行期配置的复杂度。可以直接拿去当 CH32 系列带 USB 的
常规 bootloader 用；需要更强的能力时，在上面加，而不是推倒重来。

### 13.1 已经可以直接复用的部分

| 能力            | 位置                                 | 说明                                                   |
| --------------- | ------------------------------------ | ------------------------------------------------------ |
| USB CDC 设备层  | `boot/User/usb_cdc.c`（app 侧同款）  | EP0 标准/类请求、EP2 OUT、EP3 IN，免驱（`usbser.sys`） |
| 帧协议 + CRC8   | `iap_proto.c` / `app_usb.c`          | 中断只攒字节、主循环执行命令，单命令槽 + 逐帧 ACK      |
| Flash 封装      | `img_flash.c`                        | 256 B 页擦/编程、1 KB 扇区擦，全部回读校验             |
| 密码学          | `sha256.c` / `aes.c`                 | 流式 SHA-256、HMAC-SHA256、AES-128-CTR（无外部依赖）   |
| 镜像校验 + 跳转 | `img_install.c` / `boot_ctrl.c`      | 整片复验、软件中断跳转（绕开 `csrw 0xbc0` 的坑）       |
| 主机工具        | `tools/iap_pack.py` / `iap_flash.py` | 打包/签名/加密/校验/下发，纯 Python + pyserial         |
| 免硬件测试      | `tests/run.sh`                       | 模拟 flash（含锁定语义）+ C/Python 交叉校验            |
| 集成示例        | `EXAM/APPLICATION/WS2812_Panel_IAP`   | 灯带 App 接上 boot 的完整例子：`0x2800` 链接 + `0x20 UPGRADE` |

### 13.2 换到别的 CH32 型号要改哪里

| 改点           | 文件                                           | 说明                                                                                 |
| -------------- | ---------------------------------------------- | ------------------------------------------------------------------------------------ |
| Flash 布局常量 | `shared/img_hdr.h`                             | `IMG_BOOT_SIZE` / `IMG_SLOT_OFFSET` / `IMG_SLOT_SIZE` / `IMG_FLASH_END` 按型号容量改 |
| 扇区大小       | `img_flash_erase_sector()` + `IMG_SECTOR_SIZE` | CH32V003 之类页更小，扇区语义不同要重写这一层                                        |
| 链接地址       | `boot/Ld/Link.ld`、`app/Ld/Link.ld`            | `ORIGIN` / `LENGTH` / RAM 大小（用 PIOC 的型号要扣 4 KB）                            |
| 跳转地址       | `boot_ctrl.c`                                  | `li a6, 0xXXXX` + 同名 `#error` 检查一起改                                           |
| 产品身份       | `boot/User/boot_info.h`                        | 产品 ID、boot 版本、USB PID                                                          |
| 密钥           | `boot/User/img_key.c`                          | `python tools/gen_key.py --write`                                                    |
| USB 外设       | `usb_cdc.c` / `usb_desc.c`                     | USBFS 寄存器在各型号上基本一致；换成 USBD 外设才需要替换设备层                       |

启动时钟、向量表、`system_ch32x035.c` 都是 WCH 标准模板，换型号时从对应 SDK 拷一份即可。

### 13.3 更高要求的演进路线（按需取用，不预埋在基线里）

| 级别           | 目标                       | 做法                                                        | 影响的文件                                            |
| -------------- | -------------------------- | ----------------------------------------------------------- | ----------------------------------------------------- |
| **L1（当前）** | 单槽 + 校验 + 可选加密     | —                                                           | —                                                     |
| **L2**         | 防伪造（不依赖芯片内密钥） | Ed25519/ECDSA 公钥验签，签名 64 B 放进头部扩展区            | `img_hdr.h`（hdr v3）、`ed25519.c`、`iap_pack.py`     |
| **L3**         | 可回滚 / 断点续传          | 双槽 A/B + 版本号 + 试运行确认位（flag 页扩成记录表）       | `img_hdr.h`、`img_install.c`、`main.c`                |
| **L4**         | 同一 boot 多产品           | key 记录页 + KEK 包裹（见 §12 的取舍）                      | 新增 `img_key_page.c`、`iap_proto.c`                  |
| **L5**         | 大镜像 / 慢链路            | 外部 SPI NOR 缓存 + LZ4 压缩 + 分块校验                     | `norflash.c`（可从 WS2812_Panel 搬）、`img_install.c` |
| **L6**         | 产线                       | OpenOCD 一键烧 boot + 注入密钥 + 写 SN 到用户区（`0xF200`） | `tools/` 新增脚本                                     |

### 13.4 明确不适合的场景

- **防物理攻击 / 侧信道**：密钥在片上 flash，能拆片就能读
- **无回滚**：单槽，校验失败就停在 boot（这是本基线的设计前提，不是 bug）
- **片内加密**：app 原址执行，flash 里必然是明文（加密只保护 `.img` 文件与 USB 链路）
- **大镜像 / 慢链路**：满 50 KB 上传约 2.4 s，且没有压缩

需要以上任意一条，就按 §13.3 选一级往上加；基线本身不用改结构。

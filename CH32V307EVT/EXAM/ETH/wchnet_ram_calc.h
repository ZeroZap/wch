/* =============================================================
 * wchnet_ram_calc.h
 * 自动计算 WCHNET 协议栈在当前 net_config.h 配置下所需的全部 RAM
 * 用法：在包含 <wchnet.h> 与 "net_config.h" 之后 #include 本文件
 *
 * 提供：
 *   WCHNET_TOTAL_RAM_SIZE   总字节数（库 + 应用接收缓冲）
 *   WCHNET_LIB_RAM_SIZE     仅库内部 RAM（不含用户 SocketRecvBuf）
 *
 * 示例：
 *   printf("Lib RAM  = %u B\n", WCHNET_LIB_RAM_SIZE);
 *   printf("Total    = %u B (%.1f KB)\n",
 *          WCHNET_TOTAL_RAM_SIZE,
 *          WCHNET_TOTAL_RAM_SIZE / 1024.0);
 * ============================================================= */
#ifndef __WCHNET_RAM_CALC_H__
#define __WCHNET_RAM_CALC_H__

#include "wchnet.h"     /* SOCK_INF、WCHNET_MEMP_SIZE 等 */
#include "net_config.h" /* 用户当前配置                  */

#ifdef __cplusplus
extern "C" {
#endif

/* DMA 描述符 4 × uint32_t = 16 字节 (ch32v30x_eth.h: ETH_DMADESCTypeDef) */
#define WCHNET_SIZE_ETH_DMADESC        (4u * 4u)

/* ---- 库内部 RAM（位于 NetLib/eth_driver_*.c 的全局数组） ---- */
#define WCHNET_RAM_DMA_TX_DESC         ((uint32_t)WCHNET_SIZE_ETH_DMADESC * (uint32_t)ETH_TXBUFNB)
#define WCHNET_RAM_DMA_RX_DESC         ((uint32_t)WCHNET_SIZE_ETH_DMADESC * (uint32_t)ETH_RXBUFNB)

#define WCHNET_RAM_MAC_TX_BUF          ((uint32_t)ETH_TXBUFNB * (uint32_t)ETH_TX_BUF_SZE)
#define WCHNET_RAM_MAC_RX_BUF          ((uint32_t)ETH_RXBUFNB * (uint32_t)ETH_RX_BUF_SZE)

#define WCHNET_RAM_SOCKET_INF          ((uint32_t)sizeof(SOCK_INF) * (uint32_t)WCHNET_MAX_SOCKET_NUM)

/* 这三个宏在 wchnet.h 里已经定义好，直接复用 */
#define WCHNET_RAM_MEMP                ((uint32_t)WCHNET_MEMP_SIZE)
#define WCHNET_RAM_HEAP                ((uint32_t)WCHNET_RAM_HEAP_SIZE)
#define WCHNET_RAM_ARP_TABLE           ((uint32_t)WCHNET_RAM_ARP_TABLE_SIZE)

/* 库内全局 RAM 合计（不含用户在 main.c 里声明的 SocketRecvBuf[]） */
#define WCHNET_LIB_RAM_SIZE            (WCHNET_RAM_DMA_TX_DESC +         \
                                        WCHNET_RAM_DMA_RX_DESC +         \
                                        WCHNET_RAM_MAC_TX_BUF +          \
                                        WCHNET_RAM_MAC_RX_BUF +          \
                                        WCHNET_RAM_SOCKET_INF +          \
                                        WCHNET_RAM_MEMP +                \
                                        WCHNET_RAM_HEAP +                \
                                        WCHNET_RAM_ARP_TABLE)

/* ---- 应用层 RAM（用户在 main.c 里声明的接收缓冲） ----
 * 注：SocketRecvBuf[N][RECE_BUF_LEN] 每个 socket 一份，
 *     你可以根据实际用到的 socket 数自定义，见下文 WCHNET_RAM_APP_RECVBUF_USED。 */
#define WCHNET_RAM_APP_RECVBUF_PER_SOCK ((uint32_t)RECE_BUF_LEN)
#define WCHNET_RAM_APP_RECVBUF_MAX     ((uint32_t)RECE_BUF_LEN * (uint32_t)WCHNET_MAX_SOCKET_NUM)

/* 如果你只为某些 socket 分配独立缓冲，改这里为实际数量 */
#ifndef WCHNET_APP_ACTIVE_SOCKETS
#define WCHNET_APP_ACTIVE_SOCKETS       (WCHNET_MAX_SOCKET_NUM)
#endif
#define WCHNET_RAM_APP_RECVBUF_USED    ((uint32_t)RECE_BUF_LEN * (uint32_t)WCHNET_APP_ACTIVE_SOCKETS)

/* ---- 总计 ---- */
#define WCHNET_TOTAL_RAM_SIZE          ((uint32_t)WCHNET_LIB_RAM_SIZE + (uint32_t)WCHNET_RAM_APP_RECVBUF_USED)

/* ---- 断言：超出 SRAM 时编译失败 ----
 * CH32V307 主 SRAM 起始 0x20000000，大小 0x20000 (128 KB)。
 * 默认留 32 KB 给应用+栈+堆，所以阈值设为 96 KB。
 * 如果你的项目占用更多，请调小此值或改到外部 RAM。 */
#ifndef WCHNET_RAM_BUDGET_BYTES
#define WCHNET_RAM_BUDGET_BYTES        (96u * 1024u)
#endif

#if (WCHNET_TOTAL_RAM_SIZE > WCHNET_RAM_BUDGET_BYTES)
    #error "WCHNET RAM exceeds budget, please decrease WCHNET_NUM_TCP / MSS / RECE_BUF_LEN"
#endif

#ifdef __cplusplus
}
#endif

#endif /* __WCHNET_RAM_CALC_H__ */

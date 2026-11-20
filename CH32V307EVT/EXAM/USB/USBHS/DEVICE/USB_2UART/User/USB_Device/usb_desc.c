/********************************** (C) COPYRIGHT *******************************
 * File Name          : usb_desc.c
 * Description        : Composite USB device with eight CDC ACM functions.
 *******************************************************************************/
#include "usb_desc.h"

#define WBVAL(x)                    ((uint8_t)(x)), ((uint8_t)((x) >> 8))
#define CDC_FUNCTION(ctrl, data, ep, packet, string)                              \
    0x08, 0x0B, (ctrl), 0x02, 0x02, 0x02, 0x01, (string),                        \
    0x09, 0x04, (ctrl), 0x00, 0x00, 0x02, 0x02, 0x01, (string),                  \
    0x05, 0x24, 0x00, 0x10, 0x01,                                                 \
    0x05, 0x24, 0x01, 0x00, (data),                                                \
    0x04, 0x24, 0x02, 0x02,                                                       \
    0x05, 0x24, 0x06, (ctrl), (data),                                              \
    0x09, 0x04, (data), 0x00, 0x02, 0x0A, 0x00, 0x00, (string),                  \
    0x07, 0x05, (ep), 0x02, WBVAL(packet), 0x00,                                  \
    0x07, 0x05, (0x80 | (ep)), 0x02, WBVAL(packet), 0x00

const uint8_t MyDevDescr[] =
{
    0x12, 0x01, WBVAL(0x0200),
    0xEF, 0x02, 0x01,
    DEF_USBD_UEP0_SIZE,
    WBVAL(DEF_USB_VID), WBVAL(DEF_USB_PID), WBVAL(0x0101),
    0x01, 0x02, 0x03, 0x01
};

const uint8_t MyCfgDescr_FS[] =
{
    0x09, 0x02, WBVAL(481), 0x10, 0x01, 0x00, 0x80, 0x32,
    CDC_FUNCTION(0,  1, 1, DEF_USBD_FS_PACK_SIZE, 4),
    CDC_FUNCTION(2,  3, 2, DEF_USBD_FS_PACK_SIZE, 5),
    CDC_FUNCTION(4,  5, 3, DEF_USBD_FS_PACK_SIZE, 6),
    CDC_FUNCTION(6,  7, 4, DEF_USBD_FS_PACK_SIZE, 7),
    CDC_FUNCTION(8,  9, 5, DEF_USBD_FS_PACK_SIZE, 8),
    CDC_FUNCTION(10, 11, 6, DEF_USBD_FS_PACK_SIZE, 9),
    CDC_FUNCTION(12, 13, 7, DEF_USBD_FS_PACK_SIZE, 10),
    CDC_FUNCTION(14, 15, 8, DEF_USBD_FS_PACK_SIZE, 11)
};

const uint8_t MyCfgDescr_HS[] =
{
    0x09, 0x02, WBVAL(481), 0x10, 0x01, 0x00, 0x80, 0x32,
    CDC_FUNCTION(0,  1, 1, DEF_USBD_HS_PACK_SIZE, 4),
    CDC_FUNCTION(2,  3, 2, DEF_USBD_HS_PACK_SIZE, 5),
    CDC_FUNCTION(4,  5, 3, DEF_USBD_HS_PACK_SIZE, 6),
    CDC_FUNCTION(6,  7, 4, DEF_USBD_HS_PACK_SIZE, 7),
    CDC_FUNCTION(8,  9, 5, DEF_USBD_HS_PACK_SIZE, 8),
    CDC_FUNCTION(10, 11, 6, DEF_USBD_HS_PACK_SIZE, 9),
    CDC_FUNCTION(12, 13, 7, DEF_USBD_HS_PACK_SIZE, 10),
    CDC_FUNCTION(14, 15, 8, DEF_USBD_HS_PACK_SIZE, 11)
};

const uint8_t MyLangDescr[] = {0x04, 0x03, 0x09, 0x04};
const uint8_t MyManuInfo[] =
{
    0x0E, 0x03, 'w', 0, 'c', 0, 'h', 0, '.', 0, 'c', 0, 'n', 0
};
const uint8_t MyProdInfo[] =
{
    0x22, 0x03, 'U', 0, 'S', 0, 'B', 0, ' ', 0, 't', 0, 'o', 0, ' ', 0,
    '8', 0, ' ', 0, 'U', 0, 'A', 0, 'R', 0, 'T', 0, ' ', 0, 'B', 0, 'r', 0
};
const uint8_t MySerNumInfo[] =
{
    0x12, 0x03, 'U', 0, 'S', 0, 'B', 0, '2', 0, 'U', 0, 'A', 0, 'R', 0, 'T', 0
};
const uint8_t MyPortInfo[8][22] =
{
    {0x16, 0x03, 'U', 0, 'S', 0, 'B', 0, '2', 0, 'U', 0, 'A', 0, 'R', 0, 'T', 0, '-', 0, 'A', 0},
    {0x16, 0x03, 'U', 0, 'S', 0, 'B', 0, '2', 0, 'U', 0, 'A', 0, 'R', 0, 'T', 0, '-', 0, 'B', 0},
    {0x16, 0x03, 'U', 0, 'S', 0, 'B', 0, '2', 0, 'U', 0, 'A', 0, 'R', 0, 'T', 0, '-', 0, 'C', 0},
    {0x16, 0x03, 'U', 0, 'S', 0, 'B', 0, '2', 0, 'U', 0, 'A', 0, 'R', 0, 'T', 0, '-', 0, 'D', 0},
    {0x16, 0x03, 'U', 0, 'S', 0, 'B', 0, '2', 0, 'U', 0, 'A', 0, 'R', 0, 'T', 0, '-', 0, 'E', 0},
    {0x16, 0x03, 'U', 0, 'S', 0, 'B', 0, '2', 0, 'U', 0, 'A', 0, 'R', 0, 'T', 0, '-', 0, 'F', 0},
    {0x16, 0x03, 'U', 0, 'S', 0, 'B', 0, '2', 0, 'U', 0, 'A', 0, 'R', 0, 'T', 0, '-', 0, 'G', 0},
    {0x16, 0x03, 'U', 0, 'S', 0, 'B', 0, '2', 0, 'U', 0, 'A', 0, 'R', 0, 'T', 0, '-', 0, 'H', 0}
};

const uint8_t MyQuaDesc[] =
{
    0x0A, 0x06, WBVAL(0x0200), 0xEF, 0x02, 0x01, DEF_USBD_UEP0_SIZE, 0x01, 0x00
};

uint8_t TAB_USB_FS_OSC_DESC[sizeof(MyCfgDescr_HS)] = {0x09, 0x07};
uint8_t TAB_USB_HS_OSC_DESC[sizeof(MyCfgDescr_FS)] = {0x09, 0x07};

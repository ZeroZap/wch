/********************************** (C) COPYRIGHT *******************************
 * File Name          : usb_desc.h
 * Description        : USB descriptors for eight CDC ACM ports.
 *******************************************************************************/
#ifndef USER_USB_DESC_H_
#define USER_USB_DESC_H_

#include "debug.h"

#define DEF_USB_VID                  0x1A86
#define DEF_USB_PID                  0xFE18
#define DEF_USBD_UEP0_SIZE           64
#define DEF_USBD_HS_PACK_SIZE        512
#define DEF_USBD_FS_PACK_SIZE        64

#define DEF_USBD_DEVICE_DESC_LEN     ((uint8_t)MyDevDescr[0])
#define DEF_USBD_CONFIG_FS_DESC_LEN  ((uint16_t)MyCfgDescr_FS[2] | ((uint16_t)MyCfgDescr_FS[3] << 8))
#define DEF_USBD_CONFIG_HS_DESC_LEN  ((uint16_t)MyCfgDescr_HS[2] | ((uint16_t)MyCfgDescr_HS[3] << 8))
#define DEF_USBD_LANG_DESC_LEN       ((uint16_t)MyLangDescr[0])
#define DEF_USBD_MANU_DESC_LEN       ((uint16_t)MyManuInfo[0])
#define DEF_USBD_PROD_DESC_LEN       ((uint16_t)MyProdInfo[0])
#define DEF_USBD_SN_DESC_LEN         ((uint16_t)MySerNumInfo[0])
#define DEF_USBD_QUALFY_DESC_LEN     ((uint16_t)MyQuaDesc[0])

extern const uint8_t MyDevDescr[];
extern const uint8_t MyCfgDescr_FS[];
extern const uint8_t MyCfgDescr_HS[];
extern const uint8_t MyLangDescr[];
extern const uint8_t MyManuInfo[];
extern const uint8_t MyProdInfo[];
extern const uint8_t MySerNumInfo[];
extern const uint8_t MyPortInfo[8][22];
extern const uint8_t MyQuaDesc[];
extern uint8_t TAB_USB_FS_OSC_DESC[];
extern uint8_t TAB_USB_HS_OSC_DESC[];

#endif

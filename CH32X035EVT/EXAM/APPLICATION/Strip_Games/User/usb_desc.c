/********************************** (C) COPYRIGHT *******************************
 * File Name          : usb_desc.c
 * Description        : USB CDC descriptors (see usb_desc.h for the endpoints).
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "usb_desc.h"

const uint8_t MyDevDescr[] =
{
    0x12,       /* bLength */
    0x01,       /* bDescriptorType = DEVICE */
    0x10, 0x01, /* bcdUSB 1.10 */
    0x02,       /* bDeviceClass = CDC, lets Windows pick usbser.sys */
    0x00,       /* bDeviceSubClass */
    0x00,       /* bDeviceProtocol */
    DEF_UEP0_SIZE,
    (uint8_t)DEF_USB_VID, (uint8_t)(DEF_USB_VID >> 8),
    (uint8_t)DEF_USB_PID, (uint8_t)(DEF_USB_PID >> 8),
    DEF_IC_PRG_VER, 0x00,
    0x01,       /* iManufacturer */
    0x02,       /* iProduct */
    0x03,       /* iSerialNumber */
    0x01,       /* bNumConfigurations */
};

/*
 * 9 + (9 + 5 + 5 + 4 + 5 + 7) + (9 + 7 + 7) = 67 bytes
 */
const uint8_t MyCfgDescr[] =
{
    /* configuration */
    0x09, 0x02, 0x43, 0x00, 0x02, 0x01, 0x00, 0x80, 0x32,

    /* interface 0: CDC control */
    0x09, 0x04, 0x00, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,

    /* CDC functional descriptors: header, call management, ACM, union */
    0x05, 0x24, 0x00, 0x10, 0x01,
    0x05, 0x24, 0x01, 0x00, 0x01,
    0x04, 0x24, 0x02, 0x02,
    0x05, 0x24, 0x06, 0x00, 0x01,

    /* EP1 IN, interrupt, notification */
    0x07, 0x05, 0x81, 0x03, DEF_UEP1_SIZE, 0x00, 0x01,

    /* interface 1: CDC data */
    0x09, 0x04, 0x01, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,

    /* EP2 OUT, bulk */
    0x07, 0x05, 0x02, 0x02, DEF_UEP2_SIZE, 0x00, 0x00,

    /* EP3 IN, bulk */
    0x07, 0x05, 0x83, 0x02, DEF_UEP3_SIZE, 0x00, 0x00,
};

const uint8_t MyLangDescr[] =
{
    0x04, 0x03, 0x09, 0x04
};

const uint8_t MyManuInfo[] =
{
    0x0E, 0x03, 'w', 0, 'c', 0, 'h', 0, '.', 0, 'c', 0, 'n', 0
};

const uint8_t MyProdInfo[] =
{
    0x18, 0x03, 'S', 0, 't', 0, 'r', 0, 'i', 0, 'p', 0,
                ' ', 0, 'G', 0, 'a', 0, 'm', 0, 'e', 0, 's', 0
};

const uint8_t MySerNumInfo[] =
{
    0x0A, 0x03, '0', 0, '0', 0, '0', 0, '1', 0
};

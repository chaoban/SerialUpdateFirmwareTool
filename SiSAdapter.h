﻿#ifndef SISADAPTER_H
#define SISADAPTER_H

#if 0
#define SIS_VERIFY {0x53, 0x49, 0x53, 0x5f, 0x56, 0x52, 0x46, 0x5f, 0x43, 0x4d, 0x44} //SIS_VRF_CMD
#define SIS_ACK {0x53, 0x49, 0x53, 0x5f, 0x56, 0x52, 0x46, 0x5f, 0x41, 0x43, 0x4b}    //SIS_VRF_ACK
#define SIS_VERIFY_LENGTH 11
#else
#define SIS_VERIFY {0x1f, 0x53, 0x49, 0x53, 0x5f, 0x56, 0x52, 0x46, 0x5f, 0x43, 0x4d, 0x44} //SIS_VRF_CMD
#define SIS_ACK {0x1f, 0x53, 0x49, 0x53, 0x5f, 0x56, 0x52, 0x46, 0x5f, 0x41, 0x43, 0x4b}    //SIS_VRF_ACK
#define SIS_VERIFY_LENGTH 12
#endif

#define TIMEOUT_SERIAL 3000

enum
{
    CALIBRATION_FLAG_LENGTH = 4 /sizeof(int),
    BOOT_FLAG_LENGTH = 4 /sizeof(int),
    FIRMWARE_ID_LENGTH = 12 / sizeof(int),
    FIRMWARE_INFO_LENGTH = 0x50 / sizeof(int),
    BOOTLOADER_ID_LENGTH = 4 / sizeof(int),
    BOOT_INFO_LENGTH = 0x10 / sizeof(int),
    MAX_BUF_LENGTH = 0x10000 / sizeof(int),
    _64K = 0x10000,
    _56K = 0xe000,
    _48K = 0xc000,
    _12K = 0x3000,
    _4K = 0x1000,
    _8K = 0x2000,
    _4K_ALIGN_12 = 0x1000 / 12 * 12,
    _4K_ALIGN_16 = 0x1000 / 16 * 16,

    I2C_INTERFACE = 0x1,
    USB_INTERFACE = 0x2,
};

#endif // SISADAPTER_H

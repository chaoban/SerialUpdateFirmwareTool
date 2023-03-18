#ifndef SISADAPTER_H
#define SISADAPTER_H

static inline unsigned char BCD (unsigned char x);
int getTimestamp();
void print_sep();
#ifdef _PROCESSBAR
inline void progresBar(int total, int current, int width);
#endif

#if 0
#define SIS_VERIFY {0x53, 0x49, 0x53, 0x5f, 0x56, 0x52, 0x46, 0x5f, 0x43, 0x4d, 0x44} //SIS_VRF_CMD
#define SIS_ACK {0x53, 0x49, 0x53, 0x5f, 0x56, 0x52, 0x46, 0x5f, 0x41, 0x43, 0x4b}    //SIS_VRF_ACK
#define SIS_VERIFY_LENGTH 11
#else
#define SIS_VERIFY {0x1f, 0x53, 0x49, 0x53, 0x5f, 0x56, 0x52, 0x46, 0x5f, 0x43, 0x4d, 0x44} //SIS_VRF_CMD
#define SIS_ACK {0x1f, 0x53, 0x49, 0x53, 0x5f, 0x56, 0x52, 0x46, 0x5f, 0x41, 0x43, 0x4b}    //SIS_VRF_ACK
#define SIS_VERIFY_LENGTH 12
#endif

#define TIMEOUT_SERIAL  3000
/* Special Update Flag : Update by SerialPort */
#define SERIAL_FLAG     0x5370 //ASCII: S P

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
    //chaoban test
    BOOT_FLAG_SIZE = 0x1000,
    _4K_ALIGN_52B = 79,
};

/*
 * 進度條函數 progresBar
 * inline函式的實體放標頭檔
 * 接受三個參數：total、current 和 width
 *  total 表示總共需要工作的次數
 *  current 表示當前工作的次數
 *  width 表示進度條的寬度。例如30表示總長度為30個字元
 */
#ifdef _PROCESSBAR
inline void progresBar(int total, int current, int width) {

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // 獲取標準輸出設備的句柄
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo); // 獲取標準輸出設備的屬性
    // 計算進度條填充的寬度和百分比
    float percent = (float)current / (float)total;
    int filled_width = (int)(percent * width);
    int i;

    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
    printf("[");
    for (i = 0; i < filled_width; i++) {
        printf("#");
    }
    for (i = filled_width; i < width; i++) {
        printf(".");
    }
    printf("] %.1f%%\r", percent*100);
    fflush(stdout);
    SetConsoleTextAttribute(hConsole, consoleInfo.wAttributes);
}
#endif

#endif // SISADAPTER_H

#ifndef SISADAPTER_H
#define SISADAPTER_H
#include <stdio.h>
#include <string.h>
#ifdef PROCESSBAR
#include <time.h>
#endif

int getTimestamp();
void print_sep();
#ifdef PROCESSBAR
inline void progresBar(int totalProgress , int currentProgress, int progressBarWidth, int updateTime);
#endif


#define GR_ENABLE_DEBUG {} //Disable GR Debug
#define GR_DISABLE_DEBUG {} //Enable GR Debug


#define SIS_VERIFY {0x1f, 0x53, 0x49, 0x53, 0x5f, 0x56, 0x52, 0x46, 0x5f, 0x43, 0x4d, 0x44} //SIS_VRF_CMD
#define SIS_ACK {0x1f, 0x53, 0x49, 0x53, 0x5f, 0x56, 0x52, 0x46, 0x5f, 0x41, 0x43, 0x4b}    //SIS_VRF_ACK
#define SIS_VERIFY_LENGTH 12

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
 *  totalProgress 表示總共需要工作的次數
 *  currentProgress 表示當前工作的次數
 *  progressBarWidth 表示進度條的寬度。例如30表示總長度為30個字元
 *  updateTime 表示更新頻率，單位為秒
 */
#ifdef PROCESSBAR
inline void progresBar(int totalProgress , int currentProgress, int progressBarWidth, int updateTime) {
    
    static time_t lastTime = 0; // 調整更新頻率用

    // 如果 current 等於 total，則強制更新進度條為100%
    // 避免因為更新頻率的關係導致沒有顯示完整100%
    if (currentProgress >= totalProgress) {
        printf("[");
        for (int i = 0; i < progressBarWidth; i++) {
            printf("#");
        }
        printf("] [  OK  ]");
        fflush(stdout);
        return;
    }

    // 檢查時間間隔是否符合要求
    if ((time(NULL) - lastTime) < updateTime) {
        return;
    }
	
    assert(totalProgress > 0 && "Error: total must be positive.");
    assert(currentProgress <= totalProgress && "Error: current value cannot be greater than total value.");
    assert(progressBarWidth > 0 && "Error: progress bar width must be positive.");
	
	// 計算進度條填充的寬度和百分比
    float percent = (float)currentProgress / (float)totalProgress;
    int filled_width = (int)(percent * progressBarWidth);
    int i;

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // 獲取標準輸出設備的Handle
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo); // 獲取標準輸出設備的屬性
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);

    printf("[");
    for (i = 0; i < filled_width; i++) {
        printf("#");
    }
    for (i = filled_width; i < progressBarWidth; i++) {
        printf(".");
    }
    printf("] [ %d%% ]\r", (int)(percent*100));
    fflush(stdout);

    SetConsoleTextAttribute(hConsole, consoleInfo.wAttributes);
	lastTime = time(NULL);
}
#endif

#endif // SISADAPTER_H

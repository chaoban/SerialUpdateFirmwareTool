#include <string.h>
#include <stdio.h>

typedef struct {
    char name[10];
    char desc[200];
} arg_t;

typedef struct {
    int assignPort; // 是否使用序列埠
    char serialPort[10]; // 序列埠名稱
    char fileName[100]; // 檔案名稱
} args_t;

arg_t args[11] = {
    {"-b",      "Update the bootloader"},
    {"--force", "Force firmware update without considering version"},
    {"-s",      "Scan and list all available serial ports"},
    {"-f",      "Firmware file name\n            Usage: -f [filename]"},
    {"--jump",  "Jump parameter validation"},
    {"-c",      "Specify updating firmware through which serial port\n            Usage: -c [Serial port number], such as com3"},
    {"-ba",     "Update bootloader automatically"},
    {"-g",      "Reserve RO data"},
    {"-r",      "Only update parameter"},
//    {"-w",      "Wait time set"},
    {"-a",      "Automatically detect the serial port connected to the SiS device for firmware update"},
    {"-h",      "Show Help"}
};

void print_help() {
    printf("可接受的參數：\n");
    for(unsigned int j = 0; j < sizeof(args)/sizeof(arg_t); j++) {
        printf("  %s : %s\n", args[j].name, args[j].desc);
    }
}

void process_args(int argc, char *argv[], args_t* param) {
    if(argc == 1) {
        printf("請輸入參數。\n");
        return;
    }
    else {
        for(int i = 1; i < argc; i++) {
            if (argv[i][0] == '-') {
            int found = 0; // 紀錄是否找到參數
            for(unsigned int j = 0; j < sizeof(args)/sizeof(arg_t); j++) {
                if(strcmp(argv[i], args[j].name) == 0) {
                    found = 1;
                    if(strcmp(argv[i], "-s") == 0) {
                        param->assignPort = 1;
                    }
                    else if(strcmp(argv[i], "-h") == 0) {
                        print_help();
                        return;
                    }
                    break;
                }
            }
            if(!found) {
                printf("未知參數：%s\n", argv[i]);
                printf("可接受的參數：\n");
                print_help();
                return;
            }
        }
        else if(strcmp(argv[i], "com1") == 0 ||
                strcmp(argv[i], "com2") == 0 ||
                strcmp(argv[i], "com3") == 0) {
            strcpy(param->serialPort, argv[i]);
        }
        else {
            strcpy(param->fileName, argv[i]);
        }
        }
    }
}

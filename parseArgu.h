#ifndef PARSEARGU_H
#define PARSEARGU_H
#include <string.h>
#include <stdio.h>

void print_help(const char* programName);

typedef struct {
    const char *arg;  // 參數
    const char *msg;  // 參數的說明
} arg_t;

typedef struct {
	char infile[256]; 	// 檔案名稱, 最長不超過 256 個字元
	char com[10]; 		// 序列埠名稱, 最長不超過 10 個字元
    bool a;
	bool b;
    bool ba;
    bool nc;
	bool d;
    bool dbg;
    int  dbgMode;
    bool force;
	bool h;
    bool jcp;
	bool jump;
	bool l;
	bool p;
	bool r;
    bool s;
	bool v;
    int  V;
    bool w;
} args_t;

arg_t args[] = {
	{"-h",       "Display this help information."},
    {"@<file>",	 "Load/Update options from firmware. Extension name is 'bin'."},
    {"com[0-16]","Specify updating firmware through serial com port.\n              Such as com3."},
    {"--dbg",    "Manually enable or disable GR-Uard-Debug function.\n              Ex. --dbg={0|1}"},
	{"--force",  "Force update firmware without considering version."},
	{"--jump",   "Jump some parameter validation, go on even some firmware parameters check failed."},
    {"-a",       "Automatically detect the serial port connected to the SiS device for firmware update."},
    {"-b",       "Update the bootloader."},
    {"-ba",      "Update bootloader automatically."},
    {"-nc",      "No need to confirm whether to update. Default is need to confirm."},
    {"-d",       "Dump firmware information of the device."},
	{"-l",		 "Display the information of the firmware binary file."},
    {"-p",       "Only update parameters."},
    {"-r",       "Reserve RO data."},
    {"-s",       "Scan and list all available serial ports in the host."},
    {"-v",		 "Display version and build information."},
    {"-w",       "Set the Wait time."},
    {"V",		 "Display Verbose debug message.The higher the level, the more debug messages are displayed.\n              Ex. V={1|2|3}"}
};

args_t param = {
    .infile = {0},
    .com = {0},
    .a = 		false,
    .b = 		false,
    .ba = 		false,
    .nc = 		false,
    .d = 		false,
    .dbg =      false,
    .dbgMode = 0,
    .force = 	false,
    .h = 		false,
    .jcp =      false,
    .jump = 	false,
	.l =		false,
    .p = 		false,
    .r = 		false,
    .s = 		false,
    .v = 		false,
    .V = 0,
    .w = 		false
};

void print_help(const char* programName) {
    printf("Update the SiS Pen firmware by uart comX port from object<file>.\n");
    printf("Usage: %s <file> <option(s)> <com[0-16]>\n", programName);
    printf(" At least one of the following switches must be given.\n");
    printf(" Options:\n");
    for(unsigned int j = 0; j < sizeof(args)/sizeof(arg_t); j++) {
        printf("  %-10s: %s\n", args[j].arg, args[j].msg);
    }
}

int process_args(int argc, char *argv[], args_t* param) {
    if (argc == 1) {
        print_help(argv[0]);
        return -1;
    }
    for(int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') { // 開頭為'-'的參數
            int found = 0; // 紀錄是否找到參數
            for(unsigned int j = 0; j < sizeof(args)/sizeof(arg_t); j++) {
                if (strcmp(argv[i], args[j].arg) == 0) { // To check arguments support or not
                    found = 1;
                    if (strcmp(argv[i], "-a") == 0) {
                        param->a = true;
                    }
                    else if (strcmp(argv[i], "-b") == 0) {
						param->b = true;
                    }
					else if (strcmp(argv[i], "-ba") == 0) {
						param->ba = true;
                    }
                    else if (strcmp(argv[i], "-nc") == 0) {
                        param->nc = true;
                    }
					else if (strcmp(argv[i], "-d") == 0) {
						param->d = true;
                    }
					else if (strcmp(argv[i], "--force") == 0) {
						param->force = true;
                    }
                    else if (strcmp(argv[i], "-h") == 0) {
                        param->h = true;
                    }
                    else if (strcmp(argv[i], "--jcp") == 0) {
                        param->jcp = true;
                    }
					else if (strcmp(argv[i], "--jump") == 0) {
						param->jump = true;
                    }
					else if (strcmp(argv[i], "-l") == 0) {
						param->l = true;
                    }
					else if (strcmp(argv[i], "-p") == 0) {
						param->p = true;
                    }
					else if (strcmp(argv[i], "-r") == 0) {
						param->r = true;
                    }
					else if (strcmp(argv[i], "-s") == 0) {
						param->s = true;
                    }
					else if (strcmp(argv[i], "-v") == 0) {
						param->v = true;
                    }
					else if (strcmp(argv[i], "-w") == 0) {
						param->w = true;
                    }
                    break;
                }
                if (strncmp(argv[i], "--dbg", 5) == 0) {
                    found = 1;
                    char* dbg_str = argv[i] + 5;
                    param->dbg = true;
                    if (strcmp(dbg_str, "=1") == 0) {
                        param->dbgMode = 1;
                    }
                    else if (strcmp(dbg_str, "=0") == 0) {
                        param->dbgMode = 0;
                    }
                    else {
                        param->dbg = false;
                        printf("Invalid argument for --dbg option. Valid values are = [0|1].\n");
                        return -1;
                    }
                }
            }
            if (!found) {
                printf("Unrecognized command-line option [%s]\n", argv[i]);
                printf("Please type -h to show list of classes of commands.\n");
                return -1;
            }
        } // TODO: 需要優化，判斷COM後再抓後面數字即可
        else if (strcmp(argv[i], "com1") == 0 || strcmp(argv[i], "com2") == 0 || strcmp(argv[i], "com3") == 0 ||
            strcmp(argv[i], "com4") == 0 || strcmp(argv[i], "com5") == 0 || strcmp(argv[i], "com6") == 0 ||
            strcmp(argv[i], "com7") == 0 || strcmp(argv[i], "com8") == 0 || strcmp(argv[i], "com9") == 0 ||
            strcmp(argv[i], "com10") == 0 || strcmp(argv[i], "com11") == 0 || strcmp(argv[i], "com12") == 0 ||
            strcmp(argv[i], "com13") == 0 || strcmp(argv[i], "com14") == 0 || strcmp(argv[i], "com15") == 0 ||
            strcmp(argv[i], "com16") == 0)
        {
            strcpy(param->com, argv[i]);
        } else if (strncmp(argv[i], "V", 1) == 0) {
            char* dbg_str = argv[i] + 1;
            if (strcmp(dbg_str, "=1") == 0) {
                param->V = 1;
            }
            else if (strcmp(dbg_str, "=2") == 0) {
                param->V = 2;
            }
            else if (strcmp(dbg_str, "=3") == 0) {
                param->V = 3;
            }
            else {
                param->V = 0;
                //printf("Invalid argument for --dbg option. Valid values are = [0|1].\n");
                //return -1;
            }
        } else
        {
            char *ext = strrchr(argv[i], '.');      // 取得argv中最後一個 '.' 的位置
            if (ext && strcmp(ext, ".bin") == 0) {   // 如果ext不為NULL且與".bin"相同，則argv為副檔名為bin的名稱
                strcpy(param->infile, argv[i]);
            }
            else {
                printf("Unrecognized command-line option [%s]\n", argv[i]);
                printf("Please type -h to show list of classes of commands.\n");
                return -1;
            }
        }
    }
    return 0;
}
#endif // PARSEARGU_H

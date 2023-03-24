﻿#ifndef PARSEARGU_H
#define PARSEARGU_H

#include <string.h>
#include <stdio.h>

void print_help();

typedef struct {
    const char *arg;  // 參數
    const char *msg;  // 參數的說明
} arg_t;

typedef struct {
	char infile[256]; 	// 檔案名稱, 最長不超過 256 個字元
	char outfile[256]; 	// 檔案名稱, 最長不超過 256 個字元
	char com[10]; 		// 序列埠名稱, 最長不超過 10 個字元
    bool a;
	bool b;
    bool ba;
    bool nc;
	bool d;
    bool force;
	bool h;
	bool jump;
	bool p;
	bool r;
    bool s;
	bool v;
    bool w;
} args_t;

arg_t args[] = {
    {"Input file ",	"Load the Binary firmware's name."},
    {"Output file",	"Dump to Binary firmware's file name."},
    {"comX",     "Specify updating firmware through serial comX port. (X=1, 2, 3...) Such as com3."},
    {"-a",       "Automatically detect the serial port connected to the SiS device for firmware update."},
    {"-b",       "Update the bootloader."},
    {"-ba",      "Update bootloader automatically."},
    {"-nc",      "No need to confirm whether to update."},
    {"-d",       "Dump firmware to a file."},
    {"--force",  "Force firmware update without considering version."},
    {"-h",       "Display this information."},
    {"--jump",   "Jump parameter validation."},
    {"-p",       "Only update parameter."},
    {"-r",       "Reserve RO data."},
    {"-s",       "Scan and list all available serial ports."},
    {"-v",		 "Display version information."},
    {"-w",       "Wait time set."}
};

args_t param = {
    .infile = {0},
    .outfile = {0},
    .com = {0},
    .a = false,
    .b = false,
    .ba = false,
    .nc = false,
    .d = false,
    .force = false,
    .h = false,
    .jump = false,
    .p = false,
    .r = false,
    .s = false,
    .v = false,
    .w = false
};

void print_help() {
    printf("Update SiS Device firmware by serial comX port from object <file>.\n");
    printf("Usage: sUpdateFw <option(s)> <file>\n");
    printf(" At least one of the following switches must be given:\n");
    printf(" Options:\n");
    for(unsigned int j = 0; j < sizeof(args)/sizeof(arg_t); j++) {
        printf("  %-10s: %s\n", args[j].arg, args[j].msg);
    }
}

int process_args(int argc, char *argv[], args_t* param) {
    if (argc == 1) {
        print_help();
        return -1;
    }
    for(int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') { // 開頭為'-'的參數
            int found = 0; // 紀錄是否找到參數
            for(unsigned int j = 0; j < sizeof(args)/sizeof(arg_t); j++) {
                if(strcmp(argv[i], args[j].arg) == 0) {
                    found = 1;
                    if(strcmp(argv[i], "-a") == 0) {
                        param->a = true;
                    }
                    else if(strcmp(argv[i], "-b") == 0) {
						param->b = true;
                    }
					else if(strcmp(argv[i], "-ba") == 0) {
						param->ba = true;
                    }
                    else if(strcmp(argv[i], "-nc") == 0) {
                        param->nc = true;
                    }
					else if(strcmp(argv[i], "-d") == 0) {
						param->d = true;
                    }
					else if(strcmp(argv[i], "--force") == 0) {
						param->force = true;
                    }
                    else if(strcmp(argv[i], "-h") == 0) {
                        param->h = true;
                    }
					else if(strcmp(argv[i], "--jump") == 0) {
						param->jump = true;
                    }
					else if(strcmp(argv[i], "-p") == 0) {
						param->p = true;
                    }
					else if(strcmp(argv[i], "-r") == 0) {
						param->r = true;
                    }
					else if(strcmp(argv[i], "-s") == 0) {
						param->s = true;
                    }
					else if(strcmp(argv[i], "-v") == 0) {
						param->v = true;
                    }
					else if(strcmp(argv[i], "-w") == 0) {
						param->w = true;
                    }
                    break;
                }
            }
            if(!found) {
                printf("Unrecognized command-line option [%s]\n", argv[i]);
                printf("Please type -h to show list of classes of commands\n");
                return -1;
            }
        }
        else if(strcmp(argv[i], "com1") == 0 ||
            strcmp(argv[i], "com2") == 0 ||
            strcmp(argv[i], "com3") == 0 ||
            strcmp(argv[i], "com4") == 0 ||
            strcmp(argv[i], "com5") == 0 ||
            strcmp(argv[i], "com6") == 0 ||
            strcmp(argv[i], "com7") == 0 ||
            strcmp(argv[i], "com8") == 0 ||
            strcmp(argv[i], "com9") == 0 ||
            strcmp(argv[i], "com10") == 0 ||
            strcmp(argv[i], "com11") == 0 ||
			strcmp(argv[i], "com12") == 0 ||
			strcmp(argv[i], "com13") == 0 ||
			strcmp(argv[i], "com14") == 0 ||
			strcmp(argv[i], "com15") == 0 ||
            strcmp(argv[i], "com16") == 0) 
			{
				strcpy(param->com, argv[i]);
			} else {
				strcpy(param->infile, argv[i]);
        }
    }
    return 0;
}
#endif // PARSEARGU_H

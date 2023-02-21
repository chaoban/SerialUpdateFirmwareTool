#ifndef VERSION_H
#define VERSION_H

#include <stdio.h>

#define MAIN_VERSION 1
#define SUB_VERSION 0
#define LAST_VERSION 0
//#define DESC "HIDRAW"

void printVersion()
{
#ifndef DESC
    printf("V%d.%d.%d\n", MAIN_VERSION, SUB_VERSION, LAST_VERSION);
#else
    printf("V%d.%d.%d %s\n", MAIN_VERSION, SUB_VERSION, LAST_VERSION, DESC);
#endif
}

#endif

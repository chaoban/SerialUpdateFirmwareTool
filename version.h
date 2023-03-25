#ifndef VERSION_H
#define VERSION_H
#include <stdio.h>

#define MAIN_VERSION 1
#define SUB_VERSION 0
#define LAST_VERSION 0

#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

void printVersion();

void printVersion()
{
    printf("Silicon Integrated Systems Corp. (2023) Version %d.%d.%d ", MAIN_VERSION, SUB_VERSION, LAST_VERSION);
    printf("Build %s %s", BUILD_DATE, BUILD_TIME);
    //qInfo() << "Build:" << BUILD_DATETIME;
}
#endif

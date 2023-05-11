#ifndef VERSION_H
#define VERSION_H
#include <stdio.h>

#define MAIN_VERSION 0
#define SUB_VERSION 3
#define LAST_VERSION 0

#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

void printVersion();

void printVersion()
{
    printf("SiS Update Pen Firmware tool.(Uart interface) Version %d.%d.%d  ",
           MAIN_VERSION,
           SUB_VERSION,
           LAST_VERSION);
    printf("Build %s %s\n",
           BUILD_DATE,
           BUILD_TIME);
    printf("(c) Silicon Integrated Systems Corporation.\n");
}
#endif

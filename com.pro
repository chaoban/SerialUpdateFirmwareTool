QT -= gui
QT += serialport

CONFIG += c++11 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        CRC16.cpp \
        DoFirmwareUpdate.cpp \
#        crc.c \
        main.cpp \
        scanserialport.cpp


HEADERS += \
    CRC16.h \
    DoFirmwareUpdate.h \
    ExitStatus.h \
#    crc.h \
    version.h


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


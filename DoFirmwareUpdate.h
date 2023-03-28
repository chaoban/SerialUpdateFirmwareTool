#ifndef DOFIRMWAREUPDATE_H
#define DOFIRMWAREUPDATE_H

#include <QTextStream>
#include <QSerialPort>
#include "ExitStatus.h"

#define _CHAOBAN_TEST               1
//#define _DBG_DISABLE_READCMD        1
#define _GR_INIT_FLOW               1
#define _PROCESSBAR                 1

enum SIS_817_POWER_MODE {
    POWER_MODE_ERR = EXIT_ERR,
    POWER_MODE_FWCTRL = 0x50,
    POWER_MODE_ACTIVE = 0x51,
    POWER_MODE_SLEEP  = 0x52
};
enum SIS_817_POWER_MODE sis_get_fw_mode();

struct firmwareMap {
    struct {
        unsigned int sourceTag; 		//0x0200, 6 bytes, 7501A is "virgo", 0x76 69 72 67 6f 00
        unsigned int releaseTime;		//0X0206, 6 bytes, YYYYMMDDhhmm, 20 22 12 19 18 00
        unsigned int mark;				//0x020c, 4 bytes, "SiS", 0x00 53 69 53
		unsigned int codeTag;			//0x0220, 4 bytes, V020700, 0x56 02 07 00
		unsigned int version;			//0x0230, 4 bytes, 1.05版, 0x00 01 00 05
		unsigned int chkSum;			//0x0234, 4 bytes, 0x000-0x1ff, 0x240-0x3fff
    } bootLoader;

    struct {
        unsigned int updateMark;
        unsigned int chipID;
        unsigned int vender;
		unsigned int taskID;
        unsigned int reverse;
        unsigned int chipType;
        unsigned int majVer;
        unsigned int minVer;
        unsigned int buildCount;
        unsigned int codeBaseTime;
		unsigned int codeBaseTag;
        unsigned int cdcMode;
        unsigned int buildTime;
		unsigned int interfaceID;
        unsigned int enCali;
		unsigned int bootloaderID;
		unsigned int mainCodeID;
		unsigned int mainCodeCRC;
		unsigned int bootCodeCRC;
        unsigned int enPen;
        unsigned int enButton;
		unsigned int paramCRC;
    } description;
	
	struct {
		unsigned int lastTime;
		unsigned int updateInfo;
		unsigned int lastMark;
		unsigned int lastID;
		unsigned int priorLastTime;
		unsigned int priorLastMark;
		unsigned int priorLastID;

    } updateFwInfo;

    struct {
        unsigned int time;
        unsigned int info;
    } bootFlag;
};

bool sisSwitchCmdMode(QSerialPort* serial);
bool sisFlashRom(QSerialPort* serial);
bool sisClearBootflag(QSerialPort* serial);
bool sisResetCmd(QSerialPort* serial);
bool sisChangeMode(enum SIS_817_POWER_MODE);
bool sisGetFwId(QSerialPort* serial, quint16 *);
bool sisUpdateCmd(QSerialPort* serial, unsigned int, int);
bool sisWriteDataCmd(QSerialPort* serial, const quint8 *, unsigned int);
bool sisUpdateBlock(QSerialPort* serial, quint8 *, unsigned int, unsigned int);
bool burningCode(QSerialPort* serial, quint8 *fn, bool bUpdateBootloader);
bool sisGetBootloaderId_Crc(QSerialPort* serial, quint32 *, quint32 *);
int sisCmdTx(QSerialPort* serial, int , unsigned char *);
int sisCmdRx(QSerialPort* serial, int , unsigned char *);
int verifyRxData(uint8_t *buffer);
int sisUpdateFlow(QSerialPort* serial, quint8 *sis_fw_data, bool bUpdateBootloader, bool bUpdateBootloader_auto, bool bForceUpdate, bool bJump);
int sisGetBootflag(QSerialPort* serial, quint32 *);
int sisGetFwInfo(QSerialPort* serial, quint8 *, quint32 *, quint32 *, quint16 *, quint8 *, quint16 *);
#endif // DOFIRMWAREUPDATE_H

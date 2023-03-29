#include <QCoreApplication>
#include <QSettings>
#include <QStringList>
#include <QSerialPort>
#include <QDebug>
#include <QFile>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include <iostream>
#include <stdio.h>
#include <string.h>
#include "parseArgu.h"
#include "DoFirmwareUpdate.h"
#include "SiSAdapter.h"
#include "version.h"
#include "ExitStatus.h"
#include "sis_command.h"
//#include "delay.h"
//#pragma comment(lib, "Advapi32.lib")

DWORD WINAPI RcvWaitProc(LPVOID lpParamter);
const QStringList getComportRegKey();
int GRDebugFunc(QSerialPort& serial, const QByteArray& writeData, int timeout, int count);
int testSerialPort(QString *ComPortName);
int openBinary(QString path);
int getFirmwareInfo(quint8 *sis_fw_data);
int verifyFirmwareInfo(quint8 *sis_fw_data);
void getUserInput();
extern int scanSerialport();
extern int getTimestamp();
extern void msleep(unsigned int msec);
extern void print_sep();
extern void printAddrData(quint8* sis_fw_data, const char* str, quint32 address, int length, bool bcb);
extern DateTime getCurrentDateTime();

int occupiedPortCount = 0;
int timeOutPortCount = 0;
bool mismatchKey = FALSE;
/* 讀取韌體檔案用 */
quint8 *sis_fw_data; //unsigned char * sis_fw_data;
extern QByteArray FirmwareString;
/* 建立FW資訊用 */
firmwareMap binaryMap;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    const QStringList argumentList = QCoreApplication::arguments();
    QTextStream standardOutput(stdout);
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    QString filename = "";
    QString ComPortName = "";
    QSerialPort serial; /* 開啟Serial Port用 */
    bool bAutoDetect = false;
    bool bUpdateBootloader = false;
    bool bUpdateBootloader_auto = false;
    bool bNc = false;
    bool bDump = false;
    bool bForceUpdate = false;
    bool bJump = false;
	bool bList = false;
    bool bUpdateParameter = false;
    bool bReserveRODATA = false;
    bool bWaitTime = false;
    int exitCode = CT_EXIT_AP_FLOW_ERROR;

    QByteArray DisableGRUart =  QByteArray::fromRawData("\x12\x05\x80\x05\x00\x09\x00\x01\x00\x00", 10);
    QByteArray DisableIIC =     QByteArray::fromRawData("\x12\x08\x80\x05\x00\x09\x00\x00\x00\x00", 10);
    QByteArray ResetHW =        QByteArray::fromRawData("\x12\x07\x80\x05\x00\x09\x00\x01\x00\x00", 10);
    QByteArray EnableIIC =      QByteArray::fromRawData("\x12\x08\x80\x05\x00\x09\x00\x01\x00\x00", 10);
    QByteArray InitGR =         QByteArray::fromRawData("\x12\x01\x80\x05\x00\x09\x00\x01\x00\x00", 10);
    QByteArray EnableGRUart =   QByteArray::fromRawData("\x12\x06\x80\x05\x00\x09\x00\x01\x00\x00", 10);

    /*
     * 處理輸入的參數
     * 沒有接參數的時候，顯示(Help)指令說明
     */
    exitCode = process_args(argc, argv, &param);
    if(exitCode != EXIT_OK)
        return EXIT_BADARGU;
	
    if(param.v) {
        printVersion();
        return EXIT_OK;
    }
    // 帶有Help參數時，顯示參數說明，然後不再繼續執行
    if(param.h) {
        print_help();
        return EXIT_OK;
    }
    if(param.l) {
        //TODO
        printf("Display Firmware Information of the Binary firmware file:\n");
        bList = true;
        goto lb_GetFile;
    }
    // 帶有掃描serial port時，掃描及列出後，不再繼續執行
    if(param.s) {
        printf("Scan and list all available serial ports:\n");
        scanSerialport();
        return EXIT_OK;
    }

    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
    printf("Arguments setting by user:\n");
    if(param.a) {
        bAutoDetect = true;
        printf(" * Automatically detect the serial port for SiS Device.\n");
    }
    if(param.dbg) {
        if(param.dbgMode == 1) {
            // TODO
            printf("Enable GR Uart Debug Function.\n");
        }
        if (param.dbgMode == 0) {
            // TODO
            printf("Disable GR Uart Debug Function.\n");
        }
        return EXIT_OK;
    }
    /*
     * 以下重要的參數
     */
    if(param.b) {
        bUpdateBootloader = true;
        printf(" * Update Bootloader.\n");
    }
    if(param.ba) {
        bUpdateBootloader_auto = true;
        printf(" * Automatically Update Bootloader.\n");
    }
    if(param.nc) {
        bNc = true;
        printf(" * No need to confirm update.\n");
    }
    if(param.d) {
        bDump = true;
        //TODO
        printf(" * Dump function HAS NOT SUPPORT YET.\n");
    }
    if(param.force) {
        bForceUpdate = true;
        printf(" * Force update firmware.\n");
    }
    if(param.jump) {
        bJump = true;
        printf(" * Skip parameter validation.\n");
    }
    if(param.p) {
        bUpdateParameter = true;
        //TODO
        printf(" * Only update parameter HAS NOT SUPPORT YET.\n");
    }
    if(param.r) {
        bReserveRODATA = true;
        //TODO
        printf(" * Reserve RO data HAS NOT SUPPORT YET.\n");
    }
    if(param.w) {
        bWaitTime = true;
        //TODO
        printf(" * Wait time HAS NOT SUPPORT YET.\n");
    }

    SetConsoleTextAttribute(hConsole, consoleInfo.wAttributes);
	print_sep();
    /*
     * ==============================
     * 1. 參數檢查
     * 2. 讀取韌體檔案
     * 3. 開啟Serial port
     * 4. 進入更新程序
     * ==============================
     */
#if 1 //NOT IMPLEMENT
	if(bUpdateParameter || bReserveRODATA || bDump || bWaitTime) {
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN);
		printf("Attention: -p, -r, -d, and -w are not implement yet.\n");
		SetConsoleTextAttribute(hConsole, consoleInfo.wAttributes);
	}
#else
	if(bUpdateParameter && bUpdateBootloader) {
        printf("Attention: -p and -b is conflict.\n");
        return EXIT_BADARGU;
    }
    if(bUpdateParameter && bReserveRODATA) {
        printf("Attention: -p and -r is conflict.\n");
        return EXIT_BADARGU;
    }	
    if(bForceUpdate && bJump) {
        printf("Attention: --force and --jump is conflict.\n");
        return EXIT_BADARGU;
    }
#endif
lb_GetFile:
	/* 選定韌體檔案 */
    if(param.infile[0] != '\0') {
        //printf("Open %s file\n", param.infile);
        filename = QString::fromStdString(param.infile);
        //filename = QString::fromLocal8Bit(param.infile);
    }
    else {
        printf("No binary file found.\n");
        return EXIT_BADARGU;
    }

    if(bList == true) goto lb_Openfile;

	/* 選定Com Port */
	if(param.com[0] != '\0') {
        //printf("Open the Serial %s port\n", param.com);
        ComPortName = QString::fromStdString(param.com);
        printf("Assign the %s port to update.\n", ComPortName.toStdString().c_str());
        bAutoDetect = false;
    } else if(bAutoDetect == true) {
        printf("Automatically detect the serial port that connect to SiS Device.\n");
    } else {
        printf("Please type a com[0-16] port, or type '-a' to auto detect the serial port.\n");
        return EXIT_BADARGU;
    }
	// 為了7501初期暫時的處置
    // 提示必須同時更新Bootloader
#if 1
    if (bUpdateBootloader == false){
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN);
        printf("Attention: currently 7501 must update the Bootloader at the same time. Please type '-b'.\n");
        printf("Or Bootloader will NOT UPDATE if also hasn't type '-ba'.\n");
		SetConsoleTextAttribute(hConsole, consoleInfo.wAttributes);
        if (bNc == false) getUserInput();
        //return EXIT_BADARGU;
    }
#endif
    /*
	 * 決定要不要更新Boot Loader
     * 同時下"更新Bootloader"
     *     和"自動更新Bootloader"時，
     *     以"自動"為優先
     */
    if(bUpdateBootloader_auto == false) {
        if (bUpdateBootloader == true) {
            printf("Update Bootloader.\n");
        }else {
            printf("Do not Update Bootloader.\n");
        }
    } else { // Auto update bootloader
        bUpdateBootloader = false;
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN);
        printf("Attention: -b and -ba are conflict, it will use -ba to update bootloader automatically.\n");
		SetConsoleTextAttribute(hConsole, consoleInfo.wAttributes);
    }
    print_sep();

lb_Openfile:

#if 1
	/* 開啟韌體檔案*/
	exitCode = openBinary(filename);
    if (exitCode == EXIT_OK) {
        qDebug() << "Load firmware binary file successfully:" << filename;
    }else {    
        qDebug() << "Load firmware binary file failed:" << filename;
        return exitCode;
    }   
#else	
    long file_size;

    FILE* fp = fopen(filename.toStdString().c_str(), "rb");
    if (fp == NULL) {
        printf("Failed to open the file: %s.\n", filename.toStdString().c_str());
        return EXIT_BADARGU;
    }

    printf("Open the file: %s successfully.\n", filename.toStdString().c_str());

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    if ((sis_fw_data = (quint8*)malloc(sizeof(quint8) * file_size)) == NULL) {
        printf("Failed to allocate memory.\n");
        return EXIT_ERR;
    }
    if (fread(sis_fw_data, sizeof(char), (size_t)file_size, fp) != (size_t)file_size) {
        printf("Failed to read file.\n");
        return EXIT_ERR;
    }
    fclose(fp);
#endif
	/* 抓出韌體內的資訊*/
	exitCode = getFirmwareInfo(sis_fw_data);
    if (exitCode == EXIT_OK) {
        //qDebug() << "Dump firmware binary information successfully.";
    }else {    
        qDebug() << "Dump firmware binary information failed.";
        return exitCode;
    }

    print_sep();

    if(bList == true)
        return EXIT_OK;

    //TODO
	/* 檢查韌體資訊 */
    exitCode = verifyFirmwareInfo(sis_fw_data);
    if (exitCode == EXIT_OK) {
        qDebug() << "Verify firmware binary file successfully:" << filename;
    }else {    
        qDebug() << "Verify firmware binary file failed:" << filename;
        return exitCode;
    }
    /* 開啟Serial port
     * -a: 自動測試並取得連接SIS Device的Serial Port
     * 如果使用者指定Serial Port，就不會再做自動測試
     */
    /* Auto get available serial ports that connect to  SIS Device*/
    if (bAutoDetect) {
        testSerialPort(&ComPortName);
    }
    if (ComPortName.size()>4) {
        QString tmp = "\\\\.\\";
        tmp.append(ComPortName);
        ComPortName = tmp;
    }
    serial.setPortName(ComPortName);
    /*
     * UART預設值
     */
    serial.setBaudRate(QSerialPort::Baud115200);
    serial.setDataBits(QSerialPort::Data8);
    serial.setParity(QSerialPort::NoParity);
    serial.setStopBits(QSerialPort::OneStop);
    serial.setFlowControl(QSerialPort::NoFlowControl);

    if (!serial.open(QIODevice::ReadWrite)) {
        standardOutput << QObject::tr("Failed to open serial %1 port, error: %2.").arg(ComPortName, serial.errorString()) << Qt::endl;
        return CT_EXIT_NO_COMPORT;
    }
    printf("Open serial %s port successfully.\n", ComPortName.toStdString().c_str());
	
    /* 等待使用者確認y/Y後再更新 */
    printf("\nThe process of updating the firmware will start.");
	printf("\n");
    if (bNc == false) getUserInput();

#ifdef _GR_INIT_FLOW
    /*
     * Here we can disable GR Uart Debug function
     */
    printf("Disable GR Uart debug feature ... ");
    exitCode = GRDebugFunc(serial, DisableGRUart, 10, 50);//Set Timeout=10, count=50
    if (exitCode != 0xbeef) {
        printf("Failed.\n");
        return GR_ERROR;
    }else
        printf("Success.\n");

    printf("Disable II2C ... ");
    exitCode = GRDebugFunc(serial, DisableIIC, 10, 1);
    if (exitCode != 0xbeef) {
        printf("Failed.\n");
        return GR_ERROR;
    }else
        printf("Success.\n");

    printf("Reset Hardware ... ");
    exitCode = GRDebugFunc(serial, ResetHW, 10, 1);
    if (exitCode != 0xbeef) {
        printf("Failed.\n");
        return GR_ERROR;
    }else
        printf("Success.\n");
	
    msleep(400);
	
    printf("Enable II2C ... ");
    exitCode = GRDebugFunc(serial, EnableIIC, 10, 1);
    if (exitCode != 0xbeef) {
        printf("Failed.\n");
        return GR_ERROR;
    }else
        printf("Success.\n");

    printf("Initial Hardware ... ");
    exitCode = GRDebugFunc(serial, InitGR, 10, 100);
    if (exitCode != 0xbeef) {
        printf("Failed.\n");
        return GR_ERROR;
    }else
        printf("Success.\n");
#endif

    print_sep();

    /* UPDATE Firmware */
    qDebug() << "Start Update Firmware by"  <<  serial.portName() << "port.";
    exitCode = sisUpdateFlow(&serial,
                             sis_fw_data,
                             bUpdateBootloader,
                             bUpdateBootloader_auto,
                             bForceUpdate,
                             bJump);

    printf("\nExit code : %d.\n", exitCode);
    if (exitCode == EXIT_SUCCESS) {
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
        printf("Update Firmware Success.\n");
    } else {
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
        printf("Update Firmware Failed\n");
    }
    /* Recovery the color of text in console */
    SetConsoleTextAttribute(hConsole, consoleInfo.wAttributes);

    free(sis_fw_data);

    print_sep();
	
    printf("Disable II2C ... ");
    exitCode = GRDebugFunc(serial, DisableIIC, 10, 1);
    if (exitCode != 0xbeef) {
        printf("Failed.\n");
        return GR_ERROR;
    }else
        printf("Success.\n");

    printf("Reset Hardware ... ");
    exitCode = GRDebugFunc(serial, ResetHW,10, 1);
    if (exitCode != 0xbeef) {
        printf("Failed.\n");
        return GR_ERROR;
    }else
        printf("Success.\n");

    printf("Enable GR Uart debug feature ... ");
    exitCode = GRDebugFunc(serial, EnableGRUart, 10, 1);
    if (exitCode != 0xbeef) {
        printf("Failed.\n");
        return GR_ERROR;
    }else
        printf("Success.\n");


    if (serial.isOpen()) {
        serial.close();
    }

    return exitCode;
    //  return a.exec();
}

const QStringList getComportRegKey()
{
    QStringList comports;
    QString keyPath = "HARDWARE\\DEVICEMAP\\SERIALCOMM";
    HKEY comsKey;
    LPCWSTR winKeyPath = (LPCWSTR) keyPath.constData();
    if( RegOpenKey( HKEY_LOCAL_MACHINE, winKeyPath, &comsKey ) != ERROR_SUCCESS ) {
        RegCloseKey( comsKey );
        return comports;
    }

    QSettings settings( QString("HKEY_LOCAL_MACHINE\\")+keyPath, QSettings::NativeFormat );
    QStringList keys = settings.allKeys();
    foreach( const QString &key, keys ) {
        QString newKey(key);
        newKey.replace( QString("/"), QString("\\") );
        LPCWSTR winKey = (LPCWSTR) newKey.constData();
        char *szData = new char[101];
        DWORD dwType, dwLen=100;
        if( RegQueryValueEx( comsKey, winKey,
                             NULL, &dwType, (unsigned char *)szData, &dwLen) == ERROR_SUCCESS ) {
            comports.append( QString::fromUtf16( (ushort*)szData ) );
            printf("%s. ",QString::fromUtf16( (ushort*)szData ).toStdString().c_str() );
        }
        delete[] szData;
    }
    RegCloseKey( comsKey );
    return comports;
}

DWORD WINAPI RcvWaitProc(LPVOID lpParamter)
{
    HANDLE hComm = (HANDLE) lpParamter;
    SetCommMask( hComm, EV_RXCHAR );
    DWORD dwEventMask;
    WaitCommEvent( hComm, &dwEventMask, NULL );
    return(0L);
}

int testSerialPort(QString *ComPortName)
{
    printf("\nList all com port : ");
    QStringList comports = getComportRegKey();

    int portAmount = comports.size();
    if ( portAmount == 0 ) {
        printf("Error : cannot find any existing com port, please attach UART/USB adapter.\n");
        return CT_EXIT_NO_COMPORT;
    }

    printf("\nStart polling com port ...\n");
    for (int pollingCount=portAmount-1; pollingCount>=0; pollingCount--) {
        QString curPortName = comports.at(pollingCount);
        *ComPortName = curPortName;
        if (curPortName.size()>4) {
            QString tmp = "\\\\.\\";
            tmp.append(curPortName);
            curPortName = tmp;
        }
        HANDLE hComm;
        hComm = CreateFile( (const wchar_t*) curPortName.utf16(),
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL );
        DCB tmpDCB;
        bool commState;
        commState = GetCommState(hComm, &tmpDCB);
        if ( hComm == INVALID_HANDLE_VALUE || commState == FALSE) {
            printf("Open %s failed\t--> Warning : this com port was occupied or no longer available.\n", curPortName.toStdString().c_str());
            occupiedPortCount ++;
            CloseHandle( hComm );
            continue;
        }
        else if ( commState == TRUE ) {
            tmpDCB.BaudRate = CBR_115200;
            tmpDCB.ByteSize = 8;
            tmpDCB.StopBits = ONESTOPBIT;
            tmpDCB.Parity = NOPARITY;
            SetCommState( hComm, &tmpDCB );
            printf("Open %s successfully\t--> ", curPortName.toStdString().c_str());
        }
        //BOOL status;
        char lpBuffer[] = SIS_VERIFY;
        DWORD dNoOfBytestoWrite;
        DWORD dNoOfBytesWritten = 0;
        dNoOfBytestoWrite = sizeof( lpBuffer );
        //status = WriteFile( hComm,
        WriteFile( hComm,
                   lpBuffer,
                   dNoOfBytestoWrite,
                   &dNoOfBytesWritten,
                   NULL );
        HANDLE hThread;
        hThread = CreateThread(NULL, 0, RcvWaitProc, hComm, 0, NULL);
        if ( WaitForSingleObject(hThread, TIMEOUT_SERIAL) == WAIT_TIMEOUT ) {
            printf("Warning : receiver timeout.\n");
            timeOutPortCount++;
//            CloseHandle( hComm );
            continue;
        }

        char TempChar;
        char SerialBuffer[256];
        DWORD NoByteRead;
        int inByteCount = 0;
        do {
            ReadFile( hComm,
                      &TempChar,
                      sizeof(TempChar),
                      &NoByteRead,
                      NULL );
            SerialBuffer[inByteCount] = TempChar;
            inByteCount++;
//            printf("%x , %d, %d\n", TempChar, NoByteRead, inByteCount);
        }
        while(inByteCount < SIS_VERIFY_LENGTH);
//        while( NoByteRead > 0 );

        char verifyKey[] = SIS_ACK;
        mismatchKey = FALSE;
        for ( int i=0; i<inByteCount; i++ ) {
            if ( verifyKey[i]!= SerialBuffer[i] ) {
                mismatchKey = TRUE;
            }
        }
        if (mismatchKey == FALSE) {
            printf("verifying key matches for SiS Device.\n");
            CloseHandle( hComm );
            return CT_EXIT_PASS;
            break;
        }
        else {
            printf("verifying key mismatched.\n");
#if 0
            for( int i=0; i<inByteCount; i++ )
            {
                printf("%x,", SerialBuffer[i]);
            }
            printf("\n\n");
#endif
            CloseHandle( hComm );
            continue;
        }
        CloseHandle( hComm );
    }

    printf("FAIL\n");
    if ( occupiedPortCount>0 ) {
        printf("(Open com port fail, some com ports were occupied).\n");
    }
    if (mismatchKey == TRUE) {
        printf("(Verifying key mismatch).\n");
    }
    if (timeOutPortCount>0) {
        printf("(UART RX no response).\n");
    }
    return CT_EXIT_FAIL;
}

#if 1
int openBinary(QString path)
{
    char file_data;
    QFile file(path);
	/* TODO Debug: IF Local variable will make data incorrect, WHY */
    //QByteArray FirmwareString;
	
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Could not open bin file for reading.";
        return EXIT_ERR;
    }
    while(!file.atEnd())
    {
      // return value from 'file.read' should always be sizeof(char).
      file.read(&file_data, sizeof(char));
      FirmwareString.append(file_data);
    }
    file.close();

    printf("Firmware data being read are %i bytes.\n", FirmwareString.length());
    //sis_fw_data = (unsigned char *)FirmwareString.data();
    sis_fw_data = (quint8 *)FirmwareString.data();
    return EXIT_OK;
}

int getFirmwareInfo(quint8 *sis_fw_data)
{
    // TODO: Refine later
    //binaryMap.bootLoader.sourceTag = sis_fw_data[0x200];
    //printf("sourceCode Tag: %x.\n", binaryMap.bootLoader.sourceTag);
    printf("Dump firmware binary information:\n");

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);

    printAddrData(sis_fw_data, "SourceCodeTag", 0x200, 6, true);
    printAddrData(sis_fw_data, "BinaryFileBuildTime", 0x206, 6, false);
    printAddrData(sis_fw_data, "SiSMark", 0x20c, 4, true);
    printAddrData(sis_fw_data, "CodeTag", 0x220, 4, false);
    printAddrData(sis_fw_data, "BootloaderVersion", 0x230, 4, false);
    printAddrData(sis_fw_data, "BootloaderCRC", 0x234, 4, false);
    //printAddrData(sis_fw_data, "SpecialUpdateFlag", 0x4000, 2, true);
    printAddrData(sis_fw_data, "CodeBaseTime", 0x4012, 4, false);
    //printAddrData(sis_fw_data, "FWUpdateTime", 0x40a0, 5, false);
    //printAddrData(sis_fw_data, "FWUpdateTool", 0x40a5, 2, true);
    //printAddrData(sis_fw_data, "LastUpdateTime", 0x1e000, 4, false);
	printAddrData(sis_fw_data, "BootFlag", 0x1eff0, 4, false);

    SetConsoleTextAttribute(hConsole, consoleInfo.wAttributes);

	if (0)
		return EXIT_ERR;
    return EXIT_OK;
}

int verifyFirmwareInfo(quint8 *sis_fw_data)
{
    quint32 timeStamp = getTimestamp();
    printf("Current time: %08x.\n", timeStamp);

    // Add time stamp in 0x1e000, 4bytes
    DateTime dateTime = getCurrentDateTime();

    // 將月、日、時、分分別存入 sis_fw_data 陣列的 0x1e000 開始的 4 bytes
    //sis_fw_data[0x1e000] = static_cast<quint8>(dateTime.year >> 8); // 年
    //sis_fw_data[0x1e001] = static_cast<quint8>(dateTime.year);      // 年
    sis_fw_data[0x1e000] = dateTime.month;
    sis_fw_data[0x1e001] = dateTime.day;
    sis_fw_data[0x1e002] = dateTime.hour;
    sis_fw_data[0x1e003] = dateTime.minute;

    printAddrData(sis_fw_data, "Write new update time to FW", 0x1e000, 4, false);

    //Special Update Flag : Update by serial port tool
    // quint8 *p = (quint8 *)(sis_fw_data + 0x4000);
    // *p++ = SERIAL_FLAG >> 8;
    // *p++ = SERIAL_FLAG & 0xff;
    sis_fw_data[0x4000] = SERIAL_FLAG >> 8;
    sis_fw_data[0x4001] = SERIAL_FLAG & 0xff;
    printAddrData(sis_fw_data, "Write new special update flag to FW", 0x4000, 2, true);

    if (0)
		return EXIT_ERR;

    return EXIT_OK;
}
#endif

void getUserInput() {
    char user_input;
	
    while (1) {
        printf("Continue the process? (Y/n): ");
        user_input = getchar();
        while (getchar() != '\n'); // 清除緩Clear other char in buffer
        if (user_input == 'y' || user_input == 'Y') {
            break;
        }
        else if (user_input == 'n' || user_input == 'N') {
            exit(0);
        }
    }
}

// writeData: 要下出去的Tx command
// timeout: 單位為ms毫秒
// count: 重式次數
int GRDebugFunc(QSerialPort& serial,
                const QByteArray& writeData,
                int timeout,
                int count)
{
    if (writeData.isEmpty()) {
        printf("No GR Uart switch command.\n");
        return EXIT_BADARGU;
    }

    const qint64 bytesWritten = serial.write(writeData);

    if (bytesWritten == -1) {
        printf("error: Can not send command to serial port.\n");
        return CT_EXIT_CHIP_COMMUNICATION_ERROR;
    } else if (bytesWritten != writeData.size()) {
        printf("error: Send command to serial port but interrupt.\n");
        return CT_EXIT_FAIL;
    } else if (!serial.waitForBytesWritten(1000)) { // 等待最多n毫秒(ms)，以確保資料已經成功地寫入串列埠
        printf("error: Serial port timeout.\n");
        return CT_EXIT_CHIP_COMMUNICATION_ERROR;
    }

    //msleep(10);
    QByteArray readData;
#if 1
    quint16 ackStatus;
    int retryCount = 0;
    bool isDataValid = false;
    while (!isDataValid && retryCount < count) {
        if (serial.waitForReadyRead(timeout)) {
            readData = serial.readAll();
#ifdef _CHAOBAN_TEST
            qDebug() << "read data size=" << readData.size();
            qDebug() << "read data =" << readData.toHex();
#endif
            unsigned char *rdata = (unsigned char *)readData.data();
            if(rdata[0] == 0x0e)
            {
                ackStatus = (rdata[BUF_ACK_MSB] << 8 | rdata[BUF_ACK_LSB]);
                if ((ackStatus == 0xbeef) || (ackStatus == 0xdead)) {
                    isDataValid = true;
                }
            }
/*
            if (readData.size() == 13 && readData.at(0) == 0x0e) {
            //if (readData.at(0) == GR_EVENT_ID) {
                ackStatus = readData.mid(9, 2); // 取出 byte 9 和 byte 10 的資料並接起來
                if (ackStatus == "\xbe\xef" || ackStatus == "\xde\xad") { // 判斷接起來的資料是否為 0xbeef 或 0xdead
                    isDataValid = true;
                }
            }
*/
         } else {
            // 超過 timeout 時，嘗試重新讀取的次數加 1
            retryCount++;
         }
    }
    if (isDataValid) {
#ifdef _CHAOBAN_TEST
        printf("ackStatus=%x\n", ackStatus);
#endif
        return ackStatus;
    } else {
        printf("GRDebugFunc timeout or get incorrect RX data.\n");
        return EXIT_FAIL;
    }
#else
    //QByteArray expectedData = QByteArray::fromRawData("\x0e\x0e\x09\x00\x05\x80\x00", 7);
    if (serial.waitForReadyRead(1000)) {
        readData = serial.readAll();
    }

    //qDebug() << readData.toHex();
    if (readData.size() == 7 &&
        readData.at(0) == GR_EVENT_ID &&
        readData.at(6) == SUCCESS) {
        return EXIT_OK;
    } else {
        printf("GRDebugFunc Failed\n");
        qDebug() << readData.toHex();
        return CT_EXIT_FAIL;
    }
#endif
    return EXIT_OK;
}

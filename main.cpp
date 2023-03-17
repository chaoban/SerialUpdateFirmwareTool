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
#include "DoFirmwareUpdate.h"
#include "SiSAdapter.h"
#include "version.h"
#include "ExitStatus.h"
//#pragma comment(lib, "Advapi32.lib")

DWORD WINAPI RcvWaitProc(LPVOID lpParamter);
const QStringList getComportRegKey();
int testSerialPort(QString *ComPortName);
int readBinary(QString path);
extern void print_sep();
extern int scanSerialport();
extern int getTimestamp();

int occupiedPortCount = 0;
int timeOutPortCount = 0;
bool mismatchKey = FALSE;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    //const int argumentCount = QCoreApplication::arguments().size();
    //const QStringList argumentList = QCoreApplication::arguments();
    QTextStream standardOutput(stdout);
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);

    QString ComPortName;
    QSerialPort serial; /* 開啟Serial Port用 */

    int exitCode = CT_EXIT_AP_FLOW_ERROR;
    bool bScanSeriaport = false;
    bool bAutoDetect = false;
    bool bAssignSerial = false;
    bool bUpdateBootloader = true; /* At present, 7501 must update the bootloader at the same time */
    bool bForceUpdate = false;
    QString filename = "fw.bin";
    int wait_time = 0;

    printVersion();

    int mmddHHMM = getTimestamp();
    printf("Time Stamp: %08x\n", mmddHHMM);

    print_sep();

    /* 
     * PARSE AND GET COMMAND ARGUMENTS 
     */
    if (a.arguments().contains("-b")) {
        bUpdateBootloader = true;
        printf("Argument: update bootloader\n");
    }
    if (a.arguments().contains("--force")) {
        bForceUpdate = true;
        printf("Argument: force update\n");
    }
    if (a.arguments().contains("-s")) {
        bScanSeriaport = true;
        printf("Argument: Scan All serial ports\n");
    }

    /*
     * -a: 自動測試並取得連接SIS Device的Serial Port
     * -c: 使用者指定Serial Port
     *     如果使用者指定Serial Port，就不會再做自動測試
    */
    if (a.arguments().contains("-c")) {
        bAutoDetect = false;
        bAssignSerial = true;
        int index = a.arguments().indexOf("-c");
        if (index + 1 < argc) {
            ComPortName = argv[index + 1];
        }
        printf("Argument: Assign the %s port to update\n", ComPortName.toStdString().c_str());
    } else if (a.arguments().contains("-a")) {
        bAutoDetect = true;
        bAssignSerial = false;
        printf("Argument: Auto test all serial ports that connect to SiS device\n");
    }
    if (a.arguments().contains("-f")) {
        int index = a.arguments().indexOf("-f");
        if (index + 1 < argc) {
            filename = argv[index + 1];
        }
        printf("Argument: firmware file name: %s\n", filename.toStdString().c_str());
    }
    if (a.arguments().contains("-ba")) {
        //bUpdateBootloader_auto = true;    //TODO
        printf("Argument: update bootloader automatically\n");
    }
    if (a.arguments().contains("-g")) {
        //reserve_RODATA = true;            //TODO
        printf("Argument: reserve RO data\n");
    }
    if (a.arguments().contains("-r")) {
        //update_parameter = true;          //TODO
        printf("Argument: only update parameter\n");
    }
    if (a.arguments().contains("--jump")) {
        //jump_check = true;                //TODO
        printf("Argument: jump parameter validation\n");
    }
    if (a.arguments().contains("-w=")) {
        //wait_time = atoi(arg + 3);        //TODO
        printf("Argument: wait time set: %d\n", wait_time);
    }

    print_sep();

    /* Scan and list all available serial ports */
    if (bScanSeriaport) 
        scanSerialport();
    
    /* Auto get available serial ports that connect to  SIS Device*/
    if (bAutoDetect) 
        exitCode = testSerialPort(&ComPortName);
    
    /*
     * OPEN SIS UART COMM PORT
     */
    if ((bAutoDetect) || (bAssignSerial)) {
        if (ComPortName.size()>4) {
            QString tmp = "\\\\.\\";
            tmp.append(ComPortName);
            ComPortName = tmp;
        }

        qDebug() << "Open SiS" << ComPortName << "port";
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
            standardOutput << QObject::tr("Failed to open port %1, error: %2")
                              .arg(ComPortName, serial.errorString())
                           << Qt::endl;
            return CT_EXIT_NO_COMPORT;
        }

        printf("Open %s successfully.\n", ComPortName.toStdString().c_str());
    }
    else {
        printf("Do not have Open any Serial port, please see the HELP\n");
        return EXIT_ERR;
    }

    /*
     * OPEN LOCAL FIRMWARE BIN FILE
     */
#if 1
    quint8 *sis_fw_data;
    long file_size;

    FILE* fp = fopen(filename.toStdString().c_str(), "rb");
    if (!fp) {
        printf("Failed to open the file: %s \n", filename.toStdString().c_str());
        return 1;
    }

    printf("Open the file: %s success\n", filename.toStdString().c_str());

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    if ((sis_fw_data = (quint8*)malloc(sizeof(quint8) * file_size)) == NULL) {
        printf("Failed to allocate memory\n");
        return 1;
    }

    if (fread(sis_fw_data, sizeof(char), (size_t)file_size, fp) != (size_t)file_size) {
        printf("Failed to read file\n");
        return 1;
    }
    fclose(fp);

#else
    exitCode = readBinary("FW.BIN");
    if (exitCode) {
        printf("Load Firmware Bin File Fails.\n");
        exitCode = EXIT_ERR;
        return exitCode;
    }
#endif
    print_sep();

    //TODO
    /* Here we can disable GR Uart Debug message */
    

    /* UPDATE Firmware */
    qDebug() << "Start Update Firmware by"  <<  serial.portName() << "port";
    exitCode = sisUpdateFlow(&serial, sis_fw_data,
                             bUpdateBootloader, 
                             bForceUpdate);

    print_sep();

    printf("\nExit code : %d\n", exitCode);
    if (exitCode == EXIT_SUCCESS) {
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
        printf("Update Firmware Success\n");
    } else {
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
        printf("Update Firmware Failed\n");
    }

    SetConsoleTextAttribute(hConsole, consoleInfo.wAttributes);

    free(sis_fw_data);

    if (serial.isOpen()) {
        //TODO
        /* Here we can enable GR Uart Debug message */
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
            printf("%s, ",QString::fromUtf16( (ushort*)szData ).toStdString().c_str() );
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
            printf("Open %s failed\t--> Warning : this com port was occupied or no longer available\n", curPortName.toStdString().c_str());
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
            printf("Warning : receiver timeout\n");
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
            printf("verifying key mismatched\n");
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
        printf("(Open com port fail, some com ports were occupied)\n");
    }

    if (mismatchKey == TRUE) {
        printf("(Verifying key mismatch)\n");
    }

    if (timeOutPortCount>0) {
        printf("(UART RX no response)\n");
    }

    return CT_EXIT_FAIL;
}

#if 0
unsigned char * fn; /* 讀取韌體檔案用 */
QByteArray FirmwareString;

int readBinary(QString path)
{
    char file_data;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Could not open bin file for reading";
        return -1;
    }

    while(!file.atEnd())
    {
      // return value from 'file.read' should always be sizeof(char).
      file.read(&file_data,sizeof(char));
      //TODO: 要轉換格式
      FirmwareString.append(file_data);
    }
    file.close();

    printf("Firmware data being read are %i bytes\n", FirmwareString.length());

    fn = (unsigned char *)FirmwareString.data();
    
    return EXIT_OK;
}
#endif

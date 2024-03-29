﻿#ifndef SIS_COMMAND_H
#define SIS_COMMAND_H

#define MAX_BYTE                 64

//SIS UPDATE FW
typedef int ram_size_t;
#define RAM_SIZE                ((ram_size_t)_12K)
#define PACK_SIZE                52

//SIS OP CODE
#define BIT_SISID                 0
#define BIT_CRC                   1
#define BIT_CMD                   2
#define BIT_PALD                  3

//SIS CMD DEFINE
#define CMD_SISFLASH           0x81
#define CMD_SISRESET           0x82
#define CMD_SISUPDATE          0x83
#define CMD_SISWRITE           0x84
#define CMD_SISXMODE           0x85
#define CMD_SISREAD            0x86
#define CMD_SISBRIDGE          0x01

#define CMD_SZ_FLASH           0x03 /* Cmd 0x81's Size */
#define CMD_SZ_RESET           0x03 /* Cmd 0x82's Size */
#define CMD_SZ_UPDATE          0x09 /* Cmd 0x83's Size */
#define CMD_SZ_WRITE           0x3B /* Cmd 0x84's Size */
#define CMD_SZ_XMODE           0x05 /* Cmd 0x85's Size */
#define CMD_SZ_READ            0x09 /* Cmd 0x86's Size */
#define CMD_SZ_BRIDGE          0x05 /* Cmd 0x01's Size */

#define BIT_RX_READ			   13				

//#define R_MAX_SIZE              0x34	/* TODO: 讀取長度為52bytes時，QT Serial有未知問題，讀不到資料 */
#define R_MAX_SIZE              0x30 

//FW ADDRESS
#define ADDR_FW_INFO            0xA0004000
#define ADDR_MAIN_CRC           0xA0004044
#define ADDR_FWUPDATE_INFO      0xA00040A0
#define ADDR_BOOT_FLAG			0xA001EFF0
#define ADDR_BOOTLOADER_ID		0xA0000230
#define ADDR_PKGID              0xA001F000
#define BIN_PKGID               0x4050

#define SIS_REPORTID            0x09
#define BUF_ACK_LSB             0x09
#define BUF_ACK_MSB             0x0A
#define BUF_ACK_L				0xEF
#define BUF_ACK_H				0xBE
#define BUF_NACK_L				0xAD
#define BUF_NACK_H				0xDE

#define BUF_PAYLOAD_PLACE       8
#define INT_OUT_REPORT          0x09

#define SIS_BOOTFLAG_P810       0x50383130
/* Special Update Flag : 7501 Update by Uart */
#define UPDATE_MARK             0x7501
#define NO_UPDATE_BOOT			0x6e62 //nb
#define IS_UPDATE_BOOT			0x7562 //ub


//GR UART CMD ID
#define GR_CMD_ID               0x12
#define GR_EVENT_ID             0x0E

#define SUCCESS                 0x0
#define FAIL                    0x1

//GR OP CODE
#define BIT_UART_ID             0
#define BIT_OP_LSB              1
#define BIT_OP_MSB              2
#define BIT_SIZE_LSB            3
#define BIT_SIZE_MSB            4

#define GR_OP                   0x80
#define GR_OP_INIT              0x01
#define GR_OP_W                 0x02
#define GR_OP_R                 0x03
#define GR_OP_WR                0x04
#define GR_OP_DISABLE_DEBUG     0x05
#define GR_OP_ENABLE_DEBUG      0x06
#endif // SIS_COMMAND_H

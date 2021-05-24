///*****************************************
//  Copyright (C) 2009-2021
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <IT6664_IO.h>
//   @author Hojim.Tseng@ite.com.tw
//   @date   2021/01/12
//   @fileversion: ITE_HDMI2_SPLITER_1.38
//******************************************/
#ifndef _IT6664_IO_H_
#define _IT6664_IO_H_
#include "IT6664_Typedef.h"
#include "IT6664_Config.h"
//#ifdef _MCU_IT6350
//#include "..\IO_IT6350.h"
//#else
//#include "..\IT6295_IO.h"
//#endif

#if (USING_1to8==TRUE)
#define SWAddr          0x58
#define RXP0Addr        0x70
#define RXEDID_Addr     0xD8
#define RXEDIDAddr      0xa8
#define RXMHLAddr       0xe0
#define RXCECAddr		0xC0
#define TXComAddr       0x96
#define TXP0Addr        0x60
#define TXP1Addr        0x64
#define TXP2Addr        0x68
#define TXP3Addr        0x6C
#else
#define SWAddr          0x58
#define RXP0Addr        0x70
#define RXEDID_Addr     0xD8
#define RXEDIDAddr      0xa8
#define RXMHLAddr       0xe0
#define RXCECAddr		0xC0
#define TXComAddr       0x96
#define TXP0Addr        0x68
#define TXP1Addr        0x6a
#define TXP2Addr        0x6c
#define TXP3Addr        0x6e
#endif

void IT6662_DeviceSelect(iTE_u8 u8Device);
void iTE_I2C_ReadBurst(iTE_u8 u8Adr,iTE_u8 u8Offset,iTE_u8 u8Cnt,iTE_u8* pu8Buf);
void iTE_I2C_WriteBurst(iTE_u8 u8Adr,iTE_u8 u8Offset,iTE_u8 u8Cnt,iTE_u8* pu8Buf);
void iTE_I2C_WriteByte(iTE_u8 u8Adr, iTE_u8 u8Offset, iTE_u8 u8Data);
iTE_u8 iTE_I2C_ReadByte(iTE_u8 u8Adr, iTE_u8 u8Offset);
void iTE_I2C_SetByte(iTE_u8 u8Adr,iTE_u8 u8Offset,iTE_u8 u8InvMask, iTE_u8 u8Data );
void iTE_I2C_ReadBurstP(iTE_u8 u8Adr,iTE_u8 port,iTE_u8 u8Offset,iTE_u8 u8Cnt,iTE_u8* pu8Buf);
void iTE_I2C_SetByteP(iTE_u8 u8Adr,iTE_u8 port,iTE_u8 u8Offset,iTE_u8 u8InvMask, iTE_u8 u8Data );
void iTE_I2C_WriteByteP(iTE_u8 u8Adr,iTE_u8 port, iTE_u8 u8Offset, iTE_u8 u8Data);
iTE_u8 iTE_I2C_ReadByteP(iTE_u8 u8Adr,iTE_u8 port,iTE_u8 u8Offset);




#endif

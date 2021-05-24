///*****************************************
//  Copyright (C) 2009-2021
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <IT6664_IO.c>
//   @author Hojim.Tseng@ite.com.tw
//   @date   2021/01/12
//   @fileversion: ITE_HDMI2_SPLITER_1.38
//******************************************/
#include "IT6664_EDID.h"
#include "IT6664_IO.h"
#include "IT6664_extern.h"


#define USING_1TO8 0

#define I2C_SMBusE 0

iTE_u8 g_device;

#if (USING_1to8==TRUE)
iTE_u8 port_offset = 4;
#else
iTE_u8 port_offset = 2;
#endif


#if (USING_1to8==FALSE)
#ifdef _MCU_IT6295_
iTE_u8 u8I2CBus = 0;
#else
iTE_u8 u8I2CBus = I2C_SMBusE;
#endif
iTE_u8 u8I2CAdr = 0;
extern_variables ar_gext;
extern_u8	ar_gu8;
it6664_tx ar_txmem;
extern_32 ar_gu32;
extern_variables *gext_var;
extern_u8 *gext_u8;
it6664_tx *gmem_tx;
extern_32 *gext_long;
struct PARSE3D_STR	st3DParse;

void IT6664_DeviceSelect(iTE_u8 u8Device)
{
	g_device = IT6663_C;//normal it6664/it6663  use this device name , only 1to8  use  IT6664_B/IT6664_A/IT6663_C to make a distinction
	gext_var = &ar_gext;
	gext_u8 = &ar_gu8;
	gmem_tx = &ar_txmem;
	gext_long = &ar_gu32;
}
#else
extern_variables ar_gext[3];
extern_u8	ar_gu8[3];
it6664_tx ar_txmem[3];
extern_32 ar_gu32[3];
extern_variables *gext_var;
extern_u8 *gext_u8;
it6664_tx *gmem_tx;
extern_32 *gext_long;
struct PARSE3D_STR	st3DParse;

#ifdef _MCU_IT6295_
iTE_u8 u8I2CBus = 0;
#else
iTE_u8 u8I2CBus = I2C_SMBusE;
#endif
iTE_u8 u8I2CAdr = 0;

void IT6664_DeviceSelect(iTE_u8 u8Device)
{
	//iTE_u32 tmp;
	u8I2CBus = u8Device & 0xF;
	u8I2CAdr = u8Device >> 4;
	if(u8Device == IT6664_A)
	{
		g_device = IT6664_A;
		gext_var = &ar_gext[0];
		gext_u8 = &ar_gu8[0];
		gmem_tx = &ar_txmem[0];
		gext_long = &ar_gu32[0];
	}
	else if(u8Device == IT6664_B)
	{
		g_device = IT6664_B;
		gext_var = &ar_gext[1];
		gext_u8 = &ar_gu8[1];
		gmem_tx = &ar_txmem[1];
		gext_long = &ar_gu32[1];
	}
	else if(u8Device == IT6663_C)
	{
		g_device = IT6663_C;
		gext_var = &ar_gext[2];
		gext_u8 = &ar_gu8[2];
		gmem_tx = &ar_txmem[2];
		gext_long = &ar_gu32[2];
	}

}
#endif
void iTE_I2C_ReadBurst(iTE_u8 u8Adr,iTE_u8 u8Offset,iTE_u8 u8Cnt,iTE_u8* pu8Buf)
{
	if(i2c_read_byte( u8Adr + u8I2CAdr, u8Offset, u8Cnt, pu8Buf, u8I2CBus ) == 0){
		//extern iTE_u8 CurSysStatus;
		//CurSysStatus = 0;
	}
}
void iTE_I2C_WriteBurst(iTE_u8 u8Adr,iTE_u8 u8Offset,iTE_u8 u8Cnt,iTE_u8* pu8Buf)
{
#if 0
	return i2c_write_byte( u8Adr + u8I2CAdr, u8Offset, u8Cnt, pu8Buf, u8I2CBus );
#else
	//iTE_u8 i;
	//for(i=0;i<u8Cnt;i++) iTE_Msg(("w %02x %02x %02x \r\n",(iTE_u16)u8Offset,(iTE_u16)*pu8Buf++,(iTE_u16)u8Adr));

	if(i2c_write_byte( u8Adr + u8I2CAdr, u8Offset, u8Cnt, pu8Buf, u8I2CBus ) == 0){
		//extern iTE_u8 CurSysStatus;
		//CurSysStatus = 0;
	}
#endif
}
iTE_u8 iTE_I2C_ReadByte(iTE_u8 u8Adr, iTE_u8 u8Offset)
{
	iTE_u8	u8RdData;
	if(i2c_read_byte( u8Adr + u8I2CAdr, u8Offset, 1, &u8RdData, u8I2CBus ) == 0){
		//extern iTE_u8 CurSysStatus;
		//CurSysStatus = 0;
	}
	return u8RdData;
}
void iTE_I2C_WriteByte(iTE_u8 u8Adr, iTE_u8 u8Offset, iTE_u8 u8Data)
{
	//iTE_Msg(("w %02x %02x %02x \r\n",(iTE_u16)u8Offset,(iTE_u16)u8Data,(iTE_u16)u8Adr));
	if(i2c_write_byte( u8Adr + u8I2CAdr, u8Offset, 1, &u8Data, u8I2CBus ) == 0){
		//extern iTE_u8 CurSysStatus;
		//CurSysStatus = 0;
	}
}
void iTE_I2C_SetByte(iTE_u8 u8Adr,iTE_u8 u8Offset,iTE_u8 u8InvMask, iTE_u8 u8Data )
{
	iTE_u8	u8RdData;
	if(u8InvMask){
		if(~u8InvMask){
			if(i2c_read_byte( u8Adr + u8I2CAdr, u8Offset, 1, &u8RdData, u8I2CBus ) == 0){
				//extern iTE_u8 CurSysStatus;
				//CurSysStatus = 0;
			}
			u8RdData &= ~u8InvMask;
			u8Data &= u8InvMask;
			u8Data |= u8RdData;
		}
		//iTE_Msg(("w %02x %02x %02x \r\n",(iTE_u16)u8Offset,(iTE_u16)u8Data,(iTE_u16)(u8Adr + u8I2CAdr)));
		if(i2c_write_byte( u8Adr + u8I2CAdr, u8Offset, 1, &u8Data, u8I2CBus ) == 0){
			//extern iTE_u8 CurSysStatus;
			//CurSysStatus = 0;
		}
	}else{
		if(u8Offset){
			// ToDo for other feature
		}else{
			//usleep(u8Data);
		}
	}
}
iTE_u8 iTE_I2C_ReadByteP(iTE_u8 u8Adr,iTE_u8 port,iTE_u8 u8Offset)
{
	iTE_u8	u8RdData;
	u8Adr += port*port_offset;
	if(i2c_read_byte( u8Adr + u8I2CAdr, u8Offset, 1, &u8RdData, u8I2CBus ) == 0){
		//extern iTE_u8 CurSysStatus;
		//CurSysStatus = 0;
	}
	return u8RdData;
}
void iTE_I2C_WriteByteP(iTE_u8 u8Adr,iTE_u8 port, iTE_u8 u8Offset, iTE_u8 u8Data)
{
	//iTE_Msg(("w %02x %02x %02x \r\n",(iTE_u16)u8Offset,(iTE_u16)u8Data,(iTE_u16)u8Adr));
	u8Adr += port*port_offset;
	if(i2c_write_byte( u8Adr + u8I2CAdr, u8Offset, 1, &u8Data, u8I2CBus ) == 0){
		//extern iTE_u8 CurSysStatus;
		//CurSysStatus = 0;
	}
}
void iTE_I2C_SetByteP(iTE_u8 u8Adr,iTE_u8 port,iTE_u8 u8Offset,iTE_u8 u8InvMask, iTE_u8 u8Data )
{
	iTE_u8	u8RdData;
	u8Adr += port*port_offset;
	if(u8InvMask){
		if(~u8InvMask){
			if(i2c_read_byte( u8Adr + u8I2CAdr, u8Offset, 1, &u8RdData, u8I2CBus ) == 0){
				//extern iTE_u8 CurSysStatus;
				//CurSysStatus = 0;
			}
			u8RdData &= ~u8InvMask;
			u8Data &= u8InvMask;
			u8Data |= u8RdData;
		}
		//iTE_Msg(("w %02x %02x %02x \r\n",(iTE_u16)u8Offset,(iTE_u16)u8Data,(iTE_u16)u8Adr));

		if(i2c_write_byte( u8Adr + u8I2CAdr, u8Offset, 1, &u8Data, u8I2CBus ) == 0){
			//extern iTE_u8 CurSysStatus;
			//CurSysStatus = 0;
		}
	}else{
		if(u8Offset){
			// ToDo for other feature
		}else{
			//mSleep(u8Data);
		}
	}
}
void iTE_I2C_ReadBurstP(iTE_u8 u8Adr,iTE_u8 port,iTE_u8 u8Offset,iTE_u8 u8Cnt,iTE_u8* pu8Buf)
{
	u8Adr += port*port_offset;
	if(i2c_read_byte( u8Adr + u8I2CAdr, u8Offset, u8Cnt, pu8Buf, u8I2CBus ) == 0){
		//extern iTE_u8 CurSysStatus;
		//CurSysStatus = 0;
	}
}
void iTE_I2C_WriteBurstP(iTE_u8 u8Adr,iTE_u8 port,iTE_u8 u8Offset,iTE_u8 u8Cnt,iTE_u8* pu8Buf)
{
	u8Adr += port*2;
#if 0
	return i2c_write_byte( u8Adr + u8I2CAdr, u8Offset, u8Cnt, pu8Buf, u8I2CBus );
#else
	if(i2c_write_byte( u8Adr + u8I2CAdr, u8Offset, u8Cnt, pu8Buf, u8I2CBus ) == 0){
		//extern iTE_u8 CurSysStatus;
		//CurSysStatus = 0;
	}
#endif
}


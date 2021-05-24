///*****************************************
//  Copyright (C) 2009-2021
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <IT6664_EDID.h>
//   @author Hojim.Tseng@ite.com.tw
//   @date   2021/01/12
//   @fileversion: ITE_HDMI2_SPLITER_1.38
//******************************************/
#ifndef _IT6664_EDID_H_
#define _IT6664_EDID_H_
//#include "debug_print.h"
#include "IT6664_Typedef.h"
#include "IT6664_Config.h"
#include "IT6664_extern.h"

typedef struct
{
	iTE_u8 EstablishTimeing[3];
	iTE_u8 DTDsupport[2];
	iTE_u8 STD[8][2];
	iTE_u8 block1;
}EDIDBlock0;
typedef struct
{
	iTE_u8 Audiodes;
	iTE_u8 Format[21];
}Audio_data;
typedef struct
{
	iTE_u8 Viccnt;
	iTE_u8 vic[50];
}Video_data;
typedef struct
{
	iTE_u8 SCDCPresent;
	iTE_u8 RR_Capable;
	iTE_u8 LTE_340M_scramble;
	iTE_u8 IndependentView;
	iTE_u8 DualView;
	iTE_u8 OSD_3D;
	iTE_u8 DC_420;
	iTE_u8 Byte1;
	iTE_u8 Byte2;
}VSDB2_content;
typedef struct
{
	iTE_u8 phyaddr0;
	iTE_u8 phyaddr1;
	iTE_u8 DCSupport;
	iTE_u8 Vdsb8_CNandPresent;
	iTE_u8 VideoLatency;
	iTE_u8 AudioLatency;
	iTE_u8 I_VideoLatency;
	iTE_u8 I_AudioLatency;
	iTE_u8 HDMI_VicLen;
	iTE_u8 HDMI_3DLen;
	iTE_u8 HDMIVic[4];
	iTE_u8 HDMI3D_Present;
	iTE_u8 Support3D;
	iTE_u8 _3D_Struct_All_15_8;
	iTE_u8 _3D_Struct_All_7_0;
	iTE_u8	_3D_Vic_SupCnt;
	iTE_u8 _3D_Vic_Sup[16];
	iTE_u8 _3D_Order_len;
	iTE_u16 _3D_Order[16];
}VSDB1_content;
typedef struct
{
	Audio_data audio_info;
	Video_data video_info;
	iTE_u8 speaker_info;
	VSDB2_content vsdb2;
	VSDB1_content vsdb1;
	iTE_u8 HDR_Content[2];
}EDIDBlock1;
typedef enum _CEA_TAG
{
	CEA_TAG_NONE 	= 0,
	CEA_TAG_ADB		= 1,
	CEA_TAG_VDB		= 2,
	CEA_TAG_VSDB	= 3,
	CEA_MODE_SPK	= 4,
	CEA_MODE_DTC	= 5,
	CEA_MODE_REV	= 6,
	CEA_MODE_EXT	= 7
}CEA_TAG;

enum
{
	Default_EDID2k=0x00,
	Default_EDID4k=0xA0,
	Copy_Mode=0x80,
	Compose_Mode=0x20,
	Compose_Mode_Int = 0xFF,
};
void Show_EDID(iTE_pu8 ptr);
iTE_u8 CalChecksum(iTE_u8 edid[],iTE_u8 block);
iTE_u8 CheckHPDCnt(void);
void it6664_block0_compose(iTE_pu8 arry);
void it6664_block1_compose(iTE_pu8 arry);
void it6664_block0_compose_int(iTE_pu8 arry);
void it6664_block1_compose_int(iTE_pu8 arry);
iTE_u8 it6664_Edid_Parse(iTE_u8 HPDsts);
void it6664_EdidMode_Switch(void);
void it6664_Edid_Compose(void);
void it6664_Edid_Copy(iTE_u8 hpdsts);
void it6664_Edid_DataInit(iTE_u8 port);
void it6664_Audio_Parse(iTE_u8 port,iTE_u8 table[],iTE_u8 offset);
void it6664_Video_Parse(iTE_u8 port,iTE_u8 table[],iTE_u8 offset);
void it6664_Vsdb1_Parse(iTE_u8 port,iTE_u8 table[],iTE_u8 offset);
void it6664_Vsdb2_Parse(iTE_u8 port,iTE_u8 table[],iTE_u8 offset);
void it6664_EXT_Parse(iTE_u8 port,iTE_u8 table[],iTE_u8 offset);
iTE_u8 it6664_Edid_block1_parse(iTE_u8 port,iTE_u8 tmp[]);
iTE_u8 it6664_Edid_block0_parse(iTE_u8 port,iTE_u8 tmp[]);
iTE_u8 it6664_Check_HPDsts(void);
iTE_u8 it6664_DTD2VIC(iTE_u8 table[]);
iTE_u8 it6664_read_edid(iTE_u8 port, iTE_u8 block, iTE_u8 offset, iTE_u16 length, iTE_u8 *edid_buffer);
iTE_u8 it6664_read_one_block_edid(iTE_u8 port, iTE_u8 block, iTE_u8 *edid_buffer);
iTE_u8 it6664_ComposeEDIDBlock1_VIC(iTE_pu8 ptr,iTE_u8 offset,iTE_u8 sup_vsdb2);
iTE_u8 it6664_ComposeEDIDBlock1_Audio(iTE_pu8 ptr,iTE_u8 offset);
iTE_u8 it6664_ComposeEDIDBlock1_Speaker(iTE_pu8 ptr,iTE_u8 offset);
iTE_u8 it6664_ComposeEDIDBlock1_VSDB1(iTE_pu8 ptr,iTE_u8 offset);
iTE_u8 it6664_ComposeEDIDBlock1_VSDB2(iTE_pu8 ptr,iTE_u8 offset);
iTE_u8 it6664_ComposeEDIDBlock1_ExtTag(iTE_pu8 ptr,iTE_u8 offset , iTE_u8 tag);
iTE_u8 it6664_ComposeEDIDBlock1_VIC_int(iTE_pu8 ptr,iTE_u8 offset,iTE_u8 sup_vsdb2);
iTE_u8 it6664_ComposeEDIDBlock1_Audio_int(iTE_pu8 ptr,iTE_u8 offset);
iTE_u8 it6664_ComposeEDIDBlock1_Speaker_int(iTE_pu8 ptr,iTE_u8 offset);
iTE_u8 it6664_ComposeEDIDBlock1_VSDB1_int(iTE_pu8 ptr,iTE_u8 offset);
iTE_u8 it6664_ComposeEDIDBlock1_VSDB2_int(iTE_pu8 ptr,iTE_u8 offset);
iTE_u8 it6664_ComposeEDIDBlock1_ExtTag_int(iTE_pu8 ptr,iTE_u8 offset , iTE_u8 tag);
iTE_u8 it6664_ComposeEDIDBlock1_DTD(iTE_pu8 ptr,iTE_u8 offset);
iTE_u8 CompareProductID(void);
iTE_u8 CompareResolution(iTE_u8 ref);
iTE_u8 it6664_EDIDCompare(iTE_pu8 arry1,iTE_pu8 arry2);
void EDID_48Bit_Remove(iTE_pu8 arry1);
void it6664_Edid_PhyABCD_Update(iTE_pu8 arry1);
void it6664_Edid_Compose_Int(void);
void it6664_Edid_Chk_4KDTD(iTE_pu8 arry1 ,iTE_u8 block);
void EDID_Remove4K533(iTE_pu8 arry1 ,iTE_u8 block);
void EDID_HDMI21_Remove(iTE_pu8 arry1);
void EDID_HDMI21_VicRemove(iTE_pu8 arry1);



#define Edid_Wb(u8Offset, u8Count, pu8Data)	iTE_I2C_WriteBurst(RXEDID_Addr, u8Offset, u8Count, pu8Data)
#define Edid_Rb(u8Offset, u8Count, pu8Data)	iTE_I2C_ReadBurst(RXEDID_Addr, u8Offset, u8Count, pu8Data)
#define Edid_Set(u8Offset, u8InvMask, u8Data)	iTE_I2C_SetByte(RXEDID_Addr, u8Offset, u8InvMask, u8Data)
#define Edid_W(u8Offset, u8Data)				iTE_I2C_WriteByte(RXEDID_Addr, u8Offset, u8Data)
#define Edid_R(u8Offset)						iTE_I2C_ReadByte(RXEDID_Addr, u8Offset)


#endif

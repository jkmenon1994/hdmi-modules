///*****************************************
//  Copyright (C) 2009-2021
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <IT6664_hdmi2_rx.h>
//   @author Hojim.Tseng@ite.com.tw
//   @date   2021/01/12
//   @fileversion: ITE_HDMI2_SPLITER_1.38
//******************************************/
#ifndef _IT6664_HDMI2_RX_H_
#define _IT6664_HDMI2_RX_H_

#include "IT6664_extern.h"
#include "IT6664_DefaultEDID.h"


#define EnFWDSFmt 					FALSE
#define RXReportAutoEQ				TRUE
#define RXReportBitErr				TRUE
#define EnCDSetTX                   FALSE
#define EnBlockHDMIMode				FALSE
#define EnSCDTOffResetRX			FALSE


typedef enum AutoEQ_type
{
	STAT_EQ_WaitInt = 0,
	STAT_EQ_rst,
	STAT_EQ_Start,
	STAT_EQ_WaitSarEQDone,
	STAT_EQ_CheckBiterr,
	STAT_EQ_AutoEQDone
}AutoEQstate;
typedef enum _PARSE3D_STA{
    PARSE3D_START,
    PARSE3D_LEN,
    PARSE3D_STRUCT_H,
    PARSE3D_STRUCT_L,
    PARSE3D_MASK_H,
    PARSE3D_MASK_L,
    PARSE3D_VIC,
    PARSE3D_DONE
}PARSE3D_STA;



//void h2rx_pwron(iTE_u8 port);
void it6664_h2rx_pwdon(void);
void h2rx_irq(void);
void get_vid_info(void);
void DefaultEdidSet(void);
void SetCscConvert(void);
void Check_BitErr(void);
void SetRxHpd(iTE_u8 sts);
void it6664_hdmirx(void);
void  Dump_IT666xReg(void);
void it6664_RXHDCP_OFF(iTE_u8 sts);
void it6664_SetFixEQ(iTE_u8 val);
void Set_DNScale( VTiming pCurVTiming );
void RX_FiFoRst(void);
void EDID_ParseVSDB_3Dblock(struct PARSE3D_STR *pstParse3D);
void IT6664_HDRPacketGet(void);
void it6664_RXHDCP_Set(iTE_u8 sts);

#ifdef AutoEQ
void it6664_Set_SAREQ(iTE_u8 SKEWOPT);
void it6664_SigleRSSKEW(void);
iTE_u8 it6664_RPTSAREQ(iTE_u8 SKEWOPT,iTE_u8 HDMI_mode);
void it6664_Read_SKEW(void);
iTE_u8 it6664_RptBitErr_ms(iTE_u8 Threshold);
void it6664_AutoEQ_State(void);
void Check_AMP(iTE_u8 Rec_Channel);
void it6664_ManuEQ(iTE_u8 mode);
void it6664_EQ14(iTE_u8 val);
void it6664_EQRst(void);
void it6664_EQchkOldResult(iTE_u8 hdmiver);

#endif



#endif

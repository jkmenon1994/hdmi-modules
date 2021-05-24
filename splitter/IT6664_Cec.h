///*****************************************
//  Copyright (C) 2009-2021
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <IT6664_Cec.h>
//   @author Hojim.Tseng@ite.com.tw
//   @date   2021/01/12
//   @fileversion: ITE_HDMI2_SPLITER_1.38
//******************************************/
///*****************************************
//  Copyright (C) 2009-2014
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <Cec.h>
//   @author Hanming.Cheng@ite.com.tw
//   @date   2014/12/17
//   @fileversion: ITE_SPLITER_1.01
//******************************************/
#ifndef _CEC_H_
#define _CEC_H_

#include "IT6664_IO.h"
#include "IT6664_CEC_typedef.h"
#include "IT6664_extern.h"


extern iTE_u8	g_u8CecSel;
extern iTE_u8 ucMyselfAddr;

#define CEC_RX_SELF_DIS				(1)
#define CEC_RST						(0)
#define CEC_NACK_EN					(0)
#define CEC_CAL_CNT					(1)
#define CEC_RESEND_MAX				(5)

#define IT6664_RX_LA				(DEVICE_ID_TV)
#define IT6664_TX_LA				(DEVICE_ID_FREEUSE)//(DEVICE_ID_TV)//(DEVICE_ID_FREEUSE)

#define IT6664_RX_TYPE				(CEC_DEV_TYPE_TV)
#define IT6664_TX_TYPE				(CEC_DEV_TYPE_VIDEO_PROCESSOR)//(CEC_DEV_TYPE_TV)//(CEC_DEV_TYPE_VIDEO_PROCESSOR)

#define CEC_DEV_VENDOR_ID_0		(0x00)
#define CEC_DEV_VENDOR_ID_1		(0x00)
#define CEC_DEV_VENDOR_ID_2		(0x00)

#define CEC_RX_QUEUE_SIZE	(0x01 << 6)
#define CEC_RX_QUEUE_MAX	(CEC_RX_QUEUE_SIZE - 1)
#define CEC_TX_QUEUE_SIZE	(0x01 << 6)
#define CEC_TX_QUEUE_MAX	(CEC_TX_QUEUE_SIZE - 1)

typedef struct _CEC_TX_QUEUE{
	iTE_u8	u8Rptr;
	iTE_u8	u8Wptr;
	iTE_u8	pu8Q[CEC_RX_QUEUE_SIZE];
}stCecTxQ;
typedef struct _CEC_RX_QUEUE{
	iTE_u8	u8Rptr;
	iTE_u8	u8Wptr;
	iTE_u8	pu8Q[CEC_RX_QUEUE_SIZE];
}stCecRxQ;

typedef struct _CEC_VARIABLE_{
	iTE_u8	u8MyLogAdr;
	iTE_u8	u8CecLA[4];
	iTE_u8	u8PaL, u8PaH;
	stCecRxQ	stRxQ;
	stCecTxQ		stTxQ;
	iTE_u8	u8RxTmpHeader[3];
	CEC_FRAME	stCecRxCmd, stCecTxCmd;
	iTE_u1	bTxQFull;
	iTE_u1	bTxCecDone;
	iTE_u8	u8TxCecFire;
	iTE_u8	u8TxCecInitDone;
}stCecVar;


void Cec_VarInit(iTE_u8 u8PaH, iTE_u8 u8PaL, iTE_u8 u8MyLogAdr);
void Cec_Init(iTE_u8 timerunit,iTE_u8 port);
void Cec_Irq(iTE_u8 port);
iTE_u1 Cec_FireStatus(iTE_u1 bTxDone);
iTE_u8 Cec_SwitchLA(iTE_u8 u8MyLA);
void Cec_RxFifoReset(void);

iTE_u1 Cec_RxCmdGet(void);
iTE_u1 Cec_RxCmdPush(iTE_pu8 pu8Header);
iTE_u1 Cec_RxCmdPull(void);
void Cec_TxFire(void);
void Cec_TxPolling(iTE_u8 u8LogicalAdr);
void Cec_TxFeatureAbort(iTE_u8 CecRxCmd, CecAbortReson eAbortReson);
void Cec_TxPollingMsg(iTE_u8 u8TxSel);
void Cec_TxReportPA(void);
void Cec_TxCmdPush(iTE_u8 ucFollower, eCEC_CMD TxCmd, iTE_u8 u8CecSize);
iTE_u1 Cec_TxCmdPull(void);
void Cec_TxSel(iTE_u8 u8TxSel);
void Cec_BlockSel(iTE_u8 u8CecSel);
void Cec_SysPoll(void);
void Cec_SysInit(iTE_u8 port,iTE_u8 PhyAdd_Port);

void Cec_Wb(iTE_u8 u8Offset,iTE_u8 u8Count,iTE_u8* pu8Data);
void Cec_Rb(iTE_u8 u8Offset,iTE_u8 u8Count,iTE_u8* pu8Data);
void Cec_Set(iTE_u8 u8Offset,iTE_u8 u8InvMask,iTE_u8 u8Data);
void Cec_W(iTE_u8 u8Offset,iTE_u8 u8Data);
iTE_u8 Cec_R(iTE_u8 u8Offset);
iTE_u8 CEC_I2c_Addr_Sel(void);
#endif

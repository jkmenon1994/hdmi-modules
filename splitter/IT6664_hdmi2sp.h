///*****************************************
//  Copyright (C) 2009-2021
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <IT6664_hdmi2sp.h>
//   @author Hojim.Tseng@ite.com.tw
//   @date   2021/01/12
//   @fileversion: ITE_HDMI2_SPLITER_1.38
//******************************************/
#ifndef _IT6664_HDMI2SP_H_
#define _IT6664_HDMI2SP_H_

#include "IT6664_extern.h"

#define RCLKSrcSel  		0      // 0: RINGOSC, 1: XTAL
#define FixOCLKD4 			FALSE
#define RxFixEQNum          0x9F
#define RclkAvg				0x55F0
#define TxCLKStbRef         2  // 0~3, for txclk_stable tolerance
#define ForceTxCLKStb  		FALSE  // when test "PwrDnRCLK = TRUE" must set TRUE
#define ForceVOut 			FALSE
#define ForceROn 			FALSE     // TRUE: for ATC electrical test
#define ForceTMDSStb 		FALSE

// DDC option
#define EnDDCMasterArb 		TRUE //TRUE: HDCP > SCDC > FW
#define DisH2Auto 			FALSE      //FALSE: auto mode, TRUE: manual mode
#define SCDCFlagByte 		FALSE   //FALSE: 1 byte, TRUE: 2 byte
#define EnFlagUpdRd 		TRUE
#define FlagPollSel 		FALSE      //0: VSync Event, 1: 100ms, 2: 150ms, 3: 200ms
#define ColorClip			FALSE     // 1: clip RGBY to 16~235, CbCr to 16~240
#define DNFreeGo 			FALSE
#define UpSamSel 			FALSE
#define EnUdFilt 			FALSE      //TRUE for enable UTXCLK for dither and updnsampling
#define EnDither 			FALSE
#define QuanL4B				TRUE
#define CRCBLimit           FALSE

#define XPStableTime 		2      // 0: 75us, 1: 50us, 2: 100us
#define EnXPLockChk 		FALSE
#define EnFFAutoRst			TRUE
#define EnPLLBufRst 		TRUE
#define AutoPhReSync 		TRUE
#define TXNoDefPhase		FALSE
#define FWDDCFIFOAcc 		TRUE    // default: TRUE => NewDDCFIFOAcc
#define HWDDCFIFOAcc 		FALSE   // TRUE: for HDCP2.2 debug mode
#define DDCSpeed 			DDC75K  // DDC75K, DDC125K, DDC312K
#define SSCPPos 			TRUE         //FALSE: VBlank, TRUE: VSync
#define SSCPOpt 			FALSE        //FALSE: before packet, TRUE: after packet (QD980 must set TRUE)
#define SSCPTest 			FALSE       //FALSE: SSCP per frame, TRUE: SSCP per line
#define EnSSCPTrgPkt 		FALSE   //FALSE: VBlank, TRUE: after SSCP
#define En444to420 			FALSE
#define En420to444 			TRUE
#define EnRxHP2_Dbg 		0x0F
#define EnRxHDCP2 			TRUE
#define EnRxHwReauth 		TRUE
#define RxHP2SelDone 		TRUE //1// hdcp2 done after EKS, else after rpt done



void hdmi2_irq( void );
void it6664_hdmi2sp_initial(void);
void it6664_hdmi2sp_rst(void);
void h2rx_ini(iTE_u8 port);
void detectbus(void);
void h2rx_rst(iTE_u8 port);
void h2tx_ini(iTE_u8 port);
void h2txcom_ini(void);
void h2tx_rst(iTE_u8 port);
void chgrxbank(iTE_u8 bankno);
void chgspbank(iTE_u8 bankno);
void cal_pclk(iTE_u8 port);
void setup_vbointf( int port );
void train_vbointf( void );
void set_RxFPGAIn( void );
void cal_rclk(void);
void cal_oclk(void);
iTE_u16 Pow(iTE_u16 x,iTE_u16 y);
void it6664_var_init(void);
iTE_u32 ReadSipRom(void);
void autoeq_ini(void);
void caof_ini(void);
void hdmi2_work(void);
void TimerInt(void);
iTE_u8 ReadStreamType(void);
void it6664_RX_ClockstbChk(void);
void TimerInt0(void);
void IT6664_SetRxLED(iTE_u8  LED_SEL, iTE_u8 on);

#endif

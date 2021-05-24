///*****************************************
//  Copyright (C) 2009-2021
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <IT6664_Config.h>
//   @author Hojim.Tseng@ite.com.tw
//   @date   2021/01/12
//   @fileversion: ITE_HDMI2_SPLITER_1.38
//******************************************/
#ifndef _IT6664_CONFIG_H_
#define _IT6664_CONFIG_H_


#define USING_1to8 0
#define Bond6664 0
#define DebugMessage

#ifdef DebugMessage
	#define iTE_Msg(x) printk x
	#define iTE_MsgEQ(x) printk x
	#define iTE_MsgTX(x) printk x
	#define iTE_MsgRX(x) printk x
	#define iTE_MsgMHL(x) printk x
	#define iTE_MsgHDCP(x) printk x
	#define iTE_Edid_Msg(x) printk x
	#define iTE_Cec_Msg(x) printk x
#else // DebugMessage
	#define iTE_Msg(x)
	#define iTE_MsgEQ(x)
	#define iTE_MsgTX(x)
	#define iTE_MsgRX(x)
	#define iTE_MsgMHL(x)
	#define iTE_MsgHDCP(x)
	#define iTE_MsgHDCP(x)
	#define iTE_Edid_Msg(x)
	#define iTE_Cec_Msg(x)
#endif // DebugMessage


#ifndef USING_1to8
	#define USING_1to8  TRUE
#endif

#ifndef Bond6664
	#define Bond6664 	TRUE
#endif
#define IT6663_C 0x01


//function define
//--------------------------------------------------

#define EnIntEDID  TRUE //use Ext EDID
#define AutoEQ			 // Auto EQ
#define SIP_Mean  22000
//#define Support_MHL
//#define Support_CEC
#define Disable_RXHDCP
//#define YUV_DS_Set_Limit_Range
#define Support4096DS
#define Support_HDCP_DownVersion // Just For IT6664 test
//#define Compose3D_EDID
//#define EDID_Compose_Intersection
//#define RXHDCP_Follow_SinkX
//#define DS_Switch
#define CopyMode_Remove4K533



#ifndef EnRepeater
	#define EnRepeater FALSE//TRUE
#endif

#if(EnRepeater == TRUE)
#pragma message("Enable repeater function")
#define repeater
#define HPD_Debounce		FALSE
#define EnRxHP1Rpt			TRUE
#define EnRxHP2Rpt 			TRUE
#else
#define HPD_Debounce		TRUE
#define EnRxHP1Rpt			FALSE
#define EnRxHP2Rpt 			FALSE
#endif

//--------------------------------------------------

// Normal define
#if(USING_1to8 == TRUE)
	#define	IT6664_A 0x02
	#define IT6664_B 0x21
	#define IT6663_C 0x01
	#define TxPortNum   4

	#ifndef EnableTXP0HDCP
    	#define EnableTXP0HDCP TRUE
	#endif// EnableTXP0HDCP

	#ifndef EnableTXP1HDCP
   		#define EnableTXP1HDCP TRUE
	#endif// EnableTXP1HDCP

	#ifndef EnableTXP2HDCP
   		#define EnableTXP2HDCP TRUE
	#endif// EnableTXP2HDCP

	#ifndef EnableTXP3HDCP
   		#define EnableTXP3HDCP TRUE
	#endif// EnableTXP3HDCP

	#define EDID_CopyPx  	0
	#define ABCD_CopyPx     0
	#define IT6664

	#define FixPort_Opt 0
	#define PortBypass_Opt 0
#else

	#if(Bond6664 == TRUE)
		#define TxPortNum   4
		#define IT6664
		#ifndef EnableTXP0HDCP
   		 #define EnableTXP0HDCP TRUE
		#endif// EnableTXP0HDCP

		#ifndef EnableTXP1HDCP
    		#define EnableTXP1HDCP TRUE
		#endif// EnableTXP1HDCP

		#ifndef EnableTXP2HDCP
   			#define EnableTXP2HDCP TRUE
		#endif// EnableTXP2HDCP

		#ifndef EnableTXP3HDCP
    		#define EnableTXP3HDCP TRUE
		#endif// EnableTXP3HDCP

		#define EDID_CopyPx  	0
		#define ABCD_CopyPx     0
		#define HDCP_CopyPx		0		 //RXHDCP_Follow_SinkX
		//Force 1080p and skip EDID
		#define FixPort0_1080P  FALSE
		#define FixPort1_1080P	FALSE
		#define FixPort2_1080P	FALSE
		#define FixPort3_1080P	FALSE
		#define FixPort_Opt 	((FixPort0_1080P<<0) + (FixPort1_1080P<<1) + (FixPort2_1080P<<2) + (FixPort3_1080P<<3))

		//Skip EDID
		#define  P0_SkipEdid    FALSE
		#define	 P1_SkipEdid		FALSE
		#define  P2_SkipEdid    FALSE
		#define	 P3_SkipEdid	  FALSE
		#define  PortSkipEdid_opt 	((P0_SkipEdid<<0) + (P1_SkipEdid<<1) + (P2_SkipEdid<<2) + (P3_SkipEdid<<3))

		//Force bypass output
		#define ForcePort0_Bypass	FALSE
		#define ForcePort1_Bypass	FALSE
		#define ForcePort2_Bypass	FALSE
		#define ForcePort3_Bypass	FALSE
		#define PortBypass_Opt	((ForcePort0_Bypass<<0)+(ForcePort1_Bypass<<1) + (ForcePort2_Bypass<<2)+(ForcePort3_Bypass<<3))
		//Same as above ,force bypass output , need skip SCDC ??
		#define ForcePort0_Bypass_SkipSCDC	FALSE
		#define ForcePort1_Bypass_SkipSCDC	FALSE
		#define ForcePort2_Bypass_SkipSCDC	FALSE
		#define ForcePort3_Bypass_SkipSCDC	FALSE
		#define SkipSCDC_Opt		((ForcePort1_Bypass_SkipSCDC<<0) + (ForcePort1_Bypass_SkipSCDC<<1) + (ForcePort2_Bypass_SkipSCDC<<2) + (ForcePort2_Bypass_SkipSCDC<<3))



		#pragma message("EnableIT6664")
	#else
		#define TxPortNum   2
		#define IT6663
		#ifndef EnableTXP0HDCP
    		#define EnableTXP0HDCP FALSE
		#endif// EnableTXP0HDCP

		#ifndef EnableTXP1HDCP
   		 	#define EnableTXP1HDCP TRUE
		#endif// EnableTXP1HDCP

		#ifndef EnableTXP2HDCP
    		#define EnableTXP2HDCP TRUE
		#endif// EnableTXP2HDCP

		#ifndef EnableTXP3HDCP
    		#define EnableTXP3HDCP FALSE
		#endif// EnableTXP3HDCP

		#define EDID_CopyPx  	1
		#define ABCD_CopyPx     1
		#define HDCP_CopyPx		1		//RXHDCP_Follow_SinkX
		#define FixPort1_1080P	FALSE
		#define FixPort2_1080P	FALSE
		#define FixPort_Opt 	((FixPort1_1080P<<1) + (FixPort2_1080P<<2))

		//Skip EDID
		#define  P1_SkipEdid    FALSE
		#define	 P2_SkipEdid		FALSE
		#define  PortSkipEdid_opt 	((P1_SkipEdid<<1) + (P2_SkipEdid<<2))
		//Force bypass output
		#define ForcePort1_Bypass	FALSE
		#define ForcePort2_Bypass	FALSE
		#define PortBypass_Opt	((ForcePort1_Bypass<<1) + (ForcePort2_Bypass<<2))
		//Same as above ,force bypass output , need skip SCDC ??
		#define ForcePort1_Bypass_SkipSCDC	FALSE
		#define ForcePort2_Bypass_SkipSCDC	FALSE
		#define SkipSCDC_Opt		((ForcePort1_Bypass_SkipSCDC<<1) + (ForcePort2_Bypass_SkipSCDC<<2))

		#pragma message("EnableIT6663")
	#endif
#endif//(USING_1to8 == TRUE)


#endif


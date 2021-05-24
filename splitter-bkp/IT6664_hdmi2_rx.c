///*****************************************
//  Copyright (C) 2009-2021
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <IT6664_hdmi2_rx.c>
//   @author Hojim.Tseng@ite.com.tw
//   @date   2021/01/12
//   @fileversion: ITE_HDMI2_SPLITER_1.38
//******************************************/
#include "IT6664_hdmi2_rx.h"
//#include <unistd.h>
#include <linux/delay.h>

#define mSleep(x) msleep(x)


extern extern_variables *gext_var;
extern extern_u8 *gext_u8;
extern iTE_u8 g_device;
extern extern_32 *gext_long;
iTE_u8 _CODE SymEQ[2] = {0x86,0x81};
extern struct PARSE3D_STR	st3DParse;

// CSC  RGB full range to YUV full range  or  RGB limit range to YUV limit range
iTE_u8 _CODE R2Y_F2F[2][18] =
{
	//ITU601
	{
		0xB2,0x04,0x65,0x02,0xE9,0x00,0x93,0x3C,0x18,
		0x04,0x55,0x3F,0x49,0x3D,0x9F,0x3E,0x18,0x04
	},
	//ITU709
	{
		0xB8,0x05,0xB4,0x01,0x94,0x00,0x4A,0x3C,0x17,
		0x04,0x9F,0x3F,0xD9,0x3C,0x10,0x3F,0x17,0x04
	}
};
// CSC  RGB full range to YUV limit range
iTE_u8 _CODE R2Y_F2L[2][18] =
{
	//ITU601
	{
		0x09,0x04,0x0E,0x02,0xC9,0x00,0x0F,0x3D,0x84,
		0x03,0x6D,0x3F,0xAB,0x3D,0xD1,0x3E,0x84,0x03
	},
	//ITU709
	{
		0xE4,0x04,0x77,0x01,0x7F,0x00,0xD0,0x3C,0x83,
		0x03,0xAD,0x3F,0x4B,0x3D,0x32,0x3F,0x84,0x03
	}
};
// CSC  YUV full range to RGB full range  or  YUV limit range to RGB limit range
iTE_u8 _CODE Y2R_F2F[2][18] =
{
	//ITU601
	{
		0x00,0x08,0x6B,0x3A,0x50,0x3D,0x00,0x08,0xF5,
		0x0A,0x02,0x00,0x00,0x08,0xFD,0x3F,0xDA,0x0D
	},
	//ITU709
	{
		0x00,0x08,0x55,0x3C,0x88,0x3E,0x00,0x08,0x51,
		0x0C,0x00,0x00,0x00,0x08,0x00,0x00,0x84,0x0E
	}
};
// CSC  YUV limit range to RGB full range
iTE_u8 _CODE Y2R_L2F[2][18] =
{
	//ITU601
	{
		0x4F,0x09,0x81,0x39,0xDD,0x3C,0x4F,0x09,0xC4,
		0x0C,0x01,0x00,0x4F,0x09,0xFD,0x3F,0x1F,0x10
	},
	//ITU709
	{
		0x4F,0x09,0xBA,0x3B,0x4B,0x3E,0x4F,0x09,0x57,
		0x0E,0x02,0x00,0x4F,0x09,0xFE,0x3F,0xE8,0x10
	}
};
iTE_u8 _CODE R2Y_Offset_F2F[3] =
{
	0x00,0x80,0x10
};
iTE_u8 _CODE R2Y_Offset_F2L[3] =
{
	0x10,0x80,0x10
};
iTE_u8 _CODE Y2R_Offset_F2F[3] =
{
	0x00,0x00,0x00
};
iTE_u8 _CODE Y2R_Offset_L2F[3] =
{
	0x04,0x00,0xA7
};
iTE_u8 _CODE BT2020_Y2R_F2F[18] =
{
	0x4F,0x09,0xCC,0x3A,0x7E,0x3E,0x4F,0x09,0x69,
	0x0D,0x0B,0x00,0x4F,0x09,0xFE,0x3F,0x1D,0x11
};
iTE_u8 _CODE BT2020_Y2R_Offset_F2F[3] =
{
	0x04,0x004,0xA7
};
iTE_u8 _CODE BT2020_Y2R_F2L[18] =
{
	0x00,0x08,0x87,0x3B,0xB4,0x3E,0x00,0x08,0x87,
	0x0B,0x0A,0x00,0x00,0x08,0xFE,0x3F,0xB5,0x0E
};
iTE_u8 _CODE BT2020_Y2R_Offset_F2L[3] =
{
	0x00,0x00,0x00
};
iTE_u8 _CODE BT2020_R2Y_F2F[18] =
{
	0xA8,0x04,0xCE,0x01,0x68,0x00,0xC7,0x3C,0x84,
	0x03,0xB5,0x3F,0x77,0x3D,0x06,0x3F,0x83,0x03
};
iTE_u8 _CODE BT2020_R2Y_F2L[18] =
{
	0x6C,0x05,0x1A,0x02,0x7A,0x00,0x40, 0x3C,0x18,0x04,
	0xA8, 0x3F,0x0C,0x3D, 0xDD,0x3E,0x18, 0x04
};
iTE_u8 _CODE BT2020_R2Y_Offset_F2F[3] =
{
	0x10,0x80,0x10
};
iTE_u8 _CODE BT2020_R2Y_Offset_F2L[3] =
{
	0x00,0x80,0x10
};

#if 1
void h2rx_irq(void)
{
	iTE_u8 rddata,i;
	iTE_u8 RxReg05, RxReg06, RxReg07, RxReg08, RxReg09, RxReg10, RxReg11, RxReg12, RxReg13, RxReg14, RxReg15;
	iTE_u8 SCDC_SCREN,HDMI_VSIF_3DValid;
	iTE_u8 RxReg19,RxReg1D,RxReg1A,RxReg1B,rdtxdata,HDMI_mode;
	iTE_u8 RxDeskew_err=0;
	//iTE_u8 SymlockEQ[2] = {0x86,0x81};
	//iTE_u8 SHA1H[20];
	iTE_u32 HDMI_VSIF_OUI ;
	//iTE_u16 HP1_Bstatus = (HP2_DevDepth<<8)+(HP2_DevNum<<0);


	RxReg05 = h2rxrd( 0x05);
	RxReg06 = h2rxrd( 0x06);
	RxReg07 = h2rxrd( 0x07);
	RxReg08 = h2rxrd( 0x08);
	RxReg09 = h2rxrd( 0x09);
	RxReg10 = h2rxrd( 0x10);
	RxReg11 = h2rxrd( 0x11);
	RxReg12 = h2rxrd( 0x12);
	RxReg13 = h2rxrd( 0x13);
	RxReg14 = h2rxrd( 0x14);
	RxReg15 = h2rxrd( 0x15);
	RxReg19 = h2rxrd( 0x19);
	RxReg1A = h2rxrd( 0x1A);
	RxReg1B = h2rxrd( 0x1B);
	RxReg1D = h2rxrd( 0x1D);
	h2rxwr(0x05,RxReg05);
	h2rxwr(0x06,RxReg06);
	h2rxwr(0x07,RxReg07);
	h2rxwr(0x08,RxReg08);
	h2rxwr(0x09,RxReg09);
	h2rxwr(0x10,RxReg10);
	h2rxwr(0x11,RxReg11);


	HDMI_mode = RxReg14&0x40;

	if( RxReg05!=0x00 ) {
		if( RxReg05&0x01 ) {
			 gext_u8->SDI_clkstbChk = 0;
			 if( RxReg13&0x01 ) {
				iTE_MsgRX(("#### Port 0 Power 5V ON #### \r\n"));
				h2spset(0x0C, 0x04, 0x04);//reset HDCP key
				h2spset(0x10,0x40,0x00);
				h2spset(0x10,0x40,0x40); //reset HP2 block
				h2spset(0x0C, 0x04, 0x00);
				gext_u8->RxtoggleHPD = 0;
				detectbus();
				// emily add 20151225 reset EDID when 5V-OFF => 5V-ON
				if( gext_u8->BUSMODE==MHL )
				{
					chgrxbank(3);
					h2rxwr(0xAB, 0x00);
					h2rxwr(0xAC, 0x00);
					chgrxbank(0);
				}
				gext_u8->TxAFESetDone =0;
				#ifdef Support_CEC
					Cec_TxPolling(IT6664_RX_LA);
				#endif
			}
			else {
				iTE_MsgRX(("#### Port 0 Power 5V OFF #### \r\n"));
				it6664_h2rx_pwdon();
				for(i=0;i<4;i++) h2tx_pwrdn(i);
				chgrxbank(3);
				h2rxset(0xE5,0x1C,0x00);
				chgrxbank(0);
			}//20170116
		}
		if( RxReg05&0x02 ) {
			iTE_MsgRX(("#### RX Clock On Interrupt ####\r\n"));
			RxDeskew_err=0;
			if(h2rxrd(0x13)&0x40) gext_u8->BUSMODE=MHL;
			else gext_u8->BUSMODE=HDMI;

			#ifdef repeater
			if ((!gext_u8->TXHPDsts)&&(!EnRxHP2VRRpt))
			{
				iTE_MsgRX(("no downstream RX, set device as HDCP receiver!\n"));
		    	chgspbank(1);
				h2spset(0x10, 0x08, 0x00);
				chgspbank(0);
			}
			else
			{
		   		chgspbank(1);
				h2spset(0x10, 0x08, EnRxHP2Rpt<<3);
				chgspbank(0);
			}
			#endif
		}
		if( RxReg05&0x04 ||RxDeskew_err==0x10) {
   			//h2rxset(0x23, 0x02, 0x02);//reset hdcp to hold
			RxDeskew_err=0;
			//h2rxwr( 0x05, 0x04);
			h2rxset(0x53, 0xE0, 0x00);//disalbe deskew and lag err
			h2rxset(0x54, 0xFF, 0x00);
			h2rxset(0x55, 0x07, 0x00);
			//h2rxset(0x57, 0x0D, 0x00);
			h2rxwr(0x05, 0xE0);//20170116
      		h2rxwr(0x06, 0xFE);//20170116
			//h2spset(0x4E, 0x0F, 0x0F);//disable FIFO
			rddata = h2rxrd( 0x13);
			//iTE_MsgRX(("#### RX Clock Stable Change Interrupt => RxCLK_Stb = %d ####\r\n",(rddata&0x10)>>4));
			if( rddata&0x08 )
			{
				h2rxset(0x23, 0x02, 0x00);
				gext_u8->SDI_clkstbChk = 1;
				iTE_MsgRX(("#### Clock Stable	#### \r\n"));
				if(EnBlockHDMIMode == TRUE)
				{
					h2rxset(0x40, 0x03, 0x02); //block HDMI Mode = 1 for HDCP ATC
				}
				//170104
				for (i=0;i<=3;i++) //monitor TXHPD here for later HDCP decision
				{
			  	 	rdtxdata = h2txrd(i, 0x03);
			   		if (rdtxdata&0x02)
				   		gext_var->TXHPD[i]=TRUE;
			   		else
				   		gext_var->TXHPD[i]=FALSE;
				}
				//170104
				h2rxset(0x54, 0x01, 0x01);
				if(g_device == IT6663_C)
				{
					if((!gext_u8->EQ20Going)||(!gext_u8->EQ14Going))
					{
						if((h2rxrd(0x14)&0x38) == 0x00)
						{
							if(((!gext_u8->Auto14done)&&(!HDMI_mode)) || ((!gext_u8->Auto20done) && HDMI_mode))
							{
								if(EnRxHP2Rpt == FALSE)
								{
									it6664_SetFixEQ(0x87);
								}
							}
						}
					}
				}
				//Start Timer Int
				if(gext_u8->BUSMODE == HDMI)
				{
					h2spset(0x1A,0x02,0x00);
					h2spset(0x19,0x20,0x00);
					h2spwr(0x1D,0x85);
					h2spset(0x19,0x20,0x20);
					h2spwr(0x07,0x2F);
					h2spset(0x1A,0x02,0x02);
				}
			}
			else
			{
				h2rxset(0x54, 0x01, 0x00);
				iTE_MsgRX(("#### Clock go un-Stable   #### \r\n"));
				gext_u8->CD_SET = FALSE;//161230
				gext_u8->SDI_clkstbChk = 0;
				#if (USING_1to8 == FALSE)
					gext_u8->RXHDCP = 0;
				#endif

				if(g_device == IT6663_C) {}
				 //IT6664_SetRxLED(1,0);
				#ifdef AutoEQ
				if(g_device == IT6663_C)
				{
					if(HDMI_mode)
					{
						if(gext_u8->Auto20done)
						{
							if((gext_u8->EQ20Going) && (gext_u8->AutoEQsts != STAT_EQ_WaitSarEQDone))
							{
								gext_u8->AutoEQsts = STAT_EQ_rst;
							}
						}
					}
					else
					{
						if(gext_u8->Auto14done)
						{
							if((gext_u8->EQ14Going) && (gext_u8->AutoEQsts != STAT_EQ_WaitSarEQDone))
							{
								gext_u8->AutoEQsts = STAT_EQ_rst;
							}
						}
					}

					h2rxset(0x23,0x10,0x10);
					mSleep(1);
					h2rxset(0x23,0x10,0x00);
				}
				#endif
				gext_u8->TxAFESetDone = 0;
				for(i=0;i<4;i++) h2txset(i,0x88,0x03,0x03);
				h2rxset(0x23,0x02,0x02);
				h2rxset(0x23,0x02,0x00);
			}
		}
		if( RxReg05&0x08 ) {
			iTE_MsgRX(("#### RX Clock Change Detect Interrupt ####\r\n"));
			iTE_MsgRX((" RxCLK_Stb = 0x%02x \r\n",h2rxrd(0x13)));
		}
		if( RxReg05&0x10 ) {
			iTE_MsgRX(("#### RX HDMIMode Change Interrupt => HDMIMode = %d ####\r\n", (iTE_u16)(h2rxrd( 0x13)&0x02)>>1));
		}
		if( RxReg05&0x20 ) {
			iTE_MsgRX(("#### RX ECC Error Interrupt !!! ####\r\n"));
			#ifdef AutoEQ
				if(((!gext_u8->EQ20Going) && (HDMI_mode)) || ((!gext_u8->EQ14Going)&&(!HDMI_mode)))
			#endif
				{
					chgspbank(1);
					h2spset(0x11, 0x02, 0x02);
					chgspbank(0);
					gext_u8->RxtoggleHPD++;
					if(gext_u8->RxtoggleHPD > 50)
					{
						if(EnRxHP2Rpt == FALSE)
						{
							SetRxHpd(0);
							mSleep(2000);
							SetRxHpd(1);
							iTE_MsgRX(("#### RX ECC Error Interrupt, toggle HPD !!! ####\r\n"));
						}
						gext_u8->RxtoggleHPD = 0;
					}
				}
		}
		if( RxReg05&0x40 ) {
			iTE_MsgRX(("#### RX DeSkew Error Interrupt !!! ####\r\n"));
			RxDeskew_err++;
		}

		if( RxReg05&0x80 ) {
			//iTE_MsgRX(("#### RX H2V FIFO Skew Fail Interrupt !!! ####\r\n"));
		}
	   }
		if( RxReg06&0x01 ) {
			//iTE_Msg(("RXP%d CHx SymLock Change Interrupt => RxSymLock = %d \r\n", port, (h2rxrd( 0x13)&0x80)>>7));
			//iTE_MsgRX(("#### Symbol Lock State Change #### \r\n"));
			if( RxReg13&0x80 ) {
				iTE_MsgRX(("#### Symbol Lock #### \r\n"));
				iTE_MsgRX(("#### Reg14 = 0x%02x #### \r\n",RxReg14));
				h2rxset(0x53, 0xE0, 0xE0);//enable deskew and lag err
				h2rxset(0x55, 0x07, 0x07);
				h2rxset(0x57, 0x0F, 0x0F);
				h2rxset(0x5D, 0x06, 0x06);
				h2rxset(0x5E, 0x08, 0x08);
				h2rxset(0x5F, 0x01, 0x01);

				#ifdef AutoEQ
					if(g_device == IT6663_C)
					{
						if(HDMI_mode)
						{
							if(!gext_u8->Auto20done)
							{
								if(!gext_u8->EQ20Going)
								{
									gext_u8->AutoEQsts = STAT_EQ_Start;
								}
							}
							else
							{
								//gext_u8->AutoEQsts = STAT_EQ_ChkOldEQ;
								it6664_EQchkOldResult(1);
							}
						}
						else
						{
							//ite_200225
							iTE_MsgRX(("#### restart 1.4 EQ #### \r\n"));
							gext_u8->AutoEQsts = STAT_EQ_Start;
							gext_u8->Auto14done = 0;
							/*
							if(!gext_u8->Auto14done)
							{
								if(!gext_u8->EQ14Going)
								{
									gext_u8->AutoEQsts = STAT_EQ_Start;
								}
							}
							else
							{
								//gext_u8->AutoEQsts = STAT_EQ_ChkOldEQ;
								it6664_EQchkOldResult(0);
							}
							*/
						}
					}
				#endif
			}
			else{
				iTE_MsgRX(("#### Symbol NOT Lock #### \r\n"));
				//if(RxReg14&0x20 ) iTE_MsgRX(("#### CH2 Symbol NOT Lock #### \r\n"));
				//if(RxReg14&0x10 ) iTE_MsgRX(("#### CH1 Symbol NOT Lock #### \r\n"));
				//if(RxReg14&0x08 ) iTE_MsgRX(("#### CH0 Symbol NOT Lock #### \r\n"));
				iTE_MsgRX(("#### Reg14 = 0x%02x Reg13 = 0x%02x\r\n",RxReg14,RxReg13));
				if (gext_u8->BUSMODE != MHL)
				{
					h2rxset(0x53, 0xE0, 0x00);
					h2rxset(0x55, 0x07, 0x00);
					//h2rxset(0x5D, 0x06, 0x00);
					h2rxset(0x5E, 0x08, 0x00);
					h2rxset(0x5F, 0x01, 0x00);
				}
			}
		}
		if( RxReg06&0x02 ) {
			//iTE_MsgRX(("#### RX CH0 SymFIFORst Interrupt !!! ####\r\n"));
		}
		if( RxReg06&0x04 ) {
			//iTE_MsgRX(("#### RX CH1 SymFIFORst Interrupt !!! ####\r\n"));
		}

		if( RxReg06&0x08 ) {
			//iTE_MsgRX(("#### RX CH2 SymFIFORst Interrupt !!! ####\r\n"));
		}

		if( RxReg06&0x10 ) {
			//iTE_MsgRX(("#### RX CH0 SymLockRst Interrupt !!! ####\r\n"));

		}
		if( RxReg06&0x20 ) {
			//iTE_MsgRX(("#### RX CH1 SymLockRst Interrupt !!! ####\r\n"));
		}

		if( RxReg06&0x40 ) {
			//iTE_MsgRX(("#### RX CH2 SymLockRst Interrupt !!! ####\r\n"));
		}

		if( RxReg06&0x80 ) {
			//iTE_MsgRX(("#### RX FSM Fail Interrupt !!! ####\r\n"));
		}
		if( RxReg07!=0x00 ) {
			if( RxReg07&0x01 ) {
				 iTE_MsgRX(("#### Port 0 CH0 DeSkew Lag Err #### \r\n"));
			}
			if( RxReg07&0x02 ) {
				 iTE_MsgRX(("#### Port 0 CH1 DeSkew Lag Err #### \r\n"));
			}
			if( RxReg07&0x04 ) {
				 iTE_MsgRX(("#### Port 0 CH2 DeSkew Lag Err #### \r\n"));
			}
			if( RxReg07&0x08 ) {
				 if( (RxReg13&0x40)>>6 ) {
					 iTE_MsgRX(("#### Port 0 Bus Mode : MHL #### \r\n"));
				 }
				 else {
					 iTE_MsgRX(("#### Port 0 Bus Mode : HDMI #### \r\n"));
				 }
			}
			//170109
			if( RxReg07&0x10 ) {
             	iTE_MsgRX(("#### RX SarEQ Done #### \r\n"));
				#ifdef AutoEQ
				if(g_device == IT6663_C)
				{
					gext_u8->AutoEQsts = STAT_EQ_WaitSarEQDone;
				}
				#endif
			}
			if( RxReg07&0x20 ) {
             	iTE_MsgRX(("#### RX SarEQ Fail!!!! #### \r\n"));
			}
			if( RxReg07&0x40 ) {
				#ifdef AutoEQ
					gext_u8->AutoEQsts = STAT_EQ_WaitSarEQDone;
				#endif
             	iTE_MsgRX(("#### RX HDMI 1.4 EQDone #### \r\n"));
			}
			if( RxReg07&0x80 ) {
				#ifdef AutoEQ
				if(g_device == IT6663_C)
				{
					gext_u8->AutoEQsts = STAT_EQ_WaitSarEQDone;
					chgrxbank(3);
					h2rxset(0x22,0x04,0x00);//trigger off
					h2rxset(0x22,0x38,0x00);
					chgrxbank(0);
				}
				#endif
             	iTE_MsgRX(("#### RX HDMI 1.4 EQ Fail!!! #### \r\n"));
			}
		};
		//170109
		if( RxReg08!=0x00) {
			if( RxReg08&0x01 )
			{
				if(g_device == IT6663_C)
				{
			  		chgrxbank(3);
					h2rxwr(0x20, 0x1B);//cs setting
					h2rxwr(0x21, 0x03);
					chgrxbank(0);
					it6664_SetFixEQ(0x9F);
					if((gext_u8->AutoEQsts == STAT_EQ_WaitSarEQDone) && (gext_u8->EQ14Going == 1))
					{
						gext_u8->EQ14Going= 0;
						h2rxset(0x23,0x10,0x10);//rst eq
						mSleep(1);
						h2rxset(0x23,0x10,0x00);
						h2rxwr(0x22, 0x00);//disable trigger
						gext_u8->AutoEQsts = STAT_EQ_Start; //HDMI 2.0 start
					}
				}
			  	iTE_MsgRX(("#### HDMI2 1:40 Clock Ratio mode = %d ####\r\n",(iTE_u16)(RxReg14>>6) ));
			}
			if( RxReg08& 0x02){ // SCREn change
				SCDC_SCREN=RxReg14>>7;;
				if(SCDC_SCREN==1)
					 iTE_MsgRX(("HDMI2 Scramble Enable !!  \r\n"));
				else iTE_MsgRX(("HDMI2 Scramble Disable !!  \r\n"));
			}
			if( RxReg08&0x04 ) {
				 iTE_MsgRX(("#### SCDC Scrambe Status Change #### \r\n"));
				 if((RxReg15&0x02)>>1) {
					 iTE_MsgRX(("###  SCDC Scarmable Status:  ON  ### \r\n"));
				 }
				 else iTE_MsgRX(("###  SCDC Scarmable Status: OFF  ### \r\n"));
			 }
			 if( RxReg08&0x08 ) {
				 iTE_MsgRX(("#### EDID Bus Hange #### \r\n"));

			 }
			 if( RxReg08& 0x80){ // HDMI2 Auto Det
				iTE_MsgRX(("#### HDMI2 Det State=%x  ####\r\n", (iTE_u16)(h2rxrd(0x15)&0x3C)>>2));
				h2rxset(0x4C, 0x80, 0x80);
			 }
		}
		if( RxReg09!=0x00) {
			 if( RxReg09&0x01 )
			 {
				iTE_MsgRX(("#### Port 0 HDCP Authentication Start #### \r\n"));
				if(EnRxHP1Rpt == TRUE)
				{
					h2txcomset(0x20, 0x02, 0x02);
             		h2txcomset(0x20, 0x02, 0x00);
				}
			 }

			 if( RxReg09&0x02 )
			 {
			 	 gext_u8->RXHDCP = 1;
				 iTE_MsgRX(("#### Port 0 HDCP Authentication Done #### \r\n"));
				 if (EnRxHP1Rpt)
				 {
				  	gext_u8->DevCnt_Total=0;
          			gext_u8->Depth_Total=0;
				  	if(!gext_u8->TXHPDsts)
			  		{
			   			iTE_MsgRX(("calculate SHA1 with no downstream TX \r\n"));
			   			setrx_ksv_list(0,0,0,0);
			  		}
				 }
			 }
			 if( RxReg09&0x04 ) {
				 // emily mark 20160823
				 iTE_MsgRX(("#### Port 0 HDCP Encryption chg #### \r\n"));
				 if((h2rxrd(0xCF)&0x20) == 0x00)
				 {
				 	#if (USING_1to8 == TRUE)
						iTE_u8 tmp;
						tmp= g_device;
						IT6664_DeviceSelect(IT6664_A);
						gext_u8->RXHDCP = 0;
						for(i=0;i<4;i++) gext_var->HDCPState[i] = Tx_CP_Reset;
						IT6664_DeviceSelect(IT6664_B);
						gext_u8->RXHDCP = 0;
						for(i=0;i<4;i++) gext_var->HDCPState[i] = Tx_CP_Reset;
						IT6664_DeviceSelect(tmp);
					#else
						gext_u8->RXHDCP = 0;
						for(i=0;i<4;i++) gext_var->HDCPState[i] = Tx_CP_Reset;
					#endif
					if(g_device == IT6663_C)  {} //IT6664_SetRxLED(1,0);
					gext_u8->CP_Done = 0;
					if(EnRxHP1Rpt) //clear bstatus
					{
						chgrxbank(1);
            			h2rxwr(0x10, 0x00);
            			h2rxwr(0x11, 0x00);
						chgrxbank(0);
						h2rxset(0xCE, 0x40, 0x00);
					}
					iTE_MsgRX(("#### Port 0 HDCP Encryption chg => OFF #### \r\n"));
				 }
				 else
				 {
					#if (USING_1to8 == TRUE)
						iTE_u8 tmp;
						tmp= g_device;
						IT6664_DeviceSelect(IT6664_A);
						gext_u8->RXHDCP = 1;
						for(i=0;i<4;i++)
						{
							if((!(gext_u8->CP_Done&(1<<i))) && ((h2txrd(i,0x41)&0x01) !=0x01))
							{
								gext_var->HDCPState[i] = Tx_CP_check;
							}
						}
						IT6664_DeviceSelect(IT6664_B);
						gext_u8->RXHDCP = 1;
						for(i=0;i<4;i++)
						{
							if((!(gext_u8->CP_Done&(1<<i))) && ((h2txrd(i,0x41)&0x01) !=0x01))
							{
								gext_var->HDCPState[i] = Tx_CP_check;
							}
						}
						IT6664_DeviceSelect(tmp);

					#else
						gext_u8->RXHDCP = 1;
						for(i=0;i<4;i++)
						{
							if((!(gext_u8->CP_Done&(1<<i))) && ((h2txrd(i,0x41)&0x01) !=0x01))
							{
								gext_var->HDCPState[i] = Tx_CP_check;
								if(EnRxHP1Rpt == TRUE)
								{
									gext_var->HDCPState[i] = Tx_CP_Going;
								}
							}
						}
					#endif
					//IT6664_SetRxLED(1,1);
					iTE_MsgRX(("#### Port 0 HDCP Encryption chg => ON #### \r\n"));
				 }
			 }
			 if( RxReg09&0x08 )
			 {
				 iTE_MsgRX(("#### Port 0 HDCP off interrupt #### \r\n"));
				 gext_u8->RXHDCP = 0;
				 if(g_device == IT6663_C) {}
				//IT6664_SetRxLED(1,0);
			 }
		 }
		if( RxReg10!=0x00) {
			if( RxReg10&0x80 ) {
				//iTE_MsgRX(("#### Audio FIFO Error #### \r\n"));
		}
		if( RxReg10&0x40 ) {
			 iTE_MsgRX(("#### Audio Auto Mute #### \r\n"));
		}
		if( RxReg10&0x20 ) {
			iTE_MsgRX(("#### Packet Left Mute #### \r\n"));
		}
		if( RxReg10&0x10 ) {
			iTE_MsgRX(("#### Set Mute Packet Received #### \r\n"));
		}
		if( RxReg10&0x08 ) {
			iTE_MsgRX(("#### Timer Counter Tntterrupt #### \r\n"));
		}
		if( RxReg10&0x04 ) {
			iTE_MsgRX(("#### Video Mode Changed #### \r\n"));
			h2rxset(0x40, 0x03, 0x00);//clear force HDMI Mode
		}
	// emily mark 20160812
		if( RxReg10&0x02)
		{
			//170104
			if((h2rxrd(0x19)&0x80))
			{
			    iTE_MsgRX(("#### SCDT ON ####  \r\n"));
				gext_u8->SDI_clkstbChk = 0;
				h2rxset(0x40, 0x03, 0x00);
				if(g_device == IT6663_C) {}// IT6664_SetRxLED(0,1);
				//mSleep(150);
				RX_FiFoRst();
			    if (gext_u8->RXSCDT == TRUE)  //SCDT is off and on later
				{
					gext_u8->CD_SET = FALSE;
					h2spwr(0x0D,0x00);
					iTE_MsgRX(("SCDT off and on \r\n"));
				}
				gext_u8->RXSCDT = 1; //170109

                if(gext_u8->CD_SET==FALSE)//170109
				{
					gext_u8->TXH2RSABusy =0;
				    gext_u8->TxTMDSStbFlag = 0;
				    gext_u8->TxVidStbFlag = 0;
					gext_u8->TxAFESetDone = 0;
					#ifdef IT6664
						for(i=0;i<TxPortNum;i++)
					#else
						for(i=1;i<TxPortNum+1;i++)
					#endif
					{
						if(h2txrd(i,0x03)&0x01) h2tx_pwron(i);
					}
				}
				 //170104
				h2spset(0x67, 0x0F, 0x00);
				h2spset(0x68, 0xFF, 0x00);
				if(h2rxrd(0x14)&0x01)//  >1G
				{
					chgrxbank(3);
					h2rxset(0xA7,0x40,0x40);
					chgrxbank(0);
				}
				else // < 1G
				{
					chgrxbank(3);
					h2rxset(0xA7,0x40,0x00);
					chgrxbank(0);
				}
					gext_u8->CD_SET = TRUE; //CD not received beforce SCDT on
					//161230
				 }
				 else {
					 iTE_MsgRX(("#### SCDT OFF #### \r\n"));
					 if(g_device == IT6663_C) {} //IT6664_SetRxLED(0,0);
					 gext_u8->RXSCDT = 0;
					 if (EnSCDTOffResetRX)
				 	 {
				  		h2rxset(0x23, 0x01, 0x01);
				  		h2rxset(0x23, 0x01, 0x00);
				 	 }
					 else
                 	 {
				  		h2rxset(0x40, 0x03, 0x02);
				 	 }
					 h2spwr(0x0D,0x00);
					 h2spset(0x31, 0x03, 0x00);
					 #ifdef IT6664
						for(i=0;i<TxPortNum;i++)
					 #else
						for(i=1;i<TxPortNum+1;i++)
					 #endif
					 {
						if((h2txrd(i,0x03)&0x01))
						{
							gext_var->HDCPState[i] = Tx_CP_Reset;
							gext_var->VideoState[i] = Tx_Video_waitInt;
							gext_u8->CP_Done = 0;
							gext_u8->TXH2RSABusy =0;
							gext_u8->TxTMDSStbFlag = 0;
							gext_u8->TxVidStbFlag = 0;
							gext_u8->ForceTXHDMI2 = 0;
							h2txset(i, 0x18, 0x80, 0x00);
							h2txset(i, 0x19, 0xFF, 0x00);
							h2txset(i, 0x1A, 0xFF, 0x00);
							h2txset(i, 0x1B, 0xFF, 0x00);
							h2txset(i, 0x1C, 0xFF, 0x00);
							h2txset(i, 0x88, 0x03, 0x03);//turn off tx oe
							h2txset(i, 0xFD, 0xFF, 0x00);
							h2txset(i,0x84, 0x60, 0x60);
							h2txset(i,0x84, 0x80, 0x00);
							h2txset(i,0x86, 0x08, 0x00);
							if(EnRxHP2Rpt == FALSE)
							{
								h2txset(i, 0x01, 0x02, 0x02);
								h2txset(i, 0x01, 0x02, 0x00);
							}
							gext_u8->CD_SET = FALSE;
							gext_u8->TxAFESetDone = 0;
							if(gext_u8->TXClearSCDC&(1<<i))
							{
								setup_h2scdc(i);
							}
						}
						gext_u8->TXClearSCDC = 0;
			  		}//161230
					chgrxbank(3);
					h2rxwr(0xAE,0x00);
					chgrxbank(0);
				 }
			 }
			 if( RxReg10&0x01 ) {
				 //iTE_MsgRX(("#### Video Abnormal Interrupt #### \r\n"));
			 }
		 }
		if( RxReg11!=0x00) {
			 if( RxReg11&0x80 ) {
				iTE_Msg(("#### No General Packet 2 Received #### \r\n"));
		}
			// if( RxReg11&0x40 ) {
				// h2rxwr( 0x11, 0x40);
			 //}
		if( RxReg11&0x20 ) {
			iTE_MsgRX(("#### No Audio InfoFrame Received #### \r\n"));
		}
		if( RxReg11&0x10) {
			iTE_MsgRX(("#### No AVI InfoFrame Received #### \r\n"));
		}
		if( RxReg11&0x08 ) {
			iTE_MsgRX(("#### CD Detect #### \r\n"));
			gext_u8->GCP_CD = ((h2rxrd( 0x98)&0xF0)>>4);
			if (!gext_u8->CD_SET  && EnCDSetTX) //170109
			{
				gext_u8->CD_SET = TRUE;
				for(i=0;i<4;i++) //calc TXCLK here to speed up process in TX
				{
					gext_u8->TXH2RSABusy =0;
				    gext_u8->TxTMDSStbFlag = 0;
				    gext_u8->TxVidStbFlag = 0;
					gext_u8->TxAFESetDone = 0;
				    rdtxdata = h2txrd(i, 0x03);
					if(rdtxdata&0x02)
					{
						iTE_MsgRX(("P%d connected when GCP CD on \r\n",(iTE_u16)i));
						h2tx_pwron(i);
					}
				}
			}//161230
		}
		if( RxReg11&0x04 ){
			//iTE_Msg(("#### Gen Pkt Detect #### \r\n"));
			iTE_MsgRX(("#### Vender Specific InfoFrame Detect #### \r\n"));
			chgrxbank(2);
			HDMI_VSIF_OUI=(h2rxrd( 0x2A)<<16)+ (h2rxrd( 0x29)<<8)+ (h2rxrd( 0x28));
			HDMI_VSIF_3DValid=(h2rxrd( 0x2C)&0x01);
			if(HDMI_VSIF_OUI==0xC45DD8) iTE_MsgRX(("Valid HDMI_VSIF !  \r\n"));
			else if(HDMI_VSIF_OUI==0x0C03) iTE_MsgRX(("Valid HDMI1.4 VSIF ! \r\n"));
			else iTE_MsgRX(("NOT HDMI_VSIF !	XXXXXXX  \r\n"));
			if(HDMI_VSIF_OUI==0xC45DD8 && HDMI_VSIF_3DValid==1)
				iTE_MsgRX(("HDMI_VSIF with 3D_Valid !  \r\n"));
			else iTE_MsgRX(("NOT HDMI_VSIF with 3D  Valid ! XXXXX	 \r\n"));
				chgrxbank(0);
			 }
			 if( RxReg11&0x02 ) {
				 //h2rxwr( 0x11, 0x02);
		//		 iTE_Msg(("#### ISRC2 Detect #### \r\n"));
			 }

			 if( RxReg11&0x01 ) {
				 //h2rxwr( 0x11, 0x01);
		//		 iTE_Msg(("#### ISRC1 Detect #### \r\n"));
			 }
		 }
		if( RxReg12!=0x00 ) {
			if( RxReg12&0x80 ) {
				RxReg1A=h2rxrd(0x1A);
			 	RxReg1B=h2rxrd(0x1B)&0x07;
			 	iTE_MsgRX(("#### Video Parameters Change #### \r\n"));
			 	iTE_MsgRX(("#### VidParaChange_Sts=%02x%02x \r\n", (iTE_u16)RxReg1B, (iTE_u16)RxReg1A));
			 	//h2rxwr(0x12, 0x80);
			 	h2rxwr(0x1A, 0xFF);
			 	h2rxwr(0x1B, 0x07);
				h2spset(0x0C,0x08,0x08);
				h2spset(0x0C,0x08,0x00);
		}
		if( RxReg12&0x40 ) {
			iTE_MsgRX(("#### 3D audio Valid Change #### \r\n"));
		}
		if( RxReg12&0x20 ) {
			iTE_MsgRX(("#### New DMR(HDR) Packet Received #### \r\n"));
			IT6664_HDRPacketGet();//ite_181217
		}
		if( RxReg12&0x10 ) {
			//iTE_Msg(("#### New Audio Packet Received #### \r\n"));
		}
		if( RxReg12&0x08 ) {
			iTE_MsgRX(("#### New ACP Packet Received #### \r\n"));
		}
		if( RxReg12&0x04 ) {
			iTE_MsgRX(("#### New SPD Packet Received #### \r\n"));
		}
		if( RxReg12&0x02) {
			iTE_MsgRX(("#### New MPEG InfoFrame Received #### \r\n"));
		}
		if( RxReg12&0x01) {
			iTE_MsgRX(("#### New AVI InfoFrame Received #### \r\n"));
			SetCscConvert();
			for(i=0;i<4;i++)
			{
				if(gext_var->VideoState[i] == Tx_Video_OK)
				{
						gext_var->VideoState[i] = Tx_Video_Reset;
				}
			}
		}//
	}
	h2rxwr(0x12,RxReg12);
}
#endif


void IT6664_HDRPacketGet(void)//ite_181217
{
	iTE_u8  pkt[19];

	h2rxwr(0x77, 0x87);
	chgrxbank(2);
	if(h2rxrd(0x24)==0x87)
	{
		h2rxbrd(0x24,19,pkt);
	}
	else
	{
		iTE_MsgRX(("[RX]	Latch HDR packet fail  \r\n"));
	}
	chgrxbank(0);
}

#if 1
void SetCscConvert(void)
{
	iTE_u8 u8En444to420=0,u8En420to444=0,i,RegAutoCSCSel = 0,CSCSel = 0;;
	iTE_u8 TXSupport420,TXSupport4K60,TXSupportOnly420,TXWithDVI;
	iTE_u8 Use_ITE709,Force_444420=0;
	iTE_u8 BT2020=0,limitRange;

	chgrxbank(2);
	gext_u8->InColorMode = (h2rxrd(0x15)&0x60)>>5;

	if((h2rxrd(0x16)&0xC0) == 0x80)
	{
		Use_ITE709 = 1;
	}
	else if((h2rxrd(0x16)&0xC0) == 0xC0)
	{
		Use_ITE709 = 0;
		if(((h2rxrd(0x17)&0x70) == 0x50)||((h2rxrd(0x17)&0x70) == 0x60))
		{
			BT2020 = 1;
			iTE_MsgRX(("[RX]	BT2020 = %x   \r\n",BT2020));
		}
	}
	else
	{
		Use_ITE709 = 0;
	}
	if((h2rxrd(0x17)&0x0C) == 0x04)
	{
		limitRange = 1;
	}
	else
	{
		limitRange = 0;
	}

	chgrxbank(0);
	iTE_MsgRX(("[RX]	Input color = %x (RGB/Y422/Y444/Y420)  Use_ITE709 = %x  limitRange = %x\r\n",gext_u8->InColorMode,Use_ITE709,limitRange));

	TXSupport420 = 1;
	TXSupport4K60 = 1;
	TXSupportOnly420 = 0;
	TXWithDVI = 0;
	RegAutoCSCSel = 1;

	for(i=0;i<TxPortNum;i++)
	{
		TXSupport4K60 &= gext_var->TXSupport4K60[i];
		TXSupport420 &= gext_var->TXSupport420[i];
		TXSupportOnly420 |= gext_var->TXSupportOnly420[i];
		TXWithDVI |= gext_var->DVI_mode[i];
	}
 	if(gext_u8->InColorMode==YCbCr420)//420
 	{
 		if(BT2020)
		{
			h2spbwr(0x88,0x03,(iTE_pu8)BT2020_Y2R_Offset_F2F);
			h2spbwr(0x93,0x12,(iTE_pu8)BT2020_Y2R_F2F);
			h2spbwr(0x70,0x3,(iTE_pu8)BT2020_Y2R_Offset_F2F);
			h2spbwr(0x73,0x12,(iTE_pu8)BT2020_Y2R_F2F);
		}
		else//yuv limit to RGB full
		{
			h2spbwr(0x88,0x03,(iTE_pu8)Y2R_Offset_L2F);
			h2spbwr(0x93,0x12,(iTE_pu8)Y2R_L2F[Use_ITE709]);
			h2spbwr(0x70,0x3,(iTE_pu8)Y2R_Offset_L2F);
			h2spbwr(0x73,0x12,(iTE_pu8)Y2R_L2F[Use_ITE709]);
		}
		gext_u8->CSCOutQ = 0x2;//full range
		u8En420to444 = 1;
		gext_u8->SetCVOutColorMode = YCbCr444;
		#if 0 // conv to RGB out
		gext_u8->CSCOutColorMode = RGB444;
		gext_u8->CVOutColorMode = RGB444;
		RegAutoCSCSel = 1;
		Force_444420 = 0;
		#else // conv to YUV out
		gext_u8->CSCOutColorMode = YCbCr444;
		gext_u8->CVOutColorMode = YCbCr444;
		RegAutoCSCSel = 0;
		Force_444420 = 1;
		#endif

		CSCSel = 0;
 	}
	else //others
	{
		if((h2rxrd(0x14)&0x80) != 0x80) //1.4  1/10
		{
			if(gext_u8->InColorMode == RGB444)
			{
				if(limitRange) // RGB limit to YUV limit
				{
					h2spbwr(0x70,0x03,(iTE_pu8)R2Y_Offset_F2F);
					h2spbwr(0x73,0x12,(iTE_pu8)R2Y_F2F[Use_ITE709]);
				}
				else // RGB full to YUV limit
				{
					h2spbwr(0x70,0x03,(iTE_pu8)R2Y_Offset_F2L);
					h2spbwr(0x73,0x12,(iTE_pu8)R2Y_F2L[Use_ITE709]);
				}
				CSCSel = 2;
				gext_u8->CSCOutQ = 0x1;// output limit
				gext_u8->CSCOutColorMode = YCbCr444;
			}
			else //YUV
			{
				h2spbwr(0x70,0x03,(iTE_pu8)Y2R_Offset_L2F);
				h2spbwr(0x73,0x12,(iTE_pu8)Y2R_L2F[Use_ITE709]);
				gext_u8->CSCOutQ = 0x2;// output RGB full
				CSCSel = 3;//YUV2RGB
				gext_u8->CSCOutColorMode = RGB444;
			}
		}
		else //2.0	1/40
		{
			if(gext_u8->InColorMode == RGB444)
			{
				//iTE_MsgRX(("[RX]	InColorMode = RGB444   \r\n"));
				u8En444to420 = 1;
				if(BT2020)
				{
					h2spbwr(0x88,0x03,(iTE_pu8)BT2020_R2Y_Offset_F2L);
					h2spbwr(0x93,0x12,(iTE_pu8)BT2020_R2Y_F2L);
					h2spbwr(0x70,0x3,(iTE_pu8)BT2020_R2Y_Offset_F2L);
					h2spbwr(0x73,0x12,(iTE_pu8)BT2020_R2Y_F2L);
				}
				else
				{
					if(limitRange)// RGB limit to YUV limit
					{
						h2spbwr(0x88,0x03,(iTE_pu8)R2Y_Offset_F2F);
						h2spbwr(0x93,0x12,(iTE_pu8)R2Y_F2F[Use_ITE709]);
						h2spbwr(0x70,0x3,(iTE_pu8)R2Y_Offset_F2F);
						h2spbwr(0x73,0x12,(iTE_pu8)R2Y_F2F[Use_ITE709]);
					}
					else// RGB full to YUV limit
					{
						h2spbwr(0x88,0x03,(iTE_pu8)R2Y_Offset_F2L);
						h2spbwr(0x93,0x12,(iTE_pu8)R2Y_F2L[Use_ITE709]);
						h2spbwr(0x70,0x3,(iTE_pu8)R2Y_Offset_F2L);
						h2spbwr(0x73,0x12,(iTE_pu8)R2Y_F2L[Use_ITE709]);
					}
				}
				gext_u8->CSCOutQ = 0x1;//limit range
				gext_u8->SetCVOutColorMode = YCbCr420;
				gext_u8->CVOutColorMode = YCbCr420;
				gext_u8->CSCOutColorMode = YCbCr444;
				CSCSel = 2;//RGB2YUV
			}
			else // YUV444  YUV422
			{
				gext_u8->CSCOutQ = 0x2;//full range
				if(gext_u8->InColorMode == YCbCr444)
				{
					//iTE_MsgRX(("[RX]	InColorMode = YCbCr444   \r\n"));
					u8En444to420 = 1;
					Force_444420 = 1;
					if(BT2020)
					{
						h2spbwr(0x88,0x03,(iTE_pu8)BT2020_Y2R_Offset_F2F);
						h2spbwr(0x93,0x12,(iTE_pu8)BT2020_Y2R_F2F);
						h2spbwr(0x70,0x3,(iTE_pu8)BT2020_Y2R_Offset_F2F);
						h2spbwr(0x73,0x12,(iTE_pu8)BT2020_Y2R_F2F);
					}
					else
					{
						h2spbwr(0x88,0x03,(iTE_pu8)Y2R_Offset_L2F);
						h2spbwr(0x93,0x12,(iTE_pu8)Y2R_L2F[Use_ITE709]);
						h2spbwr(0x70,0x3,(iTE_pu8)Y2R_Offset_L2F);
						h2spbwr(0x73,0x12,(iTE_pu8)Y2R_L2F[Use_ITE709]);
					}
					gext_u8->SetCVOutColorMode = YCbCr420;
					gext_u8->CVOutColorMode = YCbCr420;
					gext_u8->CSCOutColorMode = RGB444;
					CSCSel = 3;//YUV2RGB
				}
				else   //YCbCr422 not support conv & ds
				{
					if(BT2020)
					{
						h2spbwr(0x70,0x3,(iTE_pu8)BT2020_Y2R_Offset_F2F);
						h2spbwr(0x73,0x12,(iTE_pu8)BT2020_Y2R_F2F);
					}
					else
					{
						h2spbwr(0x70,0x3,(iTE_pu8)Y2R_Offset_L2F);
						h2spbwr(0x73,0x12,(iTE_pu8)Y2R_L2F[Use_ITE709]);
					}
					gext_u8->CSCOutColorMode = RGB444;
					CSCSel = 3;//YUV2RGB
					iTE_MsgRX(("[RX]	InColorMode = YCbCr422   \r\n"));
				}
			}
		}
	}
	h2spset(0x6B, 0xFF, (((RegAutoCSCSel<<6)+(gext_u8->SetCVOutColorMode<<4)+(gext_u8->CSCOutColorMode<<2)+CSCSel)));
	h2spset(0x6C, 0xFF, ((u8En444to420<<4)+(u8En420to444<<5)+(Force_444420<<3)));
	//iTE_MsgRX(("[RX]	Reg 0x6B = 0x%02x  Reg 0x6C = 0x%02x \r\n",(iTE_u16)h2sprd(0x6B),(iTE_u16)h2sprd(0x6C)));
}
#endif
void get_vid_info(void)
{
	iTE_u16 HSyncPol, VSyncPol, InterLaced;
	iTE_u16 HTotal, HActive, HFP, HSYNCW;
	iTE_u16 VTotal, VActive, VFP, VSYNCW;
	iTE_u16 rddata;
	iTE_u16 i;
	iTE_u32 PCLK, sump,TMDS;
	iTE_u32 TMDSCLK, sumt,FrameRate;
	VTiming CurVTiming;

	sumt = 0;
	for( i=0; i<100; i++)
	{
		 rddata = h2rxrd(0x48)+1;
		 sumt += rddata;
	}

	rddata = (h2rxrd(0x43)&0xE0)>>5;
    if( rddata==0 )
		TMDSCLK=((gext_long->RCLK*256*100/sumt)/2);
	else if( rddata==1 )
	    TMDSCLK=gext_long->RCLK*256*100/sumt;
	else if( rddata<4  )
        TMDSCLK=2*gext_long->RCLK*256*100/sumt;
	else
        TMDSCLK=4*gext_long->RCLK*256*100/sumt;
	//ite_190702
	sump = 0;
	for( i=0; i<10; i++ ) {
		 h2rxset(0x9A,0x80,0x00);
		 rddata = ((h2rxrd(0x9A)&0x03)<<8)+ h2rxrd(0x99);
		 h2rxset(0x9A,0x80,0x80);
		 sump += rddata;
	}
	if(sump==0)
	{
		iTE_MsgRX(("Sump = 0 !!!!! \r\n"));
	}
	//ite_190702
	PCLK=gext_long->RCLK*512*10/sump;// 512=2*256 because of 1T 2 pixel  //ite_190702
	PCLK = PCLK*107/100; // RCLK*1.07 for RCLK speed increase
	InterLaced = (h2rxrd(0x98)&0x02)>>1;
	HTotal   = ((h2rxrd(0x9C)&0x3F)<<8) + h2rxrd(0x9B);
	HActive  = ((h2rxrd(0x9E)&0x3F)<<8) + h2rxrd(0x9D);
	HFP      = ((h2rxrd(0xA1)&0xF0)<<4) + h2rxrd(0xA0);
	HSYNCW   = ((h2rxrd(0xA1)&0x01)<<8) + h2rxrd(0x9F);
	HSyncPol = h2rxrd(0xAA)&0x20>>5;
	VTotal   = ((h2rxrd(0xA3)&0x3F)<<8) + h2rxrd(0xA2);
	VActive  = ((h2rxrd(0xA5)&0x3F)<<8) + h2rxrd(0xA4);
	VFP      = ((h2rxrd(0xA8)&0xF0)<<4) + h2rxrd(0xA7);
	VSYNCW   = ((h2rxrd(0xA8)&0x01)<<8) + h2rxrd(0xA6);
	VSyncPol = (h2rxrd(0xAA)&0x40)>>6;
	CurVTiming.PCLK        = (iTE_u16)PCLK;
	CurVTiming.HActive     = HActive;
	CurVTiming.HTotal      = HTotal;
	CurVTiming.HFrontPorch = HFP;
	CurVTiming.HSyncWidth  = HSYNCW;
	CurVTiming.HBackPorch  = (HTotal - HActive - HFP - HSYNCW);
	CurVTiming.VActive     = VActive;
	CurVTiming.VTotal      = VTotal;
	CurVTiming.VFrontPorch = VFP;
	CurVTiming.VSyncWidth  = VSYNCW;
	CurVTiming.VBackPorch  = VTotal - VActive - VFP - VSYNCW;
	CurVTiming.ScanMode    = (InterLaced)&0x01;
	CurVTiming.VPolarity   = (VSyncPol)&0x01;
	CurVTiming.HPolarity   = (HSyncPol)&0x01;

	FrameRate = PCLK*1000;
    FrameRate /= CurVTiming.HTotal;
    FrameRate /= CurVTiming.VTotal;

	if(gext_u8->InColorMode == YCbCr420) HActive = 2*HActive;
	gext_u8->GCP_CD = ((h2rxrd( 0x98)&0xF0)>>4);
	if((FrameRate<40) && (HActive>=3840))
	{
		gext_u8->Rx_4K30 = 1;
	}
	else
	{
		gext_u8->Rx_4K30 = 0;
	}
	iTE_MsgRX(("Rx color depth = %02x  (0: 8bit  1: 10bit  2: 12bit)  \r\n",gext_u8->GCP_CD&0x03));

	switch(gext_u8->GCP_CD&0x03)
	{
		case 0:
			TMDS = PCLK;
			break;
		case 1:
			TMDS = 125*PCLK/100;
			break;
		case 2:
			TMDS = 150*PCLK/100;
			break;
		default:
			break;
	}
	#ifdef Support4096DS
		if(HActive == 4096)	Set_DNScale(CurVTiming);
	#endif
	iTE_MsgRX(("TMDSCLK = %ld Hz \r\n",TMDS));
	iTE_MsgRX(("Rx Video Resolution is %d x %d  %ldHz   \r\n",HActive,VActive,FrameRate));
	if((TMDS>30000)&&(FrameRate>100))//ite_171201   6G
	{
		gext_u8->HighFrameRate = 1;
	}
	else
	{
		gext_u8->HighFrameRate = 0;
	}
}
#ifdef Support4096DS
void Set_DNScale( VTiming pCurVTiming )
{
	iTE_u16 Src_HTotal,Src_Width,Src_Height,Max_YRGB,Min_YRGB,Max_CRCB,Min_CRCB,TG_HSPOL,
			TG_VSPOL,TG_HFP,TG_HSW,TG_HBP,TG_HDEW,TG_VFP,TG_VSW,TG_VBP,TG_VDEW,Ratio_Denominator
			,Ratio_Numerator,Ratio_Offset;
	iTE_u8 Double;


    // 20160926 reset downscale, and set register value,
    // release reset after register setting

	//hdmirxset(0x33, 0x08, 0x08); //reset down scale

    if(gext_u8->InColorMode==3) Double=1;
	else Double=0;
	Src_HTotal= (pCurVTiming.HTotal);

	Src_Width=(pCurVTiming.HActive)<<Double;
	Src_Height=pCurVTiming.VActive;
	Ratio_Denominator= Src_Width -2;
    Ratio_Numerator = (Src_Width/2) -1;
    Ratio_Offset    = (Src_Width/2) -1;

    //Default RGB/YUV  Full Range
	Max_YRGB = 0xFFF;
	Min_YRGB = 0x000;
	Max_CRCB = 0xFFF;
	Min_CRCB = 0x000;
    //For YUV  Limit Range
    #ifdef YUV_DS_Set_Limit_Range
    	if((gext_u8->CVOutColorMode == YCbCr444) || (gext_u8->CVOutColorMode == YCbCr420))
    	{
			Max_YRGB = 940;   // 235*4
			Min_YRGB = 64;		// 16*4
			Max_CRCB = 940;   // 235*4
			Min_CRCB = 64;		// 16*4
    	}
	#endif
    TG_HSPOL= pCurVTiming.HPolarity;
    TG_VSPOL= pCurVTiming.HPolarity;
	TG_HFP  = ((pCurVTiming.HFrontPorch)/2)<<Double;
	TG_HSW  = ((pCurVTiming.HSyncWidth)/2)<<Double;
	TG_HBP  = ((pCurVTiming.HBackPorch)/2)<<Double;
	TG_HDEW = ((pCurVTiming.HActive)/2)<<Double;
     /*
// for VIC 101/102, 98/99/100 : HDE 4096 => 1920 to VIC 31/16, 32/33/34
    if(Src_Width> 3840 ) {
	TG_HSPOL = 1;
    TG_VSPOL = 1;
	TG_HDEW = 1920;

	TG_HSW  = 44;
    TG_HBP  = 148;
    TG_HFP  = (Src_HTotal/2) -  TG_HBP - TG_HSW -1920;
	// TG_HFP and TG_HTotal diff at VIC101/VIC102

    Ratio_Numerator = TG_HDEW -1;
    Ratio_Offset    = TG_HDEW -1;
// end  for HDE 4096 to 3840 only
	}
*/
	TG_VFP  = (pCurVTiming.VFrontPorch)/2;
	TG_VSW  = (pCurVTiming.VSyncWidth)/2;
	TG_VBP  = (pCurVTiming.VBackPorch)/2;
	TG_VDEW = (pCurVTiming.VActive)/2;

	// for 4096 to 1080p only
	if(TG_HDEW == 2048)
	{
		TG_HSPOL = 1;
		TG_VSPOL = 1;
		TG_HDEW = 1920;
		TG_HSW = 44;
		TG_HBP = 148;
		//iTE_MsgRX(("Src_HTotal = %ld\r\n", Src_HTotal));
		//iTE_MsgRX(("pCurVTiming.HTotal = %ld\r\n", pCurVTiming.HTotal));
		//iTE_MsgRX(("Double = %d\r\n", Double));
		TG_HFP = ((pCurVTiming.HTotal/2)<< Double) - TG_HBP - TG_HSW -1920;
		Ratio_Numerator = TG_HDEW -1;
		Ratio_Offset = TG_HDEW - 1;
		//iTE_MsgRX(("!!!! TG_HFP = %d !!!!\r\n", TG_HFP));
		//iTE_MsgRX(("!!!! DownScale 2048x1080 to 1920x1080 !!!!\r\n"));
	}
	//iTE_MsgRX(("Target HActive= %d , HFrontPorch=%d, HSync Width=%d, HBackPorch=%d \r\n", TG_HDEW, TG_HFP,TG_HSW,  TG_HBP));
	//iTE_MsgRX(("Target VActive= %d , VFrontPorch=%d, VSync Width=%d, VBackPorch=%d \r\n", TG_VDEW, TG_VFP,TG_VSW,  TG_VBP));

	h2spwr(0x21, Src_Width&0xFF);
	h2spwr(0x22, (Src_Width&0xFF00)>>8);
	h2spwr(0x23, Src_Height&0xFF);
	h2spwr(0x24, (Src_Height&0xFF00)>>8);
	h2spwr(0x25, Ratio_Denominator&0xFF);
	h2spwr(0x26, (Ratio_Denominator&0xFF00)>>8);
	h2spwr(0x27, Ratio_Numerator&0xFF);
	h2spwr(0x28, (Ratio_Numerator&0xFF00)>>8);
	h2spwr(0x29, Ratio_Offset&0xFF);
	h2spwr(0x2A, (Ratio_Offset&0xFF00)>>8);
	h2spwr(0x2B, Max_YRGB&0xFF);
	h2spwr(0x2C, Min_YRGB&0xFF);
	h2spwr(0x2D, ((Min_YRGB&0xF00)>>4)+((Max_YRGB&0xF00)>>8));

	h2spwr(0x2E, Max_CRCB&0xFF);
	h2spwr(0x2F, Min_CRCB&0xFF);
	h2spwr(0x30, ((Min_CRCB&0xF00)>>4)+((Max_CRCB&0xF00)>>8));

	h2spwr(0x31,(0x02<<2));//force 12b in
	h2spset(0x33,0xC0,(TG_HSPOL<<7) + (TG_VSPOL<<6));
	h2spwr(0x32, TG_HFP&0xFF);
	h2spset(0x33,0x3F,(TG_HFP&0x3F00)>>8);
	h2spwr(0x34, TG_HSW&0xFF);
	h2spwr(0x35, (TG_HSW&0x3F00)>>8);
	h2spwr(0x36, TG_HBP&0xFF);
	h2spwr(0x37, (TG_HBP&0x3F00)>>8);
	h2spwr(0x38, TG_HDEW&0xFF);
	h2spwr(0x39, (TG_HDEW&0x3F00)>>8);
	h2spwr(0x3A, TG_VFP&0xFF);
	h2spwr(0x3B, (TG_VFP&0x3F00)>>8);
	h2spwr(0x3C, TG_VSW&0xFF);
	h2spwr(0x3D, (TG_VSW&0x3F00)>>8);
	h2spwr(0x3E, TG_VBP&0xFF);
	h2spwr(0x3F, (TG_VBP&0x3F00)>>8);
	h2spwr(0x40, TG_VDEW&0xFF);
	h2spwr(0x41, (TG_VDEW&0x3F00)>>8);
	//hdmirxset(0x20, 0x40, 0x00);
	//iTE_MsgRX(("Video DownScale Enable! Video DownScale Enable!\n"));

	h2spset(0x31, 0x03, 0x03);
}
#endif
void DefaultEdidSet(void)
{
	iTE_u16 i;
	iTE_u8 Trapping1,Trapping2;

	#ifdef _MCU_IT6350
		Trapping2 = GPDRG & 0x80;
		Trapping1 = GPDRC & 0x20;
	#endif
	#ifdef _MCU_IT6295
		Trapping2 = GPDRE & 0x04;
		Trapping1 = GPDRE & 0x02;
	#endif

	iTE_MsgRX(("Set Default Edid \r\n"));
	h2rxwr(0xC6, 0x00); //VSDB Start Address
	h2rxwr(0xC7, 0x00); //AB
    h2rxwr(0xC8, 0xFF); //CD
    h2rxwr(0x4B, 0xD9); //EDID SlaveAdr
    if(0)
    {
    		iTE_MsgRX(("Set Default Edid  2k\r\n"));
		for(i=0;i<0x7F;i++) h2rx_edidwr(i,Default_Edid_table2k[i]);
		h2rxwr(0xC9,Default_Edid_table2k[0x7F]);//bank0  checksum
		for(i=0x80;i<0xFF;i++) h2rx_edidwr(i,Default_Edid_table2k[i]);
		h2rxwr(0xCA,Default_Edid_table2k[0xFF]);//bank1  checksum
    }
	else
	{
		iTE_MsgRX(("Set Default Edid  4k\r\n"));
    	for(i=0;i<0x7F;i++) h2rx_edidwr(i,Default_Edid_table4k[i]);
		h2rxwr(0xC9,Default_Edid_table4k[0x7F]);//bank0  checksum
		for(i=0x80;i<0xFF;i++) h2rx_edidwr(i,Default_Edid_table4k[i]);
		h2rxwr(0xCA,Default_Edid_table4k[0xFF]);//bank1  checksum
	}
	EDID_ParseVSDB_3Dblock(&st3DParse);
}
void it6664_h2rx_pwdon(void)
{
	h2rxset(0x53, 0xE0, 0x00);
	h2rxset(0x54, 0xFF, 0x00);
	h2rxset(0x55, 0x07, 0x00);
	h2rxset(0x57, 0x0F, 0x00);
	// emily add 20151225 reset EDID when 5V-OFF => HPD off
	h2rxset(0xC5, 0x10, 0x10); // emily add for EDID reset
	h2rxset(0xC5, 0x10, 0x00); // emily add for EDID reset
	h2spset(0x0A,0x04,0x04); //reset HP2 block
	mSleep(1);
	h2spset(0x0A,0x04,0x00);
	if( gext_u8->BUSMODE==MHL ) //20170116
	{
		chgrxbank(3);
		h2rxwr(0xAB, 0x00);
		h2rxwr(0xAC, 0x00);
		chgrxbank(0);
	}
	else
	{
		SetRxHpd(FALSE);
	}
	gext_u8->BUSMODE = HDMI;
	gext_u8->Auto20done = 0;
	gext_u8->Auto14done = 0;
	gext_u8->EQ14Going = 0;
	gext_u8->EQ20Going = 0;
	gext_u8->AutoEQsts = STAT_EQ_WaitInt;
	gext_u8->RxtoggleHPD = 0;

	#if (USING_1to8 == TRUE)
		iTE_u8 tmp,i;
		tmp= g_device;
		if(tmp == IT6663_C)
		{
			chgrxbank(3);
			h2rxwr(0x27,0x9F);
			h2rxwr(0x28,0x9F);
			h2rxwr(0x29,0x9F);
			h2rxwr(0x20, 0x1B);//cs setting
			h2rxwr(0x21, 0x03);
			h2rxwr(0x2D, 0x00); // No Force SKEW
			chgrxbank(0);
			IT6664_DeviceSelect(IT6664_A);
			gext_u8->RXHDCP = 0;
			for(i=0;i<4;i++) gext_var->HDCPState[i] = Tx_CP_Reset;
			IT6664_DeviceSelect(IT6664_B);
			gext_u8->RXHDCP = 0;
			for(i=0;i<4;i++) gext_var->HDCPState[i] = Tx_CP_Reset;
			IT6664_DeviceSelect(tmp);
			gext_u8->RXHDCP = 0;
		}
		else
		{
			gext_u8->RXHDCP = 0;
		}
	#else
		chgrxbank(3);
		h2rxwr(0x27,0x9F);
		h2rxwr(0x28,0x9F);
		h2rxwr(0x29,0x9F);
		h2rxwr(0x20, 0x1B);//cs setting
		h2rxwr(0x21, 0x03);
		chgrxbank(0);
		gext_u8->RXHDCP = 0;
	#endif
	//iTE_MsgRX(("BUS Mode change to MHL mode \r\n"));
	//h2rx_rst(0);
}
void RX_FiFoRst(void)
{
	h2spwr(0x0B, 0xFF); //Reset FIFO
	h2spwr(0x0B, 0x00);
	//Reset All fifo path
	h2spset(0x4E,0x0F,0x0F);
	h2spset(0x0C,0x08,0x08);
	h2spset(0x0C,0x08,0x00);
	h2spset(0x4E,0x0F,0x00);
}
iTE_u32 tolerr[3];

#if 0
void Check_BitErr(void)
{
	iTE_u8 i;
	iTE_u8 SarEQBER[2]={0,0};
	static iTE_u8 k=0;

	if (!(KSI&0x40))//trapping 4
	{
		for(i=0;i<3;i++) tolerr[i] = 0;
	}
	h2rxset(0x3B, 0x08, 0x08);  // read CED Error from SAREQ CEDError Counter
	chgrxbank(3);
	h2rxset(0x55, 0x80, 0x00);
	h2rxwr(0xE9, 0x00);

	k=1;
    for(i=0;i<3;i++)
    {
        h2rxwr(0xE9, (i*2)<<4);
        h2rxbrd(0xEA, 0x02,SarEQBER);
		iTE_MsgRX(("BitErr of Channel%d = 0x%02x 0x%02x \r\n",(iTE_u16)i,(iTE_u16)SarEQBER[1],(iTE_u16)SarEQBER[0]));
		tolerr[i] = tolerr[i] + (iTE_u32)((SarEQBER[1]&0x7F)*0x100) + (iTE_u32)SarEQBER[0];
		if(tolerr[i] > 0xFFFF8000) tolerr[i]= 0;
	}
	for(i=0;i<3;i++) iTE_MsgRX(("Ch%d total error = 0x%04x \r\n",(iTE_u16)i,tolerr[i]));
	h2rxwr(0xE9, 0x80);
	chgrxbank(0);
}

#endif

void SetRxHpd(iTE_u8 sts)
{
	if(sts)
	{
		if(h2rxrd(0x13)&0x01)
		{
			chgrxbank(3);
			if(h2rxrd(0xAB) != 0xCA)
			{
				iTE_MsgRX(("[RX]	Set RX HPD high \r\n"));
				h2rxwr(0xAB,0xCA);
			}
			chgrxbank(0);
			h2rxset(0xC5,0x01,0x00);//enable EDID
			if(gext_u8->BUSMODE == MHL) h2rxwr(0x26,0x0C);
			else h2rxwr(0x26,0x00);
			h2rxwr(0x55,0xFF);
		}
	}
	else
	{
		chgrxbank(3);
		if(h2rxrd(0xAB) == 0xCA)
		{
			iTE_MsgRX(("[RX]	Set RX HPD low \r\n"));
			h2rxwr(0xAB, 0x4A);
        	h2rxwr(0xAB, 0x00); //no matter what, when 5V is off, first pull down HPD, then release CBUS/HPD pin
			h2rxwr(0xAC, 0x00);
		}
		chgrxbank(0);
		h2rxset(0xC5,0x01,0x01);//disable EDID
		h2rxwr(0x26,0xFF);
		h2rxwr(0x55,0x00);
	}
}
void EDID_ParseVSDB_3Dblock(struct PARSE3D_STR *pstParse3D)
{

    iTE_u8	ucTemp;
    iTE_u8	uc3DMulti;
    iTE_u8	uc3DEdidEnd = 0xFF;
    iTE_u8	ucRdPtr = pstParse3D->uc3DEdidStart;

    PARSE3D_STA	e3DEdidState = PARSE3D_START;


    //check with HDMI VSDB block of EDID
    if(pstParse3D->bVSDBspport3D == 0x00)
    {
        pstParse3D->ucVicCnt=0;
        return;
    }


    // Re-initial bVSDBspport3D =0 then check with 3D_Structure and 3D_MASK at HDMI VSDB block of EDID
    pstParse3D->bVSDBspport3D = 0x00;


    iTE_MsgMHL(("***   EDID_ParseVSDB_3Dblock   ***\r\n"));
    iTE_MsgMHL(("MHL 3D [2]LR [1]TB [0]FS \r\n"));

    if(ucRdPtr ==0)
        return;

        for(;ucRdPtr <= uc3DEdidEnd;){

            switch(e3DEdidState){
                    case	PARSE3D_START:
                            uc3DMulti = Default_Edid_table4k[ucRdPtr++];

                            if(uc3DMulti & 0x80){
                                uc3DMulti &= 0x60;
                                e3DEdidState = PARSE3D_LEN;
                            }else{
                                return;
                            }
                        break;
                    case	PARSE3D_LEN:
                            uc3DEdidEnd = (Default_Edid_table4k[ucRdPtr] >> 5) + (Default_Edid_table4k[ucRdPtr] & 0x1F) +ucRdPtr;
                            ucRdPtr += (Default_Edid_table4k[ucRdPtr] >> 5) + 1;
                            e3DEdidState = PARSE3D_STRUCT_H;
                        break;
                    case	PARSE3D_STRUCT_H:
                            switch(uc3DMulti){
                                case	0x20:
                                case	0x40:
                                        if(Default_Edid_table4k[ucRdPtr++] & 0x01){
                                            uc3DMulti |= 0x04;
                                        }
                                        e3DEdidState = PARSE3D_STRUCT_L;
                                    break;
                                default:
                                        e3DEdidState = PARSE3D_VIC;
                                    break;
                            }
                        break;
                    case	PARSE3D_STRUCT_L:
                            ucTemp = Default_Edid_table4k[ucRdPtr++];
                            if(ucTemp & 0x40)
                                uc3DMulti |= 0x02;
                            if(ucTemp & 0x01)
                                uc3DMulti |= 0x01;


                            if((uc3DMulti & 0x60) == 0x20){
                                e3DEdidState = PARSE3D_VIC;
                                uc3DMulti &= 7;

                                for(ucTemp=0; ucTemp<16; ucTemp++){
                                    pstParse3D->uc3DInfor[ucTemp] = uc3DMulti;
                                    iTE_MsgMHL(("VSD[%d]=0x%02x \r\n", (iTE_u16)ucTemp,uc3DMulti));
                                }

                            }else{
                                e3DEdidState = PARSE3D_MASK_H;
                                uc3DMulti &= 7;
                            }
                        break;
                    case	PARSE3D_MASK_H:

                            if(Default_Edid_table4k[ucRdPtr])
                                pstParse3D->bVSDBspport3D = 0x01;	//for identify 3D_MASK have Short Video Descriptor (SVD) support 3D format

                            for(ucTemp=0; ucTemp<8; ucTemp++){
                                if(Default_Edid_table4k[ucRdPtr] & (1<<ucTemp)){
                                    pstParse3D->uc3DInfor[ucTemp+8] = uc3DMulti;
                                    iTE_MsgMHL(("VSD[%d]=0x%02x\r\n",(iTE_u16) ucTemp+8,uc3DMulti));
                                }else{
                                    pstParse3D->uc3DInfor[ucTemp+8] = 0;
                                }
                            }
                            ucRdPtr++;
                            e3DEdidState = PARSE3D_MASK_L;
                        break;
                    case	PARSE3D_MASK_L:

                            if(Default_Edid_table4k[ucRdPtr])
                                pstParse3D->bVSDBspport3D = 0x01;	//for identify 3D_MASK have SVD support 3D format

                            for(ucTemp=0; ucTemp<8; ucTemp++){
                                if(Default_Edid_table4k[ucRdPtr] & (1<<ucTemp)){
                                    pstParse3D->uc3DInfor[ucTemp] = uc3DMulti;
                                    iTE_MsgMHL(("VSD[%d]=0x%02x\r\n", (iTE_u16)ucTemp, uc3DMulti));
                                }else{
                                    pstParse3D->uc3DInfor[ucTemp] = 0;
                                }
                            }
                            ucRdPtr++;
                            e3DEdidState = PARSE3D_VIC;
                        break;
                    case	PARSE3D_VIC:
                                ucRdPtr+=1;
                        break;
                    default:
                        break;
                }
        }
}

void  Dump_IT666xReg(void)
{

#if 0
    iTE_u16	i,j;
    iTE_u8 ucData;

	iTE_MsgRX(("=========================================================\r\n"));
    for(j = 0; j < 16; j++)
    {
        iTE_MsgRX((" %02X",(iTE_u16) j));
        if((j == 3)||(j==7)||(j==11))
        {
                iTE_MsgRX((" :"));
        }
    }
    iTE_MsgRX(("=========================================================\r\n"));

	chgrxbank(0);

    for(i = 0; i < 0x100; i+=16)
    {
        iTE_MsgRX(("[%03X]  ",i));
        for(j = 0; j < 16; j++)
        {
            ucData = h2rxrd((iTE_u8)((i+j)&0xFF));
            iTE_MsgRX((" %02X",(int) ucData));
            if((j == 3)||(j==7)||(j==11))
            {
                iTE_MsgRX((" :"));
            }
        }
        iTE_MsgRX(("\r\n"));
        if((i % 0x40) == 0x30)
        {
    		iTE_MsgRX(("=========================================================\r\n"));
        }
    }

	chgrxbank(1);
    for(i = 0x0; i < 0x100; i+=16)
    {
        iTE_MsgRX(("[%03X]  ",i+0x100));
        for(j = 0; j < 16; j++)
        {
            ucData = h2rxrd((iTE_u8)((i+j)&0xFF));
            iTE_MsgRX((" %02X",(iTE_u16) ucData));
            if((j == 3)||(j==7)||(j==11))
            {
                iTE_MsgRX((" :"));
            }
        }
        iTE_MsgRX(("\r\n"));
        if((i % 0x40) == 0x30)
        {
            iTE_MsgRX(("=========================================================\r\n"));
        }
    }

	chgrxbank(2);
    for(i = 0x0; i < 0x100; i+=16)
    {
        iTE_MsgRX(("[%03X]  ",i+0x200));
        for(j = 0; j < 16; j++)
        {
            ucData = h2rxrd((iTE_u8)((i+j)&0xFF));
            iTE_MsgRX((" %02X",(iTE_u16) ucData));
            if((j == 3)||(j==7)||(j==11))
            {
                iTE_MsgRX((" :"));
            }
        }
        iTE_MsgRX(("\r\n"));
        if((i % 0x40) == 0x30)
        {
            iTE_MsgRX(("=========================================================\r\n"));
        }
    }
	chgrxbank(3);
    for(i = 0x0; i < 0x100; i+=16)
    {
        iTE_MsgRX(("[%03X]  ",i+0x300));
        for(j = 0; j < 16; j++)
        {
            ucData = h2rxrd((iTE_u8)((i+j)&0xFF));
            iTE_MsgRX((" %02X",(iTE_u16) ucData));
            if((j == 3)||(j==7)||(j==11))
            {
                iTE_MsgRX((" :"));
            }
        }
        iTE_MsgRX(("\r\n"));
        if((i % 0x40) == 0x30)
        {
            iTE_MsgRX(("=========================================================\r\n"));
        }
    }
	chgrxbank(0);
#endif
}
#ifdef Disable_RXHDCP
void it6664_RXHDCP_OFF(iTE_u8 sts)
{
	static iTE_u8 stsbak = 0;
	iTE_u8 hpd=0;
	if(sts == TRUE)
	{
		//iTE_MsgRX(("RXHDCP_OFF \r\n"));
		h2spset(0x0A,0x04,0x04);
		h2rxset(0x23,0x42,0x42);
		h2spset(0x0C,0x04,0x04);
		if(stsbak != sts) hpd = 1;
	}
	else
	{
		//iTE_MsgRX(("RXHDCP_ON \r\n"));
		h2spset(0x0A,0x04,0x00);
		h2rxset(0x23,0x42,0x00);
		h2spset(0x0C,0x04,0x00);
		if(stsbak != sts) hpd = 1;
	}
	if(hpd)
	{
		SetRxHpd(FALSE);
		mSleep(2000);
		SetRxHpd(TRUE);
		stsbak = sts;
	}
}
#endif
#ifdef RXHDCP_Follow_SinkX
void it6664_RXHDCP_Set(iTE_u8 sts)
{
	static iTE_u8 stsbak = 1;//default HDCP2.2
	iTE_u8 hpd=0;

	if(sts==3)//no hdcp
	{
			h2spset(0x0A,0x04,0x04);
			h2rxset(0x23,0x42,0x42);
			h2spset(0x0C,0x04,0x04);
			//if(stsbak != sts) hpd = 1;
	}
	else if(sts==0)//only HDCP14
	{
			h2spset(0x0A,0x04,0x04);
			//h2spset(0x0C,0x04,0x04);
			//if(stsbak != sts) hpd = 1;
	}
	else//2.2
	{
			h2spset(0x0A,0x04,0x00);
			h2rxset(0x23,0x42,0x00);
			h2spset(0x0C,0x04,0x00);
			//if(stsbak != sts) hpd = 1;
	}
	if(stsbak != sts) hpd = 1;
	if(hpd)
	{
		SetRxHpd(FALSE);
		mSleep(2000);
		SetRxHpd(TRUE);
		stsbak = sts;
		iTE_MsgRX(("change RXHDC_Ver = %2x \r\n",sts));
	}
}



#endif

void it6664_SetFixEQ(iTE_u8 val)
{
	chgrxbank(3);
	h2rxwr(0x27,val);
	h2rxwr(0x28,val);
	h2rxwr(0x29,val);
	chgrxbank(0);
}
#ifdef AutoEQ
iTE_u8 CHB_TargetRS[3];
iTE_u8 Wait;
iTE_u8 EQ20[3];
iTE_u8 EQ14[3];
iTE_u8 DFE[14][3][3];//RS/CH/3
iTE_u8 fail_ch[3];
iTE_u8 EQ_sel[3];
iTE_u8 DFEBak[9];
iTE_u8 SkewResult;
iTE_u16 AMP_TimeOut[3];
iTE_u8 _CODE RS_Value[14] =  { 0x7F, 0x7E, 0x3F, 0x3E, 0x1F, 0x1E, 0x0F, 0x0E,
				0x07, 0x06, 0x03, 0x02, 0x01, 0x00 };

void it6664_AutoEQ_State(void)
{
	iTE_u8 x;
	iTE_u8 SKEWOPT = 0;
	iTE_u8 BitErrOK;
	iTE_u8 Threshold = 0x80;
	iTE_u8 symlock,Reg14,HDMI_Mode,ENScb,ret,clkstb,regB9,regBE,regBF;
	static iTE_u8 RetryCnt=0;

	if(g_device == IT6663_C)
	{
		Reg14 = h2rxrd(0x14);
		if((Reg14&0x38) == 0x38) symlock = 1;
		else symlock = 0;

		HDMI_Mode = Reg14&0x40;
		ENScb = Reg14&0x80;
		clkstb = h2rxrd(0x13)&0x10;

		switch(gext_u8->AutoEQsts)
		{
			case STAT_EQ_WaitInt:
				break;
			case STAT_EQ_rst:
				it6664_EQRst();
				break;
			case STAT_EQ_Start:
				if(clkstb)
				{
					if(HDMI_Mode)
					{
						h2rxset(0x53,0x20,0x00);//Mask ECC error int
						h2rxset(0x05,0x20,0x20);//clear ECC error int
						if(!gext_u8->Auto20done)
						{
							iTE_MsgEQ(("[EQ]	HDMI 2.0 EQ Start !! !! \r\n"));
							gext_u8->Auto20done = 0;
							gext_u8->EQ20Going = 1;
							gext_u8->EQ14Going = 0;
							if(ENScb)
							{
								//h2rxset(0x22,0x38,0x00);
								it6664_Set_SAREQ(SKEWOPT);
								gext_u8->AutoEQsts = STAT_EQ_WaitInt;
							}
							else
							{
								//iTE_MsgEQ(("[EQ]	HDMI 2.0 not Enable scramble !! !! \r\n"));
								//gext_u8->AutoEQsts = STAT_EQ_Manu;
							}
						}
					}
					else
					{
						if(!gext_u8->Auto14done)
						{
							RetryCnt++;
							it6664_EQ14(0x1F);
						}
					}
				}
				break;
			case STAT_EQ_WaitSarEQDone:
				if(gext_u8->EQ20Going)
				{
					iTE_MsgEQ(("[EQ]	2.0 SAREQ Done !! !! \r\n"));
					it6664_RPTSAREQ(SKEWOPT,HDMI_Mode);
					gext_u8->AutoEQsts = STAT_EQ_CheckBiterr;
					chgrxbank(3);
					for(x=0;x<3;x++) h2rxwr(0x27+x,EQ20[x]);
					h2rxset(0x22, 0x40, 0x40); //  DFE
					for(x=0;x<9;x++) DFEBak[x] = h2rxrd(0x4B+x);
					//for(x=0;x<9;x++) iTE_MsgEQ(("[EQ]	DFE = %02x \r\n",h2rxrd(0x4B+x)));
					h2rxwr(0xE9, 0x80);
					mSleep(10);
					h2rxwr(0xE9, 0x80);
					mSleep(10);
					chgrxbank(0);
					RX_FiFoRst();
				}
				if(gext_u8->EQ14Going)//1.4
				{
					iTE_MsgEQ(("[EQ]	1.4 EQ Done !! !! \r\n"));
					gext_u8->EQ14Going = 0;
					ret = it6664_RPTSAREQ(SKEWOPT,HDMI_Mode);
					chgrxbank(3);
					h2rxset(0x22,0x38,0x00);
					for(x=0;x<3;x++) h2rxwr(0x27+x,EQ14[x]);
					h2rxwr(0xE9, 0x80);
					mSleep(10);
					chgrxbank(0);
					if((ret)|| (RetryCnt>2))
					{
						gext_u8->AutoEQsts = STAT_EQ_CheckBiterr;
						gext_u8->Auto14done = 1;
						RetryCnt = 0;
					}
					else
					{
						gext_u8->AutoEQsts = STAT_EQ_Start;
						gext_u8->Auto14done = 0;
					}
				}
				//h2rxset(0x54,0x7F,0x00);
				h2rxset(0x53,0x20,0x20);//enable ECC error int
				h2rxset(0x05,0x20,0x20);//clear ECC error int
				break;
			case STAT_EQ_CheckBiterr:
				if(symlock)
				{
					for(x=0;x<3;x++) EQ_sel[x] = 0;
					BitErrOK=it6664_RptBitErr_ms(Threshold);
					if((HDMI_Mode)&&(gext_u8->EQ20Going))
					{
						if(BitErrOK==0)
						{
							iTE_MsgEQ(("[EQ]	Do Skew EQ  !! \r\n"));
							it6664_SigleRSSKEW();
        					it6664_Read_SKEW();
							gext_u8->AutoEQsts = STAT_EQ_AutoEQDone;
							gext_u8->Auto20done = 1;
							gext_u8->EQ20Going = 0;
							Wait = 2;
						}
						else
						{
							gext_u8->Auto20done = 1;
							gext_u8->EQ20Going = 0;
							gext_u8->AutoEQsts = STAT_EQ_AutoEQDone;
							Wait = 2;
						}
					}
					else
					{
						gext_u8->AutoEQsts = STAT_EQ_AutoEQDone;
						Wait = 2;
					}
				}
				break;
			case STAT_EQ_AutoEQDone:
				if(Wait)
				{
					Wait--;
				}
				else
				{
					if(h2rxrd(0x13)&0x10)
					{
						regB9 = h2rxrd(0xB9);
						regBE = h2rxrd(0xBE);
						regBF = h2rxrd(0xBF);
						if(symlock)
						{
							BitErrOK=it6664_RptBitErr_ms(Threshold);
							if(BitErrOK == 0)
							{
								if((!regB9)&&(!regBE)&&(!regBF))
								{
									//iTE_MsgEQ(("[EQ]	1.4 Disparity error,skip manu EQ  !! \r\n"));
								}
								else
								{
									iTE_MsgEQ(("[EQ]	1 manu EQ  !! \r\n"));
									it6664_ManuEQ(HDMI_Mode);
								}
							}
						}
						h2rxwr(0xB9,0xFF);
						h2rxwr(0xBE,0xFF);
						h2rxwr(0xBF,0xFF);
					}
				}
				break;
		default:
			break;
	}
	}
}
void it6664_EQchkOldResult(iTE_u8 hdmiver)
{
	iTE_u8 x;

	if(hdmiver)
	{
		if(gext_u8->Auto20done)
		{
			chgrxbank(3);
			h2rxwr(0x20, 0x1B);//cs setting
			h2rxwr(0x21, 0x03);
			for(x=0;x<3;x++) h2rxwr(0x27+x,EQ20[x]);
			for(x=0;x<9;x++) h2rxwr(0x4B+x,DFEBak[x]);
			h2rxset(0x4B, 0x80, 0x80);//SET DFE
			h2rxset(0x22, 0x40, 0x40);//ENDFE
			h2rxwr(0x2C,SkewResult);
			h2rxset(0x2D,0x07,0x07);
			chgrxbank(0);
			iTE_MsgEQ(("[EQ]	Use backup 2.0 EQ  \r\n"));
			gext_u8->AutoEQsts = STAT_EQ_AutoEQDone;
		}
		else
		{
			gext_u8->AutoEQsts = STAT_EQ_WaitInt;
		}
	}
	else
	{
		if(gext_u8->Auto14done)
		{
			chgrxbank(3);
			h2rxwr(0x20, 0x36);//cs setting
			h2rxwr(0x21, 0x0E);
			h2rxwr(0x2C, 0x00);//disable skew
			h2rxwr(0x2D, 0x00);//disable skew
			h2rxset(0x22, 0x40, 0x00);//DisDFE
			for(x=0;x<3;x++) h2rxwr(0x27+x,EQ14[x]);
			chgrxbank(0);
			iTE_MsgEQ(("[EQ]	Use backup 1.4 EQ  \r\n"));
			gext_u8->AutoEQsts = STAT_EQ_AutoEQDone;
		}
		else
		{
			gext_u8->AutoEQsts = STAT_EQ_WaitInt;
		}
	}
}
void it6664_EQRst(void)
{
	h2rxset(0x07,0xF0,0xF0);//clear int
	h2rxset(0x23,0x10,0x10);
	mSleep(1);
	h2rxset(0x23,0x10,0x00);
	chgrxbank(3);
	h2rxwr(0x20, 0x1B);//cs setting
	h2rxwr(0x21, 0x03);
	h2rxset(0x22,0xFF,0x00);//trigger off
	h2rxset(0x4B, 0x80, 0x00);//disable DFE value
	h2rxwr(0x2D, 0x00); // No Force SKEW
	chgrxbank(0);
	gext_u8->EQ14Going = 0;
	gext_u8->EQ20Going = 0;
	gext_u8->AutoEQsts = STAT_EQ_WaitInt;
	//h2rxset(0x54,0x7F,0x00);
	h2rxset(0x53,0x20,0x20);//enable ECC error int
	h2rxset(0x05,0x20,0x20);//clear ECC error int
}
void it6664_EQ14(iTE_u8 val)
{
	iTE_MsgEQ(("[EQ]	HDMI 1.4 EQ Start !! !! \r\n"));
	gext_u8->Auto14done = 0;
	gext_u8->EQ14Going = 1;
	gext_u8->EQ20Going = 0;
	h2rxwr(0x07, 0xff);
	h2rxwr(0x23, 0xB0);
	h2rxwr(0x23, 0xA0);
	chgrxbank(3);
	h2rxwr(0x2C, 0x00);//disable skew
	h2rxwr(0x2D, 0x00);//disable skew
	h2rxwr(0x20, 0x36);//cs setting //36
	h2rxwr(0x21, 0x0E);//E
	h2rxwr(0x26, 0x00);
	h2rxwr(0x27, val);
	h2rxwr(0x28, val);
	h2rxwr(0x29, val);
	h2rxset(0x22,0x38,0x38);
	h2rxset(0x22,0x04,0x04);
	mSleep(1);
	h2rxset(0x22,0x04,0x00);
	chgrxbank(0);
	gext_u8->AutoEQsts = STAT_EQ_WaitInt;
}
void it6664_Set_SAREQ(iTE_u8 SKEWOPT)
{
	chgrxbank(3);
    h2rxset(0x20, 0x80, 0x00); // disable CLKStb AutoEQTrg
	h2rxwr(0x22, 0x00); // MUST! disable EQTrg before EQRst
	chgrxbank(0);
	h2rxwr(0x07, 0xff);
	h2rxwr(0x23, 0xB0); // EQRst
	h2rxwr(0x23, 0xA0); // release EQRst
	h2rxset(0x3B, 0x07, 0x03);// Reg_CEDOPT[5:0]
	//h2rxwr(0x3B, 0x03);// change 20170314
	chgrxbank(3);
	// port 0 EQ setup SAREQ //
	//hdmirxset(0x20, 0x80, 0x00); // disable CLKStb AutoEQTrg
	//hdmirxset(0x22, 0x44, 0x00); // disable [4] ENDFE, set [2] EQTrg low
	h2rxwr(0x26, 0x00);
	h2rxwr(0x27, 0x1F);
	h2rxwr(0x28, 0x1F);
	h2rxwr(0x29, 0x1F);
	h2rxset(0x2D,0x07,0x00);//clear force skew
	h2rxwr(0x30, 0x80+ (SKEWOPT<<2)); // [2] SKEWOPT
	h2rxwr(0x31, 0xB0);        // AMPTime[7:0]

	h2rxwr(0x32, 0x43);
	h2rxwr(0x33, 0x47);
	h2rxwr(0x34, 0x4B);
	h2rxwr(0x35, 0x53);

	h2rxset(0x36, 0xc0, 0x00); // [7:6] AMPTime[9:8]
    //hdmirxset(0x37, 0xC3, 0x03); // [7:6] RecChannel [1:0] MacthNoSel
    //hdmirxset(0x37, 0x18, 0x08); // [4]: RSOnestage [3] IgnoreOPT
    h2rxwr(0x37, 0x0B);        // [7:6] RecChannel, [4]: RSOnestage,
	                             // [3] IgnoreOPT, [1:0] MacthNoSel
	h2rxwr(0x38, 0xF2);        // [7:4] MonTime
	//hdmirxset(0x39, 0x0F, 0x0D); // [3:0] CED Valid Threshold 0x0D
	//hdmirxset(0x39, 0x30, 0x00); // [5] POLBOPT, [4] ADDPClrOPT
    h2rxwr(0x39, 0x0D);       // [5] POLBOPT, [4] ADDPClrOPT, [3:0] CED Valid Threshold 0x0D
	h2rxset(0x4A, 0x80, 0x00);
    h2rxset(0x4B, 0x80, 0x00);
    h2rxset(0x54, 0x80, 0x80); // Reg_EN_PREEQ

	h2rxset(0x54, 0x38, 0x38); //
	h2rxwr(0x55, 0x40);  // RSM Threshold
	h2rxset(0x22, 0x04, 0x04);  // Trigger EQ
	chgrxbank(0);
}

iTE_u8 it6664_RPTSAREQ(iTE_u8 SKEWOPT,iTE_u8 HDMI_mode)
{
	iTE_u8 Reg63h, Reg64h,ret;
	iTE_u8 Reg6Dh, Reg6Eh;
	iTE_u8 CHB_DFE_A, CHB_DFE_B, CHB_DFE_C;
	iTE_u8 CHG_DFE_A, CHG_DFE_B, CHG_DFE_C;
	iTE_u8 CHR_DFE_A, CHR_DFE_B, CHR_DFE_C;
	iTE_u8 x,Valid_CED[3];
	iTE_u8 All_Ignore,TimeOut;

	ret = 0;
	chgrxbank(3);
	//Check Target RS value
    CHB_TargetRS[0] = h2rxrd(0xD5)&0x7F;
    CHB_TargetRS[1] = h2rxrd(0xD6)&0x7F;
	CHB_TargetRS[2] = h2rxrd(0xD7)&0x7F;

	for(x=0;x<3;x++)
	{
		if(HDMI_mode)
		{
			EQ20[x] = CHB_TargetRS[x]+0x80;
			iTE_MsgEQ(("CH %d, EQ %02x \r\n", x,EQ20[x]));
		}
		else
		{
			if(((h2rxrd(0xD0)&(0x03<<(2*x)))>>(2*x)) == 0x03)
			{
				Valid_CED[x] = 1;
			}
			else
			{
				iTE_MsgEQ(("CH %d, EQ fail \r\n", x));
				Valid_CED[x] = 0;
			}
		}
	}

	if(HDMI_mode)//2.0 EQ
	{
    	/* Read Back FSM DFE results */
    	h2rxset(0x4A, 0x80, 0x80); // readbacl EQ2 FSM results
		CHB_DFE_A=(h2rxrd(0x4B)&0x7F);
		CHB_DFE_B=(h2rxrd(0x4C)&0x3F);
		CHB_DFE_C=(h2rxrd(0x4D)&0x1F);
		CHG_DFE_A=(h2rxrd(0x4E)&0x7F);
		CHG_DFE_B=(h2rxrd(0x4F)&0x3F);
		CHG_DFE_C=(h2rxrd(0x50)&0x1F);
		CHR_DFE_A=(h2rxrd(0x51)&0x7F);
		CHR_DFE_B=(h2rxrd(0x52)&0x3F);
		CHR_DFE_C=(h2rxrd(0x53)&0x1F);
		h2rxset(0x4A, 0x80, 0x00); // readbakc register DFE setting

    	All_Ignore=0;
		TimeOut=0;
		for(x=0;x<3;x++)
		{
	   		h2rxset(0x37, 0xC0, x <<6 );
	   		Reg63h=h2rxrd(0x63);
       		Reg64h=h2rxrd(0x64);
       		Reg6Dh=h2rxrd(0x6D);
       		Reg6Eh=h2rxrd(0x6E);
	   		Valid_CED[x] =(Reg6Eh<<8) + Reg6Dh;
       		AMP_TimeOut[x]= (Reg64h<<8) + Reg63h;
	   		if(AMP_TimeOut[x]==0x3FFF)
	   		{
		  		TimeOut=1;
		   		//iTE_MsgEQ(("CH %d, Timeout\r\n", x));
	   		}
       		if(Valid_CED[x]==0x0)
			{
		   		All_Ignore=1;
		   		//iTE_MsgEQ(("CH %d, All Ingore !!!!!!!!\r\n", x));
	   		}
		}

		if(TimeOut==1 ) iTE_MsgEQ((" All Time Out!! \r\n"));
		else
		{
			if(All_Ignore==1 )
			{
           	 	Check_AMP(0);
           	 	Check_AMP(1);
            	Check_AMP(2);
            	iTE_MsgEQ(("After Adjustment, TargetRS are :\r\n"));
			}
			else
			{
				h2rxset(0x4B, 0x7F, CHB_DFE_A);
            	h2rxset(0x4C, 0x3F, CHB_DFE_B);
           	 	h2rxset(0x4D, 0x1F, CHB_DFE_C);
            	h2rxset(0x4E, 0x7F, CHG_DFE_A);
            	h2rxset(0x4F, 0x3F, CHG_DFE_B);
            	h2rxset(0x50, 0x1F, CHG_DFE_C);
				h2rxset(0x51, 0x7F, CHR_DFE_A);
            	h2rxset(0x52, 0x3F, CHR_DFE_B);
            	h2rxset(0x53, 0x1F, CHR_DFE_C);
            	h2rxset(0x4B, 0x80, 0x80); // SETDFE
				//iTE_Msg(("Set CHB DFE_A=%x ,  DFE_B=%x , DFE_C=%x \n",CHB_DFE_A, CHB_DFE_B, CHB_DFE_C));
            	//iTE_Msg(("Set CHG DFE_A=%x ,  DFE_B=%x , DFE_C=%x \n",CHG_DFE_A, CHG_DFE_B, CHG_DFE_C));
            	//iTE_Msg(("Set CHR DFE_A=%x ,  DFE_B=%x , DFE_C=%x \n",CHR_DFE_A, CHR_DFE_B, CHR_DFE_C));
			}
		}
		iTE_MsgEQ(("[EQ]	EQ B use %02x \r\n", EQ20[0]));
		iTE_MsgEQ(("[EQ]	EQ G use %02x \r\n", EQ20[1]));
   		iTE_MsgEQ(("[EQ]	EQ R use %02x \r\n", EQ20[2]));
		/*
		if((h2rxrd(0x14)&0x38) != 0x38)
		{
			h2rxset(0x23, 0x01, 0x01);
			h2rxset(0x23, 0x01, 0x00);
		}
		*/
		//Report Target RS value again
	}
	else //1.4 EQ
	{
		if((!Valid_CED[0]) && (!Valid_CED[1]) && (!Valid_CED[2]))  // all fail
		{
			ret = 0;
			EQ14[0] = 0x9F;
			EQ14[1] = 0x9F;
			EQ14[2] = 0x9F;
		}
		else
		{
			for(x=0;x<3;x++)
			{
				if(Valid_CED[x])
				{
					All_Ignore = CHB_TargetRS[x];// reference value  for fail channel
					break;
				}
			}
			for(x=0;x<3;x++)
			{
				if(Valid_CED[x])
				{
					EQ14[x] = CHB_TargetRS[x]+0x80;
				}
				else
				{
					EQ14[x] = All_Ignore+0x80;
				}
			}
			ret = 1;
		}
		iTE_MsgEQ(("[EQ]	Rec_B_RS =%02x, EQ use %02x \r\n", CHB_TargetRS[0],EQ14[0]));
		iTE_MsgEQ(("[EQ]	Rec_G_RS =%02x, EQ use %02x \r\n", CHB_TargetRS[1],EQ14[1]));
   		iTE_MsgEQ(("[EQ]	Rec_R_RS =%02x, EQ use %02x \r\n", CHB_TargetRS[2],EQ14[2]));
	}

	if(TimeOut==0) h2rxset(0x22, 0x44, 0x40); // Enable [6] ENDFE, disable [2] EQTrig
	#ifdef skew
    if(SKEWOPT==1) it6664_Read_SKEW();
	#endif
	chgrxbank(0);
	h2rxset(0x07,0x30,0x30);
	return ret;
}
void it6664_Read_SKEW(void)
{
	iTE_u8 Reg74h, Reg75h, Reg76h, Reg77h;
	iTE_u8 Reg78h, Reg79h, Reg7Ah;
	iTE_u8 CHB_SKEW, CHG_SKEW, CHR_SKEW;
	//iTE_u8 SKEW00_AMP, SKEWP0_AMP, SKEWP1_AMP;
	iTE_u8 SKEW00_BErr, SKEWP0_BErr, SKEWP1_BErr;
	iTE_u8 Rec_Channel;
    chgrxbank(3);
	// SKEW Read Back
	Reg74h= h2rxrd(0x74);

	CHB_SKEW=(Reg74h&0x03);
	CHG_SKEW=((Reg74h&0x0C)>>2);
	CHR_SKEW=((Reg74h&0x30)>>4);
	#if 0
	if(CHB_SKEW==2) iTE_MsgEQ(("[EQ]	CHB SKEW : ENSKEW =1, PSKEW= 0 \r\n"));
	else if (CHB_SKEW==3) iTE_MsgEQ(("[EQ]	CHB SKEW : ENSKEW =1, PSKEW= 1 \r\n"));
	else iTE_MsgEQ(("[EQ]	CHB SKEW : ENSKEW =0, PSKEW= 0 \r\n"));

	if(CHG_SKEW==2) iTE_MsgEQ(("[EQ]	CHG SKEW : ENSKEW =1, PSKEW= 0 \r\n"));
	else if (CHG_SKEW==3) iTE_MsgEQ(("[EQ]	CHG SKEW : ENSKEW =1, PSKEW= 1 \r\n"));
	else iTE_MsgEQ(("[EQ]	CHG SKEW : ENSKEW =0, PSKEW= 0 \r\n"));

	if(CHR_SKEW==2) iTE_MsgEQ(("[EQ]	CHR SKEW : ENSKEW =1, PSKEW= 0 \r\n"));
	else if (CHR_SKEW==3) iTE_MsgEQ(("[EQ]	CHR SKEW : ENSKEW =1, PSKEW= 1 \r\n"));
	else iTE_MsgEQ(("[EQ]	CHR SKEW : ENSKEW =0, PSKEW= 0 \r\n"));
	#endif

	SkewResult = h2rxrd(0x74);
    for(Rec_Channel=0; Rec_Channel<3; Rec_Channel++)
	{
        h2rxset(0x37, 0xC0, Rec_Channel<<6);
    	Reg75h= h2rxrd(0x75);
    	Reg76h= h2rxrd(0x76);
    	Reg77h= h2rxrd(0x77);
    	Reg78h= h2rxrd(0x78);
    	Reg79h= h2rxrd(0x79);
    	Reg7Ah= h2rxrd(0x7A);
        SKEW00_BErr= (Reg78h<<8)+ Reg75h;
	    SKEWP0_BErr= (Reg79h<<8)+ Reg76h;
	    SKEWP1_BErr= (Reg7Ah<<8)+ Reg77h;
		#if 0
        iTE_MsgEQ(("############################################\r\n"));
	    iTE_MsgEQ(("[EQ]	SKEW info of Selected channel: %d : \r\n", (iTE_u16)Rec_Channel));
 	    iTE_MsgEQ(("[EQ]	SKEW00 : BErr  =%02x, SKEWP0 : BErr =%02x, SKEWP1 : BErr =%02x \r\n", SKEW00_BErr, SKEWP0_BErr, SKEWP1_BErr));
        iTE_MsgEQ(("############################################\r\n"));
		#endif
	}
	chgrxbank(0);

}
void it6664_SigleRSSKEW(void)
{
	iTE_u8 SAREQDone;
	chgrxbank(3);
    h2rxset(0x22, 0x04, 0x00);
	// force RS to Target RS //
	h2rxwr(0x27, EQ20[0]);
    h2rxwr(0x28, EQ20[1]);
	h2rxwr(0x29, EQ20[2]);
	//
	h2rxwr(0x2D, 0x00); // No Force SKEW
	h2rxwr(0x30, 0x94); // [4] Manual RS, [2] SKEWOPT
	h2rxwr(0x31, 0xB0); // AMPTime[7:0]
	h2rxset(0x37, 0x10, 0x10); //[4]: RSOnestage
    h2rxset(0x54, 0x80, 0x00); // Reg_EN_PREEQ
	h2rxset(0x22, 0x04, 0x04); // Trigger EQ

	chgrxbank(0);
    SAREQDone=(h2rxrd(0x07)&0x10)>>4;
	while (!SAREQDone){
		//iTE_MsgEQ(("Wait Wait for SAREQ Done.....\r\n"));
		mSleep(1);
		SAREQDone=(h2rxrd(0x07)&0x10)>>4;
	}
    h2rxset(0x07, 0x10, 0x10);
	chgrxbank(3);
	h2rxset(0x37, 0x10, 0x00); //[4]: RSOnestage
	h2rxset(0x22, 0x04, 0x00);
	chgrxbank(0);
}
iTE_u8 it6664_RptBitErr_ms(iTE_u8 Threshold)
{
	iTE_u8 CED_Err[3][2];
	//iTE_u8 x;
	iTE_u8 i,reg[3],HDMIMODE;
	iTE_u8 BitErrOK;
    BitErrOK=1;

	reg[0] = h2rxrd(0xB9);
	reg[1] = h2rxrd(0xBE);
	reg[2] = h2rxrd(0xBF);
	HDMIMODE = (h2rxrd(0x14)&0x40);
	h2rxset(0x3B, 0x08, 0x08);  // read CED Error from SAREQ CEDError Counter
	chgrxbank(3);
	h2rxset(0x55, 0x80, 0x00);
	h2rxwr(0xE9, 0x00);

    for(i=0;i<3;i++)
    {
        h2rxwr(0xE9, (i*2)<<4);
        CED_Err[i][0] = h2rxrd(0xEA);
		CED_Err[i][1] = h2rxrd(0xEB);

		fail_ch[i] = 0;
		if(!(CED_Err[i][1]&0x80))
		{
			BitErrOK = 0;
			fail_ch[i] = 1;
		}
		else
		{
			if((CED_Err[i][0]> Threshold))
			{
				BitErrOK = 0;
				fail_ch[i] = 1;
			}
			else
			{
				EQ14[i] = h2rxrd(0x27+i);
			}
		}
		//iTE_MsgRX(("fail_ch = 0x%02x  \r\n",(iTE_u16)fail_ch[i]));
		if(fail_ch[i])
		{
			if((reg[0]==0) && (reg[1]==0) && (reg[2]==0) && (!HDMIMODE))
			{}
			else
			{
				iTE_MsgRX(("BitErr of Ch%d = 0x%02x 0x%02x \r\n",(iTE_u16)i,CED_Err[i][1],CED_Err[i][0]));
			}
		}
	}

	h2rxwr(0xE9, 0x80);
	chgrxbank(0);
	h2rxwr(0xB9, 0xFF);
	h2rxwr(0xBE, 0xFF);
	h2rxwr(0xBF, 0xFF);
	return BitErrOK;
}
void Check_AMP(iTE_u8 Rec_Channel)
{
	iTE_u8 Reg61h, Reg62h ;
	iTE_u8 Reg6Bh, Reg6Ch;
	iTE_u8 x;
	iTE_u8 AMP_A, AMP_B, AMP_C, AMP_D;
	iTE_u8 AMP_AA, AMP_BC;
	iTE_u8 AMP_AAA, AMP_BCD;
	iTE_u8 Tap1_Sign, Tap1_Value;
	iTE_u8 Tap2_Sign, Tap2_Value;
	iTE_u8 Tap3_Sign, Tap3_Value;
	iTE_u8 Tap12_Sign, Tap12_Value;
	iTE_u8 Tap123_Value;
	iTE_u8 TargetRS;
	iTE_u8 Rec_TimeOut, TimeOut_Now;
	iTE_u8 MinTap123_Value=0xFF;
	iTE_u8 MinTap12_Value=0xFF;
	iTE_u8 MinTap12_RS=0xFF;
	iTE_u8 MinTap1, MinTap2, MinTap3;
	iTE_u8 MinTap1_Sign, MinTap2_Sign, MinTap3_Sign;
	iTE_u8 temp1;

    h2rxset(0x37, 0xC0, Rec_Channel <<6 );
	Rec_TimeOut=AMP_TimeOut[Rec_Channel];
    if(Rec_Channel==0) temp1=h2rxrd(0xD5);
    if(Rec_Channel==1) temp1=h2rxrd(0xD6);
	if(Rec_Channel==2) temp1=h2rxrd(0xD7);
	TargetRS=(temp1&0x7F);
   // iTE_Msg((" -----------------------------------------------------\r\n"));
	//iTE_Msg(("Channel %d , AutoEQ TargetRS=%x \r\n", (iTE_u8)Rec_Channel, TargetRS));

	// Print AMP and Tap sign for all RS value
	for(x=0;x<14;x=x+1){

		h2rxset(0x36, 0x0F, x);
        AMP_A=h2rxrd(0x5d);
        AMP_B=h2rxrd(0x5e);
	    AMP_C=h2rxrd(0x5f);
	    AMP_D=h2rxrd(0x60);
		AMP_AA= AMP_A<<1;
		AMP_BC= AMP_B + AMP_C;
		AMP_AAA= AMP_AA + AMP_A;
		AMP_BCD= AMP_B + AMP_C + AMP_D;

		//iTE_Msg(("RS =%02x \n",  RS_Value[x]));
        //iTE_Msg(("AMP_A =%x , AMP_B= %x, AMP_C=%x, AMP_D=%x \n", AMP_A, AMP_B, AMP_C, AMP_D));
		if(AMP_A > AMP_B) {
			Tap1_Sign=1;
			Tap1_Value= (AMP_A- AMP_B)>>1;
		}
		else {
			Tap1_Sign=0;
	    	Tap1_Value=(AMP_B- AMP_A)>>1;
		}
		if(AMP_A > AMP_C) {
			Tap2_Sign=1;
			Tap2_Value= (AMP_A- AMP_C)>>1;
		}
		else {
			Tap2_Sign=0;
	    	Tap2_Value=(AMP_C- AMP_A)>>1;
		}

		if(AMP_AA > AMP_BC) {
			Tap12_Sign=1;
			Tap12_Value= (AMP_AA- AMP_BC)>>1;
		}
		else {
			Tap12_Sign=0;
	    	Tap12_Value=(AMP_BC- AMP_AA)>>1;
		}
		if(AMP_AAA > AMP_BCD) {
			Tap123_Value= (AMP_AAA- AMP_BCD)>>1;
		}
		else {
	    	Tap123_Value=(AMP_BCD- AMP_AAA)>>1;
		}
		if(AMP_A > AMP_D) {
			Tap3_Sign=1;
			Tap3_Value= (AMP_A- AMP_D)>>1;
		}
		else {
			Tap3_Sign=0;
	    	Tap3_Value=(AMP_D- AMP_A)>>1;
		}
		if(Tap1_Value>0x1F) Tap1_Value=0x1F;
        if(Tap2_Value>0x0F) Tap1_Value=0x0F;
		if(Tap3_Value>0x07) Tap1_Value=0x07;
	   	//iTE_Msg(("	Tap1_Sign=%2x, Tap1=%2x,  Tap2_Sign=%2x  Tap2=%2x, Tap12_Sign=%2x, Tap12=%2x \r\n", Tap1_Sign, Tap1_Value, Tap2_Sign, Tap2_Value, Tap12_Sign, Tap12_Value));
		//iTE_Msg(("	Tap3_Sign=%2x, Tap3=%2x,  Tap123=%2x \r\n", Tap3_Sign, Tap3_Value, Tap123_Value));

		TimeOut_Now=(Rec_TimeOut&0x01);
        Rec_TimeOut=Rec_TimeOut>>1;

		if(TimeOut_Now==0 && ((Tap12_Value< MinTap12_Value) ||
			                  (Tap12_Value== MinTap12_Value && Tap123_Value<= MinTap123_Value)  )) {
		     MinTap123_Value= Tap123_Value;
             MinTap12_Value= Tap12_Value;
			 MinTap12_RS   = RS_Value[x];
			 MinTap1       = Tap1_Value;
			 MinTap2       = Tap2_Value;
			 MinTap3       = Tap3_Value;
			 MinTap1_Sign  = Tap1_Sign;
			 MinTap2_Sign  = Tap2_Sign;
			 MinTap3_Sign  = Tap3_Sign;
			 EQ20[Rec_Channel] = MinTap12_RS+0x80;
			 //iTE_Msg(("MinTap12_RS = %2x x=%d \r\n",  MinTap12_RS, x));
		}
		DFE[x][Rec_Channel][0] = 0x40+ (MinTap1_Sign << 5 ) + MinTap1;
		DFE[x][Rec_Channel][1] = 0x20+ (MinTap2_Sign << 4 ) + MinTap2;
		DFE[x][Rec_Channel][2] = 0x10+ (MinTap3_Sign << 3 ) + MinTap3;
	}

	//Check whether AMP is done at each RS value
	Reg61h=h2rxrd(0x61);
	Reg62h=h2rxrd(0x62);
	Reg6Bh=h2rxrd(0x6B);
	Reg6Ch=h2rxrd(0x6C);
	/*
	iTE_Msg(("############################################\r\n"));
	iTE_Msg(("Channel:%d  AMP status is \r\n",  (iTE_u8)Rec_Channel));
    iTE_Msg(("AMP Done      = %02x \r\n", (Reg62h<<8) + Reg61h));
    iTE_Msg(("AMP TimeOut   = %02x \r\n", AMP_TimeOut[Rec_Channel]));
	iTE_Msg(("AMP Ignore    = %02x \r\n", (Reg6Ch<<8) + Reg6Bh));
    iTE_Msg(("############################################\r\n"));
	*/

	//if(All_Ignore==1 )
	{
	       // iTE_Msg(("####   Adjust RS to %02x     ####\r\n", MinTap12_RS ));
	        //iTE_Msg(("Tap1_Sign=%02x, Tap1=%02x,  Tap2_Sign=%02x  Tap2=%02x,  Tap12=%02x \r\n", MinTap1_Sign, MinTap1, MinTap2_Sign, MinTap2, MinTap12_Value));
	        //iTE_Msg(("Tap3_Sign=%02x, Tap3=%02x   \r\n", MinTap3_Sign, MinTap3));
			if(Rec_Channel==0 ) {
				h2rxwr(0x27, 0x80 + MinTap12_RS);

				h2rxwr(0x4B, 0x40+ (MinTap1_Sign << 5 ) + MinTap1);
				h2rxwr(0x4C, 0x20+ (MinTap2_Sign << 4 ) + MinTap2);
                h2rxwr(0x4D, 0x10+ (MinTap3_Sign << 3 ) + MinTap3);
				h2rxset(0x4B, 0x80, 0x80);// SETDFE
				h2rxset(0x4A, 0x80, 0x80); // DFERdOpt
				//iTE_Msg(("DFE_A= %02x, DFE_B= %02x, DFE_C= %02x \r\n", h2rxrd(0x4B)&0x7F, h2rxrd(0x4C)&0x3F, h2rxrd(0x4D)&0x1F));
                h2rxset(0x4A, 0x80, 0x00); // DFERdOpt

			}
			if(Rec_Channel==1 ) {
				h2rxwr(0x28, 0x80 + MinTap12_RS);
				h2rxwr(0x4E, 0x40+ (MinTap1_Sign << 5 ) + MinTap1);
				h2rxwr(0x4F, 0x20+ (MinTap2_Sign << 4 ) + MinTap2);
                h2rxwr(0x50, 0x10+ (MinTap3_Sign << 3 ) + MinTap3);
				h2rxset(0x4B, 0x80, 0x80);// SETDFE
				h2rxset(0x4A, 0x80, 0x80); // DFERdOpt
				//iTE_Msg(("DFE_A= %02x, DFE_B= %02x, DFE_C= %02x \r\n", h2rxrd(0x4E)&0x7F, h2rxrd(0x4F)&0x3F, h2rxrd(0x50)&0x1F));
                h2rxset(0x4A, 0x80, 0x00); // DFERdOpt
			}
			if(Rec_Channel==2 ) {
				h2rxwr(0x29, 0x80 + MinTap12_RS);

				h2rxwr(0x51, 0x40+ (MinTap1_Sign << 5 ) + MinTap1);
				h2rxwr(0x52, 0x20+ (MinTap2_Sign << 4 ) + MinTap2);
                h2rxwr(0x53, 0x10+ (MinTap3_Sign << 3 ) + MinTap3);
				h2rxset(0x4B, 0x80, 0x80);// SETDFE
				h2rxset(0x4A, 0x80, 0x80); // DFERdOpt
				//iTE_Msg(("DFE_A= %02x, DFE_B= %02x, DFE_C= %02x \r\n", h2rxrd(0x51)&0x7F, h2rxrd(0x52)&0x3F, h2rxrd(0x53)&0x1F));
                h2rxset(0x4A, 0x80, 0x00); // DFERdOpt
			}
		}

 }
void it6664_ManuEQ(iTE_u8 mode)
{
	iTE_u8 i,tmp;
	iTE_u8 EQ_Table[7] = {0x8F,0x86,0x83,0x81,0x80,0x9F,0xBF};//{0xBF,0x9F,0x8F,0x87,0x86,0x83,0x81};
	iTE_u8 ref[7] = {6,9,10,12,13,4,2};
	iTE_u8 ch[3];

	chgrxbank(3);
	for(i=0;i<3;i++)
	{
		ch[i] = 0;
		if(fail_ch[i])
		{
			//iTE_MsgRX(("EQ sel %d \r\n",(iTE_u16)EQ_sel[i]));
			tmp = EQ_sel[i];
			h2rxwr(0x27+i,EQ_Table[tmp]);
			iTE_MsgRX(("EQ Ch%2x sel %2x \r\n",i,h2rxrd(0x27+i)));
			ch[i] = ref[EQ_sel[i]];
		}
		if(EQ_sel[i]<=6) EQ_sel[i]++;
		else EQ_sel[i] = 0;
	}
	#if 1
	if(mode)
	{
		//set DEF here
		for(i=0;i<3;i++)
		{
			if(fail_ch[i])
			{
				if(i==0)
				{
					h2rxwr(0x4B, 0x80 + DFE[ch[0]][0][0]);
				}
				else
				{
					h2rxwr(0x4B+i*3, DFE[ch[i]][i][0]);
				}
				h2rxwr(0x4C+i*3, DFE[ch[i]][i][1]);
				h2rxwr(0x4D+i*3, DFE[ch[i]][i][2]);
			}
		}
	}
	#endif
	h2rxwr(0xE9, 0x80);
	chgrxbank(0);
}

#endif

///*****************************************
//  Copyright (C) 2009-2021
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <IT6664_hdmi2_tx.c>
//   @author Hojim.Tseng@ite.com.tw
//   @date   2021/01/12
//   @fileversion: ITE_HDMI2_SPLITER_1.38
//******************************************/
#include "IT6664_hdmi2_tx.h"

#include <linux/delay.h>

#define mSleep(x) msleep(x)


extern extern_variables *gext_var;
extern extern_32 *gext_long;
extern extern_u8 *gext_u8;
extern it6664_tx *gmem_tx;//6664 tx members
extern iTE_u8 g_device;

#ifdef RXHDCP_Follow_SinkX
iTE_u8 HdcpVerChk(iTE_u8 port)
{
	iTE_u8 RxHDCPver;
	iTE_u8 BKSV[5];
	h2txset(port, 0x41, 0x01, 0x01);
    hdcprd(port, 0x00, 5);
    h2txbrd(port, 0x5B, 5, &BKSV[0]);

	iTE_MsgTX(("*************************************************************** \r\n"));
    iTE_MsgTX(("HdcpVerChk	TXP%d: BKSV= %02X%02X%02X%02X%02X  \r\n", port, BKSV[4], BKSV[3], BKSV[2], BKSV[1], BKSV[0]));

	if(FALSE == CheckBKsv(BKSV))
	{
		RxHDCPver= 3;//can't read BKSV
	}
	else
	{
		h2txset(port, 0x42, 0x10, 0x10);
		h2txset(port, 0x41, 0x01, 0x01);
    	hdcprd(port, 0x50, 1);
		if(gext_u8->DDC_NAK)//ddc no ack
		{
			RxHDCPver= 3;
			iTE_MsgTX(("No ack , Return Version = No HDCP \r\n"));
		}
		else
		{
    		RxHDCPver = h2txrd(port, 0x4B)&0x01;
			if(RxHDCPver)
			iTE_MsgTX(("Return Version = HDCP2.2 \r\n"));
			else
			iTE_MsgTX(("Return Version = HDCP1.4 \r\n"));
		}
	}
	h2txset(port, 0x42, 0x10, 0x00);
	h2txset(port, 0x41, 0x01, 0x00);
	h2txset(port, 0x01, 0x20, 0x20);
	h2txset(port, 0x01, 0x20, 0x00);
	gext_u8->DDC_NAK = 0;
	iTE_MsgTX(("*************************************************************** \r\n"));
	return RxHDCPver;
}
void RxHdcpChkTx(iTE_u8 port)
{
	iTE_u8 tmp,reg0A,reg23,reg0C;

	reg0A= h2sprd(0x0A)&0x04;
	reg0C= h2sprd(0x0C)&0x04;
	reg23 = h2rxrd(0x23)&0x42;

	iTE_MsgTX((":::: TXP%d reg0A = %2x ,reg0C = %2x ,reg23 = %2x :::: \r\n", (iTE_u16)port,(iTE_u16)reg0A,(iTE_u16)reg0C,(iTE_u16)reg23));
	tmp= HdcpVerChk(port);
	gext_u8->RXHDCP_SEL=tmp;
	it6664_RXHDCP_Set(gext_u8->RXHDCP_SEL);
}
#endif

#if 1
void h2tx_irq(iTE_u8 port)
{
	iTE_u8 TxReg10, TxReg11, TxReg12, TxReg13, TxReg14,i,ksvlist[32],rddata;
	iTE_u16  msgcnt, passsha;
	//iTE_u16 ARi1, ARi2, BRi1, BRi2, APj, BPj;
	iTE_u8 temp[4];
	iTE_u8 CurKsvFifoWrStg, EndKsvFifoWrStg;
	iTE_u8 M0[8], BStatus[2], BV[5][4];
	static iTE_u16 ksvchkcnt[4] = {0, 0, 0, 0};

	TxReg10 = h2txrd(port, 0x10);
	TxReg11 = h2txrd(port, 0x11);
	TxReg12 = h2txrd(port, 0x12);
	TxReg13 = h2txrd(port, 0x13);
	TxReg14 = h2txrd(port, 0x14);
	h2txwr(port,0x10,TxReg10);
	h2txwr(port,0x11,TxReg11);
	h2txwr(port,0x12,TxReg12);
	h2txwr(port,0x13,TxReg13);
	h2txwr(port,0x14,TxReg14);

	if( TxReg10&0x01 )
	{
		rddata = h2txrd(port, 0x03);
		if( (rddata&0x01) )
		{
			if(EnRxHP1Rpt == TRUE)
			{
				msleep(50);
			}
			#ifdef RXHDCP_Follow_SinkX
				if(port == HDCP_CopyPx)
				{
					RxHdcpChkTx(HDCP_CopyPx);
				}
			#endif
			//SetRxHpd(TRUE);
			if(gext_u8->TXHPDsts & (1<<port))//cause fw didn't get HPD down status
			{
				h2tx_pwrdn(port);
				h2tx_pwron(port);
			}
			iTE_MsgTX((":::: TXP%d HPD Change Interrupt ON :::: \r\n", (iTE_u16)port));
			#if (USING_1to8 == FALSE)
				//SetTxLED(port,1);
			#endif

			if(EnRxHP2Rpt == TRUE)
			{
				SetRxHpd(FALSE);
				msleep(200);
				SetRxHpd(TRUE);
				if(gext_u8->RXHDCP2_chk == 1)
				{
					chgspbank(1);
					h2spset(0x1A,0x08,0x08);//set reauth for 3C-14
					chgspbank(0);
					gext_u8->RXHDCP2_chk = 0;
				}
			}
			#ifdef Support_CEC
				Cec_TxPolling(IT6664_TX_LA);
			#endif
		}
		else
		{
			#ifdef RXHDCP_Follow_SinkX
				if(port == HDCP_CopyPx)
				{
					gext_u8->RXHDCP_SEL = 0;
				}
			#endif
			iTE_MsgTX((":::: TXP%d HPD Change Interrupt OFF :::: \r\n", (iTE_u16)port));
			h2tx_pwrdn(port);
			gext_var->EDIDParseDone[port]=0;
			gext_var->DVI_mode[port] = 0;
			#if (USING_1to8 == FALSE)
				//SetTxLED(port,0);
			#endif
		}
		#if (USING_1to8 == TRUE)
			if((!gext_var->EDIDParseDone[port]) && (g_device != IT6663_C))
		#else
			if((!gext_var->EDIDParseDone[port]) && ((PortSkipEdid_opt & (1<<port)) == 0))
		#endif
			{
				{
					it6664_EdidMode_Switch();
				}
			}
	}
	if( TxReg10&0x02 )
	{
		if(( h2txrd(port, 0x03)&0x02 ))//161230
		{
			iTE_MsgTX((":::: TXP%d RxSen Change Interrupt ON  :::: \r\n", (iTE_u16)port));
			if(gext_u8->TXHPDsts & (1<<port))
			{
				h2tx_pwrdn(port);
			}
			else
			{
				if(h2txrd(port,0x03)&0x04)
				{
					 gext_var->VideoState[port] = Tx_Video_Stable;
				}
				else
				{
					h2tx_pwrdn(port);
				}
			}
			h2tx_pwron(port);
			if(EnRxHP1Rpt == TRUE)
			{
				msleep(50);
			}
			SetRxHpd(TRUE);
			#if (USING_1to8 == FALSE)
				//SetTxLED(port,1);
			#endif
		}
		else
		{
			iTE_MsgTX((":::: TXP%d RxSen Change Interrupt OFF  :::: \r\n", (iTE_u16)port));
			h2tx_pwrdn(port);
			#if (USING_1to8 == FALSE)
				//SetTxLED(port,0);
			#endif
		}
		rddata = h2txrd(port, 0x03);

		if((rddata&0x01) == 0x01)
		{
			gext_u8->TXHPDsts |= (1<<port);
		}
		else
		{
			gext_u8->TXHPDsts &= ~(1<<port);
		}
		#if (USING_1to8 == TRUE)
			if((!gext_var->EDIDParseDone[port]) && (g_device != IT6663_C))
		#else
			if((!gext_var->EDIDParseDone[port]) && ((PortSkipEdid_opt & (1<<port)) == 0))
		#endif
			{
				{
					it6664_EdidMode_Switch();
				}
			}
	}
	if( TxReg10&0x04 )
	{
		iTE_MsgTX((":::: TXP%d DDC Command Fail Interrupt ...  :::: \r\n", (iTE_u16)port));
		rddata = h2txrd(port, 0x2F)&0x01;
		h2txset(port, 0x28, 0x01, EnDDCMasterSel);
		h2txwr(port, 0x2E, 0x0F);
		h2txset(port, 0x28, 0x01, 0x00);
	}
	if( TxReg10&0x08 )
	{
		h2txset(port, 0x28, 0x01, EnDDCMasterSel);
		h2txwr(port, 0x2E, 0x09);
		h2txset(port, 0x28, 0x01, 0x00);
	}
	if( TxReg10&0x80)
	{
		rddata = h2txrd(port, 0x03);
		if((rddata&0x01)==0x01)
       	{
			if(( (rddata&0x04)==0x04 )&&((gext_u8->TxVidStbFlag&(1<<port)) == FALSE)&&((gext_u8->RXSCDT) ))
			{
   				iTE_MsgTX((":::: TXP%d H2TX Video Stable On Interrupt ... :::: \r\n", (iTE_u16)port));

				gext_u8->TXHPDsts |= (1<<port);
				if((h2rxrd(0x13)&0x10) == 0x10)
				{
					rddata = h2txrd(port,0x03);
					if(gext_var->VideoState[port] != Tx_Video_OK)
					{
						if((rddata&0x80) == 0x80)
						{
							gext_var->VideoState[port] = Tx_Video_Stable;
						}
						else if((gext_long->TXVCLK[port] == 0) || ((gext_long->TXVCLK[port]/1000) <= 30))
						{
							iTE_MsgRX(("TXP%d 480P VCLK cal fail, ignore \r\n", (iTE_u16) port));
							gext_var->VideoState[port] = Tx_Video_Stable;
						}
						else
						{
								h2txset(port, 0x84, 0x60, 0x60);
								h2txset(port, 0x84, 0x80, 0x00);
								h2txset(port, 0x86, 0x08, 0x00);
								h2txset(port, 0x86, 0x08, 0x08);
								h2txset(port, 0x84, 0xE0, 0x00);
    							h2txset(port, 0x84, 0x80, 0x80);
						}
					}
				}
				else
				{
					gext_var->VideoState[port] = Tx_Video_waitInt;
				}
			}
			else if (( (rddata&0x04)==0x00 )&&((gext_u8->TxVidStbFlag&(1<<port) )|| (!gext_u8->RXSCDT )))
			{//161230
				gext_var->VideoState[port] = Tx_Video_Stable_off;
        	}
			else if(( (rddata&0x08)==0x00 )&&(gext_u8->TxTMDSStbFlag&(1<<port)))
			{
				gext_var->VideoState[port] = Tx_Video_Reset;
        	}
			else if(( (rddata&0x08)==0x08 )&&((gext_u8->TxTMDSStbFlag&(1<<port))==FALSE ) &&(gext_u8->TxVidStbFlag&(1<<port)))
			{
				gext_var->VideoState[port] = Tx_Video_Stable;
        	}

	  	}
		else
		{
			gext_var->VideoState[port] = Tx_Video_Stable;
		}
	}
	if( TxReg13&0x40 )
	{
        iTE_MsgTX(("TXP%d KSV Read from Downstream RX completed !!!\r\n", port));
		gext_u8->TXHP2KSVChkDone |= (1<<port);
		chktx_hdcp2_ksv(port);
    } //put before auth fail for Allion ATC workaround
	if( TxReg11&0x01 )
	{
       	if((h2txrd(port, 0x42)&0x10)==0)//161230
		{
			gext_u8->TXHP2KSVChkDone &= ~(1<<port);
			gext_var->HDCPState[port] = Tx_CP_ReAuth;
		}
		else if (((gext_u8->TXH2RSABusy &(1<<port))&&( (TxReg11&0x02)==0 ))|| (gext_u8->TXHDCP2Done &(1<<port)))
		{//161230//170109
			gext_u8->TXHDCP2Done &= ~(1<<port);
			//h2txset(port, 0xC1, 0x01, 0x01);
			gext_var->HDCPState[port] = Tx_CP_ReAuth;
			gext_var->VideoState[port] = Tx_Video_waitInt;
        }
		iTE_MsgTX((":::: TXP%d HDCP%x Authentication Fail Interrupt ... :::: \r\n", (iTE_u16)port,gext_var->HDCPFireVer[port]));
		gext_u8->CP_Going &= ~(1<<port);//False
		gext_u8->CP_Done &= ~(1<<port);
		hdcpsts(gext_var->HDCPFireVer[port],port);
		if(gmem_tx->HDCPFireCnt[port] > 30)
		{
			h2txset(port, 0xC1, 0x01, 0x01);
		}
		#if 0
		if(((gext_u8->CP_Going &(1<<port))==FALSE) && ((gext_u8->CP_Done &(1<<port))==FALSE) && ((gext_u8->CP_Fail &(1<<port))==FALSE))
		{
			if( gmem_tx->HDCPFireCnt[port]>HDCPFireMax )
			{
				gext_u8->CP_Fail |= (1<<port);
				h2txset(port, 0x41, 0x01, 0x00);
				h2txset(port, 0x91, 0x10, 0x10);
				gext_u8->TxTMDSStbFlag &= ~(1<<port);//170109
				h2txVclkrst(port);
				mSleep(100);
			}
		}
		#endif
	}
	if( TxReg11&0x02)
	{
		gext_u8->TXH2RSABusy &= ~(1<<port);
        rddata = h2txrd(port, 0x42);
		gext_var->HDCPState[port] = Tx_CP_Done;
		if( (rddata&0x10)==0x00 )
		{
			gext_u8->HP2_HDCP1DownStm = 1;
		}
		else
		{
			gext_u8->TXHDCP2Done |= (1<<port);
			//170104
			if((EnTxHP2KSVChk == FALSE)&&EnRxHP2Rpt)
			{
				while(((h2txrd(port, 0x13))&0x40)==0x00)
             	{
             		iTE_MsgTX(("TXP%d wait TX read KSV done\r\n", (iTE_u16)port));
					msleep(10);
			 	}
				chktx_hdcp2_ksv(port);
			}
			//170104
		}
		iTE_MsgTX((":::: TXP%d HDCP%x Authentication Done Interrupt ... :::: \r\n", (iTE_u16)port,gext_var->HDCPFireVer[port]));
		gext_u8->CP_Going &= ~(1<<port);//False
		gext_u8->CP_Done |= (1<<port);
		gmem_tx->HDCPFireCnt[port] = 0;
		hdcpsts(gext_var->HDCPFireVer[port],port);
	}
	if( TxReg11&0x04 )
	{
		iTE_MsgTX((":::: TXP%d HDCP KSV List Check Interrupt ... %d :::: \r\n", (iTE_u16)port, ksvchkcnt[port]));
		hdcprd(port, 0x40, 1);		 // BCaps
		rddata = h2txrd(port, 0x63);
		while(((rddata&0x20)==0x00)&&(ksvchkcnt[port]<=ChkKSVListMax))
		{
			msleep(50);
			hdcprd(port, 0x40, 1);       // BCaps
            rddata = h2txrd(port, 0x63);
			ksvchkcnt[port]++;
		}
		if( (rddata&0x20)==0x00 && ksvchkcnt[port]>ChkKSVListMax )
		{
			if( (h2txrd(port, 0x10)&0x04)==0x04 )
			{
				iTE_MsgTX(("TXP%d DDC Command Fail when checking KSV List FIFO Ready !!! \r\n", (iTE_u16)port));
				rddata = 0x00;
			}
			else
			{
				iTE_MsgTX(("Error: TXP%d Wait KSV List FIFO Ready over %d times !!! \r\n", (iTE_u16)port, ChkKSVListMax));
				ksvchkcnt[port] = 0;
				h2txwr(port, 0x11, 0x04);
				h2txset(port, 0x42, 0x06, 0x06);   // Set KSV List Fail W1P
				if(gext_var->HDCPFireVer[port]==1)
				{
					gext_var->HDCPState[port] = Tx_CP_ReAuth;
					gext_var->VideoState[port] = Tx_Video_waitInt;
				}
			}
		}
		else
			ksvchkcnt[port]++;

		if((rddata&0x20)==0x20)
		{
			ksvchkcnt[port] = 0;
			h2txwr(port, 0x11, 0x04);
			h2txset(port, 0x70, 0x07, 0x05);
			for(i=0; i<4; i++)
				M0[i] = h2txrd(port, 0x71+i);

			for(i=0; i<4; i++)
			{
				h2txset(port, 0x70, 0x07, i);
				M0[4+i] = h2txrd(port, 0x75);
			}
			iTE_MsgTX(("M0: 0x "));
			for(i=0; i<8; i++)
				iTE_MsgTX(("%02X ", M0[7-i]));

			iTE_MsgTX((" \r\n"));
			hdcprd(port, 0x41, 2);	   // BStatus
			BStatus[0] = h2txrd(port, 0x64);


			iTE_MsgTX(("TXP%d Device Count = %X \r\n", port, BStatus[0]&0x7F));
			iTE_MsgTX(("TXP%d Max. Device Exceeded = %02X \r\n", port, (BStatus[0]&0x80)>>7));
			BStatus[1] = h2txrd(port, 0x65);

			iTE_MsgTX(("TXP%d Depth = %X \r\n", (iTE_u16)port, (iTE_u16)(BStatus[1]&0x07)));
			iTE_MsgTX(("TXP%d Max. Cascade Exceeded = %02X \r\n", (iTE_u16)port, (iTE_u16)(BStatus[1]&0x08)>>3));
			iTE_MsgTX(("TXP%d HDMI_MODE = %d \r\n", (iTE_u16)port, (iTE_u16)(BStatus[1]&0x10)>>4));
			iTE_MsgTX((" \r\n"));

			if( (BStatus[0]&0x80) || (BStatus[1]&0x08) )
			{
				h2txset(port, 0x42, 0x06, 0x06);   // Set KSV List Fail W1P
				//h2txset(port, 0x42, 0x06, 0x02);   // Set KSV List done for SL8800
				//mSleep(100);
				h2txset(port, 0xC2, 0x80, 0x80);
				h2txset(port, 0x41, 0x01, 0x00);    // Disable CP_Desired
				h2txset(port, 0x91, 0x10, 0x10); //set black screen
				if(EnRxHP1Rpt)
				{
					BStatus[0] = BStatus[0]+1;
					BStatus[1] = BStatus[1]+1;
					chgrxbank(1);
					if(BStatus[0]&0x80)
					{
            			h2rxwr(0x10, BStatus[0]|0x80);
            			h2rxwr(0x11, BStatus[1]);
					}
					if(BStatus[1]&0x08)
					{
            			h2rxwr(0x10, BStatus[0]);
            			h2rxwr(0x11, BStatus[1]|0x08);
					}
					chgrxbank(0);
				}
				iTE_MsgTX(("ERROR: TXP%d Max. Device or Cascade Exceeded !!! \r\n", (iTE_u16)port));
			}
			else
			{
				msgcnt = 0;
				if( EnRxH2SHA1)
				{	// HW SHA mode must use KSV FIFO
					h2txset(port, 0x37, 0x01, 0x01);	// RegDisDDCFIFO=1
					//   chgswbank(3);
					//   h2swset(0x10, 0x01<<TxPortSel[port], 0x01<<TxPortSel[port]);	  // RegEnKsvFifo
					//   h2swset(0x12, 0x7C, (port<<4)+(TxPortSel[port]<<2));	  // RegKsvSrcSel[1:0], RegKsvFifoSel[1:0]
					h2txcomset(0x20, 0x70, port<<4);
					CurKsvFifoWrStg = h2txcomrd(0x22);
					iTE_MsgTX(("TXP%d write KSV List to KSV FIFO, CurKsvFifoWrStg=%d,  \r\n", (iTE_u16)port,(iTE_u16)CurKsvFifoWrStg));
					//   chgswbank(0);
				}
				if( BStatus[0]&0x7F )
					hdcprd(port, 0x43, 5*(BStatus[0]&0x7F));	 // KSV LIST FIFO
				else
				{
                    h2txset(port, 0x41, 0x10, 0x10);
					/*
					h2txcomwr(0x20, 0x04);
                    		h2txcomwr(0x21, 0x00);
		            	for (i=0;i<HP2_DevNum;i++)
					{//getch();
		             	i2c_write(TXComAddr, 0x23, 5, &HP2_KSVLIST[i]);
		             	//getch();
					}
		            	 h2txcomwr(0x20, 0x00);
		            	*/
					iTE_MsgTX(("WARNING: TXP%d Device Count = 0 !!! \r\n", (iTE_u16)port));
					iTE_MsgTX(("manually write BKSV to fifo \r\n"));
                }
				if( EnRxH2SHA1)
				{    // HW SHA mode must use KSV FIFO
					h2txset(port, 0x37, 0x01, 0x00);	// RegDisDDCFIFO=0
					//chgswbank(3);
					EndKsvFifoWrStg = h2txcomrd(0x22);
					iTE_MsgTX(("EndKsvFifoWrStg=%d  \r\n", EndKsvFifoWrStg));
					if( (EndKsvFifoWrStg-CurKsvFifoWrStg)==0 )
					{
                        iTE_MsgTX(("Downstream repeater has zero KSV list!!\r\n"));
					}
					else if( (EndKsvFifoWrStg-CurKsvFifoWrStg)!=((BStatus[0]&0x7F)+1) )
					{
						iTE_MsgTX(("ERROR: KSV List mismatch Device Count=%d, KSV List Count=%d !!! Press any key to continue ... \r\n", (iTE_u16)(BStatus[0]&0x7F), (iTE_u16)(EndKsvFifoWrStg-CurKsvFifoWrStg-1)));
					}
					//chgswbank(0);
				}
				if( EnRxH2SHA1 )
				{	  // HW mode
					// h2swset(0x09, 0x01<<RxH2SHA1Sel, 0x00);	  // RegSoftH*RXRst=0
					// chgswbank(3);
					///////////////////////////////////////////////////////////////////////////////////////////////////
					// read back ksvlist start for debug only
					h2txcomset(0x20, 0x04, 0x04);  // RegEnFwKsvFifo=1
					chgspbank(1);
					for(i=CurKsvFifoWrStg; i<EndKsvFifoWrStg; i++)
					{
						h2spwr(0x2D, 0x80+i);		 // KSV FIFO starts from stage CurKsvFifoWrStg
						h2spbrd(0x28, 5, &ksvlist[i*5]);
					}
					//chgspbank(0);
					h2txcomset(0x20, 0x04, 0x00);  // RegEnFwKsvFifo=0

					for(i=0; i<(BStatus[0]&0x7F); i++)
					{
						gmem_tx->M[msgcnt++] = (iTE_u8) ksvlist[i*5+0];
						gmem_tx->M[msgcnt++] = (iTE_u8) ksvlist[i*5+1];
						gmem_tx->M[msgcnt++] = (iTE_u8) ksvlist[i*5+2];
						gmem_tx->M[msgcnt++] = (iTE_u8) ksvlist[i*5+3];
						gmem_tx->M[msgcnt++] = (iTE_u8) ksvlist[i*5+4];
						iTE_MsgTX(("TXP%d KSV List %d = 0x %02X %02X %02X %02X %02X \r\n", (iTE_u16)port, (iTE_u16)i, (iTE_u16)gmem_tx->M[i*5+4], (iTE_u16)gmem_tx->M[i*5+3], (iTE_u16)gmem_tx->M[i*5+2], (iTE_u16)gmem_tx->M[i*5+1], (iTE_u16)gmem_tx->M[i*5+0]));
					}
					iTE_MsgTX(("TXP%d KSV List %d = 0x %02X %02X %02X %02X %02X <= This is BKSV of downstream device \r\n", (iTE_u16)port, (iTE_u16)i, (iTE_u16)ksvlist[i*5+4], (iTE_u16)ksvlist[i*5+3], (iTE_u16)ksvlist[i*5+2], (iTE_u16)ksvlist[i*5+1], (iTE_u16)ksvlist[i*5+0]));
					iTE_MsgTX((" \r\n"));
					// read back ksvlist end for debug only

					h2spset(0x1E, 0x3F, 0x20);
					h2spset(0x1F, 0x70, port<<4);
					h2spset(0x1F, 0x80, 0x80);	   // RH*SHA1Trg
					do{
						msleep(10);
						rddata = h2sprd(0x81);
						iTE_MsgTX(("HW SHA1 Engine Busy Status = %d \r\n", (iTE_u16)(rddata&0x80)>>7));
					} while( (rddata&0x80)!=0x00 );

					for(i=0; i<5; i++)
					{
						h2spbrd(0x52+i*4, 4, &temp[0]); 		   // Read back HW SHA1 result
						gmem_tx->AV[i][0] = (iTE_u8) temp[0];
						gmem_tx->AV[i][1] = (iTE_u8) temp[1];
						gmem_tx->AV[i][2] = (iTE_u8) temp[2];
						gmem_tx->AV[i][3] = (iTE_u8) temp[3];
					}
					h2spset(0x1E, 0x20, 0x00);	// RegEnH*SHA1=0
					chgspbank(0);
					gext_u8->DevCnt_Total = gext_u8->DevCnt_Total + (BStatus[0]&0x7F) + 1;
                    gext_u8->Err_DevCnt_Total = ((BStatus[0]&0x80)>>7) || gext_u8->Err_DevCnt_Total;
                    gext_u8->Err_Depth_Total = gext_u8->Err_Depth_Total || ((BStatus[1]&0x08)>>4);
                    if (gext_u8->Depth_Total < (BStatus[1]&0x07))
					{
	                	gext_u8->Depth_Total = (BStatus[1]&0x07);
					}

					//setrx_ksv_list(BStatus[0]&0x7F, BStatus[1]&0x07, (BStatus[0]&0x80)>>7, (BStatus[1]&0x08)>>4);
				}
				   // h2txbrd(port, 0x30, (BStatus[0]&0x7F)*5, &ksvlist[0]);
				else
				{
					h2txcomset(0x20, 0x04, 0x04);  // RegEnFwKsvFifo=1
					chgspbank(1);
					for(i=CurKsvFifoWrStg; i<EndKsvFifoWrStg; i++)
					{
						h2spwr(0x2D, 0x80+i);		 // KSV FIFO starts from stage CurKsvFifoWrStg
						h2spbrd(0x28, 5, &ksvlist[i*5]);
					}
					//chgspbank(0);
					h2txcomset(0x20, 0x04, 0x00);
					for(i=0; i<(BStatus[0]&0x7F); i++)
					{
						gmem_tx->M[msgcnt++] = (iTE_u8) ksvlist[i*5+0];
						gmem_tx->M[msgcnt++] = (iTE_u8) ksvlist[i*5+1];
						gmem_tx->M[msgcnt++] = (iTE_u8) ksvlist[i*5+2];
						gmem_tx->M[msgcnt++] = (iTE_u8) ksvlist[i*5+3];
						gmem_tx->M[msgcnt++] = (iTE_u8) ksvlist[i*5+4];
						iTE_MsgTX(("TXP%d KSV List %d = 0x %02X %02X %02X %02X %02X \r\n", (iTE_u16)port, (iTE_u16)i,(iTE_u16)gmem_tx->M[i*5+4],(iTE_u16) gmem_tx->M[i*5+3], (iTE_u16)gmem_tx->M[i*5+2], (iTE_u16)gmem_tx->M[i*5+1],(iTE_u16) gmem_tx->M[i*5+0]));
					}
					iTE_MsgTX((" \r\n"));

					gmem_tx->M[msgcnt++] = BStatus[0];
					gmem_tx->M[msgcnt++] = BStatus[1];

					gmem_tx->M[msgcnt++] = M0[0];
					gmem_tx->M[msgcnt++] = M0[1];
					gmem_tx->M[msgcnt++] = M0[2];
					gmem_tx->M[msgcnt++] = M0[3];
					gmem_tx->M[msgcnt++] = M0[4];
					gmem_tx->M[msgcnt++] = M0[5];
					gmem_tx->M[msgcnt++] = M0[6];
					gmem_tx->M[msgcnt++] = M0[7];

					iTE_MsgTX(("TXP%d SHA Message Count = %d \r\n", (iTE_u16)port, msgcnt));

					gmem_tx->M[msgcnt] = 0x80;

					for(i=msgcnt+1; i<62 ;i++)	gmem_tx->M[i] = 0x00;

					gmem_tx->M[62] = ((8*msgcnt) >> 8)&0xFF ;
					gmem_tx->M[63] = (8*msgcnt) &0xFF ;

					ShowMsg(gmem_tx->M);
					#ifdef SHA_Debug
						SHA_Simple(&gmem_tx->M[0], msgcnt,(iTE_pu8)gmem_tx->AV) ;
					#endif
				}
				for(i=0; i<5; i++)	iTE_MsgTX(("TXP%d AV.H%d = 0x %02X %02X %02X %02X \r\n", (iTE_u16)port, (iTE_u16)i, (iTE_u16)gmem_tx->AV[i][3], (iTE_u16)gmem_tx->AV[i][2], (iTE_u16)gmem_tx->AV[i][1], (iTE_u16)gmem_tx->AV[i][0]));

				iTE_MsgTX((" \r\n"));

				passsha = TRUE;
				for(i=0; i<5; i++)
				{
					hdcprd(port, 0x20+i*4, 4);	 // V'Hi
					h2txset(port, 0x70, 0x07, i);
					BV[i][0] = (iTE_u8) h2txrd(port, 0x71);
					BV[i][1] = (iTE_u8) h2txrd(port, 0x72);
					BV[i][2] = (iTE_u8) h2txrd(port, 0x73);
					BV[i][3] = (iTE_u8) h2txrd(port, 0x74);
					iTE_MsgTX(("TXP%d BV.H%d = 0x %02X %02X %02X %02X \r\n", (iTE_u16)port, (iTE_u16)i, (iTE_u16)BV[i][3], (iTE_u16)BV[i][2], (iTE_u16)BV[i][1], (iTE_u16)BV[i][0]));

					if( gmem_tx->AV[i][0]!=BV[i][0] || gmem_tx->AV[i][1]!=BV[i][1] || gmem_tx->AV[i][2]!=BV[i][2] || gmem_tx->AV[i][3]!=BV[i][3])
						passsha = FALSE;
				}
				if( passsha )
				{
					h2txset(port, 0x42, 0x02, 0x02);
					iTE_MsgTX(("TXP%d SHA Check Result = PASS \r\n", (iTE_u16)port));
				}
				else
				{
					h2txset(port, 0x42, 0x06, 0x06);
					iTE_MsgTX(("TXP%d SHA Check Result = FAIL \r\n", (iTE_u16)port));
				}
				if (((EnableTXP0HDCP==FALSE)||(gext_var->TXHPD[0]==FALSE)||(gext_u8->CP_Done&(0x01))||(port==0))&&
		               ((EnableTXP1HDCP==FALSE)||(gext_var->TXHPD[1]==FALSE)||(gext_u8->CP_Done&(0x02))||(port==1))&&
		               ((EnableTXP2HDCP==FALSE)||(gext_var->TXHPD[2]==FALSE)||(gext_u8->CP_Done&(0x04))||(port==2))&&
		               ((EnableTXP3HDCP==FALSE)||(gext_var->TXHPD[3]==FALSE)||(gext_u8->CP_Done&(0x08))||(port==3))&&EnRxHP1Rpt)
		        {
                     iTE_MsgHDCP(("All TX port HDCP1 done, begin RX Repeater Steps\r\n"));
	                 if(passsha) setrx_ksv_list(gext_u8->DevCnt_Total, gext_u8->Depth_Total, gext_u8->Err_DevCnt_Total, gext_u8->Err_Depth_Total);
				}
			}
		}
		gext_u8->TXHP2KSVChkDone &= ~(1<<port); //after ksvchkdone is checked, it is set false
	}
	if( TxReg11&0x40 )
	{
		iTE_MsgTX((":::: TXP%d Packet Event Done Interrupt ... :::: \r\n", (iTE_u16)port));
		iTE_MsgTX(("TXP%d Current Event is %02x \r\n", (iTE_u16)port,(h2txrd(port, 0x0C)&0x0F)));
	}
	if( TxReg11&0x80 )
	{
		iTE_MsgTX((":::: TXP%d User Timer Interrupt ... :::: \r\n", (iTE_u16)port));
		h2txset(port, 0x19, 0x80, 0x00);
		{
			h2txset(port, 0x88, 0x03, 0x00);
			iTE_MsgTX(("[TX]	TX port%d Open TX OE \r\n",(iTE_u16) port));
			msleep(50);
			gext_var->HDCPState[port] = Tx_CP_Going;
			gext_var->VideoState[port] = Tx_Video_OK;
		}
	}
	if( TxReg12&0x01 )
	{
		iTE_MsgTX((":::: TXP%d HDMI2 SCDC update flag change Interrupt !!! :::: \r\n", (iTE_u16)port));
		rddata = h2txrd(port, 0x3E);
		iTE_MsgTX(("TXP%d SCDC Update Flags = 0x%02X \r\n",(iTE_u16) port, (iTE_u16)rddata));
		if( rddata&0x01 )
		{
			scdcwr(port, 0x10, 0x01); // W1C
			if( scdcrd(port, 0x21, 1)==TRUE )
				iTE_MsgTX(("TXP%d SCDC Scrambling_Status = %d \r\n", (iTE_u16)port, (iTE_u16)h2txrd(port, 0x30)&0x01));

			if( scdcrd(port, 0x40, 1)==TRUE )
			{
				iTE_u16 StsFlag;

				StsFlag = h2txrd(port, 0x30);
				iTE_MsgTX(("TXP%d SCDC Clock_Detected = %d \r\n", (iTE_u16)port, StsFlag&0x01));
				iTE_MsgTX(("TXP%d SCDC Ch0_Locked = %d \r\n", (iTE_u16)port, StsFlag&0x01));
				iTE_MsgTX(("TXP%d SCDC Ch1_Locked = %d \r\n", (iTE_u16)port, (StsFlag&0x02)>>1));
				iTE_MsgTX(("TXP%d SCDC Ch2_Locked = %d \r\n", (iTE_u16)port, (StsFlag&0x04)>>2));
			}
		}
		if( rddata&0x02 )
		{
			it6664_TXCED_monitor(port);
		}
		if( rddata&0x04 )
		{
			scdcwr(port, 0x10, 0x04); // W1C
			iTE_MsgTX((":::: TXP%d SCDC RR_Test ... :::: \r\n",(iTE_u16) port));
		}
			iTE_MsgTX((" \r\n"));
	}
	if( TxReg12&0x02 )
	{
		iTE_MsgTX(("::::  TXP%d HDMI2 SCDC detect read request Interrupt !!! :::: \r\n", (iTE_u16)port));

		if( scdcrd(port, 0x10, 1)==TRUE )
		{
			rddata = h2txrd(port, 0x30);
			iTE_MsgTX(("TXP%d SCDC Update Flag = 0x%02X \r\n", (iTE_u16)port, (iTE_u16)rddata));
		}
		else
		{
			iTE_MsgTX(("ERROR: TXP%d Read SCDC Update Flag Error !!! \r\n", (iTE_u16)port));
			rddata = 0x00;
		}
		if( rddata&0x01 )
		{
			scdcwr(port, 0x10, 0x01); // W1C
			if( scdcrd(port, 0x21, 1)==TRUE )
				iTE_MsgTX(("TXP%d SCDC Scrambling_Status = %d \r\n", (iTE_u16)port,(iTE_u16) h2txrd(port, 0x30)&0x01));

			if( scdcrd(port, 0x40, 1)==TRUE )
			{
				iTE_u16 StsFlag;
				StsFlag = h2txrd(port, 0x30);
				iTE_MsgTX(("TXP%d SCDC Clock_Detected = %d \r\n", (iTE_u16)port, (iTE_u16)(StsFlag&0x01)));
				iTE_MsgTX(("TXP%d SCDC Ch0_Locked = %d \r\n", (iTE_u16)port, (iTE_u16)(StsFlag&0x01)));
				iTE_MsgTX(("TXP%d SCDC Ch1_Locked = %d \r\n", (iTE_u16)port, (iTE_u16)((StsFlag&0x02)>>1)));
				iTE_MsgTX(("TXP%d SCDC Ch2_Locked = %d \r\n", (iTE_u16)port, (iTE_u16)((StsFlag&0x04)>>2)));
			}
		}
		if( rddata&0x02 )
		{
			it6664_TXCED_monitor(port);
		}
		if( rddata&0x04 )
		{
			scdcwr(port, 0x10, 0x04); // W1C
			iTE_MsgTX((":::: TXP%d SCDC RR_Test ... :::: \r\n", (iTE_u16)port));
		}
	}
	if( TxReg12&0x04 )
	{
		iTE_MsgTX((":::: TXP%d Video FIFO Error Interrupt (AutoReset) ... :::: \r\n", (iTE_u16)port));
		#if(USING_1to8==TRUE)
			if(g_device != IT6663_C)
			{
				if(gmem_tx->FiFoErrcnt[port]>15)
				{
					gmem_tx->FiFoErrcnt[port] = 0;
					rddata = g_device;
					IT6664_DeviceSelect(IT6663_C);
					if(rddata == IT6664_A)
					{
						gext_var->VideoState[2] = Tx_Video_Reset;
					}
					else if(rddata == IT6664_B)
					{
						gext_var->VideoState[1] = Tx_Video_Reset;
					}
					IT6664_DeviceSelect(rddata);
				}
				else
				{
					gmem_tx->FiFoErrcnt[port]++;
				}
			}
		#endif
	}
	if( TxReg12&0x08 )
	{
		iTE_MsgTX((":::: TXP%d DDC Bus Hang Interrupt ... :::: \r\n", (iTE_u16)port));
		rddata = h2txrd(port, 0x41)&0x01;

		if( rddata==0x01 )
		{
			h2txset(port, 0x41, 0x01, 0x00);   // Disable CP_Desired
			h2txset(port, 0x01, 0x20, 0x20);   // Enable HDCP Reset
		}

		h2txset(port, 0x28, 0x01, EnDDCMasterSel);
		h2txwr(port, 0x2E, 0x0A);			   // Generate SCL Clock
		h2txset(port, 0x28, 0x01, 0x00);

		if( rddata==0x01 )
		{
			h2txset(port, 0x01, 0x20, 0x00);   // Disable HDCP Reset
			h2txset(port, 0x41, 0x01, 0x01);   // Enable CP_Desired
			gext_u8->CP_Going &= ~(1<<port);//False
			gext_u8->CP_Done &= ~(1<<port);
		}
	}
	if( TxReg12&0x10 )
	{
		//ARi1 = h2txrd(port, 0x58);
		//ARi2 = h2txrd(port, 0x59);
		//BRi1 = h2txrd(port, 0x60);
		//BRi2 = h2txrd(port, 0x61);
		//iTE_MsgTX((":::: TXP%d HDCP Ri Check Done Interrupt ... %d :::: \r\n", (iTE_u16)port, gmem_tx.HDCPRiChkCnt[port]++));
		//iTE_MsgHDCP(("ARi = 0x%X%X%X%X  \r\n", ARi2>>4, ARi2&0x0F, ARi1>>4, ARi1&0x0F));
		//iTE_MsgHDCP(("BRi = 0x%X%X%X%X  \r\n", BRi2>>4, BRi2&0x0F, BRi1>>4, BRi1&0x0F));
	}
	if( TxReg12&0x20 )
	{
		//APj = h2txrd(port, 0x5A);
		//BPj = h2txrd(port, 0x62);
		//iTE_MsgTX((":::: TXP%d HDCP Pj Check Done Interrupt ...  :::: \r\n", (iTE_u16)port));
		//iTE_MsgTX(("APj = 0x%02X  \r\n", APj));
		//iTE_MsgTX(("BPj = 0x%02X  \r\n", BPj));
	}
	if( TxReg12&0x40 )
	{
		iTE_MsgTX((":::: TXP%d Video Parameter Change Interrupt ... :::: \r\n", (iTE_u16)port));

		iTE_u16 tmp;
		tmp = h2txrd(port, 0x9E);
		tmp += h2txrd(port, 0x9F)&0x07;
		iTE_MsgTX(("TXP%d Video Parameter Change Status = 0x%03X =>  \r\n", (iTE_u16)port, (iTE_u16)tmp));
	}
	if( TxReg13&0x04 )
	{
		iTE_MsgTX((":::: TXP%d ReAuth Request received from RX !!! :::: \r\n", (iTE_u16)port));
	}

}
#endif

void h2tx_pwron(iTE_u8 port)
{
	h2spset(0x08, 0x01<<port, 0x01<<port);
	h2txset(port, 0xC1, 0xF0, 0x80);
    h2txset(port, 0x86, 0x08, 0x08);//20170116
    h2txset(port, 0x84, 0xE0, 0x00);//20170116
	//h2txset(port, 0x88, 0x03, 0x01);
	h2txset(port, 0x84, 0x80, 0x80);//170116
    h2txset(port, 0x02, 0x01, 0x00);// RegGateRCLK=0
	h2txset(port, 0x19, 0x07, 0x07);
	h2txset(port, 0xAF, 0xFF, 0x00);
    if (gext_u8->RXSCDT||gext_u8->CD_SET)//170109
	{
		 h2txwr(port, 0x10, 0xC0);
		 h2txwr(port, 0x11, 0xFF);
		 h2txwr(port, 0x12, 0xFF);
         h2txwr(port, 0x13, 0xFF);
		 h2txwr(port, 0x14, 0xFF);
		 h2txwr(port, 0x15, 0xFF);
		 //170104
		 if ((gext_u8->TxAFESetDone &(1<<port)) == FALSE)
		 {
		  	h2txwr(port, 0x01, 0x24);//170109
		  	h2txwr(port, 0x01, 0x00);
		  	cal_pclk(port);
		  	setup_h2txafe(port);
			msleep(40);
			h2txVclkrst(port);
		  	h2txset(port, 0x18, 0x80, 0x80);    // Enable Video Stable Change Interrupt
			gext_u8->TxAFESetDone |= (1<<port);
		 }
		 //170104
    }
    gext_u8->TxVidStbFlag &= ~(1<<port);
	gext_var->HDCPState[port] = Tx_CP_Reset;
	gext_var->VideoState[port] = Tx_Video_waitInt;
	gmem_tx->HDCPFireCnt[port] = 0;
}
void h2tx_pwrdn(iTE_u8 port)
{
	iTE_u8 TXSRC2,TXSRC3,i;

    iTE_MsgTX(("[TX]		P%d power down \r\n", (iTE_u16)port));
    h2txset(port, 0x88, 0x04, 0x00);//201231 set DRV_HS = 0 when powdon
	h2spset(0x08, 0x01<<port, 0x00<<port);
	h2txset(port, 0x18, 0xDC, 0x00);    // disable Video Stable Change Interrupt
    h2txset(port, 0x19, 0x07, 0x00);    // disable auth interrupt
	h2txset(port, 0x1A, 0xFF, 0x00);
	h2txset(port, 0x1B, 0xFF, 0x00);
	h2txset(port, 0x1C, 0xFF, 0x00);
	h2txset(port, 0x84, 0x60, 0x60);//170116
	h2txset(port, 0x84, 0x80, 0x00);//170116
	h2txset(port, 0x86, 0x08, 0x00);//170116
	h2txset(port, 0x88, 0x03, 0x03);
	h2txset(port, 0x01, 0x26, 0x26);

	gext_var->TXHPD[port] = FALSE;
	gext_u8->TXHP2KSVChkDone &= ~(1<<port);
	gext_u8->TXHDCP2Done &= ~(1<<port);
	gext_u8->TXH2RSABusy &= ~(1<<port);
	gext_u8->TxTMDSStbFlag &= ~(1<<port);
    gext_u8->TxVidStbFlag &= ~(1<<port);
	gext_u8->TxAFESetDone &= ~(1<<port);
	gext_u8->CP_Going &= ~(1<<port);//False
	gext_u8->CP_Done &= ~(1<<port);
	gext_u8->DevCnt_Total = 0;
    gext_u8->Depth_Total = 0;
    gext_u8->Err_DevCnt_Total = FALSE;
    gext_u8->Err_Depth_Total = FALSE;
	gext_var->HDCPState[port] = 0;
	gext_var->HDCPWaitCnt[port] = 0;
	gext_var->VideoState[port] = 0;
	gmem_tx->FiFoErrcnt[port] = 0;
	gext_u8->ForceCPDwnVer &= ~ (1<<port);
	gext_var->TxOut_DS[port] = 0;
	gext_var->HDCPFireVer[port] = 0;
	TXSRC3 = 0;
	TXSRC2 = 0;
	#ifdef IT6664
		for(i=0;i<TxPortNum;i++)
	#else
		for(i=1;i<TxPortNum+1;i++)
	#endif
		{
			if(gext_var->TxSrcSel[i] == 3)  TXSRC3++;
			if(gext_var->TxSrcSel[i] == 2)  TXSRC2++;
		}
		if ((gext_var->TxSrcSel[port]==0x02)&&(TXSRC2<=0x01))
        {
         	h2spset(0x0B, 0x1C, 0x1C);
		  	h2spset(0x0B, 0x1C, 0x00);
			h2spwr(0x67,0x00);
		}
		if ((gext_var->TxSrcSel[port]==0x03)&&(TXSRC3<=0x01))
		{
		  	if ((gext_u8->InColorMode == YCbCr420)&&(TXSRC2<=0x01))
		  	{
		  		h2spset(0x0B, 0x1C, 0x1C);
		  		h2spset(0x0B, 0x1C, 0x00);
		  		h2spset(0x0B, 0xE0, 0xE0);
		  		h2spset(0x0B, 0xE0, 0x00);
				h2spwr(0x67,0x00);
		  	}
		  	else
		  	{
		  		h2spset(0x0B, 0xE0, 0xE0);
		  		h2spset(0x0B, 0xE0, 0x00);
		  	}
         }
	h2spset(0x0D,0x03<<(port*2),0);
	gext_var->TxSrcSel[port] = 0;

	TXSRC2 = 0;
	#ifdef IT6664
		for(i=0;i<TxPortNum;i++)
	#else
		for(i=1;i<TxPortNum+1;i++)
	#endif
		{
			if((h2txrd(i,0x03)&0x01) == 0x01) TXSRC2 = 1;
		}
		if(!TXSRC2)
		{
			if(HPD_Debounce == TRUE)
			{
				gext_u8->TimerIntFlag = 5;
			}
			else
			{
				SetRxHpd(FALSE);
			}
		}
	h2tx_rst(port);

}
void h2txVclkrst(iTE_u8 port)
{
	h2txwr(port, 0x01, 0x06);
	h2txset(port, 0x94, 0x01, 0x01);
    h2txset(port, 0x94, 0x01, 0x00);
	h2txwr(port, 0x01, 0x04);
	h2txwr(port, 0x01, 0x00);
}
void hdcpsts(iTE_u8 val,iTE_u8 port)
{
 	iTE_u8 ARi1, ARi2, BRi1, BRi2;
 	iTE_u8 BCaps, BStatus1, BStatus2;
 	iTE_u8 AuthStatus;
	iTE_u16 HDCP2FailStatus;
	iTE_u8 RxID1,RxID2,RxID3,RxID4,RxID5;

	if(val == 1)
	{
   	 	ARi1 = h2txrd(port, 0x58);
    	ARi2 = h2txrd(port, 0x59);
		BRi1 = h2txrd(port, 0x60);
    	BRi2 = h2txrd(port, 0x61);
    	AuthStatus = h2txrd(port, 0x66);
    	BCaps = h2txrd(port, 0x63);
    	BStatus1 = h2txrd(port, 0x64);
    	BStatus2 = h2txrd(port, 0x65);
    	//iTE_MsgHDCP(("[HDCP]	TXP%d AR0 = 0x%X%X%X%X  \r\n", port, ARi2>>4, ARi2&0x0F, ARi1>>4, ARi1&0x0F));
    	//iTE_MsgHDCP(("[HDCP]	TXP%d BR0 = 0x%X%X%X%X  \r\n", port, BRi2>>4, BRi2&0x0F, BRi1>>4, BRi1&0x0F));
    	iTE_MsgHDCP(("[HDCP]	TXP%d Rx HDCP Repeater = %d  \r\n", (iTE_u16)port, (iTE_u16)((BCaps&0x40)>>6)));
    	iTE_MsgHDCP(("[HDCP]	TXP%d Authentication Status = 0x%X  \r\n", (iTE_u16)port, AuthStatus));
		if( ((AuthStatus&0x80)==0x80) && ((BCaps&0x40)==0x00) && EnRxHP1Rpt )
		{//for 6664 repeater usage with downstream not repeater, KSV should be write here
       		h2txset(port, 0x41, 0x10, 0x10);//since downstream is receiver, write KSV
			iTE_MsgHDCP(("P%d write downstream Receiver BKSV to fifo!! \r\n", port));

			if(((EnableTXP0HDCP==FALSE)||(gext_var->TXHPD[0]==FALSE)||(gext_u8->CP_Done&(0x01))||(port==0))&&
			((EnableTXP1HDCP==FALSE)||(gext_var->TXHPD[1]==FALSE)||(gext_u8->CP_Done&(0x02))||(port==1))&&
			((EnableTXP2HDCP==FALSE)||(gext_var->TXHPD[2]==FALSE)||(gext_u8->CP_Done&(0x04))||(port==2))&&
			((EnableTXP3HDCP==FALSE)||(gext_var->TXHPD[3]==FALSE)||(gext_u8->CP_Done&(0x08))||(port==3))&&EnRxHP1Rpt)
			{
          		gext_u8->DevCnt_Total++;
         		gext_u8->Depth_Total = gext_u8->Depth_Total ? gext_u8->Depth_Total : 0;
          		iTE_MsgHDCP(("Set RX ksv list start \r\n"));
	      		setrx_ksv_list(gext_u8->DevCnt_Total++, gext_u8->Depth_Total, gext_u8->Err_DevCnt_Total, gext_u8->Err_Depth_Total);

			}
    	}
		if( (AuthStatus&0x80)!=0x80 )
		{
    		iTE_MsgHDCP(("[HDCP]	AuthStatus = %02x \r\n",AuthStatus));
    	}
	}
	else if(val == 2)
	{
		RxID1 = h2txrd(port,0x5B);
    	RxID2 = h2txrd(port,0x5C);
    	RxID3 = h2txrd(port,0x5D);
    	RxID4 = h2txrd(port,0x5E);
    	RxID5 = h2txrd(port,0x5F);
    	HDCP2FailStatus = ((h2txrd(port, 0x4F)<<8) + h2txrd(port, 0x4E));

    	iTE_MsgTX(("[HDCP]	TXP%d RxID = 0x%2X%2X%2X%2X%2X  \r\n", port, RxID5,
                                                                       RxID4,
                                                                       RxID3,
                                                                       RxID2,
                                                                       RxID1));

    	iTE_MsgTX(("[HDCP]	TXP%d HDCP2Tx Authentication Fail Status %04X  \r\n", port, HDCP2FailStatus));
		if( (HDCP2FailStatus==0x0000)&&((gext_u8->TXHP2KSVChkDone&(1<<port))==FALSE)&&EnRxHP2Rpt )
		{
			iTE_MsgTX(("mannually write HP2 BKSV to FIFO\r\n"));
        	h2txcomset(0x20, 0x04, 0x04);
			h2txcomwr(0x21, (h2txcomrd(0x22)));
			h2txcomwr(0x23, RxID1);
        	h2txcomwr(0x24, RxID2);
			h2txcomwr(0x25, RxID3);
			h2txcomwr(0x26, RxID4);
       	 	h2txcomwr(0x27, RxID5);
			h2txcomset(0x20, 0x04, 0x00);
        	if(((EnableTXP0HDCP==FALSE)||((gext_u8->TXHPDsts&(0x01))==FALSE)||(gext_u8->CP_Done&(0x01)))&&
			((EnableTXP1HDCP==FALSE)||((gext_u8->TXHPDsts&(0x02))==FALSE)||(gext_u8->CP_Done&(0x02)))&&
			((EnableTXP2HDCP==FALSE)||((gext_u8->TXHPDsts&(0x04))==FALSE)||(gext_u8->CP_Done&(0x04)))&&
			((EnableTXP3HDCP==FALSE)||((gext_u8->TXHPDsts&(0x08))==FALSE)||(gext_u8->CP_Done&(0x08))) &&
			(gext_u8->RXHDCP2_chk == 0))//for 3C-01-4 first RX Auth done do not return RX list
			{
          		gext_u8->DevCnt_Total++;
          		gext_u8->Depth_Total = gext_u8->Depth_Total ? gext_u8->Depth_Total : 0;
          		iTE_MsgTX(("Set RX ksv list start \r\n"));
	      		setrx_ksv_list(gext_u8->DevCnt_Total++, gext_u8->Depth_Total, gext_u8->Err_DevCnt_Total, gext_u8->Err_Depth_Total);
			}

		}

		if( HDCP2FailStatus!=0x0000 )
		{
			if(( HDCP2FailStatus&0x0010)||( HDCP2FailStatus&0x0100))
			{
            	iTE_MsgTX(("[HDCP]	Auth Fail: Read AKE Send H prime (km_stored_rdy=0) 1sec timeout ! \r\n"));
			}
			if( HDCP2FailStatus&0x0200)
            	iTE_MsgTX(("[HDCP]	Auth Fail: A1/A5 polling Rxstatus ReAuth_Req = 1 ! \r\n"));
			if( HDCP2FailStatus&0x0040)
			{
				iTE_MsgTX(("[HDCP]	Auth Fail: Read Repeater receiver ID List 3s timeout ! \r\n"));
				msleep(300);
				FireHDCP2(port);
				gext_var->HDCPState[port] = 0;
			}
        	if( HDCP2FailStatus&0x0400)
        	{
            	iTE_MsgTX(("[HDCP]	Auth Fail: Repeater device count/cascade exceed or seq_num_V Error ! \r\n"));
         		if ((h2txrd(port,0x64))&0x04)
		 		{
		 			iTE_MsgTX(("Repeater cascade exceed max!!\r\n"));
		 			setrx_ksv_list(0, 0, 0, 1);
		 		}
		 		else if ((h2txrd(port,0x64))&0x08)
		 		{
		 			iTE_MsgTX(("Repeater device count exceed max!!\r\n"));
		 			setrx_ksv_list(0, 0, 1, 0);
		 		}
        	}
		}
	}
}
void setup_h2txafe(iTE_u8 port)
{
	iTE_u8 DRV_TERMON, DRV_RTERM, DRV_ISW, DRV_ISW_C, DRV_TPRE, DRV_NOPE;//, XIP_SPARE1;

		if( gext_long->TXVCLK[port]>100000 )
			h2txset(port, 0x84, 0x07, 0x04);
		else
			h2txset(port, 0x84, 0x07, 0x03);
		if( gext_long->TXVCLK[port]>162000 )
			h2txset(port, 0x88, 0x04, 0x04);
		else
			h2txset(port, 0x88, 0x04, 0x00);


		// 2017/02/03 modified by jjtseng for Dabby_Chen request
		if( gext_long->TXVCLK[port]>375000 )
		{ 	// single-end swing = 520mV
			gext_u8->ForceTXHDMI2 |= (1<<port);
			DRV_TERMON = 1;
			DRV_RTERM = 0x5;
			DRV_ISW = 0xE;
			DRV_ISW_C = 0xD;
			DRV_TPRE = 0x0;
			DRV_NOPE = 0;
		}
		else if( gext_long->TXVCLK[port]>310000 )
		{	// single-end swing = 520mV
			gext_u8->ForceTXHDMI2 |= (1<<port);
			DRV_TERMON = 1;
			DRV_RTERM = 0x5;
			DRV_ISW = 0xD;
			DRV_ISW_C = 0xB; // reduce the clock swing
			DRV_TPRE = 0x0;
			DRV_NOPE = 0;
		//~jjtseng 2017/02/03
		} else if( gext_long->TXVCLK[port]>150000 )
		{	// single-end swing = 450mV
			gext_u8->ForceTXHDMI2 &= ~(1<<port);
			DRV_TERMON = 1;
			DRV_RTERM = 0x1;
			DRV_ISW = 0x9;
			DRV_ISW_C = 0x9;
			DRV_TPRE = 0;
			DRV_NOPE = 0;
		} else
		{	// single-end swing = 500mV
			gext_u8->ForceTXHDMI2 &= ~(1<<port);
			DRV_TERMON = 0;
			DRV_RTERM = 0x0;
			DRV_ISW = 0x3;
			DRV_ISW_C = 0x3;
			DRV_TPRE = 0;
			DRV_NOPE = 1;
		}

		h2txset(port, 0x87, 0x1F, DRV_ISW); 	// DRV_ISW[4:0]
		h2txset(port, 0x89, 0xBF, (DRV_NOPE<<7)+(DRV_TERMON<<5)+DRV_RTERM);
		h2txset(port, 0x8A, 0x0F, DRV_TPRE);
		h2txset(port, 0x8B, 0x0F, DRV_ISW_C);

}
void h2tx_enout(iTE_u8 port)
{

	iTE_u8 rddata,ret;

	//check DRV_HS again before output
	if( gext_long->TXVCLK[port]>162000 )
	{
			h2txset(port, 0x88, 0x04, 0x04);
	}
	else
	{
			h2txset(port, 0x88, 0x04, 0x00);
	}

	h2txset(port, 0x88, 0x03, 0x01);//Move from Tx power on
	h2txset(port, 0x42, 0x01, 0x01);//enable CP_Desire first before TMDS output
	ret =1;
		h2txset(port, 0x18, 0x0C, 0x0C);
		h2txwr(port,0x85,0x19);
		h2txset(port, 0x1A, 0x0B, 0x0B);
		#if (USING_1to8==TRUE)
		if(g_device == IT6663_C)
		{
			ret = setup_h2scdc(port);
			mSleep(50);
		}
		else
		#endif
		{
			if(((gext_var->TXSupport4K60[port]&0x01)&&(!gext_var->TXSupportOnly420[port]))
				|| gext_var->TXSupport420[port] || (gext_var->TXSupportOnly420[port] && (gext_var->TXSupportDC420[port]&0x03)))
			{
				ret = setup_h2scdc(port);
				msleep(50);
				if(!ret)
				{
					//iTE_MsgTX(("[TX]	TXP%d SCDC set 0 \r\n",(iTE_u16)port));
				}
				else
				{
					gext_u8->TXClearSCDC |= (1<<port);
				}
			}
			else
			{
				if(h2txrd(port,0xC0)&0x02) h2txset(port,0xC0,0x02,0x00);//disable hdmi2.0 encoding
			}
		}
		h2txset(port, 0xC1, 0x08, 0x00);
		h2txset(port, 0xC1, 0x08, 0x08);
		h2txset(port, 0xC2, 0x80, 0x80);
		h2txset(port, 0xC3, 0x30, 0x30);	// Enable GenCtrl Packet
		h2txset(port, 0x88, 0x03, 0x00); // Set DRV_RST='0' and pwd = 0
		msleep(100);//150->100
		#if (USING_1to8==TRUE)
		if(g_device == IT6663_C)
		{
			ret = setup_h2scdc(port);
		}
		else
		#endif
		{
			if(((gext_var->TXSupport4K60[port]&0x01)&&(!gext_var->TXSupportOnly420[port]))
				|| gext_var->TXSupport420[port] || (gext_var->TXSupportOnly420[port] && (gext_var->TXSupportDC420[port]&0x03)))
			{
				ret = setup_h2scdc(port);
				if(!ret) iTE_MsgTX(("[TX]	TXP%d SCDC set 0 \r\n",(iTE_u16)port));
			}
			else
			{
				if(h2txrd(port,0xC0)&0x02) h2txset(port,0xC0,0x02,0x00);//disable hdmi2.0 encoding
			}
		}

		rddata = h2txrd(port, 0x03);
		if( (rddata&0x08)==0x00 )//not stable
		{
			h2txVclkrst(port);
			h2txset(port, 0xC1, 0xF0, 0x80);
			iTE_MsgTX(("[TX]	TX port%d rddata = %2x VCLK not stable yet, reset \r\n",(iTE_u16) port,rddata));
			msleep(30);
			rddata = h2txrd(port, 0x03);
		}
		if((rddata&0x0F)==0x0F)
		{
			if(gext_u8->RXHDCP)
			{
				gext_var->HDCPState[port] = Tx_CP_Going;
				gext_var->VideoState[port] = Tx_Video_OK;
				gext_var->HDCPWaitCnt[port] =3;
				iTE_MsgTX(("[TX]	Tx_CP_Going+Tx_Video_OK \r\n"));
			}
			else //gext_u8->RXHDCP = 0
			{
				gext_var->HDCPState[port] = Tx_CP_check;
				gext_var->VideoState[port] = Tx_Video_OK;
				iTE_MsgTX(("[TX]	RXHDCP = 0 ,goto Tx_CP_check \r\n"));
			}
			if(!ret)
			{
				if((gext_u8->InColorMode == YCbCr420) && (!(gext_u8->GCP_CD&0x03)))
				{
					gext_var->HDCPState[port] = Tx_CP_check;
					if(h2txrd(port,0xC0)&0x02) h2txset(port,0xC0,0x02,0x00);//disable hdmi2.0 encoding
					iTE_MsgTX(("[TX]	420 8bit  Tx_CP_check \r\n"));
				}
				if(gext_long->TXVCLK[port]<340000)
				{
					if(h2txrd(port,0xC0)&0x02) h2txset(port,0xC0,0x02,0x00);//disable hdmi2.0 encoding
				}
				else
				{
					if(((gext_u8->ForceCPDwnVer&(1<<port)) == 0x00) && (gext_u8->ForceTXHDMI2 &(1<<port)))
					{
						if(PortBypass_Opt&(1<<port))//ite_200117
						{
							gext_var->HDCPState[port] = Tx_CP_WaitInt;
							gext_var->VideoState[port] = Tx_Video_waitInt;
							iTE_MsgTX(("[TX]	Force Bypass output stop by scdc set fail \r\n"));
						}
						else
						{
							gext_var->HDCPState[port] = Tx_CP_WaitInt;
							gext_var->VideoState[port] = Tx_Video_Stable;
							iTE_MsgTX(("[TX]	HDMI 2.0 Set SCDC fail ,goto Tx_Video_Stable \r\n"));
						}//ite_200117
					}
				}
			}
		}
		else
		{
			if((h2txrd(port,0x03)&0x03)!=0x03)
			{
				gext_var->VideoState[port] = Tx_Video_waitInt;
				gext_var->HDCPState[port] = Tx_CP_WaitInt;
			}
			else
			{
				iTE_MsgTX(("[TX]	TXP%d VCLK still unstable ... \r\n",(iTE_u16) port));
			}
		}

}
void ShowMsg(iTE_u8 M[])
{
	iTE_u16 i ;

    for(i = 0; i < 64; i++) {
        iTE_MsgTX(("%02X ",(iTE_u16)M[i])) ;
        if( i % 16 == 15)
            iTE_MsgTX((" \r\n")) ;
    }

    iTE_MsgTX((" \r\n"));
}
iTE_u8 CheckSinkSCDC(iTE_u8 port)
{
	iTE_u8 reg;

	if(scdcrd(port,0x21,1) == iTE_TRUE)
	{
		reg = h2txrd(port,0x30);
		if(reg==0x01)
			return iTE_TRUE;
		else
			return iTE_FALSE;
	}
	// read fail
	return iTE_FALSE;
}
////////////////////////////////////////////////////////////////////////////////
// SCDC functionn
////////////////////////////////////////////////////////////////////////////////
iTE_u8 scdcwr(iTE_u8 port, iTE_u8 offset, iTE_u8 data)
{
	iTE_u8 ddcwaitsts;

    if( (h2txrd(port, 0x03)&0x01)==0x00) {
        iTE_MsgTX(("[TX]	TXP%d Abort SCDC write becasue of detecting unplug !!! \r\n", (iTE_u16)port));
        return FALSE;
    }

    h2txset(port, 0x28, 0x01, EnDDCMasterSel);
    h2txwr(port, 0x2E, 0x09);
    h2txwr(port, 0x29, 0xA8);
    h2txwr(port, 0x2A, offset);
    h2txwr(port, 0x2B, 0x01);
    h2txset(port, 0x2C, 0x03, 0x00);
    h2txwr(port, 0x30, data);
    h2txwr(port, 0x2E, 0x01);

    ddcwaitsts = ddcwait(port);
    h2txset(port, 0x28, 0x01, 0x00);

    return ddcwaitsts;
}

iTE_u8 scdcrd(iTE_u8 port, iTE_u8 offset, iTE_u8 bytenum)
{
iTE_u8 ddcwaitsts;

    if( (h2txrd(port, 0x03)&0x01)==0x00 ) {
        iTE_MsgTX(("[TX]	TXP%d Abort SCDC read becasue of detecting unplug !!! \r\n", (iTE_u16)port));
        return FALSE;
    }

    h2txset(port, 0x28, 0x01, EnDDCMasterSel);
    h2txwr(port, 0x2E, 0x09);
    h2txwr(port, 0x29, 0xA8);
    h2txwr(port, 0x2A, offset);
    h2txwr(port, 0x2B, bytenum);
    h2txset(port, 0x2C, 0x03, 0x00);
    h2txwr(port, 0x2E, 0x00);
    h2txset(port, 0x28, 0x01, 0x00);

    ddcwaitsts = ddcwait(port);
    h2txset(port, 0x28, 0x01, 0x00);

    return ddcwaitsts;
}
iTE_u8 setup_h2scdc(iTE_u8 port)
{
	iTE_u8 rddata,rdtime,ret;
	iTE_u8 HF_VSDB_B6[4] = {0x00, 0x00, 0x00, 0x00};
	iTE_u8 Det_HFVSDB[4] = {0x00, 0x00, 0x00, 0x00};
	iTE_u8 H2ClkRatio;
	iTE_u8 EnH2Enc;
	iTE_u8 EnH2Scr;
	iTE_u8 EnH2DetRR;
	iTE_u8 EnFlagPolling;
	ret = 0;
	H2ClkRatio = TRUE;
	EnH2Enc = TRUE;
	EnH2Scr = TRUE;
	EnH2DetRR = FALSE; //This setting will be over-written by HF_VSDB when EnEDIDParse=TRUE and ChkHFVSDB=TRUE
	EnFlagPolling = TRUE; //This setting will be over-written by HF_VSDB when EnEDIDParse=TRUE and ChkHFVSDB=TRUE

	if (gext_u8->ForceTXHDMI2&(1<<port))
	{
		iTE_MsgTX(("[TX]	SCDC Set HDMI2.0   \r\n"));
		EnH2Enc = TRUE;
       	EnH2Scr = TRUE;
       	H2ClkRatio = TRUE;
	}
    else if( EnEDIDParse && ( ( ChkHFVSDB && Det_HFVSDB[port]==FALSE ) || ( ChkSCDC && ((HF_VSDB_B6[port]&0x80)==0x00) ) ) ) {
		iTE_MsgTX(("[TX]	SCDC Set HDMI1.4  \r\n"));
		EnH2Enc = FALSE;
        EnH2Scr = FALSE;
        H2ClkRatio = FALSE;
        EnFlagPolling = FALSE;
        EnH2DetRR = FALSE;
    } else if( EnEDIDParse && ChkHFVSDB && Det_HFVSDB[port] ) {
    	iTE_MsgTX(("[TX]	Decide by VCLK  \r\n"));
        if( gext_long->TXVCLK[port]<=340000 ) {
            EnH2Enc &= ((HF_VSDB_B6[port]&0x08)>>3);
            EnH2Scr = EnH2Enc;
            H2ClkRatio = FALSE;
        } else {
            EnH2Enc = TRUE;
            EnH2Scr = TRUE;
            H2ClkRatio = TRUE;
        }
        if( (EnPHYTest==FALSE) && ((HF_VSDB_B6[port]&0x80)==0x80) )
            EnFlagPolling = TRUE;
        else
            EnFlagPolling = FALSE;

        if( (EnPHYTest==FALSE) && ((HF_VSDB_B6[port]&0x40)==0x40) )
            EnH2DetRR = TRUE;
        else
            EnH2DetRR = FALSE;
    }

    if( EnEDIDParse && Det_HFVSDB[port]==FALSE ) {
        EnFlagPolling = FALSE;
        EnH2DetRR = FALSE;
    }
    if( EnH2DetRR ) // When EnH2DetRR is enabled, EnFlagPolling is disabled.
        EnFlagPolling = FALSE;
    h2txset(port, 0x83, 0x08, (H2ClkRatio<<3)); // HDMI2ONPLL
    h2txset(port, 0xC0, 0x46, (H2ClkRatio<<6)+(EnH2Scr<<2)+(EnH2Enc<<1));
    //idle(500000);
	//mSleep(5);
    h2txset(port, 0x3A, 0x03, (EnH2DetRR<<1)+EnFlagPolling);
    if( EnFlagPolling )
   	{
        h2txset(port, 0x1A, 0x01, 0x01);   // Enable SCDC update flag change interrupt
    }
    if( EnH2DetRR )
    {
        h2txset(port, 0x1A, 0x02, 0x02);   // Enable SCDC detect read request interrupt
    }

    if( EnH2Enc ) {
        scdcwr(port, 0x02, 0x01);
        if( scdcrd(port, 0x02, 1)==TRUE ) {
            rddata = h2txrd(port, 0x30);
            //iTE_MsgTX(("[TX]	TXP%d RX SCDC Sink Version = 0x%02X \r\n", (iTE_u16)port, (iTE_u16)rddata));
        }
		//170109
		rdtime=0;
		h2txset(port, 0x18, 0x04, 0x04);//enable ddc cmd interrupt
		do {
				scdcwr(port, 0x20, (H2ClkRatio<<1)+(EnH2Scr<<0));
				rdtime++;
				if( scdcrd(port, 0x20, 1)==TRUE )
				{
				  rddata = h2txrd(port, 0x30);
				  ret = 1;
				}
				else
				{
				  rddata = 0;
				  ret = 0;
				  iTE_MsgTX(("[TX]	TXP%d SCDC Reg 0x20 read fail \r\n", (iTE_u16) port));
				}
				if((h2txrd(port,0x03)&0x01)!=0x01)
				{
					rddata = 0;
				  	ret = 0;
					break;
				}
			}while ( ((rddata&0x03) != ((H2ClkRatio<<1)+EnH2Scr))&& (rdtime<=10));
	}
	else {
		//170109
        scdcwr(port, 0x20, 0x00);
	    if( scdcrd(port, 0x20, 1)==TRUE )
		{
            rddata = h2txrd(port, 0x30);
        }
    }

    if( EnH2DetRR ) {
        scdcwr(port, 0x30, 0x01);
        if( scdcrd(port, 0x30, 1)==TRUE ) {
            rddata = h2txrd(port, 0x30);
            iTE_MsgTX(("[TX]	TXP%d RX SCDC RR_Enable=%d \r\n", (iTE_u16)port, (iTE_u16)(rddata&0x01)));
        }
    } else if( (Det_HFVSDB[port]==TRUE) &&  ((HF_VSDB_B6[port]&0x80)==0x80) ) {
        scdcwr(port, 0x30, 0x00);
        if( scdcrd(port, 0x30, 1)==TRUE ) {
            rddata = h2txrd(port, 0x30);
            iTE_MsgTX(("[TX]	TXP%d RX SCDC RR_Enable=%d \r\n",(iTE_u16) port, (iTE_u16)(rddata&0x01)));
        }
    }

    //iTE_MsgTX(("[TX]	TXP%d Current Setting: H2ClkRatio=%d, EnH2Enc=%d, EnH2Scr=%d  ret = %02x \r\n", (iTE_u16)port,(iTE_u16) H2ClkRatio, (iTE_u16)EnH2Enc, (iTE_u16)EnH2Scr,(iTE_u16)ret));
	return ret;
}
////////////////////////////////////////////////////////////////////////////////
// TX HDCP functionn
////////////////////////////////////////////////////////////////////////////////
void hdcprd( iTE_u8 port, iTE_u8 offset, iTE_u8 bytenum )
{
// int rddata;
	iTE_u8 ret;
	iTE_u8 retry = 1;

	__RETRY:

    if( (h2txrd(port, 0x03)&0x01)==0x00 )
	{
        return;
    }

    h2txset(port, 0x28, 0x01, EnDDCMasterSel);
    h2txwr(port, 0x2E, 0x09);
	h2txwr(port, 0x2E, 0x0F);         // Abort DDC Command
    msleep(3);
	h2txwr(port, 0x2E, 0x0F);         // Abort DDC Command
	msleep(3);
    h2txwr(port, 0x2E, 0x09);           // DDC FIFO Clear

    h2txwr(port, 0x29, 0x74);           // HDCP Address
    h2txwr(port, 0x2A, offset);         // HDCP Offset
    h2txwr(port, 0x2B, bytenum);        // Read ByteNum[7:0]
    h2txwr(port, 0x2C, (bytenum&0x300)>>8);  // ByteNum[9:8]
    h2txwr(port, 0x2E, 0x00);           // HDCP Read Fire

    ret = ddcwait(port);
    h2txset(port, 0x28, 0x01, 0x00);    // Disable PC DDC Mode
	if ( ret == FALSE ) {
        if ( retry > 0 ) {
            retry--;
            msleep(10);
            goto __RETRY;
        }
    }
}

void chktx_hdcp2_ksv(iTE_u8 port)
{
iTE_u16 rddata, RxInfo, DevCnt, Depth, Err_DevCnt, Err_Depth, CurKsvFifoWrStg, EndKsvFifoWrStg;
iTE_u16 ksvtimeout;
//iTE_u16 ksvlist[160];
	//iTE_u16 i,IntMask;

    //txport = TxH2EngSel[h2eng];

    //chgswbank(3);
    //h2swset(0x10, 0x01<<TxPortSel[txport], 0x01<<TxPortSel[txport]);    // RegEnKsvFifo
    h2txcomset(0x20, 0x70, (0x40+(port<<4)));    // RegKsvSrcSel[1:0], RegKsvFifoSel[1:0]
    CurKsvFifoWrStg = h2txcomrd(0x22);
    iTE_MsgTX(("[HDCP]	TXP%d write KSV List to KSV FIFO, CurKsvFifoWrStg=%d \r\n", (iTE_u16)port, CurKsvFifoWrStg));

    RxInfo = h2txrd(port,0x64);
    RxInfo += h2txrd(port,0x65)<<8;
	if (RxInfo&0x01)
	{
		gext_u8->HP2_HDCP1DownStm = 1;
	 	iTE_MsgTX(("received hdcp1 down stream exist!!\r\n"));
	}
    if (RxInfo&0x02)
	{
		gext_u8->HP2_HDCP20DownStm = 1;
	 	iTE_MsgTX(("received hdcp20 legacy down stream exist!!\r\n"));
    }
    DevCnt = (RxInfo&0x1F0)>>4;
    Depth = (RxInfo&0xE00)>>9;
    Err_Depth = (RxInfo&0x04)>>2;
    Err_DevCnt = (RxInfo&0x08)>>3;

    h2txset(port, 0x6B, 0x04, 0x04);  // RH*KSVWrTrg=1


    //IntMask = 0x02<<(4*h2eng);
    //170104
    if ((RxInfo&0x01F0)!=0x00) //downstream has non zero KSV list num
    {
    	ksvtimeout=0;
    	do{
        	//idle(100000);
        	//usleep(1);
        	rddata = h2txrd(port, 0x13);
        	iTE_MsgTX(("Waiting for TX%d with HDCP2.2  KSV List Write Done Interrupt ... \r\n", (iTE_u16)port));
        	ksvtimeout++;
    	}while(((rddata&0x80)==0x00)&&(ksvtimeout<=200) );
    	h2txset(port, 0x13, 0x80, 0x80); // clear interrupt
    }
    //170104
    EndKsvFifoWrStg = h2txcomrd(0x22);
    iTE_MsgTX(("[HDCP]	TXP%d write KSV List to  KSV FIFO, EndKsvFifoWrStg=%d \r\n", (iTE_u16)port, EndKsvFifoWrStg));

    if( (EndKsvFifoWrStg-CurKsvFifoWrStg)!=(DevCnt+1) ) {
        iTE_MsgTX(("[HDCP]	ERROR: KSV List mismatch Device Count=%d, KSV List Count=%d !!! Press any key to continue ... \r\n", DevCnt, EndKsvFifoWrStg-CurKsvFifoWrStg-1));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // read back ksvlist start for debug only
    /*h2swset(0x12, 0x80, 0x80);  // RegEnFwKsvFifo=1
    for(i=CurKsvFifoWrStg; i<EndKsvFifoWrStg; i++) {
        h2swwr(0x1A, i);        // KSV FIFO starts from stage CurKsvFifoWrStg
        h2swbrd(0x1B, 5, &ksvlist[i*5]);
    }

    for(i=CurKsvFifoWrStg; i<DevCnt; i++)
        iTE_Msg(("TXP%d KSV List %d = 0x %02X %02X %02X %02X %02X \r\n", txport, i, ksvlist[i*5+4], ksvlist[i*5+3], ksvlist[i*5+2], ksvlist[i*5+1], ksvlist[i*5+0]));

    iTE_Msg(("TXP%d KSV List %d = 0x %02X %02X %02X %02X %02X <= This is BKSV of downstream device \r\n", txport, i, ksvlist[i*5+4], ksvlist[i*5+3], ksvlist[i*5+2], ksvlist[i*5+1], ksvlist[i*5+0]));
    iTE_Msg((" \r\n"));
    h2swset(0x12, 0x80, 0x00);  // RegEnFwKsvFifo=0
    */
	// read back ksvlist end for debug only
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    //chgswbank(1);
    h2txset(port, 0x42, 0x80, 0x80); // RH*HDCP2ListRdDone=1
    //chgswbank(0);
	//170104
    gext_u8->DevCnt_Total = gext_u8->DevCnt_Total + DevCnt + 1;
    gext_u8->Err_DevCnt_Total = Err_DevCnt || gext_u8->Err_DevCnt_Total;
    gext_u8->Err_Depth_Total = gext_u8->Err_Depth_Total || Err_Depth;
    if (Depth > gext_u8->Depth_Total)
	{
		gext_u8->Depth_Total = Depth;
	}
	/*
	if(((EnableTXP0HDCP==FALSE)||(gext_var->TXHPD[0]==FALSE)||((gext_u8->TXHDCP2Done &(0x01))==TRUE))&&
		((EnableTXP1HDCP==FALSE)||(gext_var->TXHPD[1]==FALSE)||((gext_u8->TXHDCP2Done &(0x02))==TRUE))&&
		((EnableTXP2HDCP==FALSE)||(gext_var->TXHPD[2]==FALSE)||((gext_u8->TXHDCP2Done &(0x04))==TRUE))&&
		((EnableTXP3HDCP==FALSE)||(gext_var->TXHPD[3]==FALSE)||((gext_u8->TXHDCP2Done &(0x08))==TRUE)))
	*/
	if(EnRxHP2Rpt)//this time may check HDCP2.2 done or not
    {
      	iTE_MsgTX(("[HDCP]	All TX port HDCP2 done, begin RX Repeater Steps \r\n"));
	  	setrx_ksv_list(gext_u8->DevCnt_Total, gext_u8->Depth_Total, gext_u8->Err_DevCnt_Total, gext_u8->Err_Depth_Total);
	}
	//170104
}
iTE_u8 txrunhdcp(iTE_u8 port)
{
 	iTE_u8 RxHDMIMode;
 	iTE_u8 WaitCnt;
	iTE_u8 BKSV[5],BStatus[2];
	iTE_u8 ret,Waitfail;
	static iTE_u8 retry = 0;

	ret = 0;
	Waitfail = 0;
	gext_var->HDCPFireVer[port] = 1;
    if((h2txrd(port, 0x03)&0x01)!=0x01)//HPD Fail
	{
        return FAIL;
    }
	h2txset(port, 0x19, 0x07, 0x07);
	h2txset(port, 0x1A, 0xB0, 0xB0);
    h2txset(port, 0x42, 0x10, 0x00);
    h2txset(port, 0x18, 0x04, 0x04);
    h2txset(port, 0x1A, 0x08, 0x08);
    h2txset(port, 0x41, 0x01, 0x00);
	h2txset(port, 0x01, 0x20, 0x20);
	msleep(1);
    h2txset(port, 0x01, 0x20, 0x00);
    h2txset(port, 0x49, 0xFF, 0x00);
    h2txwr(port, 0xFF, 0xC3);
    h2txwr(port, 0xFF, 0xA5);
    h2txset(port, 0xC2, 0x40, EncDis<<6);
    h2txset(port, 0xC2, 0x1F, 0x0A);
    h2txset(port, 0x6B, 0x03, (DisRiShortRead<<1)+DisR0ShortRead);
    h2txwr(port, 0xFF, 0xFF);
    h2txset(port, 0x41, 0x06, (EnSyncDetChk<<2)+(EnHDCP1p1<<1));
    h2txset(port, 0x6F, 0x0F, (EnHDCPAutoMute<<3)+(EnSyncDet2FailInt<<2)+(EnRiChk2DoneInt<<1)+EnAutoReAuth);
    h2txset(port, 0x19, 0x07, 0x07);
    h2txwr(port, 0x12, 0x30);
    h2txset(port, 0x1A, 0x30, 0x00);
    h2txset(port, 0x40, 0x01, 0x01);
    msleep(1);
    h2txset(port, 0x40, 0x01, 0x00);
    h2txwr(port, 0x48, h2txrd(port, 0x50));
    h2txwr(port, 0x49, h2txrd(port, 0x51));
    h2txwr(port, 0x4A, h2txrd(port, 0x52));
    h2txwr(port, 0x4B, h2txrd(port, 0x53));
    h2txwr(port, 0x4C, h2txrd(port, 0x54));
    h2txwr(port, 0x4D, h2txrd(port, 0x55));
    h2txwr(port, 0x4E, h2txrd(port, 0x56));
    h2txwr(port, 0x4F, h2txrd(port, 0x57));
    h2txset(port, 0x41, 0x01, 0x01);
    hdcprd(port, 0x41, 2);
    RxHDMIMode = (h2txrd(port, 0x65)&0x10)>>4;
	BStatus[0] = h2txrd(port, 0x64);
	BStatus[1] = h2txrd(port, 0x65);

	if(gmem_tx->HDCPFireCnt[port]<0xFE) gmem_tx->HDCPFireCnt[port]++;

    iTE_MsgTX(("[HDCP]	TXP%d HDCP Fire %d => Current status =%02x  \r\n", (iTE_u16)port, gmem_tx->HDCPFireCnt[port], RxHDMIMode));
	if( (BStatus[0]&0x80) || (BStatus[1]&0x08) )
	{
		h2txset(port, 0xC2, 0x80, 0x80);
	}
	else
	{
		h2txset(port, 0xC2, 0x80, 0x00);
	}
    WaitCnt = 0;

	if(h2txrd(port,0xC0)&0x01)
	{
    	while(!RxHDMIMode)
		{
        	hdcprd(port, 0x41, 2);
        	RxHDMIMode = (h2txrd(port, 0x65)&0x10)>>4;
			if( (h2txrd(port, 0x03)&0x01)!=0x01 )
			{
        		return FAIL;
    		}
        	if( WaitCnt>150 )
			{
				Waitfail = 0;
            	iTE_MsgTX(("[HDCP]	ERROR: TXP%d: Time-Out break!!! \r\n", (iTE_u16)port));
           	 	break;
        	}
			 WaitCnt++;
			 mSleep(1);
    	}
	}
    if( EnHDCPATC )
	{
        hdcprd(port, 0x00, 5);
        h2txbrd(port, 0x5B, 5, &BKSV[0]);
        iTE_MsgTX(("[HDCP]	TXP%d: BKSV= %02X%02X%02X%02X%02X  \r\n", port, BKSV[4], BKSV[3], BKSV[2], BKSV[1], BKSV[0]));
    }
	if(TRUE == CheckBKsv(BKSV))
	{
		if(!Waitfail)
		{
			gext_u8->CP_Going |= (1<<port);
			h2txset(port, 0x91, 0x10, 0x00);
			h2txset(port, 0x42, 0x01, 0x01);
			gext_var->HDCPState[port] = Tx_CP_WaitInt;
		}
		else
		{
			retry++;
			if(retry>6)
			{
				retry = 0;
				h2txwr(port, 0x0D, 0x9E);//3s
            	h2txset(port, 0x19, 0x80, 0x80);
            	h2txset(port, 0x11, 0x80, 0x80);
				h2txset(port, 0x88, 0x03, 0x03);
				h2txset(port, 0x41, 0x01, 0x00);
				gext_var->HDCPState[port] = Tx_CP_WaitInt;
			}
		}
	}
	else
	{
		h2txset(port, 0x91, 0x10, 0x10);
		h2txset(port, 0xC1, 0x01, 0x01);//Fail BKSV set mute
		gext_var->HDCPState[port] = Tx_CP_ReAuth;
	}
    return ret;
}
void txrunhdcp2(iTE_u8 port)
{
 	iTE_u16 RxHDCP2, h2arbbusy, arbbusycnt;
	iTE_u8 BKSV[5];

	arbbusycnt = 0;
	gext_var->HDCPFireVer[port] = 2;
    if( (h2txrd(port, 0x03)&0x01)!=0x01 ) {
        return;
    }
    if(0)//( EnRepeater == FALSE )
	{
		h2txset(port, 0x41, 0x01, 0x01);
        hdcprd(port, 0x00, 5);
        h2txbrd(port, 0x5B, 5, &BKSV[0]);
        //iTE_MsgTX(("[HDCP]	TXP%d: BKSV= %02X%02X%02X%02X%02X  \r\n", port, BKSV[4], BKSV[3], BKSV[2], BKSV[1], BKSV[0]));
		if(FALSE == CheckBKsv(BKSV))
		{
			iTE_MsgTX(("[HDCP]	TX Check BKSV fail \r\n"));
			h2txset(port, 0xC1, 0x01, 0x01);//Fail BKSV set mute
			h2txset(port, 0x91, 0x10, 0x10);//Fail BKSV set black screen
		}
		else
			iTE_MsgTX(("[HDCP]	TX Check BKSV ok \r\n"));
    }
    h2txset(port, 0x42, 0x10, 0x10);
    h2txset(port, 0x18, 0x04, 0x04);
    h2txset(port, 0x1A, 0x08, 0x08);
    h2txset(port, 0x41, 0x01, 0x00);
    h2txset(port, 0x11, 0x01, 0x01);
    h2txset(port, 0x48, 0x0F, LClimit);
    h2txset(port, 0x49, 0xFF, 0x00);
    h2txset(port, 0x4A, 0xFF, 0x26);
    h2txset(port, 0x6B, 0x31, (EnRepWaitTxMgm<<5)+(EnRepWaitTxEks<<4)+DisR0ShortRead);
	h2txset(port, 0x6F, 0x01, EnAutoReAuth);
	h2txset(port, 0x42, 0x60, (EnHDCP2ListSRMChk<<6)+(EnHDCP2RxIDSRMChk<<5));
    h2txset(port, 0x42, 0x08, 0x08);
    h2txset(port, 0x6F, 0x08, (EnHDCPAutoMute<<3));
    h2txset(port, 0x19, 0x07, 0x07);
    h2txset(port, 0x1B, 0xC0, 0xC0);
	do {
		h2arbbusy = h2txcomrd(0x28)&0x07;
		arbbusycnt++;
		mSleep(1);
		if(arbbusycnt > 300)
		{
			break;
		}
	} while(h2arbbusy);
    h2txset(port, 0x41, 0x01, 0x01);
    hdcprd(port, 0x50, 1);
    RxHDCP2 = h2txrd(port, 0x4B)&0x01;

    if( RxHDCP2 )
	{
		if(gmem_tx->HDCPFireCnt[port]<0xFE) gmem_tx->HDCPFireCnt[port]++;
        iTE_MsgTX(("[HDCP]	TXP%d HDCP2 authentication fire %d  \r\n", (iTE_u16)port, gmem_tx->HDCPFireCnt[port]));
		mSleep(150);
        FireHDCP2(port);
		gext_var->HDCPState[port] = Tx_CP_WaitInt;
    }
	else
	{
		#ifdef Support_HDCP_DownVersion
        	iTE_MsgTX(("[HDCP]	RX HDCP Version Not Match !! \r\n"));
			if(((gext_long->TXVCLK[port] > 330000) || (gext_u8->InColorMode == YCbCr420))
			&&(gext_var->TxSrcSel[port] != ds))
			{
        		if(gext_u8->ForceCPDwnVer&(1<<port))
        		{
        			txrunhdcp(port);
        		}
				else
				{
					iTE_MsgTX(("[HDCP]	re-output  !! \r\n"));
					gext_u8->ForceCPDwnVer |= (1<<port);
					gext_var->VideoState[port] = Tx_Video_Stable;
					gext_var->HDCPState[port] = Tx_CP_WaitInt;
					h2txset(port, 0x88, 0x03, 0x03);
        		}
			}
			else
			{
					if(gext_u8->GetSteamType)
					{
						iTE_MsgTX(("Get steam type  !! \r\n"));
						if(gext_u8->ContentType == 1)
						{
							gext_var->VideoState[port] = Tx_Video_waitInt;
							h2txset(port, 0x88, 0x03, 0x03);
							iTE_MsgTX(("Stop output to 1.4 sink \r\n"));
						}
						else
						{
							txrunhdcp(port);
							iTE_MsgTX(("output to 1.4 sink  !! \r\n"));
						}
					}
					else
					{
						txrunhdcp(port);
					}
        	}
    #else
        	gext_var->VideoState[port] = Tx_Video_Stable_off;
		#endif
		gext_u8->CP_Going &= ~(1<<port);
	}
}
void FireHDCP2(iTE_u8 port)
{
   h2txset(port, 0x01, 0x20, 0x20);  // RegSoftH*TXRst=1
   h2txset(port, 0x01, 0x20, 0x00);
   //if(gmem_tx->HDCPFireCnt[port] >= 0xFE) gmem_tx->HDCPFireCnt[port] = 0;
   gext_u8->TXH2RSABusy |= (1<<port);
   h2txwr(port, 0x11, 0xFF);
   h2txset(port, 0x42, 0x01, 0x01);        // HDCP2 authentication fire
   gext_u8->CP_Going |= (1<<port);//TRUE
}
void setrx_ksv_list(iTE_u8 devcnt, iTE_u8 depth, iTE_u8 err_devcnt, iTE_u8 err_depth)
{
	iTE_u8 enhdcp2;
	iTE_u16  rxinfo, rxbstatus;


    //////////////////////////////////////////////////////////////////////////////////////////
    // The following HDCP Repeater function is only for 1-to-1 demo only
    //////////////////////////////////////////////////////////////////////////////////////////
    if( EnRxHP2Rpt||EnRxHP1Rpt ) {
        //rxport = TxPortSel[port];
		chgspbank(1);
        enhdcp2 = (h2sprd(0x23)&0x04)>>2;
		chgspbank(0);
        // temp for old database, RX do not implement RegD6[6]
        //if( h2rxrd(rxport, 0xD0)&0xF0 )
        //    enhdcp2 = FALSE;
        //else
        //    enhdcp2 = TRUE;

        if(enhdcp2) { // HDCP2.2 Repeater Mode
        iTE_MsgTX(("HDCP2.2 Repeater Mode \r\n"));
          //  if( RxH2EngSel[0]==rxport )
          //     h2engsel = 0;
          //  else
          //      h2engsel = 1;
			//170104
            if( devcnt>=32 || err_devcnt)
                err_devcnt = TRUE;  // HDCP2.2 support 32-stage KSV List only
            //else
                //devcnt += 1;
			//170104
            if( depth>=4 )
                err_depth = TRUE;   // HDCP2.2 support 4-levels only
            else
                depth += 1;

            rxinfo = (gext_u8->HP2_HDCP20DownStm<<1) + gext_u8->HP2_HDCP1DownStm; // HDCP1.x compliant device in the topology
            rxinfo += (err_depth<<2);
            rxinfo += (err_devcnt<<3);
            rxinfo += (devcnt<<4);
            rxinfo += (depth<<9);

            chgspbank(1);
            h2spwr(0x18, rxinfo&0xFF);
            h2spwr(0x19, (rxinfo&0xFF00)>>8);
			h2spset(0x11, 0x01, 0x01); //trigger KSV list ready to HDCP2 RX engine
			chgspbank(0);
/*
            int rddata;

            do {
                idle(100000);
                rddata = h2swrd(0x21+0x20*h2engsel);
                iTE_Msg(("RXP%d Repeater Mode Waiting for HDCP2.2 Eng%d SKE_Send... \r\n", rxport, h2engsel));
            } while( (rddata&0x20)==0x00 );

            h2swwr(0x11+0x20*h2engsel, 0x01);   // RH*KSVList_Ready W1P
*/

            //rxksvrdy[rxport] = TRUE;
            //chgswbank(0);
        }
		else if (EnRxHP1Rpt)
		{    // HDCP1.4 Repeater Mode
		 	iTE_MsgTX(("HDCP1.4 Repeater Mode \r\n"));
            if( devcnt>=7 )
                err_devcnt = TRUE;  //Max ksv  = 6
            //else
               // devcnt += 1;

            if( depth>=7 )
                err_depth = TRUE;   // HDCP1.4 support 7-levels only
            else
                depth += 1;

            rxbstatus = devcnt;
            rxbstatus += (err_devcnt<<7);
            rxbstatus += (depth<<8);
            rxbstatus += (err_depth<<11);
			if(err_devcnt)
			{
				rxbstatus |= 0x0080;
			}
			if(err_depth)
			{
				rxbstatus |= 0x0800;
			}
            chgrxbank(1);
            h2rxwr(0x10, rxbstatus&0xFF);
            h2rxwr(0x11, (rxbstatus&0xFF00)>>8);
			chgrxbank(0);
			iTE_MsgTX(("Output Rpt Bstatus=%04X  \r\n", rxbstatus));

			cal_rxh1_sha(devcnt);
            h2rxset(0xCE, 0x40, 0x40);  // set KSV ready = 1

        }
    }
}
void cal_rxh1_sha( iTE_u8 DevCnt )
{
   iTE_u8 i, rddata,  ksvlist[160], temp[4];

    //h2spset(0x09, 0x01<<RxH2SHA1Sel, 0x00);     // RegSoftH*RXRst=0
    //chgspbank(1);

    //h2spset(0x12, 0x0C, port<<2);   // RegKsvFifoSel[1:0]

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // read back ksvlist start for debug only
    //chgrxbank(1);
    //DevCnt = h2rxrd(0x10)&0x7F;
    //chgrxbank(0);

    h2txcomset(0x20, 0x04, 0x04);  // RegEnFwKsvFifo=1
    chgspbank(1);
	for(i=0; i<DevCnt; i++) {
        h2spwr(0x2D, 0x80+i);        // KSV FIFO starts from stage CurKsvFifoWrStg
        h2spbrd(0x28, 5, &ksvlist[i*5]);
    }

    //for(i=0; i<DevCnt; i++)
        //iTE_MsgTX(("RX KSV List %d = 0x %02X %02X %02X %02X %02X\r\n", i, ksvlist[i*5+4], ksvlist[i*5+3], ksvlist[i*5+2], ksvlist[i*5+1], ksvlist[i*5+0]));

    //iTE_MsgTX(("\r\n"));
    h2txcomset(0x20, 0x04, 0x00);  // RegEnFwKsvFifo=0
    // read back ksvlist end for debug only
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    h2spset(0x1F, 0x70, 0x40);                  // RegSHA1Sel[2:0]=RX port
    //h2swset(0x11, 0xC0, 0x40<<RxH2SHA1Sel);     // RegEnH*SHA1
    h2spwr(0x1E, 0x20);                         // RegSHA1RdAdr (RX HDCP1.4 SHA calculation must start from stg0)
    h2spset(0x1F, 0x80, 0x80);     // RH*SHA1Trg
	h2rxset(0xCE, 0x40, 0x40);
    do {
        //idle(100000);
		mSleep(1);
        rddata = h2sprd(0x81);
        //iTE_MsgTX(("HW SHA1 Engine Busy Status = %d\r\n", (rddata&0x80)>>7));
    } while( (rddata&0x80)!=0x00 );

    chgrxbank(1);
    for(i=0; i<5; i++) {
        h2spbrd(0x52+i*4, 4, &temp[0]);            // Read back HW SHA1 result
        h2rxwr(0x14+4*i, temp[0]);
        h2rxwr(0x15+4*i, temp[1]);
        h2rxwr(0x16+4*i, temp[2]);
        h2rxwr(0x17+4*i, temp[3]);
        //iTE_MsgTX(("RX BV.H%d = 0x %02X %02X %02X %02X \r\n", i, temp[3], temp[2], temp[1], temp[0]));
    }
    chgrxbank(0);

    h2spset(0x1E, 0x20, 0x00);  // RegEnH*SHA1=0
    chgspbank(0);
}

iTE_u8 ddcwait(iTE_u8 port)
{
	iTE_u8 ddcwaitcnt, ddc_status,i,timeout;

    ddcwaitcnt = 0;
    do
	{
        ddcwaitcnt++;
		mSleep(DDCWAITTIME);
    } while( (h2txrd(port, 0x2F)&0x80)==0x00 && ddcwaitcnt<DDCWAITNUM   );

    if( ddcwaitcnt==DDCWAITNUM ) {
        ddc_status = h2txrd(port, 0x2F)&0xFE;
        iTE_MsgTX(("[DDC]	TXP%d DDC Bus Status = %02X \r\n",(iTE_u16) port,(iTE_u16) ddc_status));
        iTE_MsgTX(("[DDC]	ERROR: TXP%d DDC Bus Wait TimeOut => ", (iTE_u16)port));

        if( h2txrd(port, 0x12)&0x08 ) {
            iTE_MsgTX(("[DDC]	DDC Bus Hang !!! \r\n"));
        }
        else if( ddc_status&0x20 )
        {
        	#ifdef RXHDCP_Follow_SinkX
				if(port == HDCP_CopyPx)
				{
        			gext_u8->DDC_NAK = 1;
				}
			#endif
            iTE_MsgTX(("[DDC]	DDC NoACK !!! \r\n"));
        }
        else if( ddc_status&0x10 )
            iTE_MsgTX(("[DDC]	DDC WaitBus !!! \r\n"));
        else if( ddc_status&0x08 )
            iTE_MsgTX(("[DDC]	DDC ArbiLose !!! \r\n"));
        else
            iTE_MsgTX(("[DDC]	UnKnown Issue !!! \r\n"));


		h2txset(port, 0x35, 0x10, 0x10);
		h2txset(port, 0x35, 0x10, 0x00);
		h2txset(port, 0x28, 0x01, 0x01);
		h2txset(port, 0x28, 0x01, 0x00);
		h2txwr(port, 0x2E, 0x0A);			   // Generate SCL Clock
		for(i=0;i<2;i++)
		{
			if(i)	{h2txwr(port, 0x2E,0x0F);}// ddc abort after Generate SCL Clock
			for(timeout=0;timeout<200;timeout++)
			{
				ddc_status = h2txrd(port, 0x2F)&0xFE;
				if(ddc_status&0x80)
				{
					break ; // success
				}
				if( ddc_status & (0x20|0x10|0x08))//B_TX_DDC_NOACK|B_TX_DDC_WAITBUS|B_TX_DDC_ARBILOSE
            	{
					// HDMITX_DEBUG_PRINTF(("hdmitx_AbortDDC Fail by reg16=%02X\n",(int)uc));
                	break ;
            	}
				mSleep(1);
			}

		}
    	return FALSE;  // will try again
    }
    else
    {
    	#ifdef RXHDCP_Follow_SinkX
			if(port == HDCP_CopyPx)
			{
    			gext_u8->DDC_NAK= 0;
			}
		#endif
        return TRUE;
    }
}
void it6664_txvar_init(void)
{
	iTE_u8 *ptr;
	iTE_u8 i;

	ptr = &gmem_tx->HDCPFireCnt[0];
	for(i = 0;i<sizeof(it6664_tx);i++)
	{
		*ptr++ = 0;
	}
}

#if 1
void SetTXSource(iTE_u8 port)
{
	iTE_u8 HwEnCV2DS,vic,i;
	iTE_u8 HDMI2;
	iTE_u16 HActive;

	HDMI2 = (h2rxrd(0x14)&0x40);
	HActive  = ((h2rxrd(0x9E)&0x3F)<<8) + h2rxrd(0x9D);
	chgrxbank(2);
	gext_u8->InColorMode = (h2rxrd(0x15)&0x60)>>5;
	chgrxbank(0);

	#ifdef IT6664
	if((gext_var->VideoState[1]!=Tx_Video_OK) && (gext_var->VideoState[2]!=Tx_Video_OK)
		&& (gext_var->VideoState[0]!=Tx_Video_OK) && (gext_var->VideoState[3]!=Tx_Video_OK))
	#else
	if((gext_var->VideoState[1]!=Tx_Video_OK) && (gext_var->VideoState[2]!=Tx_Video_OK))
	#endif
	{
			if(h2sprd(0x0D) == 0x00)
			{
				// for PS4 pro
				h2spset(0x0B,0xFC,0xFC);
				h2spset(0x0B,0xFC,0x00);
				h2spset(0x4E,0x0F,0x0C);
				mSleep(10);
				h2spset(0x4E,0x0F,0x00);
				get_vid_info();
			}
	}
	#if (USING_1to8 == TRUE)
	if(g_device == IT6663_C) //first stage set original out
	{

		if(gext_u8->InColorMode == YCbCr422)
		{
			h2txset(port, 0xC0, 0x01, 0x01);
			h2spset(0x0D,0x01<<(port*2),0x01<<(port*2));//set csc
			gext_var->TxSrcSel[port] = csc;
		}
		else
		{
			h2txset(port, 0xC0, 0x01, 0x01);
			h2spset(0x0D,0x03<<(port*2),0<<(port*2));
			 gext_var->TxSrcSel[port] = ori;
		}
		if(h2rxrd(0x13)&0x02)//input HDMI mode
		{
				iTE_MsgTX(("[TX] P%d HDMI out \r\n", (iTE_u16)port));
				h2txset(port, 0xC0, 0x01, 0x01);
		}
		else//input DVI mode
		{
			iTE_MsgTX(("[TX] P%d DVI out \r\n", (iTE_u16)port));
			h2txset(port, 0xC0, 0x01, 0x00);
		}
	}
	else
	#endif
	{
	//Set DVI or HDMI
	if(h2rxrd(0x13)&0x02)//input HDMI mode
	{
		if(gext_var->DVI_mode[port] == 1)
		{
			iTE_MsgTX(("[TX]	TXP%d set DVI out \r\n", (iTE_u16)port));
			h2txset(port, 0xC0, 0x01, 0x00);
		}
		else
		{
			iTE_MsgTX(("[TX]	TXP%d set HDMI out \r\n", (iTE_u16)port));
			h2txset(port, 0xC0, 0x01, 0x01);
		}
	}
	else//input DVI mode
	{
		iTE_MsgTX(("[TX]	P%d DVI out \r\n", (iTE_u16)port));
		h2txset(port, 0xC0, 0x01, 0x00);
	}

	//Set TX vclk source
	if(gext_u8->TXHPDsts&(1<<port))
	{
		if(gext_u8->InColorMode==YCbCr420)//420
		{
			if((gext_var->TXSupport420[port] && (gext_var->TXSupport4K60[port]&0x01))
				|| (gext_var->TXSupportOnly420[port] && (gext_var->TXSupport4K60[port]&0x01)))
			{
				gext_var->TxSrcSel[port] = ori;
			}
			else if(gext_var->TXSupportOnly420[port])
			{
				if((gext_u8->GCP_CD&0x03))
				{
					gext_var->TxSrcSel[port] = ds;
					gext_u8->ForceTXHDMI2 &= ~(1<<port);
				}
				else
				{
					gext_var->TxSrcSel[port] = ori;
				}
			}
			else if((gext_var->TXSupport4K60[port]&0x01))
			{
				if(gext_var->TXSupport4K60[port]&0x10)//with 4k60 resolution
				{
					gext_var->TxSrcSel[port] = conv;
					gext_u8->ForceTXHDMI2 |= (1<<port);
				}
				else //without 4k60 resolution but support 6G
				{
					gext_var->TxSrcSel[port] = ds;
					gext_u8->ForceTXHDMI2 &= ~(1<<port);
				}
			}
			else
			{
				gext_var->TxSrcSel[port] = ds;
				gext_u8->ForceTXHDMI2 &= ~(1<<port);
			}
		}
		else //others
		{
			if(HDMI2 != 0x40) //1.4  1/10
			{
				//iTE_MsgTX(("[TX]	TXP%d gext_u8->InColorMode = %02x \r\n",(iTE_u16)port, (iTE_u16)gext_u8->InColorMode));

				if(gext_var->DVI_mode[port])
				{
					if(gext_u8->InColorMode != RGB444) //YUV
					{
						gext_var->TxSrcSel[port] = csc;
					}
					else
					{
						if((gext_u8->GCP_CD&0x03)&&((FixPort_Opt&(1<<port))==0x00))//RGB deep color mode
						{
							gext_u8->CSCOutColorMode = RGB444;
							h2spset(0x6B,0x42,0x00);//set csc bypass
							for(i=0;i<4;i++)
							{
								if(gext_var->TxSrcSel[i]==csc)
								{
									h2txset(i, 0xAE, 0x07, gext_u8->CSCOutColorMode); //set out Colormode
								}
							}
							gext_var->TxSrcSel[port] = csc;
						}
						else// RGB no deep color
						{
							if(gext_u8->Rx_4K30)
							{
								if(gext_var->TXSupport4K30[port])
								{
									gext_var->TxSrcSel[port] = ori;
								}
								else
								{
									iTE_MsgTX(("[TX]	P%d Set 4k30 ds 1\r\n",(iTE_u16)port));
									gext_var->TxSrcSel[port] = ds;
								}
							}
							else
							{
								gext_var->TxSrcSel[port] = ori;
							}
						}
					}
				}
				else  // 3G HDMI mode
				{
					if(((HActive == 3840) || (HActive == 4096)) && (gext_u8->InColorMode != YCbCr422) && (gext_long->TXVCLK[port] > 270000))
					{
						if(gext_var->TXSupport4K30[port])
						{
							gext_var->TxSrcSel[port] = ori;
						}
						else
						{
							iTE_MsgTX(("[TX]	P%d Set 4k30 ds \r\n",(iTE_u16)port));
							gext_var->TxSrcSel[port] = ds;
							gext_u8->ForceTXHDMI2 &= ~(1<<port);
						}
					}
					else
					{
						if(gext_u8->Rx_4K30)
							{
								if(gext_var->TXSupport4K30[port])
								{
									gext_var->TxSrcSel[port] = ori;
								}
								else
								{
									iTE_MsgTX(("[TX]	P%d Set 4k30 ds \r\n",(iTE_u16)port));
									gext_var->TxSrcSel[port] = ds;
								}
							}
							else
							{
								gext_var->TxSrcSel[port] = ori;
							}
					}
					if((!(gext_var->TXSupport1080p[port]&0x30)) && (gext_u8->GCP_CD&0x03) && ((FixPort_Opt&(1<<port))==0x00))
					{//not support 3G 10/12 b && input 3G deep color
						if(gext_u8->InColorMode != RGB444)
						{
							gext_var->TxSrcSel[port] = csc;//csc to RGB and 8b
						}
						else //input RGB
						{
							gext_u8->CSCOutColorMode = RGB444;
							h2spset(0x6B,0x42,0x00);//set csc bypass
							for(i=0;i<4;i++)
							{
								if(gext_var->TxSrcSel[i]==csc)
								{
									h2txset(i, 0xAE, 0x07, gext_u8->CSCOutColorMode); //set out Colormode
								}
							}
							gext_var->TxSrcSel[port] = csc;
						}
					}
				}
			}
			else //2.0	1/40
			{
				if(gext_u8->InColorMode == RGB444)
				{
					if(gext_var->TXSupport420[port] && (gext_var->TXSupport4K60[port]&0x01))
					{
						gext_var->TxSrcSel[port] = ori;
						gext_u8->ForceTXHDMI2 |= (1<<port);
					}
					else if(gext_var->TXSupportOnly420[port])
					{
						gext_var->TxSrcSel[port] = conv;
						gext_u8->ForceTXHDMI2 &= ~(1<<port);
					}
					else if((gext_var->TXSupport4K60[port]&0x01))
					{
						if((HActive == 3840) || (HActive == 4096))//4k 60 RGB
						{
							if(gext_var->TXSupport4K60[port]&0x10)
							{
								gext_var->TxSrcSel[port] = ori;
								gext_u8->ForceTXHDMI2 |= (1<<port);
							}
							else
							{
								gext_var->TxSrcSel[port] = ds;
								gext_u8->ForceTXHDMI2 &= ~(1<<port);
							}
						}
						else //other high frame rate 6G signal
						{
							gext_var->TxSrcSel[port] = ori;
							gext_u8->ForceTXHDMI2 |= (1<<port);
						}
					}
					else //not support 4k60
					{
						gext_var->TxSrcSel[port] = ds;
						gext_u8->ForceTXHDMI2 &= ~(1<<port);
					}
				}
				else // YUV444	YUV422
				{
					if(gext_var->TXSupport420[port] && (gext_var->TXSupport4K60[port]&0x01))
					{
						gext_var->TxSrcSel[port] = ori;
						gext_u8->ForceTXHDMI2 |= (1<<port);
					}
					else //not support 4k60422 4k60 yuv444
					{
						if(gext_u8->InColorMode == YCbCr444)
						{
							if(gext_var->TXSupportOnly420[port])// || gext_var->TXSupport420[port])
							{
								gext_var->TxSrcSel[port] = conv;
								gext_u8->ForceTXHDMI2 &= ~(1<<port);
							}
							else if((gext_var->TXSupport4K60[port]&0x01))
							{
								if((HActive == 3840) || (HActive == 4096))//4k 60 RGB
								{
									if(gext_var->TXSupport4K60[port]&0x10)
									{
										gext_var->TxSrcSel[port] = ori;
										gext_u8->ForceTXHDMI2 |= (1<<port);
									}
									else
									{
										gext_var->TxSrcSel[port] = ds;
										gext_u8->ForceTXHDMI2 &= ~(1<<port);
									}
								}
								else //other high frame rate 6G signal
								{
									gext_var->TxSrcSel[port] = ori;
									gext_u8->ForceTXHDMI2 |= (1<<port);
								}
							}
							else //not support 4k60
							{
								gext_var->TxSrcSel[port] = ds;
								gext_u8->ForceTXHDMI2 &= ~(1<<port);
							}
						}
						else// YUV422
						{
							gext_var->TxSrcSel[port] = ori;//not support 422ds
						}
					}
				}
				if((gext_u8->Rx_4K30) && (gext_u8->GCP_CD&0x03) && ((FixPort_Opt&(1<<port))==0x00)&& (!(gext_var->TXSupport1080p[port]&0x30)))
				{// input 4k30 10/12 b  && sink not support 3G deep color
					if(gext_var->TXSupport4K30[port])
					{
					//YUV 444 10b/12b in , RGB 444 8b out
					//RGB 444 10b/12b in , RGB 444 8b out
						gext_var->TxSrcSel[port] = csc;//use for 10/12bit ->8 b

						if(gext_u8->InColorMode == RGB444) //
						{
							gext_u8->CSCOutColorMode = RGB444;
							h2spset(0x6B,0x42,0x00);//set bypass
							iTE_MsgTX(("[TX]	P%d RGB444 bypass 8b \r\n",(iTE_u16)port));
							for(i=0;i<4;i++)
							{
								if(gext_var->TxSrcSel[i]==csc)
								{
									h2txset(i, 0xAE, 0x07, gext_u8->CSCOutColorMode); //set out Colormode
								}
							}
						}
					}
					else
					{
						gext_var->TxSrcSel[port] = ds;
						gext_u8->ForceTXHDMI2 &= ~(1<<port);
					}
				}
			}
		}
	}

	#if(FixPort_Opt != FALSE)
		if((FixPort_Opt&(1<<port)) && ((HActive == 3840) || (HActive == 4096)))
		{
			if(gext_u8->InColorMode != YCbCr422)
			{
				gext_var->TxSrcSel[port] = ds;
				gext_u8->ForceTXHDMI2 &= ~(1<<port);
			}
			iTE_MsgTX(("[TX]	P%d Fix down scale \r\n",(iTE_u16)port));
		}
	#endif

	#ifdef Support_HDCP_DownVersion
		if(gext_u8->ForceCPDwnVer & (1<<port))
		{
			if((gext_u8->InColorMode != YCbCr422) && ((HActive == 3840) || (HActive == 4096)))
			{
				gext_var->TxSrcSel[port] = ds;
			}
			gext_u8->ForceTXHDMI2 &= ~(1<<port);
			iTE_MsgTX(("[TX]	P%d Support_HDCP_DownVersion \r\n",(iTE_u16)port));
		}
	#endif

	#if(PortBypass_Opt != FALSE)
	{
		if(PortBypass_Opt&(1<<port))
		{
			gext_var->TxSrcSel[port] = ori;
		}
		if(HDMI2 == 0x40)
		{
			if((SkipSCDC_Opt & (1<<port)) == 0x00)
			{
				gext_u8->ForceTXHDMI2 |= (1<<port);
				gext_var->TXSupport4K60[port]= 1;
			}
			h2txset(port, 0x83, 0x08, 0x08); // HDMI2ONPLL
    		h2txset(port, 0xC0, 0x46, 0x46);
			iTE_MsgTX(("[TX]	P%d Force Bypass set 1/40 \r\n",(iTE_u16)port));
		}
		else
		{
			if((SkipSCDC_Opt & (1<<port)) == 0x00)
			{
				gext_u8->ForceTXHDMI2 &= ~(1<<port);
				gext_var->TXSupport4K60[port]= 1;
			}
			h2txset(port, 0x83, 0x08, 0x00); // HDMI2ONPLL
    		h2txset(port, 0xC0, 0x46, 0x00);
			iTE_MsgTX(("[TX]	P%d Force Bypass set 1/10 \r\n",(iTE_u16)port));
		}
	}
	#endif

	#ifdef DS_Switch
	if(gext_var->TxOut_DS[port] == TRUE)
	{
		if((gext_u8->InColorMode != YCbCr422) && ((HActive == 3840) || (HActive == 4096)))
		{
			gext_var->TxSrcSel[port] = ds;
			gext_u8->ForceTXHDMI2 &= ~(1<<port);
			iTE_MsgTX(("[TX]	P%d Force Ds on \r\n",(iTE_u16)port));
		}
	}
	#endif

	SetDeepColor(port,gext_var->TxSrcSel[port]);


	if((gext_u8->GCP_CD&0x03) != VID8BIT)//for allion test fail
	{
		h2txset(port, 0xC1, 0x04, 0x04);
	}
	else
	{
		h2txset(port, 0xC1, 0x04, 0x00);
	}
	if((gext_u8->InColorMode==YCbCr420) && (gext_u8->GCP_CD&0x03) && (gext_var->TxSrcSel[port] == conv))
	{
		// for 420 10/12 b conv to  4k444 8b
		h2spset(0x67,(0x01<<gext_var->TxSrcSel[port]),(0x01<<gext_var->TxSrcSel[port]));
	}
	if(gext_u8->HighFrameRate) //ite_171201
	{
		gext_var->TxSrcSel[port] = ori;

		if(gext_long->TXVCLK[port] > 300000)
		{
			gext_u8->ForceTXHDMI2 |= (1<<port);
		}
		else
		{
			gext_u8->ForceTXHDMI2 &= ~(1<<port);
		}
	}
	h2spset(0x0D,0x03<<(port*2),gext_var->TxSrcSel[port]<<(port*2));
	}
	//Set TX InfoFrame
	switch( gext_var->TxSrcSel[port] )
	{
		case 0 :
				h2txset(port, 0xAD, 0x81, 0x00); //set out Colormode
				iTE_MsgTX(("[TX]	Tx P%d original out, out color = %d \r\n", (iTE_u16)port, (iTE_u16)gext_u8->InColorMode));
				h2txset(port, 0xA0, 0x30, 0x00);
				//hdmirxset(0x65, 0x30, 0x00);
				break;
		case 1 :
				h2txset(port, 0xAD, 0x85, 0x85); //set out Colormode
				h2txset(port, 0xAE, 0x07, gext_u8->CSCOutColorMode); //set out Colormode
				h2txset(port, 0xAE, 0xC0, (gext_u8->CSCOutQ<<6)); //set out Colormode
				iTE_MsgTX(("[TX]	Tx P%d CSC out, out color = %d \r\n",(iTE_u16) port, (iTE_u16)gext_u8->CSCOutColorMode));
				h2txset(port, 0xA0, 0x30, 0x00);
				//hdmirxset(0x65, 0x30, 0x10);
				break;
		case 2 :
				h2txset(port, 0xAD, 0x81, 0x81); //set out Colormode
				h2txset(port, 0xAE, 0x07, gext_u8->CVOutColorMode); //set out Colormode
				if (gext_u8->InColorMode!=YCbCr420)
				{
					h2txset(port, 0xA0, 0x30, 0x20);//cts div 2 for 444 to 420
				}
				else
				{
					h2txset(port, 0xA0, 0x30, 0x10);//cts mul 2 for 420 to 444
					h2txset(port, 0xAF, 0xC0, gext_u8->GCP_CD<<6);
				}
				iTE_MsgTX(("[TX]	Tx P%d Chroma conv out, out color = %d \r\n", (iTE_u16)port, (iTE_u16)gext_u8->CVOutColorMode));
				//hdmirxset(0x65, 0x30, 0x20);
				break;
		case 3 :
				HwEnCV2DS = (gext_u8->InColorMode == YCbCr420) ? TRUE : FALSE;
				vic = SelDsVic();
				if (HwEnCV2DS == 0)
				{
					h2txset(port, 0xA0, 0x30, 0x30);//cts div 4
					h2txset(port, 0xAD, 0xA1, 0xA0);
					h2txset(port, 0xB0, 0xFF, vic);
					iTE_MsgTX(("[TX]	ENCV2DS = %d, Tx P%d Down scale out, out color = %d \r\n", (iTE_u16)HwEnCV2DS, (iTE_u16)port, (iTE_u16)gext_u8->InColorMode));
				}
				else
				{
					h2txset(port, 0xAD, 0xA1, 0xA1); //set out Colormode+vic
					h2txset(port, 0xAE, 0x07, gext_u8->CVOutColorMode); //set out Colormode
					h2txset(port, 0xB0, 0xFF, vic);
					h2txset(port, 0xA0, 0x30, 0x20);//cts div 2
					iTE_MsgTX(("[TX]	Tx P%d 420 to 444 and Down scale out, out color = %d \r\n", (iTE_u16)port, (iTE_u16)gext_u8->CVOutColorMode));
				}
				//h2txset(port,0xA3,0x80,0x80);//ite_180605  disable HDR packet for ds
				//for bt2020
				chgrxbank(2);
				if((h2rxrd(0x17)&0x60) == 0x60)
				{
					h2txset(port, 0xAD, 0x02, 0x02); //set out C
					if(HwEnCV2DS)//420
					{
						h2txset(port, 0xAE, 0x32, 0x02);
					}
					else //RGB
					{
						h2txset(port, 0xAE, 0x30, 0x00);
					}
				}
				chgrxbank(0);
				break;
		default :
				break;
	}
}

#endif


iTE_u8 SelDsVic(void)
{
	iTE_u8 ret=16,i,vic;
				   //		  60Hz    50Hz     24Hz    25Hz      30Hz
	iTE_u8 table[10][2] = {
							{16,97},{31,96},{32,93},{33,94},{34,95},
							{16,102},{31,101},{32,98},{33,99},{34,100},
				   		  };

	chgrxbank(2);
	vic = h2rxrd(0x18);
	chgrxbank(0);
	//iTE_MsgTX(("IN VIC is %d \r\n",(iTE_u16)vic));
	for(i=0;i<10;i++)
	{
		if(vic == table[i][1])
		{
			ret = table[i][0];
			break;
		}
	}
	//iTE_MsgTX(("OUT VIC is %d \r\n",(iTE_u16)ret));
	return ret;
}
void SetDeepColor(iTE_u8 port,iTE_u8 src)
{
	iTE_u8 Force8b=0,i;

	for(i=0;i<4;i++)
	{
		if((gext_u8->TXHPDsts & (1<<i)) && ((gext_var->TXSupport1080p[i]&0x30)==0x00))
		{
			Force8b = 1;
			break;
		}
	}
	//Set TX Deep color
	if(gext_var->TxSrcSel[port] == ds)
	{
		if(Force8b)
		{
			iTE_MsgTX(("[TX]	Force 8b out \r\n"));
			if ((gext_u8->GCP_CD&0x03) == VID12BIT)
			{
		 		iTE_MsgTX(("[TX]	12 b to 8 b \r\n"));
				h2spset(0x67,0x08,0x08);//force ds 8bit output
           		for(i=0;i<4;i++)
           		{
           			if(gext_var->TxSrcSel[i] == ds)
           			{
						h2txset(i, 0xAF, 0xC0, 0x80); //set output audio div if 12/10 to 8 bit is performed
           			}
           		}
			}
			else if ((gext_u8->GCP_CD&0x03) == VID10BIT)
			{
				h2spset(0x67,0x08,0x08);//force ds 8bit output
		 		iTE_MsgTX(("[TX]	10 b to 8 b \r\n"));
				for(i=0;i<4;i++)
           		{
           			if(gext_var->TxSrcSel[i] == ds)
           			{
						h2txset(i, 0xAF, 0xC0, 0x40); //set output audio div if 12/10 to 8 bit is performed
           			}
           		}
			}
			else
			{
		 		iTE_MsgTX(("[TX]	8 b to 8 b \r\n"));
           		h2txset(port, 0xAF, 0xC0, 0x00);
			}
		}
		else
		{
			iTE_MsgTX(("[TX]	Ori CD out \r\n"));
			h2spset(0x67,0x08,0x00);//disable force ds 8bit output
			for(i=0;i<4;i++)
			{
					if(gext_var->TxSrcSel[i] == ds)
           			{
						h2txset(i, 0xAF, 0xC0, 0x00);// no change
					}
			}
		}
		gext_u8->ForceTXHDMI2 &= ~(1<<port);
	}
	else //for 7-19  ite_170831
	{
		if ((gext_u8->GCP_CD&0x03) == VID8BIT) h2txset(port, 0xC1, 0xF0, 0x00);

		if(gext_var->TxSrcSel[port] == csc)// force csc out 8b
		{
			if((gext_u8->GCP_CD&0x03) == VID12BIT)
			{
				h2spset(0x67,0x02,0x02);//force ds 8bit output
           		h2txset(port, 0xAF, 0xC0, 0x80); //set output audio div if 12/10 to 8 bit is performed
			}
			else if((gext_u8->GCP_CD&0x03) == VID10BIT)
			{
				h2spset(0x67,0x02,0x02);//force ds 8bit output
				h2txset(port, 0xAF, 0xC0, 0x40);
			}
			else
			{
           		h2txset(port, 0xAF, 0xC0, 0x00);
			}
			iTE_MsgTX(("[TX]	Froce CSC out 8b \r\n"));
			gext_u8->ForceTXHDMI2 &= ~(1<<port);
			h2txset(port, 0xC1, 0xF0, 0x00); //not follow input color depth
		}
	}


}
iTE_u8 CheckBKsv(iTE_u8 ksv[5])
{
	iTE_u8 i,j,cnt,ret;
	cnt = 0;
	for(i=0;i<5;i++)
	{
		for(j=0;j<8;j++)
		{
			if(ksv[i]&1<<j)
			{
				cnt++;
			}
		}
	}

	if(cnt == 20) ret=1;
	else ret=0;

	return ret;
}
void Show_status(void)
{
	iTE_u8 i,vic;

	iTE_MsgTX(("=========>  Show_status	 ===========>\r\n"));
	chgrxbank(2);
	vic = h2rxrd(0x18);
	chgrxbank(0);
	iTE_MsgTX(("Input VIC is %d \r\n",(iTE_u16)vic));
	#ifdef IT6664
		for(i=0;i<TxPortNum;i++)
	#else
		for(i=1;i<TxPortNum+1;i++)
	#endif
		{
			if(gext_var->TxSrcSel[i] == 0) iTE_MsgTX(("TXP%d source select Original	\r\n",(iTE_u16)i));
			if(gext_var->TxSrcSel[i] == 1) iTE_MsgTX(("TXP%d source select CSC  \r\n",(iTE_u16)i));
			if(gext_var->TxSrcSel[i] == 2) iTE_MsgTX(("TXP%d source select Convert  \r\n",(iTE_u16)i));
			if(gext_var->TxSrcSel[i] == 3) iTE_MsgTX(("TXP%d source select Down scale  \r\n",(iTE_u16)i));

		}
	//get_vid_info();
	switch(gext_u8->InColorMode)
	{
		case RGB444:
			iTE_MsgTX(("Input color mode is RGB444 \r\n"));
			break;
		case YCbCr444:
			iTE_MsgTX(("Input color mode is YCbCr444 \r\n"));
			break;
		case YCbCr422:
			iTE_MsgTX(("Input color mode is YCbCr422 \r\n"));
			break;
		case YCbCr420:
			iTE_MsgTX(("Input color mode is YCbCr420 \r\n"));
			break;
		default:
			break;
	}
	iTE_MsgTX(("==================================> \r\n"));

}
void TxPortAdoGatting(iTE_u8 port,iTE_u8 enable)
{
        iTE_u8 regA1 ;
        iTE_u8 regA2 ;
        iTE_u8 regA3 ;

        if( enable )
        {
                regA1 = 0x84 ;
                regA2 = 0xFF ;
                regA3 = 0x10 ; // disable audio output
                //printf("disable audio output \r\n");
        }
        else
        {
                regA1 = 0 ;
                regA2 = 0 ;
                regA3 = 0 ;
                //printf("eable audio output \r\n");
        }
        h2txset(port, 0xA1, 0x84, regA1);
        h2txset(port, 0xA2, 0xFF, regA2);
        h2txset(port, 0xA3, 0x10, regA3);
        //printf("A1 = 0x%02x  A2 = 0x%02x  A3 = 0x%02x  \r\n",h2txrd(port,0xA1),h2txrd(port,0xA2),h2txrd(port,0xA3));
}

#if 0

#if (USING_1to8 == FALSE)
void SetTxLED(iTE_u8 port,iTE_u8 sts)
{
	iTE_u8 GPIO_1,GPIO_2;
	#ifdef _MCU_IT6350
	GPIO_1 = GPDRF;
	GPIO_2 = GPDRA;
	#else //6295
	GPIO_1 = GPDRA;
	GPIO_2 = GPDRB;
	#endif

#if((Bond6664==TRUE) || (BondCH6002==TRUE))
	iTE_u8 LED_sts[4] ={0x40,0x80,0x01,0x02};

	if(sts)
	{
		if(port < 2)
		{
			GPIO_1 |=  LED_sts[port];
		}
		else
		{
			GPIO_2 |=  LED_sts[port];
		}
	}
	else
	{
		if(port < 2)
		{
			GPIO_1 &=  ~LED_sts[port];
		}
		else
		{
			GPIO_2 &=  ~LED_sts[port];
		}
	}
/*
	switch(port)
	{
		case 0:
				if(sts) GPDRF |= 0x40;
				else GPDRF &= ~0x40;
				break;
		case 1:
				if(sts) GPDRF |= 0x80;
				else GPDRF &= ~0x80;
				break;
		case 2:
				if(sts) GPDRA |= 0x01;
				else GPDRA &= ~0x01;
				break;
		case 3:
				if(sts) GPDRA |= 0x02;
				else GPDRA &= ~0x02;
				break;
		default:
				break;
	}
*/
#else
	iTE_u8 LED_sts[4] ={0x00,0x80,0x40,0x00};
	if(sts)
	{
		GPIO_1 |=  LED_sts[port];
	}
	else
	{
		GPIO_1 &=  ~LED_sts[port];
	}

/*
	switch(port)
	{
		case 1:
				if(sts) GPDRF |= 0x80;
				else GPDRF &= ~0x80;
				break;
		case 2:
				if(sts) GPDRF |= 0x40;
				else GPDRF &= ~0x40;
				break;
		default:
				break;
	}
	*/
#endif


	#ifdef _MCU_IT6350
	GPDRF = (GPDRF&0x3F)|GPIO_1;
	GPDRA = (GPDRA&0xFC)|GPIO_2;
	#else //6295
	GPDRA = (GPDRA&0x3F)|GPIO_1;
	GPDRB = (GPDRB&0xFC)|GPIO_2;
	#endif
}
#endif

#endif
void  Dump_TxReg(iTE_u8 port)//ite_190909
{

#if 1
    iTE_u16	i,j;
    iTE_u8 ucData;

	iTE_MsgTX((" P%d Dump  \r\n",(iTE_u16)port));
	iTE_MsgTX(("       "));
    for(j = 0; j < 16; j++)
    {
        iTE_MsgTX((" %02X",(iTE_u16) j));
        if((j == 3)||(j==7)||(j==11))
        {
                iTE_MsgTX((" :"));
        }
    }
	iTE_MsgTX((" \r\n"));
    iTE_MsgTX(("=============================================================\r\n"));
    for(i = 0; i < 0x100; i+=16)
    {
        iTE_MsgTX(("[%03X]  ",i));
        for(j = 0; j < 16; j++)
        {
            ucData = h2txrd(port,(iTE_u8)((i+j)&0xFF));
            iTE_MsgTX((" %02X",(iTE_u16) ucData));
            if((j == 3)||(j==7)||(j==11))
            {
                iTE_MsgTX((" :"));
            }
        }
        iTE_MsgTX(("\r\n"));
        if((i % 0x40) == 0x30)
        {
    		iTE_MsgTX(("=============================================================\r\n"));
        }
    }


#endif
}

void it6664_TXCED_monitor(iTE_u8 port)
{
	iTE_u8 CED[7];
	scdcwr(port, 0x10, 0x02); // W1C
	if( scdcrd(port, 0x50, 7)==TRUE )
	{
		h2txbrd(port, 0x30, 7, &CED[0]);
		iTE_MsgTX(("Ch0 Valid=%d, CED = 0x%04X \r\n",(iTE_u16)((CED[1]&0x80)>>7), (iTE_u16)(((CED[1]&0xEF)<<8)+CED[0])));
		iTE_MsgTX(("Ch1 Valid=%d, CED = 0x%04X \r\n",(iTE_u16)((CED[3]&0x80)>>7), (iTE_u16)(((CED[3]&0xEF)<<8)+CED[2])));
		iTE_MsgTX(("Ch2 Valid=%d, CED = 0x%04X \r\n",(iTE_u16)((CED[5]&0x80)>>7), (iTE_u16)(((CED[5]&0xEF)<<8)+CED[4])));
	}
}


#if 1

void it6664_hdmitx_handler(void)
{
	iTE_u8 port;
	iTE_u8 dbg;

	dbg = 0;

	#if (USING_1to8 == TRUE)

		if(g_device == IT6663_C)
		{
			for(port = 0;port<4;port++)
			{
				if(gext_var->HDCPState[port])
				{
					gext_var->HDCPState[port] = Tx_CP_WaitInt;
				}
			}
		}

		if(gext_var->VideoState[0]||gext_var->VideoState[1]||gext_var->VideoState[2]||gext_var->VideoState[3]||
		   gext_var->HDCPState[0]||gext_var->HDCPState[1]||gext_var->HDCPState[2]||gext_var->HDCPState[3])
		{
			if((gext_var->HDCPState[0] != Tx_CP_check)&&(gext_var->HDCPState[1] != Tx_CP_check)&&
			   (gext_var->HDCPState[2] != Tx_CP_check)&&(gext_var->HDCPState[3] != Tx_CP_check))
			{
				iTE_MsgTX((" \r\n"));
				iTE_MsgTX(("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \r\n"));
				iTE_MsgTX(("g_device = %02x \r\n",g_device));
				dbg = 1;
			}
		}
	#endif

	for(port = 0;port<4;port++)
	{
		if((port == 0) || (port == 3))
		{
			#ifdef IT6664
				it6664_hdmitx_video_state(port);
			#endif
		}
		else
		{
			it6664_hdmitx_video_state(port);
		}
		if((!gext_var->TxSrcSel[port])&&((h2txrd(port,0xFD)&0xF0)==0xF0))
		{
			h2txset(port, 0xAF, 0xC0, 0x40);
			h2txset(port, 0xC1, 0xF0, 0x00);
		}
	}

	#if (USING_1to8 == TRUE)
		if(dbg)
		{
			iTE_MsgTX(("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \r\n"));
			iTE_MsgTX((" \r\n"));
		}
	#endif
}

void it6664_hdmitx_video_state(iTE_u8 port)
{
	iTE_u8 EnHDCP[4]={EnableTXP0HDCP,EnableTXP1HDCP,EnableTXP2HDCP,EnableTXP3HDCP};

	switch(gext_var->VideoState[port])
	{
		case Tx_Video_waitInt:
					break;
		case Tx_Video_Stable:
					if((!gext_var->EDIDParseDone[port]) && (g_device != IT6663_C))
					{
					#if (USING_1to8 == FALSE)
						if((PortSkipEdid_opt & (1<<port)) == 0)
					#endif
						{
							it6664_EdidMode_Switch();
						}
					}
					if((h2txrd(port,0x03)&0x01)==0x01)
					{
						SetTXSource(port);
						gext_u8->TxVidStbFlag |= (1<<port);
						if ((gext_u8->TxAFESetDone &(1<<port))==FALSE)
						{
							cal_pclk(port);
							setup_h2txafe(port);
							mSleep(30);
		    				h2txVclkrst(port);
							gext_u8->TxAFESetDone |= (1<<port);
						}
						gext_u8->TxTMDSStbFlag |= (1<<port);
						h2tx_enout(port);
						h2txwr(port, 0x12, 0xFF);
					}
					else
					{
						gext_var->VideoState[port] = Tx_Video_waitInt;
					}
					break;
		case Tx_Video_Stable_off:
					iTE_MsgTX(("[TX]	TXP%d H2TX Video Stable Off Interrupt ... \r\n", (iTE_u16)port));
            		h2txset(port, 0x1A, 0x4F, 0x00);
            		h2txset(port, 0x88, 0x03, 0x03);
            		gext_u8->TxVidStbFlag &= ~(1<<port);
					gext_u8->TxTMDSStbFlag &= ~(1<<port);
					h2txVclkrst(port);
					gext_var->VideoState[port] = Tx_Video_waitInt;
					gext_var->HDCPState[port] = Tx_CP_Reset;
					break;
		case  Tx_Video_Reset:
					iTE_MsgTX(("[TX]	TXP%d Video Reset ... \r\n", (iTE_u16)port));
					gext_u8->TxTMDSStbFlag &= ~(1<<port);
					gext_u8->TxVidStbFlag &= ~(1<<port);
					h2txset(port, 0x88, 0x03, 0x03);
					h2txVclkrst(port);
					h2txset(port,0x12,0x04,0x04);
					mSleep(30);
					gext_var->VideoState[port] = Tx_Video_waitInt;
					gext_var->HDCPState[port] = Tx_CP_Reset;
					break;
		case Tx_Video_OK:
					#if (USING_1to8==TRUE)
						if(g_device == IT6663_C)
						{
							h2txset(port, 0x91, 0x10, 0x00);
							h2txset(port, 0xC1, 0x01, 0x00);
							gext_var->HDCPState[port] = Tx_CP_WaitInt;
						}
						else
					#endif
						{
							h2txset(port, 0x91, 0x10, 0x00);
							h2txset(port, 0xC1, 0x01, 0x00);
						}
					if(gext_u8->ForceTXHDMI2&(1<<port))
					{
						gext_var->TXSCDC_chk[port] = 3;
					}
					gext_var->VideoState[port] = Tx_Video_waitInt;
					break;
		default:
			break;
	}

	#if (USING_1to8==TRUE)
		if(EnHDCP[port])
		{
			if(g_device !=IT6663_C) it6664_hdcp_state(port);
		}
	#else
		if(EnHDCP[port]) it6664_hdcp_state(port);
	#endif
}

#endif

void it6664_hdcp_state(iTE_u8 port)
{
	iTE_u8 ret;
	iTE_u8 RXHDCP_Ver,tmp;

	if(((gext_u8->TXHPDsts & (1<<port))==FALSE) && (gext_var->HDCPState[port]!=Tx_CP_WaitInt))
	{
		gext_var->HDCPState[port] = Tx_CP_WaitInt;
    }
	switch(gext_var->HDCPState[port])
	{
		case Tx_CP_WaitInt:
			break;
		case Tx_CP_Reset:
			h2txset(port, 0x41, 0x01, 0x00);
			h2txset(port, 0x01, 0x20, 0x20);
			h2txset(port, 0x01, 0x20, 0x00);
			if(gext_u8->RXHDCP == 0)
			{
				h2txset(port, 0x91, 0x10, 0x00);
				h2txset(port, 0xC1, 0x01, 0x00);
				h2txset(port, 0xC2, 0x80, 0x00);
			}
			gext_u8->TXHDCP2Done &= ~(1<<port);
			gext_u8->CP_Going &= ~(1<<port);
			gext_u8->CP_Done &= ~(1<<port);
			gext_u8->TXH2RSABusy &= ~(1<<port);
			gext_var->HDCPState[port] = Tx_CP_WaitInt;
			gext_var->HDCPFireVer[port] = 0;
			break;

		case Tx_CP_Going:
			if(gext_var->HDCPWaitCnt[port])
			{
				gext_var->HDCPWaitCnt[port]--;
				mSleep(50);// for normal and 2.2 rep
			}
			else
			{
				if(gmem_tx->HDCPFireCnt[port] > 30)
				{
					h2txset(port, 0xC1, 0x01, 0x01);
				}
				h2txset(port, 0x1A, 0x44, 0x44);
      			h2txset(port, 0x1C, 0x07, 0x07);
				h2txset(port, 0x19, 0x07, 0x07);
				h2txset(port, 0x1A, 0xB0, 0xB0);
		  		h2txset(port, 0x1B, 0xFF, 0xFF);
				gext_u8->TXHDCP2Done &= ~(1<<port);

				tmp = g_device;
				IT6664_DeviceSelect(IT6663_C);
				RXHDCP_Ver = h2rxrd(0xD6)&0x40;
				IT6664_DeviceSelect(tmp);

				if((RXHDCP_Ver == 0x40)&& (gmem_tx->HDCPFireCnt[port]<14))
				{
					if(!gext_u8->TXH2RSABusy)
					{
						txrunhdcp2(port);
					}
				}
				else
				{
					if(gmem_tx->HDCPFireCnt[port] > 30)
					{
						h2txset(port, 0xC1, 0x01, 0x01);
					}
					gext_u8->TXHDCP2Done &= ~(1<<port);
					ret = txrunhdcp(port);
					if(ret == FAIL)
					{
						gext_var->HDCPState[port] = Tx_CP_Reset;
						gext_var->VideoState[port] = Tx_Video_waitInt;
					}
				}
			}
			break;

		case Tx_CP_Done:
			iTE_MsgHDCP(("[HDCP]	TXP%d HDCP_Done	\r\n",(iTE_u16)port));
			h2txset(port, 0x91, 0x10, 0x00);
			h2txset(port, 0xC1, 0x01, 0x00);
			h2txset(port, 0xC2, 0x80, 0x00);
			gext_u8->TXH2RSABusy &= ~(1<<port);
			gext_var->HDCPState[port] = Tx_CP_WaitInt;
			for (tmp=0;tmp<4;tmp++)
			{
				iTE_MsgTX(("last update, P%d HDCP is %2x \r\n", tmp, (gext_u8->CP_Done &(1<<tmp))>>tmp));
			}
			Show_status();
			break;

		case Tx_CP_ReAuth:
			iTE_MsgHDCP(("[HDCP]	TXP%d HDCP ReAuth	\r\n",(iTE_u16)port));
			h2txset(port, 0x41, 0x01, 0x00);
			h2txset(port, 0x01, 0x20, 0x20);
			h2txset(port, 0x01, 0x20, 0x00);

			gext_u8->TXHDCP2Done &= ~(1<<port);
			gext_u8->CP_Going &= ~(1<<port);
			gext_u8->CP_Done &= ~(1<<port);
			gext_u8->TXH2RSABusy &= ~(1<<port);
			gext_var->HDCPWaitCnt[port] = 0;
			gext_var->HDCPState[port] = Tx_CP_WaitInt;
			if(EnRxHP2Rpt== TRUE)
			{
				gext_var->VideoState[port] = Tx_Video_Reset;//for Allion SL8800
			}
			else
			{
				if(gmem_tx->HDCPFireCnt[port] > 30)
				{
					gext_var->VideoState[port] = Tx_Video_Reset;
				}
				else
				{
					if((h2txrd(port, 0x42)&0x10)==0)
					{
						txrunhdcp(port);
					}
					else
					{
						txrunhdcp2(port);
					}
				}
			}
			break;

		case Tx_CP_check:
			if(gext_u8->RXHDCP)
			{
				if((gext_u8->CP_Done & (1<<port)) == FALSE)
				{
					gext_var->HDCPState[port] = Tx_CP_Going;
				}
				RXHDCP_Ver = h2rxrd(0xD6)&0x40;
				#ifdef repeater
				{
					if(RXHDCP_Ver == 0x40)//for 2.0 rep
					{
						gext_var->HDCPWaitCnt[port] = 5;
					}
					else
					{
						gext_var->HDCPWaitCnt[port] = 0;//for 1.4 rep
					}
				}
				#else
				{
					gext_var->HDCPWaitCnt[port] = 5;
				}
				#endif
			}
			break;

		default:
			break;
	}
}

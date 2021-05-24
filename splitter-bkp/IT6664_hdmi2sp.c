///*****************************************
//  Copyright (C) 2009-2021
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <IT6664_hdmi2sp.c>
//   @author Hojim.Tseng@ite.com.tw
//   @date   2021/01/12
//   @fileversion: ITE_HDMI2_SPLITER_1.38
//******************************************/
#include "IT6664_hdmi2sp.h"
#include <linux/delay.h>

#ifdef	Support_CEC
	iTE_u8 CEC_Timer_Unit;
#endif
extern extern_variables *gext_var;
extern extern_32 *gext_long;
extern extern_u8 *gext_u8;
extern iTE_u8 g_device;
iTE_u8 _CODE HP2_KSVLIST[5] =
{
	0x33, 0x33, 0x33, 0x33, 0x33
};


void hdmi2_irq( void )
{
	iTE_u8 SysIntSts;

    SysIntSts = h2sprd(0x05);

    if( SysIntSts==0x00 )
	{
        return;
    }
	else
	{
		#if (USING_1to8==TRUE)
			iTE_MsgRX((" \r\n"));
			iTE_MsgRX(("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \r\n"));
			iTE_MsgRX(("g_device = %02x \r\n",g_device));
		#endif
   		if( SysIntSts&0x10 )
		{
		printk("***************** h2rx_irq ************\n");
        	h2rx_irq();
			#ifdef Support_MHL
			if( gext_u8->BUSMODE==MHL )
            	mhlrx_irq();
			#endif
    	}
    	if( SysIntSts&0x60 )
		{
		
		printk("***************** h2sp_irq ************\n");
        	h2sp_irq();
    	}

    	if(SysIntSts&0x02)
		{
		
		printk("***************** h2tx_irq(P1)************\n");
        	h2tx_irq(P1);
			#ifdef Support_CEC
				Cec_Irq(P1);
			#endif
    	}
    	if(SysIntSts&0x04)
		{
		
		printk("***************** h2tx_irq(P2)************\n");
        	h2tx_irq(P2);
			#ifdef Support_CEC
				//For Port2 and Port3 CEC
				//FW can set converter register(0x58) Reg0xF7[0] to select Port2/Port3
				//Reg0xF7[0] = 1(Port2 ), Reg0xF7[0] = 0(Port3)
				Cec_Irq(P2);
			#endif
    	}
		#ifdef IT6664
		if(SysIntSts&0x01)
		{
        	h2tx_irq(P0);
			#ifdef Support_CEC
				Cec_Irq(P0);
			#endif
    	}
    	if(SysIntSts&0x08)
		{
        	h2tx_irq(P3);
    	}
		#endif
		#ifdef Support_CEC
			if(SysIntSts&0x08)
			{
				Cec_Irq(P3);//RX CEC
    		}
		#endif
		#if (USING_1to8==TRUE)
			iTE_MsgRX((" \r\n"));
			iTE_MsgRX(("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \r\n"));
		#endif
	}
}

void h2sp_irq( void )
{
	iTE_u8 SwIntSts[2];
	iTE_u8 rddata,i,reg21,reg22,reg23;
	iTE_u16 tmpdata1,tmpdata2;

    //h2spbrd(0x06, 2, &SwIntSts[0]);
    SwIntSts[0]=h2sprd(0x06);
	SwIntSts[1]=h2sprd(0x07);
	h2spwr( 0x06,  SwIntSts[0]);
	h2spwr( 0x07,  SwIntSts[1]);
	chgspbank(1);
	reg21 = h2sprd(0x21);
	reg22 = h2sprd(0x22);
	reg23 = h2sprd(0x23);
	h2spwr(0x21,reg21);
	h2spwr(0x22,reg22);
	h2spwr(0x23,reg23);
	chgspbank(0);

		 if( SwIntSts[0]&0x01 ) {
			 //h2spset( 0x06, 0x01, 0x01);
			 iTE_MsgRX(("#### HP2 Auth start Int #### \r\n"));
			 if (EnRxHP2Rpt == FALSE)
			 {
			 	#if (USING_1to8==TRUE)
			 		if(g_device == IT6663_C)
				#endif
		 	 		{
						gext_u8->HDCPTimerCnt = 0; //ite_200206
		  	 		}
			 }
			 else
			 {
				gext_u8->RXHDCP2_chk = 1;
				if(h2txcomrd(0x22))
				{
					h2txcomset(0x20, 0x06, 0x06);
					h2txcomwr(0x21, 0x00);
					h2txcomset(0x20, 0x04, 0x00);
				}
			 }
		 }
    	 if( SwIntSts[0]&0x02 ) {
			 //h2spset( 0x06, 0x02, 0x02);
			 iTE_MsgRX(("#### HP2 Auth done Int #### \r\n"));
			 //IT6664_SetRxLED(1,1);
			 gext_u8->RXHDCP2_chk = 0;
			 tmpdata1=((h2rxrd(0x9C)&0x3F)<<8)+h2rxrd(0x9B);
			 tmpdata2=((h2rxrd(0x9E)&0x3F)<<8)+h2rxrd(0x9D);
			 rddata = h2rxrd(0x98)>>4;
			 #if (USING_1to8 == TRUE)
				iTE_u8 tmp;
				tmp= g_device;
				IT6664_DeviceSelect(IT6664_A);
				gext_u8->RXHDCP = 1;
				for(i=0;i<4;i++)
				{
					if(gext_u8->TXHPDsts & (1<<i))
					{
						gext_var->HDCPState[i] = Tx_CP_check;
					}
				}
				IT6664_DeviceSelect(IT6664_B);
				gext_u8->RXHDCP = 1;
				for(i=0;i<4;i++)
				{
					if(gext_u8->TXHPDsts & (1<<i))
					{
						gext_var->HDCPState[i] = Tx_CP_check;
					}
				}
				IT6664_DeviceSelect(tmp);
			#else
				gext_u8->RXHDCP = 1;
				for(i=0;i<4;i++)
				{
					if(gext_u8->TXHPDsts & (1<<i))
					{
						gext_var->HDCPState[i] = Tx_CP_check;
					}
					//iTE_MsgRX(("gext_var->HDCPState[%x] = %2x \r\n",i,gext_var->HDCPState[i]));
				}
			#endif
			if (EnRxHP2Rpt == TRUE)
			{
				for(i=0;i<4;i++)
				{
			 		if(gext_u8->CP_Done & (1<<i))
			 		{
			 			gext_var->HDCPState[i] = Tx_CP_ReAuth;
						gext_var->HDCPWaitCnt[i] = 3;
						gext_u8->CP_Done &= ~(1<<i);
			 		}
				}
			}
			else
			{
				if(tmpdata2<0xFF00)
				{
					i = tmpdata1%4;
				}
				rddata &= 0x01;
				if(rddata && i)
				{
					for(i=0;i<4;i++)
					{
						h2txwr(i,0xFD,0xF0);
					}
					chgrxbank(3);
					h2rxwr(0xAE,0x01);
					chgrxbank(0);
				}
			}
		 }
		 if( SwIntSts[0]&0x04 ) {
			 //h2spset( 0x06, 0x04, 0x04);
			 iTE_MsgRX(("#### HP2 go-Off Int #### \r\n"));
			 gext_u8->RXHDCP = 0;
			 if(g_device == IT6663_C) {}

			 //IT6664_SetRxLED(1,0);
			 gext_u8->GetSteamType = 0;
		 }
		 if( SwIntSts[0]&0x08 ) {
			 //h2spset( 0x06, 0x08, 0x08);
			 iTE_MsgRX(("#### HP2 EncEn Chg Int #### \r\n"));
			 chgspbank(1);
			 rddata=h2sprd(0x23);
			 chgspbank(0);
			 if (rddata&0x10 )
			 {
				iTE_MsgRX(("encyption is on  \r\n"));
				#if (USING_1to8 == TRUE)
					iTE_u8 tmp;
					tmp= g_device;
					IT6664_DeviceSelect(IT6664_A);
					gext_u8->RXHDCP = 1;
					for(i=0;i<4;i++)
					{
						if((!(gext_u8->CP_Done&(1<<i))) && (gext_u8->TXHPDsts & (1<<i)))
						{
					 		gext_var->HDCPState[i] = Tx_CP_check;
						}
					}
					IT6664_DeviceSelect(IT6664_B);
					gext_u8->RXHDCP = 1;
					for(i=0;i<4;i++)
					{
						if((!(gext_u8->CP_Done&(1<<i))) && (gext_u8->TXHPDsts & (1<<i)))
						{
					 		gext_var->HDCPState[i] = Tx_CP_check;
						}
					}
					IT6664_DeviceSelect(tmp);
				#else
					gext_u8->RXHDCP = 1;
					for(i=0;i<4;i++)
					{
						if((!(gext_u8->CP_Done&(1<<i))) && (gext_u8->TXHPDsts & (1<<i)))
						{
					 		gext_var->HDCPState[i] = Tx_CP_check;
						}
					}
				#endif

				if(EnRxHP2Rpt == TRUE)
				{
					for(i=0;i<4;i++)
					{
						if(((gext_u8->CP_Done&(1<<i))) && (gext_u8->TXHPDsts & (1<<i)))
						{
					 		h2txset(i, 0xC2, 0x80, 0x00);// encyption on
						}
					}
				}
				chgrxbank(3);
				h2spset(0x67,0xFF,h2rxrd(0xAE));
				chgrxbank(0);
			 }
			 else
			 {
			 	if(g_device == IT6663_C) {}
			//IT6664_SetRxLED(1,0);
                iTE_MsgRX(("encyption is off  \r\n"));
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
				#endif
				for(i=0;i<4;i++)
				{
					gext_u8->CP_Done &= ~(1<<i);
					gext_var->HDCPState[i] = Tx_CP_Reset;
					h2txset(i, 0xFD, 0xFF, 0x00);
				}
				gext_u8->GetSteamType = 0;
			 }

		 }
		 if( SwIntSts[0]&0x10 ) {
			 //h2spset( 0x06, 0x10, 0x10);
			 iTE_MsgRX(("#### HP2 process update Int #### \r\n"));
			 if( reg21&0x02 ) {
				 iTE_MsgRX(("#### Rx HP2 No Store KM rcved #### \r\n"));
                 //h2spset( 0x21, 0x02, 0x02);
				 #ifdef repeater
				 	//170104  for repeater
				 	gext_u8->DevCnt_Total = 0;
                 	gext_u8->Depth_Total = 0;
				 	gext_u8->Err_DevCnt_Total = FALSE;
                	gext_u8->Err_Depth_Total = FALSE;
				 	gext_u8->HP2_HDCP1DownStm = 0;
			 	 	gext_u8->HP2_HDCP20DownStm = 0;
				 	h2txcomwr(0x20, 0x02);
				 	h2txcomwr(0x20, 0x00);
				 	//170104
				 #endif
             }
			 if( reg21&0x04 ) {
				 iTE_MsgRX(("#### Rx HP2 Stored KM rcved #### \r\n"));
				 //h2spset( 0x21, 0x04, 0x04);
				 #ifdef repeater
				 	//170104  for repeater
				 	gext_u8->DevCnt_Total = 0;
                 	gext_u8->Depth_Total = 0;
				 	gext_u8->Err_DevCnt_Total = FALSE;
                 	gext_u8->Err_Depth_Total = FALSE;
				 	gext_u8->HP2_HDCP1DownStm = 0;
			 	 	gext_u8->HP2_HDCP20DownStm = 0;
				 	h2txcomwr(0x20, 0x02);
				 	h2txcomwr(0x20, 0x00);
				 	//170104
				 #endif
             }
			 if( reg21&0x08 ) {
				 iTE_MsgRX(("#### Rx HP2 RSA failed #### \r\n"));
				 //h2spset( 0x21, 0x08, 0x08);
             }
			 if( reg21&0x10 ) {
				 iTE_MsgRX(("#### Rx HP2 Locality check msg rcved #### \r\n"));
				 //h2spset( 0x21, 0x10, 0x10);
             }
			 if( reg21&0x20 ) {
				 iTE_MsgRX(("#### Rx HP2 SKE msg rcved #### \r\n"));
				 //h2spset( 0x21, 0x20, 0x20);
             }
				 //170209
             if( reg21&0x40 ) {
				 iTE_MsgRX(("#### Rx HP2 Rpt Send Ack msg rcved #### \r\n"));
				 //h2spset( 0x21, 0x40, 0x40);
             }
			 if( reg21&0x80 ) {
				 iTE_MsgRX(("#### Rx HP2 Rpt Strm manage msg rcved #### \r\n"));
				 ReadStreamType();//ITE_171212
				 //h2spset( 0x21, 0x80, 0x80);
             }
			 if( reg22&0x01 ) {
				 iTE_MsgRX(("#### Rx HP2 CERT Rd done #### \r\n"));
				 //h2spset( 0x22, 0x01, 0x01);
             }
			 if( reg22&0x02 ) {
				 iTE_MsgRX(("#### H Rd done #### \r\n"));
				 //h2spset( 0x22, 0x02, 0x02);
             }
			 if( reg22&0x04 ) {
				 iTE_MsgRX(("#### Rx paring rd done #### \r\n"));
				 //h2spset( 0x22, 0x04, 0x04);
             }
			 if( reg22&0x08 ) {
				 iTE_MsgRX(("#### Rx HP2 L' Rd done #### \r\n"));
				 //h2spset( 0x22, 0x08, 0x08);
             }
			 if( reg22&0x10 ) {
				 iTE_MsgRX(("#### Rx HP2 V' Rd done #### \r\n"));
				 //h2spset( 0x22, 0x10, 0x10);
             }
			 if( reg22&0x20 ) {
				 iTE_MsgRX(("#### Rx HP2 M' Rd done #### \r\n"));
				 //h2spset( 0x22, 0x20, 0x20);
             }
			 if( reg22&0x40 ) {
				 iTE_MsgRX(("#### Rx HP2 Msg Rd Err !!!!#### \r\n"));
				 //h2spset( 0x22, 0x40, 0x40);
             }
			 if( reg22&0x80 ) {
				 iTE_MsgRX(("#### Rx HP2 Auto Reauth issued !!!!#### \r\n"));
				 //h2spset( 0x22, 0x80, 0x80);
             }
		 }
		 if( SwIntSts[0]&0x20 ) {
			 //h2spset( 0x06, 0x20, 0x20);
			 iTE_MsgRX(("#### HP SHA1 Done Int #### \r\n"));
		 }
		 if( SwIntSts[1]&0x01 ) {
			 //h2spset( 0x07, 0x01, 0x01);
			 iTE_MsgRX(("#### ORIFIFO Err Int #### \r\n"));
		 }
		 if( SwIntSts[1]&0x02 ) {
			 //h2spset( 0x07, 0x02, 0x02);
			 iTE_MsgRX(("#### CSCFIFO Err Int #### \r\n"));
		 }
	     if( SwIntSts[1]&0x04 ) {
			 //h2spset( 0x07, 0x04, 0x04);
			 iTE_MsgRX(("#### Chroma FIFO Err Int #### \r\n"));
	     }
		 if( SwIntSts[1]&0x08 ) {
			 //h2spset( 0x07, 0x08, 0x08);
			 iTE_MsgRX(("#### Down scale FIFO Err Int #### \r\n"));
		 }
		 if( SwIntSts[1]&0x10 ) {
			 //h2spset( 0x07, 0x10, 0x10);
			 //iTE_MsgRX(("#### Timer 0 timeup Int #### \r\n"));
			TimerInt0();
		 }
		 if( SwIntSts[1]&0x20 ) {
			 //h2spset( 0x07, 0x20, 0x20);
			 iTE_MsgRX(("#### Timer 1 timeup Int #### \r\n"));
			 TimerInt();
		 }

}

void it6664_hdmi2sp_initial(void)
{
	iTE_u8 VenID[2], DevID[2], RevID,i;
	iTE_u16 HP2_RxInfo;

		h2spwr(0x0F, 0x00);
		h2spbrd(0x00, 2, &VenID[0]);
		h2spbrd(0x02, 2, &DevID[0]);

		h2spbrd(0x00, 2, &VenID[0]);
		h2spbrd(0x02, 2, &DevID[0]);

		RevID = h2sprd(0x03);

		iTE_MsgRX((KERN_ERR "############################################### \r\n"));
		iTE_MsgRX((KERN_ERR "#             HDMI2SP Initialization 		  # \r\n"));
		iTE_MsgRX((KERN_ERR "############################################### \r\n"));

		if( ( ((VenID[0]!=0x54) || (VenID[1]!=0x49) || (DevID[0]!=0x64) || (DevID[1]!=0x66)) && (RevID!=0xA0) )) {
			iTE_MsgRX((KERN_ERR "Current DevID=%02X%02X \r\n", DevID[1], DevID[0]));
			iTE_MsgRX((KERN_ERR "Current VenID=%02X%02X \r\n", VenID[1], VenID[0]));
			//return FAIL;
		}else if (( ((VenID[0]!=0x54) || (VenID[1]!=0x49) || (DevID[0]!=0x63) || (DevID[1]!=0x66)) && (RevID!=0xA0) )) {
        	iTE_MsgRX((KERN_ERR "Current DevID=%02X%02X \r\n", DevID[1], DevID[0]));
        	iTE_MsgRX((KERN_ERR "Current VenID=%02X%02X \r\n", VenID[1], VenID[0]));
   	 	}
		if(h2sprd(0x15)&0x01)  iTE_MsgRX(("RevID is A1 \r\n"));

		it6664_hdmi2sp_rst();
		if(!Bond6664)
		{
			h2txwr(0, 0x03, 0x03);
			h2txwr(0, 0x84, 0x60);
			h2txwr(0, 0x86, 0x00);
        	h2txwr(0, 0x88, 0x0B);
			#ifdef Support_CEC
				h2txwr(3, 0x03, 0x03);
			#endif
			h2txwr(3, 0x84, 0x60);
			h2txwr(3, 0x86, 0x00);
        	h2txwr(3, 0x88, 0x0B);
		}
		h2spset(0x08, 0x0F, 0x0F);//enable 4 port
		h2spset(0x0D, 0xFF, 0x00);//default source select original;
		h2spset(0x6B, 0x3C, (((RGB444)<<4)+((RGB444)<<2)));//set default RGB
		h2spset(0x6C, 0x38, ((En444to420<<4)+(En420to444<<5)));
		if (EnRxHP2_Dbg!=0) h2spset(0x18, 0x10, 0x10);
		chgspbank(1);
		h2spset(0x10, 0x49, (EnRxHDCP2<<6)+(EnRxHP2Rpt<<3)+RxHP2SelDone);
		h2spset(0x1D, 0x80, EnRxHwReauth<<7);
		h2spset(0x20, 0x78, EnRxHP2_Dbg<<3);
		chgspbank(0);
		h2spset(0x19, 0x3F, 0x0F);
		h2spwr(0x2B, 0xFF);
		h2spwr(0x2D, 0x0F);
		h2spwr(0x2E, 0xFF);
		h2spwr(0x30, 0x0F);
		if(EnRxHP2VRRpt)
		{
			h2txcomwr(0x20, 0x04);
			h2txcomwr(0x21, 0x00);
			for (i=0;i<HP2_DevNum;i++) h2txcombwr(0x23, 4, &HP2_KSVLIST[i]);
			h2txcomwr(0x20, 0x00);
			iTE_MsgRX(("write KSV to fifo done \r\n"));
			chgspbank(1);
			HP2_RxInfo = (HP2_DevDepth<<9)+(HP2_DevNum<<4);
			h2spwr(0x18, HP2_RxInfo&0xFF);
			h2spwr(0x19, (HP2_RxInfo&0xFF00)>>8);
			chgspbank(0);
		}
		h2spset(0x6D, 0x30, (FwEnCV2DS<<4));
		msleep(10);
		for(i=0;i<=3;i++)
		{
			h2txset(i, 0x41, 0x01, 0x00);
			h2txset(i, 0xC1, 0x01, 0x01);
			h2txset(i, 0x88, 0x01, 0x01);
		}
		//170104
		gext_u8->DevCnt_Total = 0;
   		gext_u8->Depth_Total = 0;
    	gext_u8->Err_DevCnt_Total = FALSE;
    	gext_u8->Err_Depth_Total = FALSE;
		//170104


		h2spset(0x1A,0x01,0x00);
		h2spset(0x19,0x10,0x00);
		h2spwr(0x1C,0x2F);//500ms
		h2spset(0x19,0x10,0x10);
		h2spwr(0x07,0x1F);
		h2spset(0x1A,0x01,0x01);
}

void it6664_hdmi2sp_rst(void)
{

		iTE_u8 i,tmp,time;

		//h2spset(0x41, 0x90, 0x80);  // ForceWrUpd=1 and SWGateRCLK=0

		// H2SP Initial Setting
		h2spwr(0x0A, 0x01);//161230
		h2spwr(0x0A, 0x00);
		//reset siprom

		tmp = h2sprd(0x60);

		if(tmp != 0x19)
		{
			h2spwr(0xFF, 0xC3);
			h2spwr(0xFF, 0xA5);
			h2spwr(0x5F, 0x04);
			h2spwr(0x58, 0x12);
			h2spwr(0x58, 0x02);
			h2spwr(0x5F, 0x00);
			h2spwr(0xFF, 0xFF);

			time = 50;
			do{
				msleep(1);
				tmp = h2sprd(0x60);
				time--;
			}while((tmp!=0x19)&&(time));

			msleep(10);
			chgspbank(1);
			h2spset(0x73,0x04,0x04);
			chgspbank(0);

		}

		h2spwr(0x10, 0x6E);// increase ring osc freq from 0x6E

		h2spwr(0xF0, 0x71);
		h2spset(0x0E, 0x07, (RCLKSrcSel<<2)+RCLKFreqSel);
		h2spset(0x08, 0x0F, 0x0F);

		#if (USING_1to8==TRUE)
			printk("*********USING_1to8 *********\n");
		if(g_device == IT6664_B)
		{
			printk(KERN_INFO "*********using g_device iT6664_B *********\n");
			h2spwr(0xF0, (RXP0Addr+2)|0x01);	// enable RXP0 slave address
			h2spwr(0xF1, (TXComAddr+2)|0x01);	 // enable TXcom slave address
		}
		else
		#endif
		{
			printk(KERN_INFO "*********using g_device iT6663_C *********\n");
			h2spwr(0xF0, RXP0Addr|0x01);	// enable RXP0 slave address
			h2spwr(0xF1, TXComAddr|0x01);	 // enable TXcom slave address
		}
		cal_rclk();
		h2txcomset(0x50, 0x04, 0x00);

			printk(KERN_INFO "*********enable port slave addresses *********\n");
		// Enable Slave Address
		#if (USING_1to8==TRUE)
		if(g_device == IT6664_B)
		{
			printk(KERN_INFO "*********using g_device iT6664_B *********\n");
			h2txcomwr(0x2C, (TXP0Addr+2)|0x01);    // enable TXcom slave address
			h2txcomwr(0x2D, (TXP1Addr+2)|0x01);    // enable TXP1 slave address
			h2txcomwr(0x2E, (TXP2Addr+2)|0x01);    // enable TXP2 slave address
			h2txcomwr(0x2F, (TXP3Addr+2)|0x01);    // enable TXP3 slave address
		}
		else
		#endif
		{
			printk(KERN_INFO "*********using g_device iT6663_c *********\n");
			h2txcomwr(0x2C, TXP0Addr|0x01);    // enable TXcom slave address
			h2txcomwr(0x2D, TXP1Addr|0x01);    // enable TXP1 slave address
			h2txcomwr(0x2E, TXP2Addr|0x01);    // enable TXP2 slave address
			h2txcomwr(0x2F, TXP3Addr|0x01);    // enable TXP3 slave address
		}
		//170104
		h2rx_rst(P0);

		if(g_device == IT6663_C)
		{
			if(!gext_u8->EDIDCopyDone)
			{
				if( EnIntEDID==TRUE )
				{
					h2rxset(0x34, 0x01, 0x00);  //emily 20161222 Enable ERCLK even no 5VDet
					DefaultEdidSet();
					h2rxset(0xC5, 0x01, 0x00);
					h2rxset(0x34, 0x01, 0x01);
				}
				else h2rxset(0xC5, 0x01, 0x01);
			}
		}
    	h2txcomwr(0x20, 0x02); //reset KSV FIFO
    	h2txcomwr(0x20, 0x00);
		//170104
		#ifdef IT6664
		for(i=0;i<TxPortNum;i++)
		#else
		h2tx_pwrdn(0);
		h2tx_pwrdn(3);
		for(i=1;i<TxPortNum+1;i++)
		#endif
		{
			h2txset(i, 0xC1, 0x01, 0x01); //set AVMUTE
			h2txset(i, 0x01, 0x01, 0x01);
			h2txset(i, 0x01, 0x01, 0x00);
			h2tx_ini(i);
			h2tx_rst(i);
			h2tx_pwrdn(i);
			#ifdef Support_CEC
				h2txset(i, 0x02, 0x03, 0x00);
    			Cec_Init(CEC_Timer_Unit, i);
			#endif
		}
		#ifdef IT6663
			#ifdef Support_CEC
				h2txset(3, 0x02, 0x03, 0x00);
    			Cec_Init(CEC_Timer_Unit, 3);
			#endif
		#endif
		h2txcomset(0x15, 0x08, 0x08);
}

void h2tx_rst(iTE_u8 port)
{
	h2txVclkrst(port);
	h2txset(port, 0x01, 0x20, 0x20);
	h2txset(port, 0x01, 0x20, 0x00);
	h2spwr(0x0C, 0x01<<(port+4));
    h2spwr(0x0C, 0x00);
	h2txset(port, 0x18, 0x80, 0x00); //disable interrupt
	h2txwr(port, 0x19, 0x00);
	h2txwr(port, 0x1A, 0x00);
	h2txwr(port, 0x1B, 0x00);
	h2txwr(port, 0x1C, 0x00);
	// Enable TX DDC Master Reset
	h2txset(port, 0x35, 0x10, 0x10);	// DDC Master Reset
	h2txset(port, 0x35, 0x30, 0x00);	// disable ddc cmd fail cnt
	//h2tx_ini(port);
	//gext_var->TxAFESpd[port] = 1;//HIGH;		   // set default AFE speed as High after reset
	gext_u8->TxVidStbFlag &= ~(1<<port);
}
void h2txcom_ini(void)
{
    h2txcomwr(0xFF, 0xC3);
    h2txcomwr(0xFF, 0xA5);
    h2txcomwr(0xF8, 0xFF);
	h2txcomset(0x15, 0x38, (TxCLKStbRef<<4)+(ForceTxCLKStb<<3));
    h2txcomwr(0x20, 0x02);
	h2txcomwr(0x20, 0x00);
}

void h2tx_ini(iTE_u8 port)
{
    h2txset(port,0x08, 0x1C, (ForceTMDSStb<<4)+(ForceVOut<<3)+(ForceROn<<2));
    h2txset(port, 0x02, 0x02, 0x02);
    h2txset(port, 0x41, 0x01, 0x00);
	h2txset(port, 0xC0, 0x01, 0x01);//default HDMI
    h2txset(port, 0x34, 0xC0, (FWDDCFIFOAcc<<7)+(HWDDCFIFOAcc<<6));
    h2txset(port, 0x35, 0x03, DDCSpeed);
    h2txset(port, 0x3A, 0xFC, (EnDDCMasterArb<<7)+(DisH2Auto<<6)+(SCDCFlagByte<<5)+(EnFlagUpdRd<<4)+(FlagPollSel<<2));
    h2txset(port, 0x93, 0xFF, (CRCBLimit<<7)+(QuanL4B<<6)+(EnDither<<4)+(EnUdFilt<<3)+(DNFreeGo<<2)+(UpSamSel<<1)+ColorClip);
    h2txset(port, 0x94, 0x3E, (XPStableTime<<4)+(EnXPLockChk<<3)+(EnPLLBufRst<<2)+(EnFFAutoRst<<1));
    //h2txset(port, 0xBF, 0xC8, (TxCLKStbRef<<6)+(ForceTxCLKStb<<3));
    h2txset(port, 0xC0, 0x10, AutoPhReSync<<4);
	h2txset(port, 0xC1, 0x04, TXNoDefPhase<<2);
    h2txset(port, 0xC3, 0x0F, (EnSSCPTrgPkt<<3)+(SSCPTest<<2)+(SSCPOpt<<1)+SSCPPos);
	gext_u8->TXHP2KSVChkDone &= ~(1<<port);
    gext_u8->TXHDCP2Done &= ~(1<<port);
	gext_u8->TXH2RSABusy &= ~(1<<port);
	gext_var->TXHPD[port] = FALSE;//170104
    h2txset(port, 0x18, 0x03, 0x03);   // Enable HPD and RxSen Interrupt
	h2txwr(port, 0x19, 0x07);
	h2txwr(port, 0x1A, 0x03);  //enable 12~14 enterrupt
	h2txwr(port, 0x1B, 0xFF);
    h2txwr(port, 0x1C, 0x03);
    h2txwr(port, 0x88, 0x54);
	h2txwr(port, 0x8a, 0x00);
	h2txwr(port, 0x8b, 0x07);
}

void h2rx_rst(iTE_u8 port)
{
  	//if (!FixRXSet)//170117
  	{
  		h2rxwr( 0x22, 0x08);	  // RegRegRst=1
  		//h2rxwr( 0x22, 0x00);
  		h2rxwr( 0x23, 0x01);	  // Reg_SWRst=1
  	}
  	h2rxwr( 0x22, 0x17);
  	h2rxwr( 0x24, 0xF8); //Reset MHL FSM
  	msleep(10);
  	h2rxwr( 0x23, 0xA0);	  //
  	h2rxwr( 0x22, 0x00);
  	h2rxwr( 0x24, 0x00);
	//170117
	caof_ini();
	h2rx_ini(port);
	#ifdef Support_MHL
	if(g_device == IT6663_C) mhlrx_ini();
	#endif
	//detectbus();
	if( gext_u8->BUSMODE==MHL ) {
		#ifdef Support_MHL
	    mhlrxset(0x2A, 0x01, 0x01); //enable HW rstddfsm
    	mhlrxset(0x0F, 0x10, 0x00);
		#endif
		h2rxwr(0xAB, 0x00);
	    h2rxwr(0xAC, 0x00);
	}
    else
	{
		h2rxwr(0x0F, 0x03);
		h2rxwr(0xAB, 0x4A);
		h2rxwr(0xAC, 0x40);
		h2rxwr(0x0F, 0x00);
	}
	it6664_h2rx_pwdon();
    gext_u8->CD_SET = FALSE;
}

#if 1
void detectbus(void)
{
	iTE_u8 reg13h,i;

	chgrxbank(0);
	reg13h = h2rxrd(0x13);
	if( (reg13h&0x40) ||((reg13h&0x01)==0x00))//20170116
	{
		gext_u8->BUSMODE = MHL;
		iTE_MsgRX(("Bus mode = MHL  \r\n"));
		#ifdef Support_MHL
		mhlrx_ini();
		#endif
	 	//h2rxset(0x34, 0x01, 0x00);    // temp for FW work-around
	}
	else//20170116
	{
		if (gext_u8->BUSMODE == MHL)//previous MHL
		{
		  iTE_MsgRX(("from MHL to HDMI!! \r\n"));
		  h2rxwr(0x0F, 0x03);
	      h2rxwr(0xAB, 0x4A);
	      h2rxwr(0xAC, 0x40);
	      h2rxwr(0x0F, 0x00);
		  msleep(50);
	      h2rx_ini(P0);//reset hdmi
        }
		  chgrxbank(3);
		  h2rxset(0x3A, 0x06, 0x02);             // set Reg_ENHEQ
    	  chgrxbank(0);
          h2rxset(0x29, 0x01, 0x00); // emily Enable 20161221 MHL AUTOPWD
          h2rxset(0x26, 0x0C, 0x00); // emily 20161221 not PWD CH1/CH2 at HDMI
		  h2spset(0x69, 0x2F, 0x00); // added for Using PCLK as VCLK at MHL mode
		gext_u8->BUSMODE = HDMI;
	}//20170116

	for(i=0;i<4;i++)
	{
		#if (USING_1to8 == TRUE)
			if(g_device != IT6663_C)
			{
				if(h2txrd(i,0x03)&0x01)
				{
					SetRxHpd(TRUE);
					break;
				}
			}
			else
			{
				if((i==1) || (i==2))
				{
					if(h2txrd(i,0x03)&0x01)
					{
						SetRxHpd(TRUE);
						break;
					}
				}
			}
		#else
			if(gext_var->EDIDParseDone[i])
			{
				SetRxHpd(TRUE);
				break;
			}
			if( (h2txrd(i,0x03)&0x01)&&((FixPort_Opt & (1<<i))||(PortBypass_Opt & (1<<i))))
			{
				SetRxHpd(TRUE);
				break;
			}
		#endif
	}
	iTE_MsgRX(("detect bus done \r\n"));
}
#endif


void h2rx_ini(iTE_u8 port)
{
    //iTE_Msg(("RX Initial Setting ... \r\n"));
	gext_u8->RXSCDT = 0;
	h2rxwr( 0x56, 0xFF);			// Enable RxIntEn[31:24]
	h2rxwr( 0x57, 0xFF);
	//h2rxset(0x5D, 0x08, 0x08);
	chgrxbank(3);
	h2rxset(0xA8, 0x08, 0x08); // Reg_ALPWDB
	h2rxset(0xA7,0x40,0x40);
	h2rxset(0x26, 0x20, 0x00);
	if(g_device == IT6663_C)
	{
		h2rxset(0x27, 0xFF, 0x9F);
		h2rxset(0x28, 0xFF, 0x9F);
		h2rxset(0x29, 0xFF, 0x9F);
	}
	#if (USING_1to8==TRUE)
	else
	{
		h2rxset(0x27, 0xFF, 0xFF);
		h2rxset(0x28, 0xFF, 0xFF);
		h2rxset(0x29, 0xFF, 0xFF);
	}
	#endif
	chgrxbank(0);
	//170109
	h2rxset(0x28, 0x59, 0x59); //default Auto Power down option //tmp for AWA chip
	h2rxset(0x2A, 0x01, 0x01); //default invert COF Clock
	//170109
	h2rxset(0x43, 0x02, 0x00);//161230
	h2rxset(0x44, 0x3F, 0x19); //1G
	h2rxset(0x3c, 0x01, 0x00); //by ysliu 2017/03/07
	h2rxwr(0x45, 0xDF); //modify lowest for FPGA freq  //170109
    h2rxset(0x46, 0x3F, 0x15); //1P48G
	h2rxset(0x47, 0xFF, 0x88); //3P4G
    //h2rxset(0x60, 0x03, 0x00); //change interrupt type
	h2rxwr(0x49, RXMHLAddr|0x01); //enable MHL port
	//h2rxwr(0x45, 0xDF); //modify lowest for FPGA freq //170109
	//h2rxwr(0x22, 0x10); //auto reset
	h2rxwr(0x23, 0xA0); //auto reset
	//h2rxset(0xCE, 0x80, EnRxHP1Rpt<<7);
//	h2rxwr(0xF1, 0x02);//ECC Reauth thres
	h2rxwr(0x53, 0x0F); //default interrupt on: PWR5V, RXCLK on
	h2rxwr(0xE3, EnRxHDCP2<<2);
	h2rxset(0xCE, 0x80, EnRxHP1Rpt<<7);
	// emily add 20161222 for intEDID initial start
	h2rxset(0x3C, 0x20, 0x00); // emily 20161221 MHL Don't Bypass HCLK/DCLK buffer
	chgrxbank(3);
	h2rxset(0xe3, 0x01, 0x01);
	h2rxset(0xe3, 0x06, 0x03);
	h2rxwr(0xf0, 0xa0);
	chgrxbank(0);
	//20170327 MHLCTS
	h2rxset(0x28, 0x88, 0x88);
	h2rxset(0x3B, 0x20, 0x20);//enable cedopt
	h2rxwr(0x26,0xFF);
	h2rxset(0x42, 0x20, 0x00);//Modify for 8192x1080P
	h2rxset(0x5D, 0x06, 0x06);
	chgrxbank(3);
	h2rxset(0xA6, 0x08, 0x08);// set Reg_P0_IPLLENI = 1
	chgrxbank(0);
}
void chgrxbank(iTE_u8 bankno)
{
	h2rxset(0x0f, 0x03, bankno&0x03);
}

void chgspbank(iTE_u8 bankno)
{
	h2spset(0x0f, 0x01, bankno&0x01);
}
void cal_rclk( void )
{
	iTE_u8  UseSip=0;
	iTE_u32 TimeLoMax;
	iTE_u32 sum2;
	iTE_u32 Sipdata;
	iTE_u8 t1usint;
	iTE_u32 t1usflt;

	Sipdata = ReadSipRom();
	iTE_MsgRX(("Sipdata = 0x%02x  \r\n",Sipdata));

	if(Sipdata>SIP_Mean) //+-40%
	{
		if((Sipdata - SIP_Mean) <= 12000) UseSip = 1;
		else UseSip = 0;
	}
	else
	{
		if((SIP_Mean - Sipdata) <= 12000) UseSip = 1;
		else UseSip = 0;
	}
	if(!UseSip)
	{
		#ifdef  MCU_6350
			iTE_u32 rddata;
			sum2 =0;
        	h2spset(0x11, 0x80, 0x80);
			mSleep(99);
        	h2spset(0x11, 0x80, 0x00);
        	rddata = h2sprd(0x12);
       	 	rddata += (h2sprd(0x13)<<8);
        	rddata += (h2sprd(0x14)<<16);
			sum2 += rddata;
			gext_long->RCLK = sum2/100;
			#ifdef	Support_CEC
				CEC_Timer_Unit = (sum2*107/16000);
    			//cecwr(0x0C, CEC_Timer_Unit);
				//Set RX CEC
			#endif
    		iTE_MsgRX(("No SIP rom data or over mean value  \r\n"));
    		iTE_MsgRX(("RCLK= %u Hz \r\n", sum2/100));
			TimeLoMax = sum2/10;//(10*RCLK);
		#else
			gext_long->RCLK = SIP_Mean;
			TimeLoMax = SIP_Mean*10;
			iTE_MsgRX(("Use mean RCLK= %u Hz \r\n", gext_long->RCLK));
		#endif
	}
	else
	{
		gext_long->RCLK = Sipdata;
		TimeLoMax = Sipdata*10;
		//iTE_MsgRX(("Use SIP rom data \r\n"));
		iTE_MsgRX(("RCLK= %u Hz \r\n", gext_long->RCLK));
		#ifdef	Support_CEC
			CEC_Timer_Unit = (iTE_u8)(Sipdata*107/16000);
			iTE_MsgRX(("CEC_Timer_Unit= %2x \r\n", CEC_Timer_Unit));
		#endif
	}
	//TimeLoMax = TimeLoMax/2;// rep 1.4 can't
	iTE_MsgRX(("TimeLoMax= %u Hz \r\n", TimeLoMax));
    h2txcomwr(0x11, TimeLoMax&0xFF);
    h2txcomwr(0x12, (TimeLoMax&0xFF00)>>8);
    h2txcomset(0x13, 0x03, (TimeLoMax&0x30000)>>16);
	sum2 = gext_long->RCLK;
	t1usint = (iTE_u8) (gext_long->RCLK/1000);
    t1usflt = gext_long->RCLK%1000;
	t1usflt <<= 8;
	t1usflt /= 1000;
	h2spset(0x1E, 0x3F, t1usint&0x3F);
    h2spwr(0x1F, (iTE_u8)t1usflt);
	//170109
	gext_long->RCLK = sum2;

}

void cal_pclk(iTE_u8 port)
{
	iTE_u8 PixRpt[4] = {1, 1, 1, 1};
	iTE_u8 OutColorDepth;
	iTE_u16 rddata, predivsel,i;
	iTE_u32 sum,TXPCLK;
    // PCLK Count Pre-Test
    h2txset(port, 0x07, 0x80, 0x80);
	msleep(1);
    h2txset(port, 0x07, 0x80, 0x00);

    rddata = h2txrd(port, 0x06);
    rddata += ((h2txrd(port, 0x07)&0x0F)<<8);

    if( RCLKFreqSel==0 )
        rddata *= 2;

    if( rddata<16 )
        predivsel = 7;
    else if( rddata<32 )
        predivsel = 6;
    else if( rddata<64 )
        predivsel = 5;
    else if( rddata<128 )
        predivsel = 4;
    else if( rddata<256 )
        predivsel = 3;
    else if( rddata<512 )
        predivsel = 2;
    else if( rddata<1024 )
        predivsel = 1;
    else
        predivsel = 0;

//  iTE_Msg(("predivsel=%d \r\n", predivsel));
	//predivsel = 2;
    sum = 0;
    for(i=0; i<10; i++) {
        h2txset(port, 0x07, 0xF0, (0x80+(predivsel<<4)));
        h2txset(port, 0x07, 0xF0, (predivsel<<4));

        rddata = h2txrd(port, 0x06);
        rddata += ((h2txrd(port, 0x07)&0x0F)<<8);

        if( RCLKFreqSel==0 )
            rddata *= 2;

        sum += rddata;

    }

    sum /=(iTE_u32) (10*Pow(2,predivsel));

    TXPCLK = gext_long->RCLK*2048*2/sum;
	gext_u8->GCP_CD = ((h2rxrd( 0x98)&0xF0)>>4);

	OutColorDepth = (gext_u8->GCP_CD&0x03);
    h2txset(port, 0xAF, 0xC0, 0x00);


        switch( OutColorDepth ) {
        case VID10BIT: gext_long->TXVCLK[port] =(iTE_u32) (TXPCLK*PixRpt[port]/4)*5; break;
        case VID12BIT: gext_long->TXVCLK[port] =(iTE_u32) (TXPCLK*PixRpt[port]/2)*3; break;
        default      : gext_long->TXVCLK[port] =(iTE_u32) (TXPCLK*PixRpt[port]);
        }
	if((gext_long->TXVCLK[port]/1000) > 620)
	{
		gext_long->TXVCLK[port] = 0;
	}
	sum = gext_long->TXVCLK[port]*107/100;
	TXPCLK = TXPCLK*107/100;
	iTE_MsgRX(("TXP%d Count PCLK= %u MHz \r\n",(iTE_u16) port, TXPCLK/1000));
	iTE_MsgRX(("TXP%d Count VCLK= %u MHz \r\n",(iTE_u16) port, sum/1000));
}
void cal_oclk(void)
{
 	iTE_u16 oscdiv;
	iTE_u32 OSCCLK;

    if( RCLKFreqSel==0 )
        OSCCLK = gext_long->RCLK*2;
    else if( RCLKFreqSel==1 )
        OSCCLK = gext_long->RCLK*4;
    else if( RCLKFreqSel==2 )
        OSCCLK = gext_long->RCLK*8;
    else
        OSCCLK = gext_long->RCLK*16;

     //iTE_MsgRX(("OSCCLK=%3.3fMHz \r\n", OSCCLK/1000));
	 iTE_MsgRX(("OSCCLK= %u MHz \r\n", OSCCLK/1000));

     oscdiv =(iTE_u16) (OSCCLK/1000/10);
     if( ((OSCCLK/1000/oscdiv)-10)>(10-(OSCCLK/1000/(oscdiv+1))) )
         oscdiv++;

     if( FixOCLKD4 )
         oscdiv = 4;

     //iTE_MsgRX(("OCLK=%fMHz \r\n", (float)OSCCLK/1000/oscdiv));
	 iTE_MsgRX(("OCLK= %u MHz \r\n", OSCCLK/1000/oscdiv));

     mhlrxset(0x01, 0x70, oscdiv<<4);
	 mhlrxset(0x01, 0xF0, 0xD0);
     iTE_MsgRX((" \r\n"));
}
iTE_u16 Pow(iTE_u16 x,iTE_u16 y)
{
	iTE_u16 sum;
	iTE_u8	i;
	sum = 1;
	if(y)
	{
		for(i=0;i<y;i++)
		{
			sum *= x;
		}
	}
	return sum;
}
void it6664_var_init(void)
{
	iTE_u8 *ptr;
	iTE_u8 i;

	iTE_MsgRX(("Device = %02x \r\n",g_device));
	ptr = &gext_var->TxSrcSel[0];
	for(i = 0;i<sizeof(extern_variables);i++)
	{
		*ptr++ = 0;
	}
	it6664_txvar_init();
}
iTE_u32 ReadSipRom(void)
{
	iTE_u8 Offset[6]={0x0f, 0x5F, 0x5F, 0x58, 0x58, 0x57};
	iTE_u8 WData[6]= {0x00, 0x04, 0x05, 0x12, 0x02, 0x01};
	iTE_u8 i,tmp,tmp2;
	iTE_u16 Addr;
	iTE_u16 sipdata[2];
	iTE_u32 ret;

	h2spwr(0xFF, 0xC3);
	h2spwr(0xFF, 0xA5);
	for(i=0;i<6;i++) //Set normal read
	{
		h2spwr(Offset[i],WData[i]);
	}
	for(i=0;i<2;i++)
	{
		sipdata[i] = 0;
		Addr = i;
		h2spwr(0x50, (Addr&0xF00)>>8);
		h2spwr(0x51, Addr&0xFF);
		h2spwr(0x54, 0x04);
		tmp2 = h2sprd(0x61);
		tmp = h2sprd(0x62);
		sipdata[i] = ((iTE_u16) tmp2) + ((iTE_u16) tmp)*0x100;
	}
	if((sipdata[0] == 0xFFFF) && (!sipdata[1])) // block 1 data valid
	{
		Addr = 0x4B0;
	}
	else // block 0 data valid
	{
		Addr = 0xB0;
	}
	for(i=0;i<2;i++)
	{
		sipdata[i] = 0;
		Addr += i;
		h2spwr(0x50, (Addr&0xF00)>>8);
		h2spwr(0x51, Addr&0xFF);
		h2spwr(0x54, 0x04);
		tmp2 = h2sprd(0x61);
		tmp = h2sprd(0x62);
		sipdata[i] = ((iTE_u16) tmp2) + ((iTE_u16) tmp)*0x100;
		//iTE_Msg(("sipdata %02x = %02x \r\n",i,sipdata[i]));
	}
	ret = 0;
	ret = sipdata[1]&0x00FF ;
	ret <<= 16 ;
	ret += sipdata[0] ;

	if((sipdata[1]&0xC000) == 0xC000)
	{
		ret /= 100;
	}
	h2spwr(0x5F, 0x00);
	h2spwr(0x0F, 0x00);
	h2spwr(0xFF, 0xFF);

	return ret;
}
void caof_ini( void )
{
	iTE_u8 Reg08h;
	iTE_u16 Port0_Status;
	iTE_u8 Port0_DoneInt;
	iTE_u8 waitcnt;


    chgrxbank(0);
    // Port 0
	h2rxset(0x29, 0x01, 0x01);
    h2rxset(0x2A, 0x41, 0x41); // CAOF RST, inverse CAOFCLK

	chgrxbank(3);
	h2rxset(0x3A, 0x80, 0x00); // Reg_CAOFTrg low
	h2rxset(0x3B, 0xC0, 0x00); // Reg_ENSOF, Reg_ENCAOF
	h2rxset(0xA0, 0x80, 0x80);
	h2rxset(0xA1, 0x80, 0x80);
	h2rxset(0xA2, 0x80, 0x80);
    h2rxset(0xA7, 0x10, 0x10); // set Reg_PHSELEN high
    h2rxset(0x48, 0x80, 0x80); // for read back sof value registers

    chgrxbank(0);
	h2rxset(0x2A, 0x40, 0x00); // de-assert CAOF RST
	h2rxset(0x24, 0x04, 0x04); // IPLL RST
	h2rxwr(0x25, 0x00);        // Disable AFE PWD
	h2rxwr(0x26, 0x00);
    h2rxwr(0x27, 0x00);
	h2rxwr(0x28, 0x00);
    h2rxset(0x3C, 0x10, 0x00); //disable PLLBufRst
    // Port 0
	chgrxbank(3);
	h2rxset(0x3A, 0x80, 0x80); // Reg_CAOFTrg high



	// wait for INT Done
	chgrxbank(0);
    //Reg08h= h2rxrd(0x08)&0x30;
	waitcnt=0;
	//while (Reg08h==0x00)
	while(1)
	{
    	Reg08h= h2rxrd(0x08)&0x30;
    	if((Reg08h&0x30)==0x10)
    	{
    		iTE_MsgRX(("CAOF ok\n"));
    		break;
    	}
		//iTE_MsgRX(("Wait for CAOF Done!!!!!!\n"));
		if(waitcnt>2)
		{
        	h2rxset(0x2A, 0x40, 0x40); // reset CAOF
			h2rxset(0x2A, 0x40, 0x00);
		}
		waitcnt++;
		if(waitcnt>30)
		{
				iTE_MsgRX(("Skip CAOF  \r\n"));
				h2rxset(0x0F, 0x03, 0x03);
				h2rxset(0x3A, 0x80, 0x00);
				h2rxset(0x0F, 0x03, 0x00);
				h2rxset(0x2A, 0x40, 0x40);
				h2rxset(0x2A, 0x40, 0x00);
				break;
		}

	}
	chgrxbank(3);
	Port0_Status=(h2rxrd(0x5A)<<4)+(h2rxrd(0x59)&0x0F);
	Port0_DoneInt= h2rxrd(0x59)&0xC0;
	//iTE_Msg(("CAOF     CAOF    CAOF     CAOF    CAOF     CAOF \r\n"));
	//iTE_MsgRX(("Port 0 CAOF Int=%02x  ,CAOF Status=%02x \r\n",Port0_DoneInt,  Port0_Status));

	h2rxset(0x3A, 0x80, 0x00); // Reg_CAOFTrg low
	h2rxset(0xA0, 0x80, 0x00);
	h2rxset(0xA1, 0x80, 0x00);
	h2rxset(0xA2, 0x80, 0x00);
	// De-assert Port 0
	chgrxbank(0);
	h2rxset(0x08, 0x30, 0x30);
	h2rxset(0x29, 0x01, 0x00);
    h2rxset(0x24, 0x04, 0x00); // IPLL RST low
    h2rxset(0x3C, 0x10, 0x10); //Enable PLLBufRst
    h2rxset(0xCE, 0x20, 0x00); //NOT Enable 1.1 feature
}
void TimerInt(void) //ite_191219
{
	iTE_u8 rddata,i,reg14;

	i= 0;
	rddata = 0;
	h2spset(0x1A,0x02,0x00);
	h2spset(0x19,0x20,0x00);
	h2spwr(0x1D,0x00);
	reg14 = (h2rxrd(0x14)&0x38)>>3;
	if((reg14 == 0x00) && (h2rxrd(0x13)&0x10))
	{
		h2spset(0x1A,0x02,0x00);
		h2spset(0x19,0x20,0x00);
		h2spwr(0x1D,0x81);//ite_180208
		h2spset(0x19,0x20,0x20);
		h2spwr(0x07,0xFF);
		h2spset(0x1A,0x02,0x02);

		chgrxbank(3);
		rddata = h2rxrd(0xE5);
		if(rddata&0x0C)
		{
			h2rxset(0xE5,0x0C,0x00);
			i = 1;
		}
		else
		{
			h2rxset(0xE5,0x0C,0x0C);
			i = 2;
		}
		iTE_MsgRX(("FW Set RX H%d mode \r\n",i));
		h2rxset(0xE5,0x10,0x10);
		chgrxbank(0);
	}

}
//ITE_171212

#if 1
void TimerInt0(void)
{
	iTE_u8 rddata,i;
	rddata = 0;
	if(gext_u8->HPDTimerCnt)
	{
		gext_u8->HPDTimerCnt--;
		if(gext_u8->HPDTimerCnt == 0)
		{
			iTE_MsgRX(("Timer set HPD \r\n"));
			SetRxHpd(TRUE);
			if(gext_u8->BUSMODE == MHL) h2rxwr(0x26,0x0C);
			else h2rxwr(0x26,0x00);
		}
	}
	if(gext_u8->HDCPTimerCnt)
	{
		gext_u8->HDCPTimerCnt--;
		if(gext_u8->HDCPTimerCnt == 0)
		{
			chgspbank(1);
			rddata=h2sprd(0x23);
			chgspbank(0);
			if(rddata&0x10)
			{
				iTE_MsgRX(("ok \r\n"));
			}
			else
			{
				h2rxwr(0x26,0xFF);
				SetRxHpd(FALSE);
				gext_u8->HPDTimerCnt = 4;
			}
		}
	}

	if(HPD_Debounce == TRUE)
	{
		if(gext_u8->TimerIntFlag)
		{
			gext_u8->TimerIntFlag--;

			if(gext_u8->TimerIntFlag == 0)
			{
				#ifdef IT6664
					for(i=0;i<TxPortNum;i++)
				#else
					for(i=1;i<TxPortNum+1;i++)
				#endif
					{
						if((h2txrd(i,0x03)&0x01) == 0x01)
						{
							rddata = 1;
							break;
						}
			 		}
			 		if(!rddata)
			 		{
						SetRxHpd(FALSE);
						iTE_MsgRX(("Debounce set low \r\n"));
			 		}
			}
		}
	}

	for(i=0;i<4;i++)
	{
		if(gext_var->TXSCDC_chk[i])
		{
			if(gext_var->TXSCDC_chk[i]==1)
			{
				if(CheckSinkSCDC(i)==iTE_FALSE)
				{
					//h2txset(i, 0x88, 0x03, 0x03);//turn off tx oe
					gext_var->VideoState[i] = Tx_Video_Reset;
				}
				gext_var->TXSCDC_chk[i] = 0;
			}
			else
			{
				gext_var->TXSCDC_chk[i]--;
			}
		}
	}

}
//ITE_171212
iTE_u8 ReadStreamType(void)
{
	iTE_u8 ret=0,i;

	chgspbank(1);
	h2spwr(0x17,0x3A);
	ret = h2sprd(0x25);
	iTE_MsgRX(("Content type  is %2x \r\n",ret));
	chgspbank(0);
	gext_u8->ContentType = ret;
	gext_u8->GetSteamType = 1;

	if(gext_u8->ContentType)
	{
		for(i=0;i<4;i++)
		{
			if((gext_var->HDCPFireVer[i]==1)&&(gext_u8->CP_Done &(1<<i)))
			{
				h2txset(i, 0x88, 0x03, 0x03);
			}
		}

	}

	return ret;
}
#endif

void it6664_RX_ClockstbChk(void)
{
	iTE_u8 reg1,reg2,refsum,HS1P48G;

	reg1 = h2rxrd(0x43);
	reg2 = h2rxrd(0x48);
	reg1 = (reg1&0xF0)>>4;
	HS1P48G = (h2rxrd(0x14)&0x02)>>1;
	if(reg1==0)
	{
		refsum = reg2;
	}
	else
	{
		refsum = reg2/reg1;
	}
	// if rclk = 20, refsum = 34 at 148.5Mhz
	// if rclk = 21, refsum = 36 at 148.5Mhz
	// if rclk = 24, refsum = 41 at 148.5Mhz
	if((refsum<=41)&&(!HS1P48G)) // clock over 148.5 but reg14 is not
	{
		reg1 = h2rxrd(0x47);
		h2rxwr(0x47,reg1);
	}

}

#if 0
void IT6664_SetRxLED(iTE_u8  LED_SEL, iTE_u8 on)
{
	iTE_u8 Led_GPIO;
	#ifdef _MCU_IT6350
		iTE_u8 BitSet[2] = {0x08,0x10};// 0: clk ,1: hdcp
		Led_GPIO = GPDRB;
	#endif
	#ifdef _MCU_IT6295_
		iTE_u8 BitSet[2] = {0x10,0x20};// 0: clk ,1: hdcp
		Led_GPIO = GPDRA;
	#endif

	if(on)	Led_GPIO |= BitSet[LED_SEL];
	else	Led_GPIO &= ~BitSet[LED_SEL];

	#ifdef _MCU_IT6350
		GPDRB = Led_GPIO;
	#endif
	#ifdef _MCU_IT6295_
		GPDRA = Led_GPIO;
	#endif

}
#endif



#if 0
void hdmi2_work(void)
{
	iTE_u8 reg1,reg2,reg3;
	static iTE_u32 cnt = 0;
	//static iTE_u8 tmp = 0;
	//static iTE_u8 tmp1 = 0;
	static iTE_u8 tmp2 = 0;

	#if 0
		iTE_u8 Reg74h,CHB_SKEW,CHG_SKEW,CHR_SKEW;
	#endif
	if(cnt>0x100)
	{
		cnt = 0;
		#ifndef repeater
		{
			iTE_u8 port;
			for(port=0;port<4;port++)
			{
				if(gext_u8->CP_Done &(1<<port))
				{
					iTE_MsgHDCP(("-----------------  P%d  --------------- \r\n",(iTE_u16)port));
					hdcpsts(gext_var->HDCPFireVer[port],port);
					iTE_MsgHDCP(("-----------------  P%d  --------------- \r\n",(iTE_u16)port));
				}
			}
		}
		#endif
		#if 1
		if(h2rxrd(0x14)&0x38)
		{
			chgrxbank(3);
			reg1 = h2rxrd(0x27);
			reg2 = h2rxrd(0x28);
			reg3 = h2rxrd(0x29);

			iTE_MsgRX(("------------- RX CED check ------------ \r\n"));
			iTE_MsgRX(("B/G/R  = 0x%02x  0x%02x 0x%02x \r\n",reg1,reg2,reg3));
			#if 0
				iTE_MsgRX(("B_DFE  = 0x%02x  0x%02x 0x%02x\r\n",h2rxrd(0x4B),h2rxrd(0x4C),h2rxrd(0x4D)));
				iTE_MsgRX(("G_DFE  = 0x%02x  0x%02x 0x%02x\r\n",h2rxrd(0x4E),h2rxrd(0x4F),h2rxrd(0x50)));
				iTE_MsgRX(("R_DFE  = 0x%02x  0x%02x 0x%02x\r\n",h2rxrd(0x51),h2rxrd(0x52),h2rxrd(0x53)));
				Reg74h= h2rxrd(0x74);

				CHB_SKEW=(Reg74h&0x03);
				CHG_SKEW=((Reg74h&0x0C)>>2);
				CHR_SKEW=((Reg74h&0x30)>>4);

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
			chgrxbank(0);
			Check_BitErr();
			iTE_MsgRX(("--------------------------------------- \r\n"));
		}
		#endif
	}
	else
	{
		cnt++;
	}
	#ifdef Disable_RXHDCP
		if (!(KSI&0x40))//  GPIO setting here
		{
			it6664_RXHDCP_OFF(TRUE);
		}
		else
		{
			it6664_RXHDCP_OFF(FALSE);
		}
	#endif

	#ifdef DS_Switch
		iTE_u8 port;
		reg1 = 0;
		//Set  GPIO define here
		//------------------
		//reg1 |= GPIO0<<0;
		//reg1 |= GPIO1<<1;
		//reg1 |= GPIO2<<2;
		//reg1 |= GPIO3<<3;
		//------------------
		for(port = 0;port<4;port++)
		{
			if (reg1&(1<<port))//  GPIO check status here
			{
				if((gext_var->TxSrcSel[port] != ds)&&(gext_u8->TXHPDsts & (1<<port))&&(!gext_var->TxOut_DS[port]))
				{
					gext_var->TxOut_DS[port] = 1;
					gext_var->VideoState[port] = Tx_Video_Reset;
					iTE_MsgRX(("---------   Set  DS ------- \r\n"));
				}
			}
			else
			{
				if((gext_var->TxSrcSel[port] == ds)&&(gext_u8->TXHPDsts & (1<<port))&&(gext_var->TxOut_DS[port]==1))
				{
					gext_var->TxOut_DS[port] = 0;
					gext_var->VideoState[port] = Tx_Video_Reset;
					iTE_MsgRX(("---------   Set  Resume ------- \r\n"));
				}
			}
		}
	#endif
	#if 0
	if(cnt%30==0)//for mi box
	{
		if((h2rxrd(0x13)&0x10)&&(h2rxrd(0x14)&0x02))//clock stable && over 1.5G
		{
			iTE_u16  HActive =0;
			HActive = ((h2rxrd(0x9E)&0x3F)<<8)+h2rxrd(0x9D);

			reg1 = h2rxrd(0x1A);
		//	if(reg1)
		//	{
		//		iTE_MsgRX(("0x1A = %2x\r\n",reg1));
		//	}
			if(reg1 && ((HActive == 3840) || (HActive == 4096)))
			{
				if((!tmp)|| ((h2rxrd(0x12)&0x80)))
				{
					h2spwr(0x0B, 0xFF); //Reset FIFO
					h2spwr(0x0B, 0x00);
					//Reset All fifo path
					h2spset(0x4E,0x0F,0x0F);
					h2spset(0x0C,0x08,0x08);
					h2spset(0x0C,0x08,0x00);
					h2spset(0x4E,0x0F,0x00);
					h2rxwr(0x12,0xFF);
					iTE_MsgRX(("Reset all fifo for 4k ds\r\n"));
				}
				tmp = 1;
			}
			else
			{
				tmp = 0;
			}
			if(tmp1 != h2rxrd(0x1A))
			{
				tmp = 0;
			}
			tmp1 = h2rxrd(0x1A);
			//iTE_MsgRX(("tmp1 = %2x tmp = %2x\r\n",tmp1,tmp));
		}
	}
	#endif
	if(gext_u8->SDI_clkstbChk)
	{
		if(cnt%30 == 0)
		{
			tmp2++;
			if(tmp2>8)
			{
				tmp2 = 0;
				it6664_RX_ClockstbChk();
				iTE_MsgRX(("----------clkstbChk---------- \r\n"));
			}
		}
	}
	else
	{
		tmp2 = 0;
	}
}

#endif

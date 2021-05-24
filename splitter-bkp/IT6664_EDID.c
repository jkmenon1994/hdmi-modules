///*****************************************
//  Copyright (C) 2009-2021
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <IT6664_EDID.c>
//   @author Hojim.Tseng@ite.com.tw
//   @date   2021/01/12
//   @fileversion: ITE_HDMI2_SPLITER_1.38
//******************************************/
#include "IT6664_EDID.h"
#include "Edid_DtdInfo.h"

#include <linux/delay.h>

#define mSleep(x) msleep(x)


#if (USING_1to8==TRUE)
EDIDBlock0 EDID0data[8];//0x6
EDIDBlock1 EDID1data[8];//0x4a
iTE_u8 SupportVesaDTD[8];
static iTE_u8 HPD[8] = {0,0,0,0,0,0,0,0};
#else
EDIDBlock0 EDID0data[4];//0x6
EDIDBlock1 EDID1data[4];//0x4a
iTE_u8 SupportVesaDTD[4];
static iTE_u8 HPD[4] = {0,0,0,0};
#endif
extern extern_variables *gext_var;
extern extern_u8 *gext_u8;
extern iTE_u8 g_device;
iTE_u8 Compose_block0[128];
iTE_u8 Compose_block1[128];
iTE_u8 Device_off;
iTE_u8 _CODE EDID_Block0FIX[52] =
{
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
	0x26, 0x85, 0x64, 0x66, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1B,
	0x01, 0x03,
	0x80, 0x5F, 0x36, 0x78, 0x0A,
	0x23, 0xAD, 0xA4, 0x54, 0x4D, 0x99, 0x26, 0x0F, 0x47, 0x4A,
	0x00, 0x00, 0x00, 0xFC, 0x00, 0x49, 0x54, 0x45, 0x2D, 0x53,
	0x70, 0x6C, 0x69, 0x74, 0x74, 0x65,0x72
};

#ifdef EDID_Compose_Intersection
#ifdef IT6663
iTE_u8 ProductName[2][0x50];
#else
iTE_u8 ProductName[4][0x50];
#endif
#endif

void it6664_EdidMode_Switch(void)
{
	iTE_u8 hpdsts,u8Temp;

	#if (USING_1to8==TRUE)
		iTE_MsgTX((" \r\n"));
		iTE_MsgTX(("~~~~~~~~~~~~~~~EdidMode_Switch~~~~~~~~~~~~~~~~~ \r\n"));
		iTE_MsgTX(("g_device = %02x \r\n",g_device));
		if(g_device == IT6664_A)	Device_off = 0;
		else if(g_device == IT6664_B)	Device_off = 1;
	#else
		Device_off = 0;
	#endif
	hpdsts = it6664_Check_HPDsts();
	if(hpdsts)
	{
		it6664_Edid_Parse(hpdsts);
	}
	#ifdef _MCU_IT6350
		u8Temp = GPDRG & 0x80;//j30
		u8Temp |= GPDRC & 0x20;//j26
	#endif

	#ifdef _MCU_IT6295
		u8Temp = 0;
		if((GPDRE & 0x02) == 0x02)//j5 = 1
		{
			u8Temp |= Compose_Mode;
		}
		if((GPDRE & 0x04) == 0x04)//j6 = 1
		{
			u8Temp |= Copy_Mode;
		}
	#endif

	if ((EnIntEDID == 1))//trapping 3
	{
		switch(u8Temp & 0xA0)
		{
			case Default_EDID2k:
					iTE_Edid_Msg((">>> Default_EDID 2k<<<\r\n"));
					if(hpdsts) SetRxHpd(TRUE);
					#ifdef Support_CEC
					Cec_SysInit(0x03,0xFF);
					#endif
					break;
			case Default_EDID4k:
					iTE_Edid_Msg((">>> Default_EDID 4k<<<\r\n"));
					if(hpdsts) SetRxHpd(TRUE);
					#ifdef Support_CEC
					Cec_SysInit(0x03,0xFF);
					#endif
					break;
			case Copy_Mode:
					iTE_Edid_Msg((">>> Copy_Mode <<<\r\n"));
					#if (USING_1to8==TRUE)
						if(hpdsts && (g_device == 0x21)) it6664_Edid_Copy(hpdsts);
					#else
						if(hpdsts) it6664_Edid_Copy(hpdsts);
					#endif
					break;
			case Compose_Mode:
					iTE_Edid_Msg((">>> Compose_Mode <<<\r\n"));
					if(hpdsts) it6664_Edid_Compose();
					break;

			#ifdef EDID_Compose_Intersection
			case 0xFF:
					iTE_Edid_Msg((">>> Compose_Mode Intersection <<<\r\n"));
					if(hpdsts) it6664_Edid_Compose_Int();
					break;
			#endif
			default:
					break;
		}
	}
	else
	{
		iTE_Edid_Msg((">>> Use_EXT_EDID <<<\r\n"));
		if(hpdsts) SetRxHpd(TRUE);
		h2rxset(0xC5, 0x01, 0x01);
	}
	#if (USING_1to8==TRUE)
		iTE_MsgTX(("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \r\n"));
		iTE_MsgTX((" \r\n"));
	#endif
}
iTE_u8 it6664_Edid_Parse(iTE_u8 HPDsts)
{
	iTE_u8 i,j,ret,ret1,tmp0[128],tmp1[128],tmp2[128],edidok,block0ok=0,block1ok=0,retry;
	ret = 0;
	ret1 = 0;
	retry = 0;
	edidok = HPDsts;

	#ifdef IT6664
		for(i=0;i<TxPortNum;i++)
	#else
		for(i=1;i<TxPortNum+1;i++)
	#endif
		{
			__RETRY:
			#if (USING_1to8 == TRUE)
			if((HPDsts & (1<<i)) && ((gext_u8->TXH2RSABusy&(1<<i))==FALSE))
			#else
			if((HPDsts & (1<<i)) && ((gext_u8->TXH2RSABusy&(1<<i))==FALSE)&&((PortSkipEdid_opt & (1<<i)) == 0))
			#endif
			{
				iTE_Edid_Msg(("**** P%d Read EDID ******************************** \r\n",(iTE_u16)i));
				ret = it6664_read_one_block_edid(i,0,tmp0);
				if(!ret)
				{
					edidok &= ~(1<<i);
					block0ok = 0;
					iTE_Edid_Msg(("Read P%d EDID0 fail ... \r\n",(iTE_u16)i));
				}
				if((tmp0[0x00] == 0x00) && (tmp0[0x01] == 0xFF)
					&&(tmp0[0x02] == 0xFF) && (tmp0[0x03] == 0xFF)
					&&(tmp0[0x04] == 0xFF) && (tmp0[0x05] == 0xFF)
					&&(tmp0[0x06] == 0xFF) && (tmp0[0x07] == 0x00))
				{
					block0ok = 1;
					if(!tmp0[0x7E]) gext_var->DVI_mode[i] = 1;
				}
				else
				{
					iTE_Edid_Msg(("P%d EDID0 Header fail ... \r\n",(iTE_u16)i));
					block0ok = 0;
				}
				if((tmp0[0x7E]==0x01) && ret)
				{
					ret1 = it6664_read_one_block_edid(i,1,tmp1);
					if(!ret1)
					{
						edidok &= ~(1<<i);
						iTE_Edid_Msg(("Read P%d EDID1 fail ... \r\n",(iTE_u16)i));
					}
					if((tmp1[0x00] == 0x02) && (tmp1[0x01] == 0x03))
					{
						block1ok = 1;
						if(!tmp0[0x7E]) gext_var->DVI_mode[i] = 0;
					}
					else
					{
						//iTE_Edid_Msg(("P%d DVI use extension block... \r\n",(iTE_u16)i));
						gext_var->DVI_mode[i] = 1;
						ret1 = 0;
						block1ok = 1;
					}
				}
				else
				{
					if(tmp0[0x7E]==0x03)// for ATC  4 block EDID test
					{
						it6664_read_one_block_edid(i,1,tmp2);
						ret1 = it6664_read_one_block_edid(i,2,tmp1);
						it6664_read_one_block_edid(i,3,tmp2);
						if((tmp1[0x00] == 0x02) && (tmp1[0x01] == 0x03))
						{
							block1ok = 1;
						}
					}
				}

				if(ret) it6664_Edid_block0_parse(i,tmp0);
				if(ret1) it6664_Edid_block1_parse(i,tmp1);

				if((block0ok && block1ok) || (block0ok && (!tmp0[0x7E])))
				{
					//iTE_Edid_Msg(("**** P%d EDIDParseDone \r\n",(iTE_u16)i));
					gext_var->EDIDParseDone[i] = 1;
					HPD[i+Device_off*4] = 1;
					retry = 0;
				}
				else
				{
					HPD[i+Device_off*4] = 0;
					iTE_Edid_Msg(("**** P%d Read EDID fail \r\n",(iTE_u16)i));
					gext_var->EDIDParseDone[i] = 0;
					h2txset(i, 0x35, 0x10, 0x10);
					h2txset(i, 0x35, 0x10, 0x00);
					h2txset(i, 0x28, 0x01, 0x01);
					h2txset(i, 0x28, 0x01, 0x00);
					retry++;
					if(retry<3) goto __RETRY;
					else
					{
						retry = 0;
						gext_u8->TXHPDsts &= ~(1<<i);
					}

				}
				iTE_Edid_Msg(("P%d TXSupport420 = %02x  	 \r\n",(iTE_u16)i,gext_var->TXSupport420[i]));
				iTE_Edid_Msg(("P%d TXSupportOnly420 = %02x   \r\n",(iTE_u16)i,gext_var->TXSupportOnly420[i]));
				iTE_Edid_Msg(("P%d TXSupport4K60 = %02x  	 \r\n",(iTE_u16)i,gext_var->TXSupport4K60[i]));
				iTE_Edid_Msg(("P%d TXSupport1080p60 = %02x  \r\n",(iTE_u16)i,gext_var->TXSupport1080p[i]));
				iTE_Edid_Msg(("P%d TXSupport4K30 = %02x  	 \r\n",(iTE_u16)i,gext_var->TXSupport4K30[i]));
				iTE_Edid_Msg(("P%d TXDVIMode = %02x  	 \r\n",(iTE_u16)i,gext_var->DVI_mode[i]));
				iTE_Edid_Msg(("************************************************** \r\n"));
			}
			ret = 0;
			ret1 = 0;
			block1ok = 0;
			block0ok = 0;
			for(j=0;j<128;j++)
			{
				tmp0[j]=0;
				tmp1[j]=0;
				tmp2[j]=0;
			}
		}

	return edidok;
}
void it6664_Edid_Copy(iTE_u8 hpdsts)
{
	iTE_u8 tmp[128],port,ret=0,ret1=0,retry=0,ret_edid0=0,ret_edid1=0,blk1,device;
	iTE_u16 i,sum;
	iTE_u8  GPIO_Compose,GPIO_Copy;

	//iTE_Edid_Msg(("hpdsts = %d \r\n",(iTE_u16)hpdsts));
	//iTE_Edid_Msg(("g_device = %02x \r\n",g_device));
	blk1 = 0;
	device = g_device;
	IT6664_DeviceSelect(IT6663_C);
	h2rxset(0x34, 0x01, 0x00);  //emily 20161222 Enable ERCLK even no 5VDet
	IT6664_DeviceSelect(device);

	#ifdef _MCU_IT6350
		GPIO_Compose = GPDRC & 0x20;
		GPIO_Copy = GPDRG & 0x80;
	#endif
	#ifdef _MCU_IT6295
		GPIO_Compose = GPDRE & 0x02;
		GPIO_Copy = GPDRE & 0x04;
	#endif

	if((hpdsts & (1<<EDID_CopyPx)) || (GPIO_Compose))
	{
		if(GPIO_Compose)
		{
			port = 0;
			while(1)
			{
				if(hpdsts <= 1)
				{
					break;
				}
				else
				{
					hpdsts = hpdsts/2;
					port++;
				}
			}
		}
		else //for copy mode
		{
			#if (USING_1to8==TRUE)
				IT6664_DeviceSelect(IT6664_B);
				SetRxHpd(TRUE);
			#endif
			port = EDID_CopyPx;
		}

		if(!gext_u8->EDIDCopyDone)
		{
			__RETRY:
			IT6664_DeviceSelect(IT6663_C);
			iTE_Edid_Msg(("EDID Copy Port%d \r\n",(iTE_u8)port));
			h2rxwr(0xC6, 0x00); //VSDB Start Address
			h2rxwr(0xC7, EDID1data[port].vsdb1.phyaddr0); //AB
  			h2rxwr(0xC8, EDID1data[port].vsdb1.phyaddr1); //CD
  			h2rxwr(0x4B, 0xD9); //EDID SlaveAdr
  			#if (USING_1to8==TRUE)
				if(GPIO_Copy)
				{
					IT6664_DeviceSelect(IT6664_B);
				}
				else
				{
					IT6664_DeviceSelect(device);
				}
			#endif
			if(it6664_read_one_block_edid(port,0,tmp) == FALSE)
			{
				ret_edid0 = 0;
				iTE_Edid_Msg(("Read P%d EDID fail ... \r\n",(iTE_u16)port));
			}
			else
			{
				ret_edid0 = 1;
				IT6664_DeviceSelect(IT6663_C);

				if(gext_var->TXSupport4K30[port]==1)
				{
					it6664_Edid_Chk_4KDTD(tmp,0);
					tmp[0x7F] = CalChecksum(tmp,0);
				}
				#ifdef CopyMode_Remove4K533
				EDID_Remove4K533(tmp,0);
				#endif
				sum = CalChecksum(tmp,0);
				if(sum == tmp[0x7F])
				{
					h2rxwr(0xC9,tmp[0x7F]);//bank0  checksum
					ret = it6664_EDIDCompare(tmp,Compose_block0);
					for(i=0;i<0x7F;i++)
					{
						h2rx_edidwr(i,tmp[i]);
						Compose_block0[i] = tmp[i];
					}
					blk1 = tmp[0x7E];
				}
				else
				{
					if((tmp[0x00] == 0x00) && (tmp[0x01] == 0xFF)
						&&(tmp[0x02] == 0xFF) && (tmp[0x03] == 0xFF)
						&&(tmp[0x04] == 0xFF) && (tmp[0x05] == 0xFF)
						&&(tmp[0x06] == 0xFF) && (tmp[0x07] == 0x00))
					{
						h2rxwr(0xC9,sum);//block0  checksum
						for(i=0;i<0x7F;i++) h2rx_edidwr(i,tmp[i]);
						blk1 = tmp[0x7E];
						//iTE_Edid_Msg(("Port%d block0 checksum update \r\n",(iTE_u8)port));
					}
					else
					{
						iTE_Edid_Msg(("Port%d block0 invalid ! use default EDID \r\n",(iTE_u8)port));
					}

				}
			}
			#if (USING_1to8==TRUE)
				if(GPIO_Copy)
				{
					IT6664_DeviceSelect(IT6664_B);
				}
				else
				{
					IT6664_DeviceSelect(device);
				}
			#endif
			if(it6664_read_one_block_edid(port,1,tmp)==FALSE)
			{
				ret_edid1 = 0;
				iTE_Edid_Msg(("Read P%d EDID fail ... \r\n",(iTE_u16)port));
			}
			else
			{
				ret_edid1 =1;
				IT6664_DeviceSelect(IT6663_C);
				if(blk1)
				{
					//iTE_Edid_Msg(("EDID_48Bit_Remove ok ... \r\n"));
					if(gext_var->TXSupport4K30[port]==1)
					{
						it6664_Edid_Chk_4KDTD(tmp,1);
						tmp[0x7F] = CalChecksum(tmp,0);
					}
					EDID_48Bit_Remove(tmp);
					EDID_HDMI21_Remove(tmp);
					EDID_HDMI21_VicRemove(tmp);
					it6664_Edid_PhyABCD_Update(tmp);
					#ifdef CopyMode_Remove4K533
					EDID_Remove4K533(tmp,1);
					#endif
					sum = CalChecksum(tmp,0);
					if(sum == tmp[0x7F])
					{
						ret1 = it6664_EDIDCompare(tmp,Compose_block1);
						for(i=0;i<0x7F;i++)
						{
							h2rx_edidwr(i+0x80,tmp[i]);
							Compose_block1[i] = tmp[i];
						}
						h2rxwr(0xCA,tmp[0x7F]);//bank1  checksum
					}
					else
					{
						if((tmp[0x00] == 0x02) && (tmp[0x01] == 0x03))
						{
							for(i=0;i<0x7F;i++) h2rx_edidwr(i+0x80,tmp[i]);
							h2rxwr(0xCA,sum);//bank1  checksum
							//iTE_Edid_Msg(("Port%d block1 checksum update \r\n",(iTE_u8)port));
						}
					}
				}
			}
			//iTE_Edid_Msg(("EDID Copy Port%d end \r\n",(iTE_u8)port));
			#if (USING_1to8==TRUE)
				if(GPIO_Copy)
				{
					IT6664_DeviceSelect(IT6664_B);
				}
				else
				{
					IT6664_DeviceSelect(device);
				}
			#endif
			gext_u8->EDIDCopyDone = 1;

			if(gext_var->DVI_mode[port])
			{
				IT6664_DeviceSelect(IT6663_C);
				chgspbank(1);
				h2spset(0x10,0x40,0x00);
				chgspbank(0);
			}
			else
			{
				IT6664_DeviceSelect(IT6663_C);
				chgspbank(1);
				h2spset(0x10,0x40,0x40);
				chgspbank(0);
			}
			if((!ret_edid0) || (blk1&&(!ret_edid1)))
			{
				retry++;
				if(retry<2) goto __RETRY;
				else
				{
					retry = 0;
					#if (USING_1to8==TRUE)
						if(GPIO_Copy)
						{
							IT6664_DeviceSelect(IT6664_B);
						}
						else
						{
							IT6664_DeviceSelect(device);
						}
					#endif
					gext_u8->TXHPDsts &= ~(1<<i);
					HPD[EDID_CopyPx] = 0;
					gext_var->EDIDParseDone[port] = 0;
					#if (USING_1to8==TRUE)
						IT6664_DeviceSelect(IT6663_C);
					#endif

				}
			}
			if(ret1 || ret)
			{
				//toggle RX HPD
				h2rxset(0xC5, 0x01, 0x01);
				SetRxHpd(FALSE);
				it6664_h2rx_pwdon();
				mSleep(2000);
				SetRxHpd(TRUE);
				h2rxset(0xC5, 0x01, 0x00);
			}
		}
	}
	#ifdef Support_CEC
		Cec_SysInit(0x03,port);// update RX physical address
	#endif
	h2rxset(0xC5, 0x01, 0x00);
	h2rxset(0x34, 0x01, 0x01);
	#if (USING_1to8==TRUE)
		if(GPIO_Copy)
		{
			IT6664_DeviceSelect(IT6664_B);
		}
		else
		{
			IT6664_DeviceSelect(device);
		}
	#endif
}
#ifdef EDID_Compose_Intersection
void it6664_Edid_Compose_Int(void)
{
	iTE_u8 i,tmp[128],compose,cnt,cnt1;
	iTE_u8 checksum,ret,ret1;

	compose = 0;
	cnt =0;
	cnt1 = 0;
	#if (USING_1to8==TRUE)
		iTE_u8 dev_bak;
		dev_bak = g_device;
		IT6664_DeviceSelect(IT6664_A);
		for(i=0;i<TxPortNum;i++)
		{
			if(gext_var->EDIDParseDone[i]) cnt++;
			if(gext_u8->TXHPDsts &(1<<i)) cnt1++;
		}
		IT6664_DeviceSelect(IT6664_B);
		for(i=0;i<TxPortNum;i++)
		{
			if(gext_var->EDIDParseDone[i]) cnt++;
			if(gext_u8->TXHPDsts &(1<<i)) cnt1++;
		}
		if(cnt>1) compose = 1;
		IT6664_DeviceSelect(dev_bak);
	#else
		#ifdef IT6664
			for(i=0;i<TxPortNum;i++)
		#else
			for(i=1;i<TxPortNum+1;i++)
		#endif
		{
			if(gext_var->EDIDParseDone[i]) cnt++;
			if(gext_u8->TXHPDsts &(1<<i)) cnt1++;
		}
		if(cnt<2) compose = 0;
		else
		{
			if(cnt == cnt1) compose = 1;
		}
		#ifdef EDID_Compose_Intersection
		/*
			if(CompareProductID() == TRUE)
			{
				compose = 0;
				cnt1 = 1;
			}
		*/
		#endif
	#endif

	if(compose)
	{
		IT6664_DeviceSelect(IT6663_C);
		h2rxset(0x34, 0x01, 0x00);  //emily 20161222 Enable ERCLK even no 5VDet
		h2rxwr(0xC6, 0x00); //VSDB Start Address
		h2rxwr(0xC7, 0x00); //AB
    	h2rxwr(0xC8, 0xFF); //CD
    	h2rxwr(0x4B, 0xD9); //EDID SlaveAdr

		it6664_block0_compose_int(tmp);
		ret = it6664_EDIDCompare(tmp,Compose_block0);
		IT6664_DeviceSelect(IT6663_C);
		for(i=0;i<0x7F;i++)
		{
			h2rx_edidwr(i,tmp[i]);
			if(ret) Compose_block0[i] = tmp[i];
		}
		checksum = CalChecksum(tmp,0);
		Show_EDID(tmp);
		h2rxwr(0xC9,checksum);//block0  checksum
		Compose_block0[127] = checksum;
		for(i=0;i<0x80;i++) tmp[i] = 0;

		#ifdef IT6663 //for 1 to 2
			if((!gext_var->DVI_mode[1]) && (!gext_var->DVI_mode[2]))
			{
				it6664_block1_compose_int(tmp);
			}
			else
			{
				if(gext_var->DVI_mode[1])
				{
					it6664_read_one_block_edid(2,1,tmp);
				}
				else if(gext_var->DVI_mode[2])
				{
					it6664_read_one_block_edid(1,1,tmp);
				}
			}
		#else //for 1to 4 and 1 to 8
			it6664_block1_compose_int(tmp);
		#endif
		it6664_Edid_PhyABCD_Update(tmp);
		EDID_HDMI21_VicRemove(tmp);
		ret1 = it6664_EDIDCompare(tmp,Compose_block1);
		IT6664_DeviceSelect(IT6663_C);
		for(i=0;i<0x7F;i++)
		{
			h2rx_edidwr(i+0x80,tmp[i]);
			if(ret1) Compose_block1[i] = tmp[i];
		}
		checksum = CalChecksum(tmp,0);
		tmp[0x7F] = checksum;
		h2rxwr(0xCA,checksum);//block1  checksum
		Show_EDID(tmp);
		//h2rxset(0x40,0x01,0x01);
		if(ret1 || ret)
		{
			chgspbank(1);
			h2spset(0x10,0x40,0x40);
			chgspbank(0);
			//toggle RX HPD
			h2rxset(0xC5, 0x01, 0x01);
			SetRxHpd(FALSE);
			it6664_h2rx_pwdon();
			mSleep(2000);
			SetRxHpd(TRUE);
			h2rxset(0xC5, 0x01, 0x00);
		}
		h2rxset(0xC5, 0x01, 0x00);
		h2rxset(0x34, 0x01, 0x01);
		#if (USING_1to8==TRUE)
			IT6664_DeviceSelect(IT6664_B);
			gext_u8->EDIDCopyDone = 0;
			IT6664_DeviceSelect(dev_bak);
		#else
			gext_u8->EDIDCopyDone = 0;
		#endif
		iTE_Edid_Msg(("EDID Compose end \r\n"));
	}
	else
	{
		if((!gext_u8->EDIDCopyDone) && (cnt1==1))
		{
			it6664_Edid_Copy(gext_u8->TXHPDsts);
		}
	}
}
#endif
void it6664_Edid_Compose(void)
{
	iTE_u8 i,tmp[128],compose,cnt,cnt1;
	iTE_u8 checksum,ret,ret1;

	compose = 0;
	cnt =0;
	cnt1 = 0;
	#if (USING_1to8==TRUE)
		iTE_u8 dev_bak;
		dev_bak = g_device;
		IT6664_DeviceSelect(IT6664_A);
		for(i=0;i<TxPortNum;i++)
		{
			if(gext_var->EDIDParseDone[i]) cnt++;
			if(gext_u8->TXHPDsts &(1<<i)) cnt1++;
		}
		IT6664_DeviceSelect(IT6664_B);
		for(i=0;i<TxPortNum;i++)
		{
			if(gext_var->EDIDParseDone[i]) cnt++;
			if(gext_u8->TXHPDsts &(1<<i)) cnt1++;
		}
		if(cnt>1) compose = 1;
		IT6664_DeviceSelect(dev_bak);
	#else
		#ifdef IT6664
			for(i=0;i<TxPortNum;i++)
		#else
			for(i=1;i<TxPortNum+1;i++)
		#endif
		{
			if(gext_var->EDIDParseDone[i]) cnt++;
			if(gext_u8->TXHPDsts &(1<<i)) cnt1++;
		}
		if(cnt<2) compose = 0;
		else
		{
			if(cnt == cnt1) compose = 1;
		}
		#ifdef EDID_Compose_Intersection
		/*
			if(CompareProductID() == TRUE)
			{
				compose = 0;
				cnt1 = 1;
			}
		*/
		#endif
	#endif

	if(compose)
	{
		IT6664_DeviceSelect(IT6663_C);
		h2rxset(0x34, 0x01, 0x00);  //emily 20161222 Enable ERCLK even no 5VDet
		h2rxwr(0xC6, 0x00); //VSDB Start Address
		h2rxwr(0xC7, 0x00); //AB
    	h2rxwr(0xC8, 0xFF); //CD
    	h2rxwr(0x4B, 0xD9); //EDID SlaveAdr

		it6664_block0_compose(tmp);
		ret = it6664_EDIDCompare(tmp,Compose_block0);
		IT6664_DeviceSelect(IT6663_C);
		for(i=0;i<0x7F;i++)
		{
			h2rx_edidwr(i,tmp[i]);
			if(ret) Compose_block0[i] = tmp[i];
		}
		checksum = CalChecksum(tmp,0);
		Show_EDID(tmp);
		h2rxwr(0xC9,checksum);//block0  checksum
		Compose_block0[127] = checksum;
		for(i=0;i<0x80;i++) tmp[i] = 0;

		#ifdef IT6663 //for 1 to 2
			if((!gext_var->DVI_mode[1]) && (!gext_var->DVI_mode[2]))
			{
				it6664_block1_compose(tmp);
			}
			else
			{
				if(gext_var->DVI_mode[1])
				{
					it6664_read_one_block_edid(2,1,tmp);
				}
				else if(gext_var->DVI_mode[2])
				{
					it6664_read_one_block_edid(1,1,tmp);
				}
			}
		#else //for 1to 4 and 1 to 8
			it6664_block1_compose(tmp);
		#endif
		it6664_Edid_PhyABCD_Update(tmp);
		ret1 = it6664_EDIDCompare(tmp,Compose_block1);
		IT6664_DeviceSelect(IT6663_C);
		for(i=0;i<0x7F;i++)
		{
			h2rx_edidwr(i+0x80,tmp[i]);
			if(ret1) Compose_block1[i] = tmp[i];
		}
		checksum = CalChecksum(tmp,0);
		tmp[0x7F] = checksum;
		h2rxwr(0xCA,checksum);//block1  checksum
		Show_EDID(tmp);
		//h2rxset(0x40,0x01,0x01);
		if(ret1 || ret)
		{
			chgspbank(1);
			h2spset(0x10,0x40,0x40);
			chgspbank(0);
			//toggle RX HPD
			h2rxset(0xC5, 0x01, 0x01);
			SetRxHpd(FALSE);
			it6664_h2rx_pwdon();
			mSleep(2000);
			SetRxHpd(TRUE);
			h2rxset(0xC5, 0x01, 0x00);
		}
		h2rxset(0xC5, 0x01, 0x00);
		h2rxset(0x34, 0x01, 0x01);
		#if (USING_1to8==TRUE)
			IT6664_DeviceSelect(IT6664_B);
			gext_u8->EDIDCopyDone = 0;
			IT6664_DeviceSelect(dev_bak);
		#else
			gext_u8->EDIDCopyDone = 0;
		#endif
		iTE_Edid_Msg(("EDID Compose end \r\n"));
	}
	else
	{
		if((!gext_u8->EDIDCopyDone) && (cnt1==1))
		{
			it6664_Edid_Copy(gext_u8->TXHPDsts);
		}
	}
}
#ifdef EDID_Compose_Intersection
iTE_u8 CompareProductID(void)
{
	iTE_u8 i,j,ref,ret;
	ret = 1;

	#ifdef IT6664
		for(i=0;i<TxPortNum;i++)
	#else
		for(i=1;i<TxPortNum+1;i++)
	#endif
		{
			if(HPD[i])
			{
				ref = i;
				break;
			}
		}
	#ifdef IT6664
		for(i=0;i<TxPortNum;i++)
	#else
		for(i=1;i<TxPortNum+1;i++)
	#endif
		{
			if(HPD[i])
			{
				for(j=0;j<10;j++)
				{
					if(ProductName[i][j] != ProductName[ref][j])
					{
						ret = 0;
						break;
					}
				}
			}
		}
	return ret;
}
#endif
void it6664_block0_compose(iTE_pu8 arry)
{
	#if (USING_1to8==FALSE)
		iTE_u8 port,ref=0,cnt=0,STD[16];
	#endif
	iTE_u8 i,EST[3],j,tmpdata=0;
	iTE_u16 sum ,checksum;
	for(i=0;i<35;i++) *(arry+i) = EDID_Block0FIX[i];

	//Est
	#if (USING_1to8==TRUE)
		iTE_u8 tmp;
		for(j=0;j<3;j++)
		{
			for(i=0;i<8;i++)
			{
				if(HPD[i])
				{
					tmpdata &= EDID0data[i].EstablishTimeing[j];
				}
			}
			EST[j] = tmpdata;
			tmpdata = 0;
		}
		for(i=0;i<3;i++) *(arry+0x23+i) = EST[i];
		for(i=0;i<16;i++) *(arry+0x26+i) = 0x01;//STD
		for(i=0;i<8;i++)
		{
			if(HPD[i])
			{
				if(i<4)
				{
					if(g_device != IT6664_B) IT6664_DeviceSelect(IT6664_B);
					tmp = i;
				}
				if(i>3)
				{
					if(g_device != IT6664_A) IT6664_DeviceSelect(IT6664_A);
					tmp = i-4;
				}
				if(gext_var->TXSupport4K60[tmp])
				{
					for(j=1;j<19;j++) 	*(arry+0x36+j-1) = Dtd18Byte[2][j];//DTD2 4K60
					break;
				}
			}
		}
	#else
		for(j=0;j<3;j++)
		{
			#ifdef IT6664
				for(i=0;i<TxPortNum;i++)
			#else
				for(i=1;i<TxPortNum+1;i++)
			#endif
				{
					if(HPD[i])
					{
						tmpdata &= EDID0data[i].EstablishTimeing[j];
					}
				}
			EST[j] = tmpdata;
			tmpdata = 0;
		}
		for(i=0;i<3;i++) *(arry+0x23+i) = EST[i];
		#ifdef IT6664
			for(port=0;port<TxPortNum;port++)
		#else
			for(port=1;port<TxPortNum+1;port++)
		#endif
			{
				if(HPD[port])
				{
					ref = port;
					break;
				}
			}
		for(i=0;i<8;i++)//ref 8
		{
			#ifdef IT6664
				for(port=0;port<TxPortNum;port++)
			#else
				for(port=1;port<TxPortNum+1;port++)
			#endif
				{
					if(HPD[port])
					{
						for(j=0;j<8;j++)
						{
							if((EDID0data[port].STD[j][0] == EDID0data[ref].STD[i][0])&&
							(EDID0data[port].STD[j][1] == EDID0data[ref].STD[i][1]))
							{
								cnt++;
							}
						}
					}
				}
				if(cnt == CheckHPDCnt())
				{
					STD[i*2] = EDID0data[ref].STD[i][0];
					STD[i*2+1] = EDID0data[ref].STD[i][1];
				}
				else
				{
					STD[i*2] = 0x01;
					STD[i*2+1] = 0x01;
				}
				cnt = 0;
		}
		for(i=0;i<16;i++) *(arry+0x26+i) = STD[i];//STD
		#ifdef IT6664
			for(i=0;i<TxPortNum;i++)
		#else
			for(i=1;i<TxPortNum+1;i++)
		#endif
			{
				if(HPD[i])
				{
					if(gext_var->TXSupport4K60[i])
					{
						for(j=1;j<19;j++) 	*(arry+0x36+j-1) = Dtd18Byte[2][j];//DTD2 4K60
						break;
					}
				}
			}
	#endif
	for(i=1;i<19;i++)
	{
		*(arry+0x48+i-1) = Dtd18Byte[0][i];//DTD1 1080p
	}
	for(i=0;i<16;i++) *(arry+0x5A+i) = EDID_Block0FIX[i+35];
	for(i=0;i<20;i++) *(arry+0x6A+i) = 0;
	#if (USING_1to8==TRUE)
		for(i=0;i<8;i++)
	#else
	#ifdef IT6664
		for(i=0;i<TxPortNum;i++)
	#else
		for(i=1;i<TxPortNum+1;i++)
	#endif
	#endif
		{
			if(EDID0data[i].block1)
			{
				*(arry+0x7E) = 1;//use block1
				break;
			}
			else
			{
				*(arry+0x7E) = 0;//use block1
			}
		}
	*(arry+0x7F) = 0;
	//checksum
	for(i=0;i<0x7F;i++) sum += *(arry+i);
	checksum= 0x100- (sum & 0xFF);
	*(arry+0x7F) = checksum;
}
void it6664_block1_compose(iTE_pu8 arry)
{
	iTE_u8 i,offset,Compose_HDR,sup_vsdb2=0;
	//iTE_u8 tmp[70];

	#if (USING_1to8==TRUE)
		iTE_u8 k;
		for(k=0;k<2;k++)
		{
			if(k) IT6664_DeviceSelect(IT6664_B);
			else IT6664_DeviceSelect(IT6664_A);
			for(i=0;i<TxPortNum;i++)
			{
				if(gext_var->TXSupport4K60[i])
				{
					sup_vsdb2 = 1;
					break;
				}
				if(gext_var->TXSupportOnly420[i])
				{
					sup_vsdb2 |= 0x80;
				}
			}
		}
	#else
		#ifdef IT6664
			for(i=0;i<TxPortNum;i++)
		#else
			for(i=1;i<TxPortNum+1;i++)
		#endif
			{
				if(gext_var->TXSupport4K60[i])
				{
					sup_vsdb2 = 1;
					break;
				}
				if(gext_var->TXSupportOnly420[i])
				{
					sup_vsdb2 |= 0x80;
				}
			}
	#endif
	Compose_HDR = 1;
	*(arry) = 0x02;
	*(arry+1) = 0x03;
	*(arry+3) = 0xE0;
	offset = it6664_ComposeEDIDBlock1_VIC(arry,4,sup_vsdb2);
	offset = it6664_ComposeEDIDBlock1_Audio(arry,offset);
	offset = it6664_ComposeEDIDBlock1_Speaker(arry,offset);
	offset = it6664_ComposeEDIDBlock1_VSDB1(arry,offset);
	if((sup_vsdb2 & 0x01) == 0x01)
	{
		offset = it6664_ComposeEDIDBlock1_VSDB2(arry,offset);
		offset = it6664_ComposeEDIDBlock1_ExtTag(arry,offset,0x0F);
	}
	else if((sup_vsdb2 & 0x80) == 0x80)
	{
		offset = it6664_ComposeEDIDBlock1_ExtTag(arry,offset,0x0E);
	}
	#if (USING_1to8==TRUE)
		for(i=0;i<8;i++)
		{
			if(HPD[i])
			{
				if(i<4)
				{
					if(g_device != IT6664_B) IT6664_DeviceSelect(IT6664_B);
					k = i;
				}
				if(i>3)
				{
					if(g_device != IT6664_A) IT6664_DeviceSelect(IT6664_A);
					k = i-4;
				}
				Compose_HDR &= gext_var->TXSupportHDR[k];
			}
		}
	#else
		#ifdef IT6664
			for(i=0;i<TxPortNum;i++)
		#else
			for(i=1;i<TxPortNum+1;i++)
		#endif
			{
				if(HPD[i])
				{
					Compose_HDR &= gext_var->TXSupportHDR[i];
				}
			}
	#endif
	if(Compose_HDR)
	{
		offset = it6664_ComposeEDIDBlock1_ExtTag(arry,offset,0x06);
	}
	*(arry+2) = offset;
	//for(i=4;i<offset;i++) *(arry+i) = tmp[i];
	for(i=offset;i<0x80;i++) *(arry+i) = 0;
}
iTE_u8 it6664_ComposeEDIDBlock1_VIC(iTE_pu8 ptr,iTE_u8 offset,iTE_u8 sup_vsdb2)
{
	iTE_u8 ret,i,tar,ref,j,k;
	iTE_u8 validcnt;
	iTE_u8 tmp[25]={0};
	iTE_u8 Vic3DPrimary[12] = {60,62,19,4,20,5,32,34,31,16,1,3};

	validcnt = 0;
	tar = 100;
	ret = 0;
	ref = 0;

	#if (USING_1to8==TRUE)
		for(i=0;i<8;i++)
	#else
		#ifdef IT6664
			for(i=0;i<TxPortNum;i++)
		#else
			for(i=1;i<TxPortNum+1;i++)
		#endif
	#endif
			{
				if(HPD[i] && EDID0data[i].block1)
				{
					if(EDID1data[i].video_info.Viccnt <= tar)
					{
						tar = EDID1data[i].video_info.Viccnt;
						ref= i;
					}
				}
			}
		tar =0;
		for(i=0;i<EDID1data[ref].video_info.Viccnt;i++)
		{
			tar = EDID1data[ref].video_info.vic[i];
			#if (USING_1to8==TRUE)
				for(k=0;k<8;k++)
			#else
				#ifdef IT6664
					for(k=0;k<TxPortNum;k++)
				#else
					for(k=1;k<TxPortNum+1;k++)
				#endif
			#endif
				{
					if(HPD[k] && EDID0data[k].block1)
					{
						for(j=0;j<EDID1data[k].video_info.Viccnt;j++)
						{
							if((EDID1data[k].video_info.vic[j]&0x7F) == (tar&0x7F)) validcnt++;
						}
					}
				}
			if(validcnt == CheckHPDCnt())
			{
				tmp[ret] = tar;
				ret++;
			}
			validcnt= 0;
		}
	//for(i=0;i<ret;i++) printf("%02x ",tmp[i]);
	//printf("\r\n ");
	for(i=0;i<ret;i++)
	{
		if(tmp[i] == 96)
		{
			tmp[i] = tmp[0];
			tmp[0] = 96;
		}
		if(tmp[i] == 97)
		{
			tmp[i] = tmp[1];
			tmp[1] = 97;
		}
	}
	if(sup_vsdb2)
	{
		if((tmp[0]!=96) && (tmp[1]!=97))
		{
			tmp[ret] = tmp[0];
			tmp[ret+1] = tmp[1];
			tmp[1] = 97;
			tmp[0] = 96;
			ret+=2;
		}
	}
	validcnt= 0;
	tar = 2;
	for(i=0;i<12;i++)
	{
		for(k=0;k<ret;k++)
		{
			if((tmp[k] == Vic3DPrimary[i])&&(tmp[tar] != Vic3DPrimary[i]))
			{
				//printf("1 tmp[ %02x] = %02x \r\n",i,tmp[i]);
				validcnt = tmp[k];
				tmp[k] = tmp[tar];
				tmp[tar] = validcnt;
				//printf("2 tmp[%02x] = %02x \r\n",tar,tmp[tar]);
				tar++;
				break;
			}
		}
	}
	//for(i=0;i<ret;i++) printf("%02x ",tmp[i]);
	//printf("\r\n ");
	*(ptr+offset) = 0x40+ret;
	for(i=0;i<ret;i++) *(ptr+offset+i+1) = tmp[i];

	return ret+offset+1;
}
iTE_u8 it6664_ComposeEDIDBlock1_Audio(iTE_pu8 ptr,iTE_u8 offset)
{
	iTE_u8 ret,i,j,k,b1,b2,b0;
	iTE_u8 cnt[7];
	#if (USING_1to8==TRUE)
		iTE_u8 tmp[7][8][3];
	#else
		iTE_u8 tmp[7][4][3];
	#endif

	ret = 0;
	for(i=0;i<7;i++) cnt[i]=0;

	#if (USING_1to8==TRUE)
		for(i=0;i<8;i++)
	#else
		#ifdef IT6664
			for(i=0;i<TxPortNum;i++)
		#else
			for(i=1;i<TxPortNum+1;i++)
		#endif
	#endif
		{
			if(HPD[i] && EDID0data[i].block1)
			{
				for(j=0;j<21;j+=3)
				{
					if(((EDID1data[i].audio_info.Format[j]&0x78)>>3) == 1)//LPCM
					{
						tmp[0][i][0] = EDID1data[i].audio_info.Format[j];
						tmp[0][i][1] = EDID1data[i].audio_info.Format[j+1];
						tmp[0][i][2] = EDID1data[i].audio_info.Format[j+2];
						cnt[0]++;
					}
					if(((EDID1data[i].audio_info.Format[j]&0x78)>>3) == 2)//AC3
					{
						tmp[1][i][0] = EDID1data[i].audio_info.Format[j];
						tmp[1][i][1] = EDID1data[i].audio_info.Format[j+1];
						tmp[1][i][2] = EDID1data[i].audio_info.Format[j+2];
						cnt[1]++;
					}
					if(((EDID1data[i].audio_info.Format[j]&0x78)>>3) == 7)//DTS
					{
						tmp[2][i][0] = EDID1data[i].audio_info.Format[j];
						tmp[2][i][1] = EDID1data[i].audio_info.Format[j+1];
						tmp[2][i][2] = EDID1data[i].audio_info.Format[j+2];
						cnt[2]++;
					}
					if(((EDID1data[i].audio_info.Format[j]&0x78)>>3) == 0x09)// 1 bit audio
					{
						tmp[3][i][0] = EDID1data[i].audio_info.Format[j];
						tmp[3][i][1] = EDID1data[i].audio_info.Format[j+1];
						tmp[3][i][2] = EDID1data[i].audio_info.Format[j+2];
						cnt[3]++;
					}
					if(((EDID1data[i].audio_info.Format[j]&0x78)>>3) == 0x0A)//Dolby Digital
					{
						tmp[4][i][0] = EDID1data[i].audio_info.Format[j];
						tmp[4][i][1] = EDID1data[i].audio_info.Format[j+1];
						tmp[4][i][2] = EDID1data[i].audio_info.Format[j+2];
						cnt[4]++;
					}
					if(((EDID1data[i].audio_info.Format[j]&0x78)>>3) == 0x0B)//DTS HD
					{
						tmp[5][i][0] = EDID1data[i].audio_info.Format[j];
						tmp[5][i][1] = EDID1data[i].audio_info.Format[j+1];
						tmp[5][i][2] = EDID1data[i].audio_info.Format[j+2];
						cnt[5]++;
					}
					if(((EDID1data[i].audio_info.Format[j]&0x78)>>3) == 0x0C)//Dolby TrueHD
					{
						tmp[6][i][0] = EDID1data[i].audio_info.Format[j];
						tmp[6][i][1] = EDID1data[i].audio_info.Format[j+1];
						tmp[6][i][2] = EDID1data[i].audio_info.Format[j+2];
						cnt[6]++;
					}
				}
			}
		}

	for(i=0;i<7;i++)
	{
		b0 = 0xFF;
		b1 = 0xFF;
		b2 = 0xFF;
		if(cnt[i] == CheckHPDCnt())
		{
			#if (USING_1to8==TRUE)
				for(k=0;k<8;k++)
			#else
				#ifdef IT6664
					for(k=0;k<TxPortNum;k++)
				#else
					for(k=1;k<TxPortNum+1;k++)
				#endif
			#endif
				{
					if(HPD[k] && EDID0data[k].block1)
					{
						if(b0 >= tmp[i][k][0]) b0 = tmp[i][k][0];
						b1 &= tmp[i][k][1];
						if(b2 >= tmp[i][k][2]) b2 = tmp[i][k][2];
					}
				}
			*(ptr+offset+1+ret) = b0;
			*(ptr+offset+2+ret) = b1;
			*(ptr+offset+3+ret) = b2;
			ret += 3;
		}
	}
	*(ptr+offset) = 0x20 + ret;


	return ret+offset+1;
}
iTE_u8 it6664_ComposeEDIDBlock1_Speaker(iTE_pu8 ptr,iTE_u8 offset)
{
	iTE_u8 ret,i;

	*(ptr+offset)= 0x83;
	*(ptr+offset+1) = 0xFF;

	#if (USING_1to8==TRUE)
		for(i=0;i<8;i++)
	#else
		#ifdef IT6664
			for(i=0;i<TxPortNum;i++)
		#else
			for(i=1;i<TxPortNum+1;i++)
		#endif
	#endif
		{
			if(HPD[i] && EDID0data[i].block1)
			{
				if(EDID1data[i].speaker_info) *(ptr+offset+1) &= EDID1data[i].speaker_info;
				*(ptr+offset+2) = 0;
				*(ptr+offset+3) = 0;
			}
		}
	ret = 4+offset;
	return ret;
}
iTE_u8 it6664_ComposeEDIDBlock1_VSDB1(iTE_pu8 ptr,iTE_u8 offset)
{
	iTE_u8 tmp,tmp1,off,cnt;
	iTE_u8 i,j,phyaddr0 =0x11,phyaddr1=0x00,I_lat,lat,vid;
	iTE_u8 fix[7] = {0x03, 0x0C, 0x00 , phyaddr0 , phyaddr1 , 0xB8 , 0x3C};
	iTE_u8 Vic[30],tmpcnt;
	iTE_u8 HDMI_Len,Compose3D,vic_len;
	#ifdef Compose3D_EDID
	iTE_u16 Vic3DMask,tmp3D[16],validcnt;
	iTE_u8 Vic3DPrimary[12] = {60,62,19,4,20,5,32,34,31,16,1,3};
	#endif

	#ifdef IT6664
	//printf("gext_u8->TXHPDsts =%02x \r\n ",gext_u8->TXHPDsts);
		if(gext_u8->TXHPDsts&(1<<ABCD_CopyPx))
		{
			fix[3] =  EDID1data[ABCD_CopyPx].vsdb1.phyaddr0;
			fix[4] =  EDID1data[ABCD_CopyPx].vsdb1.phyaddr1;
		}
	#else
	//printf("gext_u8->TXHPDsts =%02x \r\n ",gext_u8->TXHPDsts);
		if(gext_u8->TXHPDsts&(1<<ABCD_CopyPx))
		{
			fix[3] =  EDID1data[ABCD_CopyPx].vsdb1.phyaddr0;
			fix[4] =  EDID1data[ABCD_CopyPx].vsdb1.phyaddr1;
		}
	#endif


	vic_len = (*(ptr+4)&0x1F);
	for(i=0;i<vic_len;i++)
	{
		Vic[i] = *(ptr+5+i);//get vic data for 3D compose use
	}
	tmpcnt = 0;
	cnt = 0;
	off = 1;
	I_lat = 1;
	lat = 1;
	for(i=0;i<7;i++) *(ptr+offset+1+i) = fix[i];
	off = 8;
	tmp = 0xFF;
	tmp1 = 0xFF;

	#if (USING_1to8==TRUE)
		for(i=0;i<8;i++)
	#else
		#ifdef IT6664
			for(i=0;i<TxPortNum;i++)
		#else
			for(i=1;i<TxPortNum+1;i++)
		#endif
	#endif
		{
			if(HPD[i] && EDID0data[i].block1)
			{
				cnt++;
				if((EDID1data[i].vsdb1.Vdsb8_CNandPresent & 0x80) != 0x80) lat = 0;
				if((EDID1data[i].vsdb1.Vdsb8_CNandPresent & 0x40) != 0x40) I_lat = 0;
				tmp &= EDID1data[i].vsdb1.Vdsb8_CNandPresent;
			}
		}
	*(ptr+offset+off) = tmp;
	off++;
	tmp = 0xFF;
	//Latency
	if(lat)
	{
		#if (USING_1to8==TRUE)
			for(i=0;i<8;i++)
		#else
			#ifdef IT6664
				for(i=0;i<TxPortNum;i++)
			#else
				for(i=1;i<TxPortNum+1;i++)
			#endif
		#endif
			{
				if(HPD[i] && EDID0data[i].block1)
				{
					tmp &= EDID1data[i].vsdb1.VideoLatency;
					tmp1 &= EDID1data[i].vsdb1.AudioLatency;
				}
			}
		*(ptr+off+offset) = tmp;
		*(ptr+off+1+offset) = tmp1;
		off += 2;
		tmp = 0xFF;
		tmp1 = 0xFF;
	}
	if(I_lat)
	{
		#if (USING_1to8==TRUE)
			for(i=0;i<8;i++)
		#else
			#ifdef IT6664
				for(i=0;i<TxPortNum;i++)
			#else
				for(i=1;i<TxPortNum+1;i++)
			#endif
		#endif
			{
				if(HPD[i] && EDID0data[i].block1)
				{
					tmp &= EDID1data[i].vsdb1.I_VideoLatency;
					tmp1 &= EDID1data[i].vsdb1.I_AudioLatency;
				}
			}
		*(ptr+off+offset) = tmp;
		*(ptr+off+1+offset) = tmp1;
		off += 2;
	}
	tmp = 0xFF;
	tmp1 = 0;
	// 3D present  VIC  len
	#if (USING_1to8==TRUE)
		for(i=0;i<8;i++)
	#else
		#ifdef IT6664
			for(i=0;i<TxPortNum;i++)
		#else
			for(i=1;i<TxPortNum+1;i++)
		#endif
	#endif
		{
			if(HPD[i] && EDID0data[i].block1)
			{
				tmp &= EDID1data[i].vsdb1.HDMI3D_Present;
				if(EDID1data[i].vsdb1.HDMI_VicLen) tmp1 |= 0x01;
				else tmp1 &= ~0x01;
				if(EDID1data[i].vsdb1.HDMI_3DLen) tmp1 |= 0x10;
				else tmp1 &= ~0x10;
			}
		}
	*(ptr+off+offset) = tmp;
	if(tmp&0x80) Compose3D = tmp;
	//iTE_Edid_Msg(("1. tmp1 =%02x  \r\n",tmp1));
	//iTE_Edid_Msg(("2. tmp =%02x  \r\n",tmp));
	off++;
	I_lat = 0;
	//HDMI  VIC
	if(tmp1&0x01)
	{
		for(j=1;j<5;j++)
		{
			vid = 0;
			#if (USING_1to8==TRUE)
				for(i=0;i<8;i++)
			#else
				#ifdef IT6664
					for(i=0;i<TxPortNum;i++)
				#else
					for(i=1;i<TxPortNum+1;i++)
				#endif
			#endif
				{
					if(HPD[i] && EDID0data[i].block1)
					{
						for(lat=0;lat<EDID1data[i].vsdb1.HDMI_VicLen;lat++)
						{
							if(EDID1data[i].vsdb1.HDMIVic[lat] == j)
							{
								vid++;
							}

						}
					}
				}
			//iTE_Edid_Msg(("vid =%02x   j = %d  \r\n",vid,(int)j));
			if(vid==cnt)
			{
				*(ptr+off+offset+1+I_lat) = j;
				I_lat++;
			}
		}
	}
	*(ptr+off+offset) = (I_lat<<5);
	HDMI_Len = off;
	off += (1+I_lat);

	#ifdef Compose3D_EDID
	//3D info
	if(Compose3D&0x80)
	{
		if((Compose3D & 0x20) || (Compose3D & 0x40))//3D_Multi_present Structure_all
		{
			*(ptr+off+offset) = 0x01;
			*(ptr+off+offset+1) = 0x4F;
			off+=2;
			tmpcnt += 2;
		}
		if(Compose3D & 0x40)//3D_Multi_present Mask
		{
			Vic3DMask = 0;
			for(j=0;j<12;j++)
			{
				I_lat = 0;
				#if (USING_1to8==TRUE)
					for(i=0;i<8;i++)
				#else
					#ifdef IT6664
						for(i=0;i<TxPortNum;i++)
					#else
					for(i=1;i<TxPortNum+1;i++)
				#endif
				#endif
						{
							if(HPD[i] && EDID0data[i].block1)
							{
								for(lat = 0;lat<EDID1data[i].vsdb1._3D_Vic_SupCnt;lat++)
								{
									if(EDID1data[i].vsdb1._3D_Vic_Sup[lat] == Vic3DPrimary[j])
									{
										I_lat++;
										break;
									}
								}
							}
						}
				if(I_lat == CheckHPDCnt())
				{
					//iTE_Edid_Msg(("2. Vic3DPrimary[j] =%02x  \r\n",Vic3DPrimary[j]));
					for(tmp1=0;tmp1<vic_len;tmp1++)
					{
						if(Vic3DPrimary[j] == Vic[tmp1])
						{
							Vic3DMask |= 1<<tmp1;
							//iTE_Edid_Msg(("Vic3DMask =%04x  \r\n",Vic3DMask));
						}
					}
				}
			}
			*(ptr+off+offset) = (iTE_u8)((Vic3DMask&0xFF00)>>8);
			*(ptr+off+offset+1) = (iTE_u8)((Vic3DMask&0x00FF));
			off+=2;
			tmpcnt += 2;
		}

		#if (USING_1to8==TRUE)
			for(i=0;i<8;i++)
		#else
			#ifdef IT6664
				for(i=0;i<TxPortNum;i++)
			#else
				for(i=1;i<TxPortNum+1;i++)
			#endif
		#endif
			{
				if(HPD[i] && EDID0data[i].block1)
				{
					tmp = i;
					break;
				}
			}

		vid = 0;
		for(j=0;j<EDID1data[tmp].vsdb1._3D_Vic_SupCnt;j++)
		{
			validcnt = 0;
			#if (USING_1to8==TRUE)
				for(i=0;i<8;i++)
			#else
				#ifdef IT6664
				for(i=0;i<TxPortNum;i++)
				#else
				for(i=1;i<TxPortNum+1;i++)
				#endif
			#endif
			{
				if(HPD[i] && EDID0data[i].block1)
				{
					for(I_lat=0;I_lat<EDID1data[i].vsdb1._3D_Order_len;I_lat++)
					{
						//printf("_3D_Order[I_lat] = %4x\r\n",EDID1data[i].vsdb1._3D_Order[I_lat]);
						//printf("compare = %4x\r\n",EDID1data[tmp].vsdb1._3D_Order[j]);
						if(EDID1data[i].vsdb1._3D_Order[I_lat] == EDID1data[tmp].vsdb1._3D_Order[j])
						{
							validcnt++;
							break;
						}
					}
				}
			}
			if(CheckHPDCnt() == validcnt)
			{
				tmp3D[vid] = EDID1data[tmp].vsdb1._3D_Order[j];
				//printf("tmp3D[%2x] = %4x \r\n",vid,tmp3D[vid]);
				vid++;
			}
		}
		lat = 0;
		cnt=0;
		validcnt= 0;
		if(vid)
		{
			for(i=0;i<vid;i++)
			{
				I_lat = 0;
				tmp1 = (iTE_u8)((tmp3D[i]&0x00FF));//vic
				tmp = (iTE_u8)((tmp3D[i]&0xFF00)>>8);//3d type

				for(j=0;j<vic_len;j++)
				{
					if(tmp1 == Vic[j])
					{
						cnt = j;
					}
				}
				if(tmp3D[i])
				{
					lat = (cnt<<4)+tmp;
					*(ptr+off+offset) = lat;
					if(tmp==0x08)
					{
						*(ptr+off+offset+1) = 0x10;
						off+=2;
						validcnt+=2;
					}
					else
					{
						off+=1;
						validcnt+=1;
					}
				}
			}
		}
		validcnt+=tmpcnt;//HDMI_3D_LEN
		*(ptr+HDMI_Len+offset) |= validcnt;
	}
	#endif
	*(ptr+offset) = 0x60+off-1;

	return off+offset;
}
iTE_u8 it6664_ComposeEDIDBlock1_VSDB2(iTE_pu8 ptr,iTE_u8 offset)
{
	iTE_u8 i;

	iTE_u8 fix[8] = {0x67 ,0xD8 ,0x5D ,0xC4 ,0x01 ,0x78,0x80,0xFF};

	#if (USING_1to8==TRUE)
		for(i=0;i<8;i++)
	#else
		#ifdef IT6664
			for(i=0;i<TxPortNum;i++)
		#else
			for(i=1;i<TxPortNum+1;i++)
		#endif
	#endif
		{
			if(HPD[i] && EDID0data[i].block1)
			{
				fix[6] |= EDID1data[i].vsdb2.Byte1;
				fix[7] &= EDID1data[i].vsdb2.Byte2;
			}
		}
	for(i=0;i<8;i++) *(ptr+offset+i) = fix[i];

	return offset+8;
}
iTE_u8 it6664_ComposeEDIDBlock1_ExtTag(iTE_pu8 ptr,iTE_u8 offset , iTE_u8 tag)
{
	iTE_u8 ret,i,valid=0;
	iTE_u8 fix[3] = {0xE2,0x0F,0x03};
	iTE_u8 fix2[6] = {0xE5 ,0x0E ,0x60 ,0x61 ,0x65 ,0x66};//4K2K 420 only
	iTE_u8 tmp,tmp1;
	if(tag == 0x0F)
	{
		#if (USING_1to8==TRUE)
				for(i=0;i<8;i++)
		#else
			#ifdef IT6664
				for(i=0;i<TxPortNum;i++)
			#else
				for(i=1;i<TxPortNum+1;i++)
			#endif
		#endif
			{
				if(HPD[i] && EDID0data[i].block1)
				{
					if(gext_var->TXSupport420[i])
					{
						valid=1;
						break;
					}
				}
			}
		if(valid)
		{
			for(i=0;i<3;i++)
			{
				*(ptr+offset+i) = fix[i];
			}
			ret = offset+3;
		}
		else
		{
			ret = offset;
		}
	}
	else if(tag == 0x06)
	{
		tmp = 0xFF;
		tmp1 = 0xFF;

		#if (USING_1to8==TRUE)
			for(i=0;i<8;i++)
		#else
			#ifdef IT6664
				for(i=0;i<TxPortNum;i++)
			#else
				for(i=1;i<TxPortNum+1;i++)
			#endif
		#endif
			{
				if(HPD[i] && EDID0data[i].block1)
				{
					tmp &= EDID1data[i].HDR_Content[0];
					tmp1 &= EDID1data[i].HDR_Content[1];
				}
			}
		*(ptr+offset) = 0xE3;
		*(ptr+offset+1) = 0x06;
		*(ptr+offset+2) = tmp;
		*(ptr+offset+3) = tmp1;
		ret = offset + 4;
	}
	else if(tag == 0x0E)
	{
		for(i=0;i<6;i++)
		{
			*(ptr+offset+i) = fix2[i];
		}
		ret = offset+6;
	}

	return ret;
}
#ifdef EDID_Compose_Intersection
void it6664_block0_compose_int(iTE_pu8 arry)
{
	iTE_u8 i,j,port,ref=0,cnt=0,EdidOff=0;
	iTE_u8 EST[3],tmpdata=0,STD[16];
	iTE_u16 sum ,checksum;
	for(i=0;i<35;i++) *(arry+i) = EDID_Block0FIX[i];

	iTE_Edid_Msg(("EDID intersection  \r\n"));
	//Est
	#if 0//(USING_1to8==TRUE)//NOT Modify for intersection //Tranmin for refresh EDID whatever plug in/out
		iTE_u8 tmp;
		for(j=0;j<3;j++)
		{
			for(i=0;i<8;i++)
			{
				if(HPD[i])
				{
					tmpdata &= EDID0data[i].EstablishTimeing[j];
				}
			}
			EST[j] = tmpdata;
			tmpdata = 0;
		}
		for(i=0;i<3;i++) *(arry+0x23+i) = EST[i];
		for(i=0;i<16;i++) *(arry+0x26+i) = 0x01;//STD
		for(i=0;i<8;i++)
		{
			if(HPD[i])
			{
				if(i<4)
				{
					if(g_device != IT6664_B) IT6664_DeviceSelect(IT6664_B);
					tmp = i;
				}
				if(i>3)
				{
					if(g_device != IT6664_A) IT6664_DeviceSelect(IT6664_A);
					tmp = i-4;
				}
				if(gext_var->TXSupport4K60[tmp])
				{
					for(j=1;j<19;j++) 	*(arry+0x36+j-1) = Dtd18Byte[2][j];//DTD2 4K60
					break;
				}
			}
		}
	#else
		for(j=0;j<3;j++)
		{
			#if (USING_1to8==TRUE)
				for(i=0;i<8;i++)
			#else
			#ifdef IT6664
				for(i=0;i<TxPortNum;i++)
			#else
				for(i=1;i<TxPortNum+1;i++)
			#endif
			#endif
				{
					if(HPD[i])
					{
						tmpdata &= EDID0data[i].EstablishTimeing[j];
					}
				}
			EST[j] = tmpdata;
			tmpdata = 0;
		}
		for(i=0;i<3;i++) *(arry+0x23+i) = EST[i];

		#if (USING_1to8==TRUE)
			for(port=0;port<8;port++)
		#else
		#ifdef IT6664
			for(port=0;port<TxPortNum;port++)
		#else
			for(port=1;port<TxPortNum+1;port++)
		#endif
		#endif
			{
				if(HPD[port])
				{
					ref = port;
				}
			}
		for(i=0;i<8;i++)//ref 8
		{
			#if (USING_1to8==TRUE)
				for(port=0;port<8;port++)
			#else
				for(port=0;port<TxPortNum;port++)
			#ifdef IT6664
			#else
				for(port=1;port<TxPortNum+1;port++)
			#endif
			#endif
				{
					if(HPD[port])
					{
						for(j=0;j<8;j++)
						{
							if((EDID0data[port].STD[j][0] == EDID0data[ref].STD[i][0])&&
							(EDID0data[port].STD[j][1] == EDID0data[ref].STD[i][1]))
							{
								cnt++;
							}
						}
					}
				}
				if(cnt == CheckHPDCnt())
				{
					STD[i*2] = EDID0data[ref].STD[i][0];
					STD[i*2+1] = EDID0data[ref].STD[i][1];
				}
				else
				{
					STD[i*2] = 0x01;
					STD[i*2+1] = 0x01;
				}
				cnt = 0;
		}
		for(i=0;i<16;i++) *(arry+0x26+i) = STD[i];//STD
		EdidOff = 0x36;
		cnt = 0;

		#if (USING_1to8==TRUE)
			for(i=0;i<8;i++)
		#else
		#ifdef IT6664
			for(i=0;i<TxPortNum;i++)
		#else
			for(i=1;i<TxPortNum+1;i++)
		#endif
		#endif
			{
			#if (USING_1to8==TRUE)
				{
				iTE_u8 k;

				if(i<4)
				{
					if(g_device != IT6664_B) IT6664_DeviceSelect(IT6664_B);
					k = i;
				}
				if(i>3)
				{
					if(g_device != IT6664_A) IT6664_DeviceSelect(IT6664_A);
					k = i-4;
				}

				if(HPD[i] && (gext_var->TXSupport4K60[k]&0x10))//201217
				{
					cnt++;
				}
				}
			#else

				if(HPD[i] && (gext_var->TXSupport4K60[i]&0x10))//201217
				{
					cnt++;
				}
			#endif
			}

		if(cnt == CheckHPDCnt())
		{
			for(j=1;j<19;j++)
			{
				*(arry+EdidOff+j-1) = Dtd18Byte[2][j];//DTD2 4K60
			}
			EdidOff += 18;
		}

	#endif

	cnt = 0;
	if(CompareResolution(ref)==FALSE)
	{
		for(i=0;i<17;i++) *(arry+EdidOff+i) = EDID_Block0FIX[i+35];
		EdidOff += 17;
	}
	else
	{
		#ifdef IT6664
			port= 0;
		#else
			port= 1;
		#endif

		if(HPD[port])
		{
			cnt = ProductName[port][0];
			for(i=1;i<cnt;i++) *(arry+EdidOff+i-1) =  ProductName[port][i];
			EdidOff += cnt;
		}
		else //use default
		{
			for(i=0;i<17;i++) *(arry+EdidOff+i) = EDID_Block0FIX[i+35];
			EdidOff += 17;
		}
	}

	ref = 0x7E - EdidOff;
	for(i=0;i<ref;i++) *(arry+EdidOff+i-1) = 0;//201217

	#if (USING_1to8==TRUE)
		for(i=0;i<8;i++)
	#else
	#ifdef IT6664
		for(i=0;i<TxPortNum;i++)
	#else
		for(i=1;i<TxPortNum+1;i++)
	#endif
	#endif
		{
			//if(EDID0data[i+Device_off*4].block1)
			if(EDID0data[i].block1)//Tranmin for fix a exceed issue for 1to 8
			{
				*(arry+0x7E) = 1;//use block1
				break;
			}
			else
			{
				*(arry+0x7E) = 0;//use block1
			}
		}
	*(arry+0x7F) = 0;
	//checksum
	for(i=0;i<0x7F;i++) sum += *(arry+i);
	checksum= 0x100- (sum & 0xFF);
	*(arry+0x7F) = checksum;
}
void it6664_block1_compose_int(iTE_pu8 arry)
{
	iTE_u8 i,offset,Compose_HDR,sup_vsdb2=0x81;
	//iTE_u8 tmp[70];

	#if (USING_1to8==TRUE)
		iTE_u8 k;
		iTE_u8 dev_bak = g_device;
		iTE_u8 offset_bak = Device_off;
		for(k=0;k<2;k++)
		{
			if(k)
			{
				IT6664_DeviceSelect(IT6664_B);
				Device_off = 1;
			}
			else
			{
				IT6664_DeviceSelect(IT6664_A);
				Device_off = 0;
			}
			for(i=0;i<TxPortNum;i++)
			{
				if(HPD[i+Device_off*4])
				{
					if((!gext_var->TXSupportOnly420[i]) || (!gext_var->TXSupport420[i]))
					{
						sup_vsdb2 &= ~0x80;
					}
					if(((gext_var->TXSupport4K60[i]&0x10) != 0x10) &&
						((gext_var->TXSupport4K60[i]&0x01) != 0x01))
					{
						sup_vsdb2 &= ~0x01;
					}
				}
			}
		}
		IT6664_DeviceSelect(dev_bak);
		Device_off = offset_bak;
	#else
		#ifdef IT6664
			for(i=0;i<TxPortNum;i++)
		#else
			for(i=1;i<TxPortNum+1;i++)
		#endif
			{
				if(HPD[i])
				{
					if((!gext_var->TXSupportOnly420[i]) || (!gext_var->TXSupport420[i]))
					{
						sup_vsdb2 &= ~0x80;
					}
					if(((gext_var->TXSupport4K60[i]&0x10) != 0x10) &&
						((gext_var->TXSupport4K60[i]&0x01) != 0x01))
					{
						sup_vsdb2 &= ~0x01;
					}
				}
			}
	#endif
	Compose_HDR = 1;
	*(arry) = 0x02;
	*(arry+1) = 0x03;
	*(arry+3) = 0xE0;
	offset = it6664_ComposeEDIDBlock1_VIC_int(arry,4,sup_vsdb2);
	offset = it6664_ComposeEDIDBlock1_Audio_int(arry,offset);
	offset = it6664_ComposeEDIDBlock1_Speaker_int(arry,offset);
	offset = it6664_ComposeEDIDBlock1_VSDB1_int(arry,offset);
	if((sup_vsdb2 & 0x01) == 0x01)
	{
		offset = it6664_ComposeEDIDBlock1_VSDB2_int(arry,offset);
		offset = it6664_ComposeEDIDBlock1_ExtTag_int(arry,offset,0x0F);
	}
	else if((sup_vsdb2 & 0x80) == 0x80)
	{
		offset = it6664_ComposeEDIDBlock1_ExtTag_int(arry,offset,0x0E);
	}
	#if (USING_1to8==TRUE)
		for(i=0;i<8;i++)
		{
			if(HPD[i])
			{
				if(i<4)
				{
					if(g_device != IT6664_B) IT6664_DeviceSelect(IT6664_B);
					k = i;
				}
				if(i>3)
				{
					if(g_device != IT6664_A) IT6664_DeviceSelect(IT6664_A);
					k = i-4;
				}
				Compose_HDR &= gext_var->TXSupportHDR[k];
			}
		}
	#else
		#ifdef IT6664
			for(i=0;i<TxPortNum;i++)
		#else
			for(i=1;i<TxPortNum+1;i++)
		#endif
			{
				if(HPD[i])
				{
					Compose_HDR &= gext_var->TXSupportHDR[i];
				}
			}
	#endif
	if(Compose_HDR)
	{
		offset = it6664_ComposeEDIDBlock1_ExtTag_int(arry,offset,0x06);
	}
	*(arry+2) = offset;
	//for(i=4;i<offset;i++) *(arry+i) = tmp[i];

	//Add DTD 1366x768 and 2560x1440 Here
	offset = it6664_ComposeEDIDBlock1_DTD(arry,offset);



	for(i=offset;i<0x80;i++) *(arry+i) = 0;
}
iTE_u8 it6664_ComposeEDIDBlock1_VIC_int(iTE_pu8 ptr,iTE_u8 offset,iTE_u8 sup_vsdb2)
{
	iTE_u8 ret,i,tar,ref,j,k;
	iTE_u8 validcnt;
	iTE_u8 tmp[25]={0};

	validcnt = 0;
	tar = 100;
	ret = 0;
	ref = 0;

	#if (USING_1to8==TRUE)
		for(i=0;i<8;i++)
	#else
		#ifdef IT6664
			for(i=0;i<TxPortNum;i++)
		#else
			for(i=1;i<TxPortNum+1;i++)
		#endif
	#endif
			{
				if(HPD[i] && EDID0data[i].block1)
				{
					if(EDID1data[i].video_info.Viccnt <= tar)
					{
						tar = EDID1data[i].video_info.Viccnt;
						ref= i;
					}
				}
			}
		tar =0;
		for(i=0;i<EDID1data[ref].video_info.Viccnt;i++)
		{
			tar = EDID1data[ref].video_info.vic[i];
			#if (USING_1to8==TRUE)
				for(k=0;k<8;k++)
			#else
				#ifdef IT6664
					for(k=0;k<TxPortNum;k++)
				#else
					for(k=1;k<TxPortNum+1;k++)
				#endif
			#endif
				{
					if(HPD[k] && EDID0data[k].block1)
					{
						for(j=0;j<EDID1data[k].video_info.Viccnt;j++)
						{
							if((EDID1data[k].video_info.vic[j]&0x7F) == (tar&0x7F)) validcnt++;
						}
					}
				}
			if(validcnt == CheckHPDCnt())
			{
				tmp[ret] = tar;
				ret++;
			}
			validcnt= 0;
		}

	for(i=0;i<ret;i++)
	{
		if(tmp[i] == 96)
		{
			tmp[i] = tmp[0];
			tmp[0] = 96;
		}
		if(tmp[i] == 97)
		{
			tmp[i] = tmp[1];
			tmp[1] = 97;
		}
	}

	if(sup_vsdb2&0x01)//201217
	{
		if((tmp[0]!=96) && (tmp[1]!=97))
		{
			tmp[ret] = tmp[0];
			tmp[ret+1] = tmp[1];
			tmp[1] = 97;
			tmp[0] = 96;
			ret+=2;
		}
	}
	*(ptr+offset) = 0x40+ret;
	for(i=0;i<ret;i++) *(ptr+offset+i+1) = tmp[i];

	return ret+offset+1;
}
iTE_u8 it6664_ComposeEDIDBlock1_Audio_int(iTE_pu8 ptr,iTE_u8 offset)
{
	iTE_u8 ret,i,j,k,b1,b2,b0;
	iTE_u8 cnt[7];
	#if (USING_1to8==TRUE)
		iTE_u8 tmp[7][8][3];
	#else
		iTE_u8 tmp[7][4][3];
	#endif

	ret = 0;

	for(i=0;i<7;i++) cnt[i]=0;

	#if (USING_1to8==TRUE)
		for(i=0;i<8;i++)
	#else
		#ifdef IT6664
			for(i=0;i<TxPortNum;i++)
		#else
			for(i=1;i<TxPortNum+1;i++)
		#endif
	#endif
		{
			if(HPD[i] && EDID0data[i].block1)
			{
				for(j=0;j<21;j+=3)
				{
					if(((EDID1data[i].audio_info.Format[j]&0x78)>>3) == 1)//LPCM
					{
						tmp[0][i][0] = EDID1data[i].audio_info.Format[j];
						tmp[0][i][1] = EDID1data[i].audio_info.Format[j+1];
						tmp[0][i][2] = EDID1data[i].audio_info.Format[j+2];
						cnt[0]++;
					}
					if(((EDID1data[i].audio_info.Format[j]&0x78)>>3) == 2)//AC3
					{
						tmp[1][i][0] = EDID1data[i].audio_info.Format[j];
						tmp[1][i][1] = EDID1data[i].audio_info.Format[j+1];
						tmp[1][i][2] = EDID1data[i].audio_info.Format[j+2];
						cnt[1]++;
					}
					if(((EDID1data[i].audio_info.Format[j]&0x78)>>3) == 7)//DTS
					{
						tmp[2][i][0] = EDID1data[i].audio_info.Format[j];
						tmp[2][i][1] = EDID1data[i].audio_info.Format[j+1];
						tmp[2][i][2] = EDID1data[i].audio_info.Format[j+2];
						cnt[2]++;
					}
					if(((EDID1data[i].audio_info.Format[j]&0x78)>>3) == 0x09)// 1 bit audio
					{
						tmp[3][i][0] = EDID1data[i].audio_info.Format[j];
						tmp[3][i][1] = EDID1data[i].audio_info.Format[j+1];
						tmp[3][i][2] = EDID1data[i].audio_info.Format[j+2];
						cnt[3]++;
					}
					if(((EDID1data[i].audio_info.Format[j]&0x78)>>3) == 0x0A)//Dolby Digital
					{
						tmp[4][i][0] = EDID1data[i].audio_info.Format[j];
						tmp[4][i][1] = EDID1data[i].audio_info.Format[j+1];
						tmp[4][i][2] = EDID1data[i].audio_info.Format[j+2];
						cnt[4]++;
					}
					if(((EDID1data[i].audio_info.Format[j]&0x78)>>3) == 0x0B)//DTS HD
					{
						tmp[5][i][0] = EDID1data[i].audio_info.Format[j];
						tmp[5][i][1] = EDID1data[i].audio_info.Format[j+1];
						tmp[5][i][2] = EDID1data[i].audio_info.Format[j+2];
						cnt[5]++;
					}
					if(((EDID1data[i].audio_info.Format[j]&0x78)>>3) == 0x0C)//Dolby TrueHD
					{
						tmp[6][i][0] = EDID1data[i].audio_info.Format[j];
						tmp[6][i][1] = EDID1data[i].audio_info.Format[j+1];
						tmp[6][i][2] = EDID1data[i].audio_info.Format[j+2];
						cnt[6]++;
					}
				}
			}
		}

	for(i=0;i<7;i++)
	{
		b0 = 0xFF;
		b1 = 0xFF;
		b2 = 0xFF;
		if(cnt[i] == CheckHPDCnt())
		{
			#if (USING_1to8==TRUE)
				for(k=0;k<8;k++)
			#else
				#ifdef IT6664
					for(k=0;k<TxPortNum;k++)
				#else
					for(k=1;k<TxPortNum+1;k++)
				#endif
			#endif
				{
					if(HPD[k] && EDID0data[k].block1)
					{
						if(b0 >= tmp[i][k][0]) b0 = tmp[i][k][0];
						b1 &= tmp[i][k][1];
						if(b2 >= tmp[i][k][2]) b2 = tmp[i][k][2];
					}
				}
			*(ptr+offset+1+ret) = b0;
			*(ptr+offset+2+ret) = b1;
			*(ptr+offset+3+ret) = b2;
			//printf("%02x  %02x  %02x  \r\n", *(ptr+offset+1+ret),*(ptr+offset+2+ret),*(ptr+offset+3+ret));
			ret += 3;
		}
	}
	*(ptr+offset) = 0x20 + ret;


	return ret+offset+1;
}
iTE_u8 it6664_ComposeEDIDBlock1_Speaker_int(iTE_pu8 ptr,iTE_u8 offset)
{
	iTE_u8 ret,i;

	*(ptr+offset)= 0x83;
	*(ptr+offset+1) = 0xFF;

	#if (USING_1to8==TRUE)
		for(i=0;i<8;i++)
	#else
		#ifdef IT6664
			for(i=0;i<TxPortNum;i++)
		#else
			for(i=1;i<TxPortNum+1;i++)
		#endif
	#endif
		{
			if(HPD[i] && EDID0data[i].block1)
			{
				if(EDID1data[i].speaker_info) *(ptr+offset+1) &= EDID1data[i].speaker_info;
				*(ptr+offset+2) = 0;
				*(ptr+offset+3) = 0;
			}
		}
	ret = 4+offset;
	return ret;
}
iTE_u8 it6664_ComposeEDIDBlock1_VSDB1_int(iTE_pu8 ptr,iTE_u8 offset)
{
	iTE_u8 tmp,tmp1,off,cnt;
	iTE_u8 i,j,phyaddr0 =0x11,phyaddr1=0x00,I_lat,lat,vid;
	iTE_u8 fix[7] = {0x03, 0x0C, 0x00 , phyaddr0 , phyaddr1 , 0xB8 , 0x3C};

	#ifdef IT6664
	//printf("gext_u8->TXHPDsts =%02x \r\n ",gext_u8->TXHPDsts);
		if(gext_u8->TXHPDsts&(1<<ABCD_CopyPx))
		{
			fix[3] =  EDID1data[ABCD_CopyPx].vsdb1.phyaddr0;
			fix[4] =  EDID1data[ABCD_CopyPx].vsdb1.phyaddr1;
		}
	#else
	//printf("gext_u8->TXHPDsts =%02x \r\n ",gext_u8->TXHPDsts);
		if(gext_u8->TXHPDsts&(1<<ABCD_CopyPx))
		{
			fix[3] =  EDID1data[ABCD_CopyPx].vsdb1.phyaddr0;
			fix[4] =  EDID1data[ABCD_CopyPx].vsdb1.phyaddr1;
		}
	#endif

#if (USING_1to8==TRUE)
		for(i=0;i<8;i++)
#else
	#ifdef IT6664
		for(i=0;i<TxPortNum;i++)
	#else
		for(i=1;i<TxPortNum+1;i++)
	#endif
#endif
		{
			if(HPD[i])
			{
				if((EDID1data[i].vsdb1.DCSupport&0x30) != 0x30)
				{
					fix[5] = EDID1data[i].vsdb1.DCSupport;
				}
			}
		}
	cnt = 0;
	off = 1;
	I_lat = 1;
	lat = 1;
	for(i=0;i<7;i++) *(ptr+offset+1+i) = fix[i];
	off = 8;
	tmp = 0xFF;
	tmp1 = 0xFF;

	#if (USING_1to8==TRUE)
		for(i=0;i<8;i++)
	#else
		#ifdef IT6664
			for(i=0;i<TxPortNum;i++)
		#else
			for(i=1;i<TxPortNum+1;i++)
		#endif
	#endif
		{
			if(HPD[i] && EDID0data[i].block1)
			{
				cnt++;
				if((EDID1data[i].vsdb1.Vdsb8_CNandPresent & 0x80) != 0x80) lat = 0;
				if((EDID1data[i].vsdb1.Vdsb8_CNandPresent & 0x40) != 0x40) I_lat = 0;
				tmp &= EDID1data[i].vsdb1.Vdsb8_CNandPresent;
			}
		}
	*(ptr+offset+off) = tmp;
	off++;
	tmp = 0xFF;
	//Latency
	if(lat)
	{
		#if (USING_1to8==TRUE)
			for(i=0;i<8;i++)
		#else
			#ifdef IT6664
				for(i=0;i<TxPortNum;i++)
			#else
				for(i=1;i<TxPortNum+1;i++)
			#endif
		#endif
			{
				if(HPD[i] && EDID0data[i].block1)
				{
					tmp &= EDID1data[i].vsdb1.VideoLatency;
					tmp1 &= EDID1data[i].vsdb1.AudioLatency;
				}
			}
		*(ptr+off+offset) = tmp;
		*(ptr+off+1+offset) = tmp1;
		off += 2;
		tmp = 0xFF;
		tmp1 = 0xFF;
	}
	if(I_lat)
	{
		#if (USING_1to8==TRUE)
			for(i=0;i<8;i++)
		#else
			#ifdef IT6664
				for(i=0;i<TxPortNum;i++)
			#else
				for(i=1;i<TxPortNum+1;i++)
			#endif
		#endif
			{
				if(HPD[i] && EDID0data[i].block1)
				{
					tmp &= EDID1data[i].vsdb1.I_VideoLatency;
					tmp1 &= EDID1data[i].vsdb1.I_AudioLatency;
				}
			}
		*(ptr+off+offset) = tmp;
		*(ptr+off+1+offset) = tmp1;
		off += 2;
	}
	tmp = 0xFF;
	tmp1 = 0;
	// 3D present  VIC  len
	#if (USING_1to8==TRUE)
		for(i=0;i<8;i++)
	#else
		#ifdef IT6664
			for(i=0;i<TxPortNum;i++)
		#else
			for(i=1;i<TxPortNum+1;i++)
		#endif
	#endif
		{
			if(HPD[i] && EDID0data[i].block1)
			{
				tmp &= EDID1data[i].vsdb1.HDMI3D_Present;
				if(EDID1data[i].vsdb1.HDMI_VicLen) tmp1 |= 0x01;
				else tmp1 &= ~0x01;
				if(EDID1data[i].vsdb1.HDMI_3DLen) tmp1 |= 0x10;
				else tmp1 &= ~0x10;
			}
		}
	*(ptr+off+offset) = tmp;
	//iTE_Edid_Msg(("1. tmp1 =%02x  \r\n",tmp1));
	off++;
	I_lat = 0;
	//HDMI  VIC
	if(tmp1&0x01)
	{
		for(j=1;j<5;j++)
		{
			vid = 0;
			#if (USING_1to8==TRUE)
				for(i=0;i<8;i++)
			#else
				#ifdef IT6664
					for(i=0;i<TxPortNum;i++)
				#else
					for(i=1;i<TxPortNum+1;i++)
				#endif
			#endif
				{
					if(HPD[i] && EDID0data[i].block1)
					{
						for(lat=0;lat<EDID1data[i].vsdb1.HDMI_VicLen;lat++)
						{
							if(EDID1data[i].vsdb1.HDMIVic[lat] == j)
							{
								vid++;
							}

						}
					}
				}
			//iTE_Edid_Msg(("vid =%02x   j = %d  \r\n",vid,(int)j));
			if(vid==cnt)
			{
				*(ptr+off+offset+1+I_lat) = j;
				I_lat++;
			}
		}
	}
	*(ptr+off+offset) = (I_lat<<5);
	off += (1+I_lat);
	//3D info



	*(ptr+offset) = 0x60+off-1;

	return off+offset;
}
iTE_u8 it6664_ComposeEDIDBlock1_VSDB2_int(iTE_pu8 ptr,iTE_u8 offset)
{
	iTE_u8 i;

	iTE_u8 fix[8] = {0x67 ,0xD8 ,0x5D ,0xC4 ,0x01 ,0x78,0x80,0xFF};

	#if (USING_1to8==TRUE)
		for(i=0;i<8;i++)
	#else
		#ifdef IT6664
			for(i=0;i<TxPortNum;i++)
		#else
			for(i=1;i<TxPortNum+1;i++)
		#endif
	#endif
		{
			if(HPD[i] && EDID0data[i].block1)
			{
				fix[6] |= EDID1data[i].vsdb2.Byte1;
				fix[7] &= EDID1data[i].vsdb2.Byte2;
			}
		}
	for(i=0;i<8;i++) *(ptr+offset+i) = fix[i];

	return offset+8;
}
iTE_u8 it6664_ComposeEDIDBlock1_ExtTag_int(iTE_pu8 ptr,iTE_u8 offset , iTE_u8 tag)
{
	iTE_u8 ret,i,valid=0;
	iTE_u8 fix[3] = {0xE2,0x0F,0x03};
	iTE_u8 fix2[6] = {0xE5 ,0x0E ,0x60 ,0x61 ,0x65 ,0x66};//4K2K 420 only
	iTE_u8 tmp,tmp1;
	if(tag == 0x0F)
	{
		#if (USING_1to8==TRUE)
				for(i=0;i<8;i++)
		#else
			#ifdef IT6664
				for(i=0;i<TxPortNum;i++)
			#else
				for(i=1;i<TxPortNum+1;i++)
			#endif
		#endif
			{
				if(HPD[i] && EDID0data[i].block1)
				{
					if(gext_var->TXSupport420[i])
					{
						valid=1;
						break;
					}
				}
			}
		if(valid)
		{
			for(i=0;i<3;i++)
			{
				*(ptr+offset+i) = fix[i];
			}
			ret = offset+3;
		}
		else
		{
			ret = offset;
		}
	}
	else if(tag == 0x06)
	{
		tmp = 0xFF;
		tmp1 = 0xFF;

		#if (USING_1to8==TRUE)
			for(i=0;i<8;i++)
		#else
			#ifdef IT6664
				for(i=0;i<TxPortNum;i++)
			#else
				for(i=1;i<TxPortNum+1;i++)
			#endif
		#endif
			{
				if(HPD[i] && EDID0data[i].block1)
				{
					tmp &= EDID1data[i].HDR_Content[0];
					tmp1 &= EDID1data[i].HDR_Content[1];
				}
			}
		*(ptr+offset) = 0xE3;
		*(ptr+offset+1) = 0x06;
		*(ptr+offset+2) = tmp;
		*(ptr+offset+3) = tmp1;
		ret = offset + 4;
	}
	else if(tag == 0x0E)
	{
		for(i=0;i<6;i++)
		{
			*(ptr+offset+i) = fix2[i];
		}
		ret = offset+6;
	}

	return ret;
}
#endif
iTE_u8 it6664_ComposeEDIDBlock1_DTD(iTE_pu8 ptr,iTE_u8 offset)
{
	iTE_u8 i,ret,tmp;

	i=0;
	tmp = 0xFF;
	ret = offset;

#if (USING_1to8==TRUE)
		for(i=0;i<8;i++)
#else
	#ifdef IT6664
		for(i=0;i<TxPortNum;i++)
	#else
		for(i=1;i<TxPortNum+1;i++)
	#endif
#endif
		{
			if(HPD[i])
			{
				if((SupportVesaDTD[i]&0x01) == 0x00)
				{
					tmp &= ~0x0F;//2560x1440
				}
				if((SupportVesaDTD[i]&0x02) == 0x00)
				{
					tmp &= ~0xF0;//1366x768
				}
			}
		}

	if(tmp&0x0F)
	{
		for(i=0;i<18;i++) *(ptr+offset+i) = Dtd18Byte[3][i+1];
		ret += 18;
		offset += 18;
	}
	if(tmp&0xF0)
	{
		for(i=0;i<18;i++) *(ptr+offset+i) = Dtd18Byte[4][i+1];
		ret += 18;
		offset += 18;
	}
	tmp = 0;

#if (USING_1to8==TRUE)
		for(i=0;i<8;i++)
#else
	#ifdef IT6664
		for(i=0;i<TxPortNum;i++)
	#else
		for(i=1;i<TxPortNum+1;i++)
	#endif
#endif
		{
#if (USING_1to8==TRUE)
			{
				iTE_u8 k;

				if(i<4)
				{
					if(g_device != IT6664_B) IT6664_DeviceSelect(IT6664_B);
					k = i;
				}
				if(i>3)
				{
					if(g_device != IT6664_A) IT6664_DeviceSelect(IT6664_A);
					k = i-4;
				}

				if(HPD[i] && gext_var->TXSupport1080p[k])
					{
						tmp++;
					}
			}
#else
			if(HPD[i] && gext_var->TXSupport1080p[i])
			{
				tmp++;
			}
#endif
		}

	if(tmp == CheckHPDCnt())
	{
		for(i=0;i<18;i++) *(ptr+offset+i) = Dtd18Byte[0][i+1];
		ret += 18;
	}
	return ret;
}
iTE_u8 CompareResolution(iTE_u8 ref)
{
	iTE_u8 i,ret,tmp[4];
	ret= 1;

	#if (USING_1to8==TRUE)
			for(i=0;i<8;i++)
	#else
	#ifdef IT6664
		for(i=0;i<TxPortNum;i++)
	#else
		for(i=1;i<TxPortNum+1;i++)
	#endif
	#endif
		{
			if(HPD[i])
			{
#if (USING_1to8==TRUE)
				{
					iTE_u8 k;

					if(i<4)
					{
						if(g_device != IT6664_B) IT6664_DeviceSelect(IT6664_B);
						k = i;
					}
					if(i>3)
					{
						if(g_device != IT6664_A) IT6664_DeviceSelect(IT6664_A);
						k = i-4;
					}

					if(gext_var->TXSupport4K60[k])
					{
						tmp[i] = 10;
					}
					else if(gext_var->TXSupportOnly420[k])
					{
						tmp[i] = 9;
					}
					else if(gext_var->TXSupport4K30[k])
					{
						tmp[i] = 8;
					}
					else if(SupportVesaDTD[i]&0x01)
					{
						tmp[i] = 7;
					}
					else if(gext_var->TXSupport1080p[k])
					{
						tmp[i] = 6;
					}
					else
					{
						tmp[i] = 5;
					}
				}
#else
				if(gext_var->TXSupport4K60[i])
				{
					tmp[i] = 10;
				}
				else if(gext_var->TXSupportOnly420[i])
				{
					tmp[i] = 9;
				}
				else if(gext_var->TXSupport4K30[i])
				{
					tmp[i] = 8;
				}
				else if(SupportVesaDTD[i]&0x01)
				{
					tmp[i] = 7;
				}
				else if(gext_var->TXSupport1080p[i])
				{
					tmp[i] = 6;
				}
				else
				{
					tmp[i] = 5;
				}
#endif
			}
		}
	#if (USING_1to8==TRUE)
		for(i=0;i<8;i++)
	#else
	#ifdef IT6664
		for(i=0;i<TxPortNum;i++)
	#else
		for(i=1;i<TxPortNum+1;i++)
	#endif
	#endif
		{
			if(HPD[i] && (tmp[i] != tmp[ref]))
			{
				ret= 0;
			}
		}

	return ret;
}
iTE_u8 it6664_Check_HPDsts(void)
{

	iTE_u8 i,ret = 0;

	#ifdef IT6664
	for(i=0;i<TxPortNum;i++)
	#else
	for(i=1;i<TxPortNum+1;i++)
	#endif
	{
		if((h2txrd(i,0x03)&0x01) == 0x01) gext_u8->TXHPDsts |= (1<<i); //double confirm for calpclk spend 500ms issue
		else gext_u8->TXHPDsts &= ~(1<<i);
		if(HPD[i+Device_off*4] != ((gext_u8->TXHPDsts & (1<<i))>>i))
		{
			if(gext_u8->TXHPDsts & (1<<i))
			{
				iTE_Edid_Msg(("EDID check Port%d HPD on \r\n",(iTE_u16)i));
				ret |= 1<<i;
			}
			else
			{
				if(i == EDID_CopyPx) gext_u8->EDIDCopyDone = 0;
				iTE_Edid_Msg(("EDID check Port%d HPD off \r\n",(iTE_u16)i));
				gext_var->EDIDParseDone[i] = 0;//Tranmin for refresh EDID whatever plug in/out
			}
			HPD[i+Device_off*4] = (gext_u8->TXHPDsts & (1<<i))>>i;
			it6664_Edid_DataInit(i);
		}
	}
	return ret;
}

void it6664_Edid_DataInit(iTE_u8 port)
{
	iTE_u8 *ptr;
	iTE_u8 i;

	ptr = &EDID0data[port+Device_off*4].EstablishTimeing[0];
	for(i=0;i<sizeof(EDID0data[0]);i++) *ptr++ = 0;

	ptr = &EDID1data[port+Device_off*4].audio_info.Audiodes;
	for(i=0;i<sizeof(EDID1data[0]);i++) *ptr++ = 0;

	SupportVesaDTD[port+Device_off*4] = 0;
	gext_var->TXSupport420[port] = 0;
	gext_var->TXSupportDC420[port] = 0;
	gext_var->TXSupportOnly420[port] = 0;
	gext_var->TXSupport4K60[port] = 0;
	gext_var->TXSupport1080p[port] = 0;
	gext_var->TXSupport4K30[port] = 0;
	gext_var->EDIDParseDone[port]=0;
	gext_var->TXSupportHDR[port] = 0;
	gext_u8->EDIDCopyDone = 0;
}
iTE_u8 it6664_Edid_block0_parse(iTE_u8 port,iTE_u8 tmp[])
{
	iTE_u8 i,j,DTD[18],vic,ID;

	ID = 0;
	for(i=0;i<3;i++) EDID0data[port+Device_off*4].EstablishTimeing[i] = tmp[0x23+i];
	//iTE_Edid_Msg(("P%d EstablishTimeing = %02x %02x %02x \r\n",(iTE_u16)port,(iTE_u16)EDID0data[port].EstablishTimeing[0],(iTE_u16)EDID0data[port].EstablishTimeing[1],(iTE_u16)EDID0data[port].EstablishTimeing[2]));
	for(i=0;i<8;i++)
	{
		EDID0data[port+Device_off*4].STD[i][0] = tmp[0x26+i*2];
		EDID0data[port+Device_off*4].STD[i][1] = tmp[0x26+i*2+1];
	}
	for(i=0;i<17;i+=2)
	{
		if((tmp[0x26+i]==0xD1) && (tmp[0x26+i+1]==0xC0)) gext_var->TXSupport1080p[port] |= 0x01;
	}
	for(j=0;j<2;j++)
	{
		for(i=0;i<18;i++) DTD[i] = tmp[0x36+i+j*18];
		vic = it6664_DTD2VIC(DTD);
		//iTE_Edid_Msg(("Read DTD to VIC = 0x%02x  \r\n",(iTE_u16)vic));
		EDID0data[port+Device_off*4].DTDsupport[j] = vic ;
		if(vic == 16) gext_var->TXSupport1080p[port] |= 0x01;
		if(vic == 95) gext_var->TXSupport4K30[port] = 1;
		if(vic == 97) gext_var->TXSupport4K60[port] |= 0x10;
	}
	#ifdef EDID_Compose_Intersection
		for(i=0x36;i<0x7E;i++)
		{
			if((tmp[i]==0x00) && (tmp[i+1]==0x00) && (tmp[i+2]==0x00) && (tmp[i+3]==0xFC) && (tmp[i+4]==0x00))
			{
				ID = i;
				break;
			}
		}
		if(ID)
		{
			j=1;
			for(i=ID;i<0x7E;i++)
			{
				ProductName[port][j] = tmp[i];
				j++;
			}
			ProductName[port][0] = j;
			//iTE_Msg(("ID total length =0x%02X   \r\n", j));
		}
	#endif
	if(tmp[0x7E]) EDID0data[port+Device_off*4].block1 = 1;

	return TRUE;
}
iTE_u8 it6664_Edid_block1_parse(iTE_u8 port,iTE_u8 tmp[])
{
	iTE_u8 offset,length,offset_max,i,j;
	iTE_u8 cea_type,vic,DTD[18],DTDoffset,tmp1;

	DTDoffset = tmp[2];
	offset = 4;
    length = (tmp[4] & 0x1F);
	//iTE_Msg(("length=0x%02X   \r\n", (int)(tmp[4] & 0x1F)));
    offset_max = tmp[2];//block1 DTD
    //iTE_Msg(("offset_max=0x%02X\r\n", (int)offset_max));
	for(;;)
	{
        cea_type = (tmp[offset] & 0xE0) >> 5;
		//iTE_Msg(("CEA Type=0x%02X, len=%d\r\n", (int)cea_type, (int)(length - 1)));
		switch(cea_type)
		{
			case CEA_TAG_ADB:
				//iTE_Edid_Msg(("=== Read P%d EDID CEA_TAG_ADB === \r\n",(iTE_u16)port));
				it6664_Audio_Parse(port,tmp,offset);
				break;
			case CEA_TAG_VDB:
				//iTE_Edid_Msg(("=== Read P%d EDID CEA_TAG_VDB === \r\n",(iTE_u16)port));
				it6664_Video_Parse(port,tmp,offset);
				break;
			case CEA_TAG_VSDB:
				//iTE_Edid_Msg(("=== Read P%d EDID CEA_TAG_VSDB === \r\n",(iTE_u16)port));
				if((tmp[offset+1] == 0x03) && (tmp[offset+2] == 0x0C) && (tmp[offset+3] == 0x00))
				{
					it6664_Vsdb1_Parse(port,tmp,offset);
				}
				else if((tmp[offset+1] == 0xD8) && (tmp[offset+2] == 0x5D) && (tmp[offset+3] == 0xC4))
				{
					it6664_Vsdb2_Parse(port,tmp,offset);
				}
				else
				{
					//iTE_Edid_Msg(("VSDB Header error \n"));
				}
				break;
			case CEA_MODE_SPK:
				//iTE_Edid_Msg(("=== Read P%d EDID CEA_MODE_SPK === \r\n",(iTE_u16)port));
				EDID1data[port+Device_off*4].speaker_info = tmp[offset+1];
				//iTE_Edid_Msg(("speaker_info = 0x%02x \r\n",(iTE_u16)EDID1data[port].speaker_info));
				break;
			case CEA_MODE_DTC:
				break;
			case CEA_MODE_EXT:
				//iTE_Edid_Msg(("=== Read P%d EDID CEA_MODE_EXT === \r\n",(iTE_u16)port));
				it6664_EXT_Parse(port,tmp,offset);
				break;
			default:
                break;
		}
		offset += length+1;
		length = (tmp[offset]&0x1F);
		//iTE_Msg(("offset=0x%02X   \r\n", (int)offset));
		//iTE_Msg(("length=0x%02X   \r\n", (int)length));
        if (offset >= offset_max)
		{
            break;
		};
	}


	// just parse , not save
	tmp1 = 0xFF - DTDoffset;
	tmp1 = tmp1/18;
	for(j=0;j<tmp1;j++)
	{
		for(i=0;i<18;i++) DTD[i] = tmp[DTDoffset+i+j*18];
		vic = it6664_DTD2VIC(DTD);
		/*
		if((vic == 97) || (vic == 96)) gext_var->TXSupport4K60[port] = 1;
		if(vic == 16) gext_var->TXSupport1080p[port] = 1;
		if(vic == 95) gext_var->TXSupport4K30[port] = 1;
		*/
		switch(vic)
		{
			case 16:
					gext_var->TXSupport1080p[port] |= 0x01;
					break;
			case 95:
					gext_var->TXSupport4K30[port] = 1;
					break;
			case 96:
			case 97:
					gext_var->TXSupport4K60[port] |= 0x10;
					break;
			case 0xF0: // 2560x1440
					SupportVesaDTD[port+Device_off*4] |= 0x01;
					break;
			case 0xF1: // 1366x768
					SupportVesaDTD[port+Device_off*4] |= 0x02;
					break;
			default:
					break;
		}
	}

	return TRUE;
}
void it6664_Audio_Parse(iTE_u8 port,iTE_u8 table[],iTE_u8 offset)
{
	iTE_u8 i,des;
	des = table[offset] & 0x1F;
	EDID1data[port+Device_off*4].audio_info.Audiodes = des;
	for(i=0;i<des;i++) EDID1data[port+Device_off*4].audio_info.Format[i] = table[offset+1+i];
	//iTE_Edid_Msg(("Audio Data \r\n"));
	//for(i=0;i<des;i++) iTE_Edid_Msg(("0x%02x ",(iTE_u16)EDID1data[port].audio_info.Format[i]));
	//iTE_Edid_Msg(("\r\n"));
}
void it6664_Video_Parse(iTE_u8 port,iTE_u8 table[],iTE_u8 offset)
{
	iTE_u8 i,cnt,tmp;
	cnt = table[offset] & 0x1F;
	EDID1data[port+Device_off*4].video_info.Viccnt = cnt;

	for(i=0;i<cnt;i++)
	{
		EDID1data[port+Device_off*4].video_info.vic[i] =  table[offset+1+i];
		tmp = EDID1data[port+Device_off*4].video_info.vic[i];
		if(((tmp >= 1)&&(tmp <= 64)) || ((tmp >= 129)&&(tmp <= 192)))//remove native bit
		{
			tmp &= 0x7F;
			EDID1data[port+Device_off*4].video_info.vic[i] = tmp;
		}
		if(tmp == 16) gext_var->TXSupport1080p[port] |= 0x01;
		if((tmp == 95)||(tmp == 94)) gext_var->TXSupport4K30[port] = 1;
		if((tmp == 96)||(tmp == 97)) gext_var->TXSupport4K60[port] |= 0x10;
		//iTE_Edid_Msg(("0x%02x  ",(iTE_u16)EDID1data[port].video_info.vic[i]));
	}
	//iTE_Edid_Msg(("\r\n"));
}
void it6664_Vsdb1_Parse(iTE_u8 port,iTE_u8 table[],iTE_u8 offset)
{
	iTE_u8 next_offset = 0,i,tmp,len,tolen,u8_3DType;
	iTE_u16 u16tmp;

	tolen = table[offset]&0x1F;
	//iTE_Edid_Msg(("VSDB1 phyaddr01 = 0x%02x  0x%02x \r\n",(iTE_u16)table[offset+4],(iTE_u16)table[offset+5]));
	EDID1data[port+Device_off*4].vsdb1.phyaddr0 = table[offset+4];
	EDID1data[port+Device_off*4].vsdb1.phyaddr1 = table[offset+5];
	gext_var->CEC_AB[port] = table[offset+4];
	gext_var->CEC_CD[port] = table[offset+5];
	if(tolen>5)
	{
		EDID1data[port+Device_off*4].vsdb1.DCSupport = table[offset+6];
		gext_var->TXSupport1080p[port] |= (EDID1data[port+Device_off*4].vsdb1.DCSupport & 0x30);
		//iTE_Edid_Msg(("DCSupport =  0x%02x \r\n",(iTE_u16)EDID1data[port].vsdb1.DCSupport));
		if((tolen == 6)||(tolen == 7)) return;

		EDID1data[port+Device_off*4].vsdb1.Vdsb8_CNandPresent = table[offset+8];

		if( EDID1data[port+Device_off*4].vsdb1.Vdsb8_CNandPresent & 0x80)
   		{
    	   	EDID1data[port+Device_off*4].vsdb1.VideoLatency = table[offset+9];
			EDID1data[port+Device_off*4].vsdb1.AudioLatency = table[offset+10];
    	   	next_offset += 2;
			if(tolen == 10) return;
  	 	}
    	if( EDID1data[port+Device_off*4].vsdb1.Vdsb8_CNandPresent & 0x40)
   		{
       		EDID1data[port+Device_off*4].vsdb1.I_VideoLatency = table[offset+11];
			EDID1data[port+Device_off*4].vsdb1.I_AudioLatency = table[offset+12];
        	next_offset += 2;
			if(tolen == 12) return;
   	 	}
		EDID1data[port+Device_off*4].vsdb1.HDMI3D_Present = table[offset+9+next_offset];
		EDID1data[port+Device_off*4].vsdb1.HDMI_VicLen = (table[offset+10+next_offset]&0xE0) >> 5;
		EDID1data[port+Device_off*4].vsdb1.HDMI_3DLen = table[offset+10+next_offset]&0x1F;
		//iTE_Edid_Msg(("viclen =  0x%02x \r\n",(iTE_u16)EDID1data[port].vsdb1.HDMI_VicLen));
		for(i=0;i<EDID1data[port+Device_off*4].vsdb1.HDMI_VicLen;i++)
		{
			EDID1data[port+Device_off*4].vsdb1.HDMIVic[i] = table[offset+11+next_offset+i];
			gext_var->TXSupport4K30[port] = 1;
			//iTE_Edid_Msg(("HDMIVic %d = 0x%02x \r\n",(iTE_u16)i,(iTE_u16)EDID1data[port].vsdb1.HDMIVic[i]));
		}
		next_offset += EDID1data[port+Device_off*4].vsdb1.HDMI_VicLen;

		if(EDID1data[port+Device_off*4].vsdb1.HDMI_3DLen)
		{
			//iTE_Edid_Msg(("HDMI_3DLen\r\n"));
			EDID1data[port+Device_off*4].vsdb1.Support3D = 1;
			tmp = EDID1data[port+Device_off*4].vsdb1.HDMI3D_Present;
			if(((tmp&0x60)==0x20) || ((tmp&0x60)==0x40))
			{
				EDID1data[port+Device_off*4].vsdb1._3D_Struct_All_15_8 = table[offset+11+next_offset];
				EDID1data[port+Device_off*4].vsdb1._3D_Struct_All_7_0 = table[offset+11+next_offset+1];
				next_offset+=2;
			}
			if((tmp&0x60)==0x40)
			{
				len = 0;
				//EDID1data[port+Device_off*4].vsdb1._3D_MASK_All_15_8 = table[offset+11+next_offset];
				//EDID1data[port+Device_off*4].vsdb1._3D_MASK_All_7_0 = table[offset+11+next_offset+1];
				u16tmp = (table[offset+11+next_offset]*0x100) + table[offset+11+next_offset+1];
				for(i=0;i<16;i++)
				{
					if(u16tmp & (1<<i))
					{
						EDID1data[port+Device_off*4].vsdb1._3D_Vic_Sup[len]
						= EDID1data[port+Device_off*4].video_info.vic[i];
						//printf(" VIC is %2x \r\n",EDID1data[port+Device_off*4].video_info.vic[i]);
						//printf("3D SUPPORT VIC is %2x \r\n",EDID1data[port+Device_off*4].vsdb1._3D_Vic_Sup[len]);
						len++;
					}
				}
				EDID1data[port+Device_off*4].vsdb1._3D_Vic_SupCnt = len;
				//iTE_Edid_Msg(("len = %2x\r\n",len));
				next_offset+=2;
			}
			len = table[offset] & 0x1F;
			tmp = len - 10 - next_offset;
			len = 0;
			tolen = 0;
			for(i=0;i<tmp;i++)
			{
				//EDID1data[port+Device_off*4].vsdb1._3D_Order[i] = table[offset+11+next_offset+i];
				tolen = (table[offset+11+next_offset+i]&0xF0)>>4;
				u8_3DType= table[offset+11+next_offset+i]&0x0F;
				EDID1data[port+Device_off*4].vsdb1._3D_Order[len]
				= (u8_3DType*0x100) + EDID1data[port+Device_off*4].video_info.vic[tolen];
				//printf("3D order  is  %4x \r\n",EDID1data[port+Device_off*4].vsdb1._3D_Order[len]);
				if(u8_3DType == 0x08) i++;
				len++;
			}
			EDID1data[port+Device_off*4].vsdb1._3D_Order_len = len;
		}
	}
}
void it6664_Vsdb2_Parse(iTE_u8 port,iTE_u8 table[],iTE_u8 offset)
{
	iTE_u8 tmp;

	//iTE_Edid_Msg(("Vsdb2 bit6 = 0x%02x   \r\n",(iTE_u16)table[offset+6]));
	//iTE_Edid_Msg(("Vsdb2 bit7 = 0x%02x   \r\n",(iTE_u16)table[offset+7]));
	tmp = table[offset+6];
	EDID1data[port+Device_off*4].vsdb2.Byte1 = tmp;
	EDID1data[port+Device_off*4].vsdb2.SCDCPresent = (tmp & 0x80)>>7;
	EDID1data[port+Device_off*4].vsdb2.RR_Capable = (tmp & 0x40)>>6;
	EDID1data[port+Device_off*4].vsdb2.LTE_340M_scramble = (tmp & 0x08)>>3;
	EDID1data[port+Device_off*4].vsdb2.IndependentView = (tmp & 0x04)>>2;
	EDID1data[port+Device_off*4].vsdb2.DualView = (tmp & 0x02)>>1;
	EDID1data[port+Device_off*4].vsdb2.OSD_3D = tmp & 0x01;
	tmp = table[offset+7];
	EDID1data[port+Device_off*4].vsdb2.Byte2 = tmp;
	EDID1data[port+Device_off*4].vsdb2.DC_420 = tmp & 0x07;
	gext_var->TXSupportDC420[port] = tmp & 0x07;
	gext_var->TXSupport4K60[port] |= 0x01;
}
void it6664_EXT_Parse(iTE_u8 port,iTE_u8 table[],iTE_u8 offset)
{
	iTE_u8 tag,len,i,k,tmp;
	iTE_u8 vic;

	tag = table[offset+1];
	len = table[offset]&0x1F;
	if(tag == 0xE)//only support 420
	{
		for(i=0;i<len;i++)
		{
			if((table[offset+2] == 0x60) || (table[offset+2] == 0x61))
			gext_var->TXSupportOnly420[port] |= 0x01;
			if((table[offset+2] == 0x65) || (table[offset+2] == 0x66))
			gext_var->TXSupportOnly420[port] |= 0x10;
		}
	}
	else if(tag == 0xF)//support 420
	{
		for(i=0;i<len-1;i++)
		{
			tmp = table[offset+2+i] ;//bitmap table
			//iTE_Edid_Msg(("bit table = 0x%02x   \r\n",(iTE_u16)tmp));
			if(tmp)
			{
				for(k=0;k<8;k++)
				{
					if(tmp & (1<<k))//bitmap compare
					{
						vic = EDID1data[port+Device_off*4].video_info.vic[i*8+k];//real vic
						if((vic == 0x60) || (vic == 0x61)) gext_var->TXSupport420[port] |= 0x01;
						if((vic == 0x65) || (vic == 0x66)) gext_var->TXSupport420[port] |= 0x10;
					}
				}
				if((gext_var->TXSupport420[port])&&(!(gext_var->TXSupport4K60[port]&0x01)))
				{
					gext_var->TXSupport420[port] = 0;
					gext_var->TXSupportOnly420[port] = 1;
				}
			}
		}
	}
	else if(tag == 0x6)//HDR
	{
		gext_var->TXSupportHDR[port] = 1;
		EDID1data[port+Device_off*4].HDR_Content[0] = table[offset+2];
		EDID1data[port+Device_off*4].HDR_Content[1] = table[offset+3];
	}
}

iTE_u8 it6664_DTD2VIC(iTE_u8 table[])
{
	iTE_u8 vic=0,i,j,valid=0;

	for(i=0;i<5;i++)
	{
		if((table[0] == Dtd18Byte[i][1]) && (table[1] == Dtd18Byte[i][2]))
		{
			for(j=2;j<11;j++)
			{
				if(table[j] == Dtd18Byte[i][j+1]) valid+=1;
			}
			if(valid == 9) vic= Dtd18Byte[i][0];
		}
		valid = 0;
	}
	return vic;
}
iTE_u8 it6664_EDIDCompare(iTE_pu8 arry1,iTE_pu8 arry2)
{
	iTE_u8 i,ret=0;

	for(i=0;i<127;i++)
	{
		if(*(arry1+i) == *(arry2+i))
		{
			ret = 0;
		}
		else
		{
			ret = 1;
			break;
		}
	}
	iTE_Msg(("ret = %d  ,i =%d \r\n",ret,i));
	return ret;
}
void EDID_48Bit_Remove(iTE_pu8 arry1)
{
	iTE_u8 i,j;
	j=0xFF;

	for(i=0;i<0x7F;i++)
	{
		if((*(arry1+i) == 0x03) && (*(arry1+i+1) == 0x0C) && (*(arry1+i+2) == 0x00))
		{
			j=i;
			break;
		}
	}
	if(j !=0xFF)
	{
		if((*(arry1+j-1)&0x1F)>5)
		{
			*(arry1+j+5) &= ~0x40;
		}
	}
	j=0xFF;
	for(i=0;i<0x7F;i++)
	{
		if((*(arry1+i) == 0xD8) && (*(arry1+i+1) == 0x5D) && (*(arry1+i+2) == 0xC4))
		{
			j=i;
			break;
		}
	}
	if(j !=0xFF)
	{
		if((*(arry1+j-1)&0x1F)>6)
		{
			*(arry1+j+6) &= ~0x04;
		}
	}

}
void it6664_Edid_PhyABCD_Update(iTE_pu8 arry1)
{
	iTE_u8 i,u8AB,u8CD;
	iTE_u16 ABCD;
	for(i=0;i<0x80;i++)
	{
		if((*(arry1+i) == 0x03) && (*(arry1+i+1) == 0x0C) && (*(arry1+i+2) == 0x00))
		{
			i=i+3;
			break;
		}
	}
	u8AB = *(arry1+i);
	u8CD = *(arry1+i+1);
	ABCD = (iTE_u16)(u8AB*0x100)+ (iTE_u16)(u8CD);
	iTE_Edid_Msg(("ABCD = %04x \r\n",ABCD));
	if((ABCD&0xF000) == 0)
	{
		ABCD += 0x1000;
	}
	else if((ABCD&0x0F00) == 0)
	{
		ABCD += 0x0100;
	}
	else if((ABCD&0x00F0) == 0)
	{
		ABCD += 0x0010;
	}
	else if((ABCD&0x000F) == 0)
	{
		ABCD += 0x0001;
	}
	else
	{
		ABCD = 0xFFFF;
	}
	iTE_Edid_Msg(("ABCD = %04x \r\n",ABCD));
	*(arry1+i) = (iTE_u8)(ABCD>>8);
	*(arry1+i+1) = (iTE_u8)(ABCD & 0xFF);

}
void it6664_Edid_Chk_4KDTD(iTE_pu8 arry1 ,iTE_u8 block)
{
	iTE_u8 DTD[18];
	iTE_u8 i,j,vic,cnt,off;
	if(block)
	{
		off = arry1[2];
		cnt = (128-off)/18;//Max DTD cnt
		for(j=0;j<cnt;j++)
		{
			for(i=0;i<18;i++) DTD[i] = arry1[off+i+j*18];
			vic = it6664_DTD2VIC(DTD);
			if((vic == 95) && ((DTD[17]&0x1E)!=0x1E))
			{
				arry1[off+j*18+17] = 0x1E;
			}
		}
	}
	else
	{
		for(j=0;j<2;j++)
		{
			for(i=0;i<18;i++) DTD[i] = arry1[0x36+i+j*18];
			vic = it6664_DTD2VIC(DTD);
			if((vic == 95) && ((DTD[17]&0x1E)!=0x1E))
			{
				arry1[0x36+j*18+17] = 0x1E;
			}
		}
	}

}
iTE_u8 Is_DTD_4K533(iTE_pu8 DTD)
{
	iTE_u16 DTD_Vclk,H_Act,V_Act,H_tmp;
	iTE_u8 ret=0;

	H_tmp = (iTE_u16) DTD[1];
	DTD_Vclk = (H_tmp <<8) + (iTE_u16) DTD[0];
	H_tmp = (iTE_u16) DTD[4]>>4;
	H_Act = (H_tmp<<8) + (iTE_u16) DTD[2];
	H_tmp = (iTE_u16) DTD[7]>>4;
	V_Act = (H_tmp<<8) + (iTE_u16) DTD[5];
	if((DTD_Vclk > 50000)&&(DTD_Vclk<55000)
			&&(H_Act==3840)&&(V_Act==2160))
	{
		ret = 1;
	}
	return ret;

}
void EDID_Remove4K533(iTE_pu8 arry1 ,iTE_u8 block)
{
	iTE_u8 DTD[18];
	iTE_u8 i,j,cnt,off;
	iTE_u8 Default_DTD[18] =
	{	//480p
		0x8C, 0x0A, 0xD0, 0x8A, 0x20, 0xE0,
		0x2D, 0x10, 0x10, 0x3E, 0x96, 0x00,
		0x6D, 0x55, 0x21, 0x00, 0x00, 0x18
	};

	if(block)
	{
		off = arry1[2];
		cnt = (128-off)/18;//Max DTD cnt
		for(j=0;j<cnt;j++)
		{
			for(i=0;i<18;i++) DTD[i] = arry1[off+i+j*18];
			if(Is_DTD_4K533(DTD)==iTE_TRUE)
			{
				for(i=0;i<18;i++)  arry1[off+i+j*18] = Default_DTD[i];
			}
		}
	}
	else
	{
		for(j=0;j<2;j++)
		{
			for(i=0;i<18;i++) DTD[i] = arry1[0x36+i+j*18];

			if(Is_DTD_4K533(DTD)==iTE_TRUE)
			{
				for(i=0;i<18;i++)  arry1[0x36+i+j*18] = Default_DTD[i];
			}
		}
	}
}
void EDID_HDMI21_Remove(iTE_pu8 arry1)
{
	iTE_u8 i,j,len,shift_offset,x;
	iTE_u8 tmp[128];

	j=0xFF;

	for(i=0;i<0x7F;i++)
	{
		if((*(arry1+i) == 0xD8) && (*(arry1+i+1) == 0x5D) && (*(arry1+i+2) == 0xC4))
		{
			j=i;
			break;
		}
	}

	len = *(arry1+j-1)&0x1F;
	*(arry1+j+6) &= 0x0F;
	if(len>7)
	{
		shift_offset = len-7;
		*(arry1+j-1) = 0x67;
		*(arry1+2) -= shift_offset;//DTD offset
		x=0;
		for(i=(j+len);i<0x7F;i++)
		{
			tmp[x] = *(arry1+i);
			x++;
		}
		for(i=0;i<shift_offset;i++)
		{
			tmp[x+i] = 0;
		}
		for(i=0;i<(x+shift_offset);i++)
		{
			*(arry1+j+7+i) =  tmp[i];
		}

	}
}
void EDID_HDMI21_VicRemove(iTE_pu8 arry1)
{
	iTE_u8 offset,tmp,offset_max,length,i,skip;
	iTE_u8 tar=0, tmpdata[50];
	iTE_u8 tmpdata2[128];

	skip = 0;
	offset = 4;
	length = (*(arry1+4) & 0x1F);
	offset_max = *(arry1+2);
	for(;;)
	{
    tmp = (*(arry1+offset) & 0xE0) >> 5;
		switch(tmp)
		{
			case CEA_TAG_VDB:
						tar = offset;
				break;

			default:
        break;
		}
		offset += length+1;
		length = (*(arry1+offset)&0x1F);

    if (offset >= offset_max)
		{
       break;
		};
	}
	//iTE_Edid_Msg(("VDB start = 0x%02x \r\n", *(arry1+tar)));
	length = (*(arry1+tar)&0x1F);
	offset_max = 0;
	for(i=0;i<length;i++)
	{
		tmp = *(arry1+tar+i);
		if(((tmp>128)&&(tmp<192))||(tmp<117))
		{
			tmpdata[offset_max] = tmp;
			offset_max++;
		}
	}
	//iTE_Edid_Msg(("offset_max = 0x%02x \r\n", offset_max));
	if(offset_max<length)
	{
		offset = length-offset_max;// shift
		tmpdata[0] -= offset;
	}
	else if(offset_max==length)
	{
		skip = 1;
	}
	//iTE_Edid_Msg(("skip = %x \r\n", skip));
	if(!skip)
	{
		tmp = 0;
		for(i=(tar+length);i<0x7F;i++)
		{
			tmpdata2[tmp] = *(arry1+i);
			tmp++;
		}
		for(i=0;i<offset;i++)
		{
			tmpdata2[tmp+i] = 0;
		}
		for(i=0;i<offset_max;i++)
		{
			*(arry1+tar+i) = tmpdata[i];
		}
		for(i=0;i<tmp;i++)
		{
			*(arry1+tar+offset_max+i) = tmpdata2[i];
		}
		*(arry1+2)-=offset;
	}
}
#if 1
void Show_EDID(iTE_pu8 ptr)
{
	iTE_u8 i,j;

	iTE_Edid_Msg(("----------  EDID Compose BLOCK  ------------\r\n"));
	for(j=0;j<128;j+=8)
	{
		for(i=0;i<8;i++) iTE_Edid_Msg((" %02x",(iTE_u16)*(ptr+i+j)));
		iTE_Edid_Msg(("\r\n"));
	}
	iTE_Edid_Msg(("---------------------------------------------\r\n"));

}
#endif
iTE_u8 CheckHPDCnt(void)
{
	iTE_u8 i,ret=0;

	#if (USING_1to8==TRUE)
		for(i=0;i<8;i++)
	#else
		for(i=0;i<4;i++)
	#endif
		{
			if(HPD[i] && EDID0data[i].block1) ret++;
		}
	return ret;
}
iTE_u8 CalChecksum(iTE_u8 edid[],iTE_u8 block)
{
	iTE_u16 i,sum,Checksum;
	sum=0;
	Checksum=0;
	if(block==0)
	{
		for(i=0x00;i<0x7F;i++)	sum=sum+edid[i];
	}
	Checksum= 0x100- (sum & 0xFF);
	return Checksum;
}
iTE_u8 it6664_read_edid(iTE_u8 port, iTE_u8 block, iTE_u8 offset, iTE_u16 length, iTE_u8 *edid_buffer)
{
	iTE_u8 result = FALSE;
    iTE_u16 off = block*128 + offset;
    //iTE_u8 segment = off / 256;
    iTE_u16 retry = 0;

    offset = off % 256;

	// Enable DDC Bus Reset
	// hdmitxset(0x28, 0x01, EnDDCMasterSel);   // Enable PC DDC Mode
	// Do not handle the DDC Bus Hang here
	// hdmitxwr(0x2E, 0x0F);  // Abort DDC Command
__RETRY:

	h2txset(port, 0x18, 0x0C, 0x0C);    // Enable DDC Command Fail Interrupt
	h2txset(port, 0x1A, 0x08, 0x08);    // Enable DDC Bus Hang Interrupt

	h2txset(port, 0x28, 0x01, 0x01);    // Enable PC DDC Mode

	h2txwr(port, 0x2E, 0x09);                  // DDC FIFO Clear
	h2txwr(port, 0x29, 0xA0);                  // EDID Address
	h2txwr(port, 0x2A, offset);                // EDID Offset
	h2txwr(port, 0x2B, length);                // Read ByteNum[7:0]
	h2txwr(port, 0x2C, (length & 0x300) >> 8); // Read ByteNum[9:8]
	h2txwr(port, 0x2D, block / 2);           // EDID Segment

	if ((h2txrd(port,0x03) & 0x01) == 0x01) {
		h2txwr(port,0x2E, 0x03);  // EDID Read Fire

		if (ddcwait(port)) {
			h2txbrd(port,0x30, length, edid_buffer);
			result = TRUE;
		} else {
			iTE_Edid_Msg(("ERROR: DDC EDID Read Fail !!!\r\n"));
            if ( retry > 0 ) {
                retry--;
               mSleep(50);
                goto __RETRY;
            }
		}
	} else {
		iTE_Edid_Msg(("Abort EDID read becasue of detecting unplug !!!\r\n"));
	}

	h2txset(port,0x28, 0x01, 0x00);   // Disable PC DDC Mode

	return result;
}
iTE_u8 it6664_read_one_block_edid(iTE_u8 port,iTE_u8 block, iTE_u8 *edid_buffer)
{
	iTE_u8 offset;
	iTE_u8 i;
	iTE_u8 read_len = 32;
    iTE_u8 retry = 3;

__RETRY:
	offset = 0;
	for (i = 0; i < 128 / read_len; i++) {
		if (it6664_read_edid(port,block, offset, read_len, edid_buffer)) {
			edid_buffer += read_len;
			offset += read_len;
			continue;
		} else {
            if ( retry > 0 ) {
                retry--;
                goto __RETRY;
            }
            else {
                iTE_Edid_Msg(("ERROR: read edid block 0, offset %d, length %d fail.\r\n", (iTE_u16)offset, (iTE_u16)read_len));
                return FALSE;
            }
		}
	}

	return TRUE;
}




///*****************************************
//  Copyright (C) 2009-2021
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   <IT6664.h>
//   @author Hojim.Tseng@ite.com.tw
//   @date   2021/01/12
//   @fileversion: ITE_HDMI2_SPLITER_1.38
//******************************************/
#ifndef _IT6664_H_
#define _IT6664_H_
//#include "IT6664_EDID.h"
//#ifdef _MCU_IT6350
//#include "..\..\IT6350\code\api\debug\debug_print.h"
//#endif
#include "IT6664_extern.h"
#include "version.h"

enum
{
    STAT_MCU_INIT,
    STAT_CHECK_TRAPING,
    STAT_CHECK_DEV_READY,
    STAT_DEV_INIT,
    STAT_IDLE,
    STAT_IDLE2,
};
void it6664(void);

#endif

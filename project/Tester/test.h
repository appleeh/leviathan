/********************************************************************************/
/*																				*/
/*   �� �ҽ� �ڵ��� �Ǹ��� ����ϴ� �ּ��� �������� ���ÿ�.							*/
/*   Copyright 2021 by keh													*/
/*																				*/
/********************************************************************************/

#pragma once

#include "comLogger.h"
#include "data.h"

extern int g_SleepMilliSec_Param;
extern int g_SleepMilliSec_Event;
extern int g_nSendParamCnt;
extern int g_nTotEventSendCount;
extern int g_nParameterThreadCnt;
extern int g_IsLogProcTime;
extern int g_nTotParameterSendCount;
extern int g_nParameterCntWithEventTrigger;



extern CMetaData g_cMetaData;

bool init();
void destroy();
void mainLoop();

// LibraryTester.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include "test.h"
#include "init.h"
#include "comCore.h"
#include "exception_handler.h"

int g_nTotEventSendCount;
int g_SleepMilliSec_Param;
int g_SleepMilliSec_Event;
int g_nSendParamCnt;
int g_nParameterThreadCnt;
int g_IsLogProcTime; // InfiniA_Library Proc Time threshold
int g_nTotParameterSendCount;
int g_nParameterCntWithEventTrigger;


// ���� ��Ÿ�����͸� �о�ͼ� �׽�Ʈ �Ѵ�.
CMetaData g_cMetaData;


// ���̺귯���� �׽�Ʈ �ϱ� ���� config ������ ���� �׽�Ʈ ȯ���� �����Ѵ�.
bool initTestProgramConfig()
{
	char szDir[256], szBuf[256];
	DWORD nRes;

	// �̺�Ʈ �����͸� ������ ����
	g_nTotEventSendCount = GetPrivateProfileInt(_T("PROGRAM"), _T("TOTAL_EVENT_SEND_COUNT"), 0, g_pProcessConfig);

	// ������ �ϳ��� �Ķ���͸� ������ �ֱ� ����
	g_SleepMilliSec_Param = GetPrivateProfileInt(_T("PROGRAM"), _T("SLEEP_MILISEC_PARAM"), 0, g_pProcessConfig);

	// �̺�Ʈ Loop ���� �̺�Ʈ �ϳ��� ������ �ֱ�
	g_SleepMilliSec_Event = GetPrivateProfileInt(_T("PROGRAM"), _T("SLEEP_MILISEC_EVENT"), 0, g_pProcessConfig);

	nRes = GetProfilePath(_T("PROGRAM"), _T("METADATA_PATH"), _T("Data//"), szDir, g_pProcessConfig);
	_stprintf(szBuf, _T("%s%s"), szDir, _T("Data_Parameter.txt"));
	g_cMetaData.setParamPath(szBuf);
	nRes = GetProfilePath(_T("PROGRAM"), _T("METADATA_PATH"), _T("Data//"), szDir, g_pProcessConfig);
	_stprintf(szBuf, _T("%s%s"), szDir, _T("Data_EventMap.txt"));
	g_cMetaData.setEventPath(szBuf);

	// �Ķ���͸� ������ �Ѳ����� ������ �Ķ���� ���� ����
	g_nSendParamCnt = GetPrivateProfileInt(_T("PROGRAM"), _T("SEND_PARAMCNT"), 100, g_pProcessConfig);

	// �Ķ���͸� ������ �������� �� ���� ����(��Ƽ������ �׽�Ʈ�� ����)
	g_nParameterThreadCnt = GetPrivateProfileInt(_T("PROGRAM"), _T("PARAMETER_THREAD_COUNT"), 0, g_pProcessConfig);

	// �׽�Ʈ ���α׷����� �����͸� ����� Send �ϴµ� ���� �ɸ��� �ѽð��� �Ʒ��� �и������带 �Ѿ�� �α� ������ ���� ����
	// 50 milliseconds �� �����Ͽ�, �����͸� ���� ������ ������ �ɸ��� �ѽð��� 50�̸������带 �Ѿ������ �α׸� ���⵵�� �����Ѵ�.
	g_IsLogProcTime = (unsigned int)GetPrivateProfileInt(_T("PROGRAM"), _T("IS_LOG_PROC_TIME"), 0, g_pProcessConfig);
	// ����Ʈ 0 : ���� ����� �������� ����

	g_nTotParameterSendCount = GetPrivateProfileInt(_T("PROGRAM"), _T("TOTAL_PARAMETER_SEND_COUNT"), 0, g_pProcessConfig);;
	g_nParameterCntWithEventTrigger = GetPrivateProfileInt(_T("PROGRAM"), _T("SEND_PARAMCNT_WITH_EVENT"), 0, g_pProcessConfig);;

	return true;
}



int main()
{
	exception_handler::instance()->initialize("handler.dmp");
	exception_handler::instance()->run();


	// �׽�Ʈ ���α׷� ������ ���� config ����
	if (!initTestProgramConfig()) {
		return 0;
	}

	// ��Ÿ������ �޸𸮸� �ʱ�ȭ
	if (!g_cMetaData.init()) {
		return 0;
	}

	// InfinA_Library.dll �� ����ϱ� ���� �ش� ���̺귯�� �ʱ�ȭ
	if (!initConfig_forInfiniALibrary()) {
		return 0;
	}


	if(EqLib_CommConnect()) {
		printf("EDA Server Connect Success!!\n");
	}
	else {
		printf(EqLib_GetCurrentError()); // error Msg
	}

	// ������ mainLoop ���� �ֱ������� �̺�Ʈ�� ������
	mainLoop();

	return 0;
}


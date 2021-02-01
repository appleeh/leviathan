
#include "RTSQueueHandler.h"
#include "comEnv.h"

static func_type g_fProcessLoop = NULL;
static void *gs_pChild = NULL;

CRTSQueueHandler::CRTSQueueHandler() :CRTSQueue()
{

}


CRTSQueueHandler::~CRTSQueueHandler()
{
	destroyWorkerThread(NULL);
}

int CRTSQueueHandler::init(int idx)
{
	char szSect[16];
	sprintf(szSect, "IOCP_%d", idx);
	int nThreadCnt = GetPrivateProfileInt(szSect, "THR_COUNT", 1, g_pSystem);
	int nNum = GetPrivateProfileInt(szSect, "MAX", 1, g_pSystem);
	return init(nNum, nThreadCnt);
}


int CRTSQueueHandler::init(int nMax, int nThreadCnt)
{
	int nErrorCode;
	nErrorCode = CRTSQueue::init(nMax, nThreadCnt);

	if(nErrorCode != CERROR_NONE) {
		printf("CRTSQueueHandler::init [%s]\n", err_desc_socket[nErrorCode]);
		return 0;
	}

	return 1;
}

int CRTSQueueHandler::start(func_type pFunction, void *pChild)
{
	int i;
	int nThreadCount = getThreadCount();

	m_hCreateThread =  CreateEvent(0, FALSE, FALSE, 0);
	m_WorkerThreadVector.reserve(nThreadCount);
	STThreadInfo *pthInfo;
	g_fProcessLoop = pFunction;
	gs_pChild = pChild;

	for(i = 0 ; i < nThreadCount; i++) {
		pthInfo = new STThreadInfo;
		pthInfo->pClass = this;
		pthInfo->bActive = true;
        pthInfo->nSeqNum    = i;
		if (!THREAD_CREATE(&pthInfo->hTHID, CRTSQueueHandler::WorkerThread, pthInfo)) {
			printf("Create WorkerThread[%p]\n",pthInfo->hTHID);
			delete pthInfo;
			return 0;
		}
		WaitForSingleObject(m_hCreateThread, INFINITE);
        ResetEvent(m_hCreateThread);
		m_WorkerThreadVector.push_back(pthInfo);
	}
	CloseHandle(m_hCreateThread);
	return m_WorkerThreadVector.size();
}

void CRTSQueueHandler::destroyWorkerThread(HANDLE hID)
{
	STThreadInfo *pthInfo;
	HANDLE hTmp;

	THREADVECTOR::iterator it = m_WorkerThreadVector.begin();
	while (it != m_WorkerThreadVector.end()) 
	{
		pthInfo = (*it);
		
		if(hID != NULL && hID != pthInfo->hTHID) continue;

		hTmp = pthInfo->hTHID;

		//TerminateThread(pthInfo->hTHID, 0);	// 1. �� �����尡 (�� target �����幮 �ۿ���) target �����带 ���� �����Ŵ
		pthInfo->bActive = false;				// 2. target ������ ������ while ���� �ڿ������� ���������� �����ν� �����Ŵ
												// 3. target �����忡 �̺�Ʈ�� ������ (PostQueuedCompletionStatus) ���� ��� retrun  �Ͽ� ������ �����Ŵ

											/* �����带 �����Ű�� ������� ���� �� 3������ �ִ�. 
												���⼭��, 2�� ����� ����ߴ�.										
											*/
//		WaitForSingleObject(pthInfo->hTHID, INFINITE);
		CloseHandle(pthInfo->hTHID);		

		delete pthInfo;

		//gs_cLogger.PutLogQueue(LEVEL_TRACE, "destroyWorkerThread[%d]",hTmp);
		
		it++;
	}

	m_WorkerThreadVector.clear();
}


THREAD_RET_TYPE THREAD_CALLING_CONVENTION 
CRTSQueueHandler::WorkerThread(void *ptr)
{
	STThreadInfo *pthInfo = (STThreadInfo *)ptr;
	CRTSQueueHandler *pBasic = (CRTSQueueHandler *)pthInfo->pClass;

	DWORD			bytesTrans		= 0;
	BOOL			bSuccess		= TRUE;		
	LPVOID 	        lpCompletion	= NULL;
	LPOVERLAPPED	lpOverlapped	= NULL;
	STComplete stComplete;

	SetEvent(pBasic->m_hCreateThread);
	while(pthInfo->bActive) 
	{
		/*
			����° ���ڴ� �׸� �߿����� �ʰ� ���δ�. (��ü ������ ����)
			(������ �׹�° ������ lpOverlapped �� Ȯ������ ����, �� �ȿ� ��ü�����Ͱ� ����Ǿ� ��ġ�� ����)
		*/
		bSuccess = GetQueuedCompletionStatus(
			pBasic->getRTSQueue(), 
			&bytesTrans,
			(PULONG_PTR)&lpCompletion, 
			&lpOverlapped, 
			INFINITE); 


		if(lpOverlapped == NULL ) {	
            printf("= ERROR WorkerThread lpOverlappedEx is NULL WSAGetLastError[%d]",WSAGetLastError()); 
			continue;
		}
		stComplete.bSuccess = bSuccess;
		stComplete.bytesTrans = bytesTrans;
		stComplete.lpOverlapped = lpOverlapped;
		stComplete.lpCompletion = lpCompletion;
		stComplete.pChild = gs_pChild;
		g_fProcessLoop(&stComplete);
		//pBasic->processLoop(bSuccess, lpOverlapped, bytesTrans);

	}
	return 0;
}


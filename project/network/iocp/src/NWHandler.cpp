
#include "NWHandler.h"
#include "comEnv.h"

//static func_type g_fProcessLoop = NULL;
//static void *gs_pChild = NULL;

CNWHandler::CNWHandler() :CRTSQueue()
{
	m_fProcessLoop = NULL;
	m_pChild = NULL;
}


CNWHandler::~CNWHandler()
{
	destroyWorkerThread(NULL);
}





bool CNWHandler::start(func_type pFunction, void *pChild)
{
	int i;
	int nThreadCount = getThreadCount();

	m_hCreateThread =  CreateEvent(0, FALSE, FALSE, 0);
	m_WorkerThreadVector.reserve(nThreadCount);
	STNWThreadInfo *pthInfo;
	m_fProcessLoop = pFunction;
	m_pChild = pChild;

	for(i = 0 ; i < nThreadCount; i++) {
		pthInfo = new STNWThreadInfo;
		pthInfo->pClass = this;
		pthInfo->bActive = true;
        pthInfo->nSeqNum    = i;
		if (!THREAD_CREATE(&pthInfo->hTHID, CNWHandler::WorkerThread, pthInfo)) {
			goto ERROR_START;
		}
		WaitForSingleObject(m_hCreateThread, INFINITE);
        ResetEvent(m_hCreateThread);
		m_WorkerThreadVector.push_back(pthInfo);
	}
	CloseHandle(m_hCreateThread);
	return true;

ERROR_START :
	_stprintf(g_szMessage, _T("[THREAD_CREATE FAIL] i[%d] nThreadCnt[%d]"), i, nThreadCount);
	comErrorPrint(g_szMessage);
	destroyWorkerThread(NULL);
	CloseHandle(m_hCreateThread);
	return false;
}

void CNWHandler::destroyWorkerThread(HANDLE hID)
{
	STNWThreadInfo *pthInfo;
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
CNWHandler::WorkerThread(void *ptr)
{
	STNWThreadInfo *pthInfo = (STNWThreadInfo *)ptr;
	CNWHandler *pBasic = (CNWHandler *)pthInfo->pClass;

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
			gs_cLogger.DebugLog(LEVEL_ERROR, "[GetQueuedCompletionStatus FAIL] nSeq[%d] hTHID[%p]", pthInfo->nSeqNum, pthInfo->hTHID);
			continue;
		}
		stComplete.bSuccess = bSuccess;
		stComplete.bytesTrans = bytesTrans;
		stComplete.lpOverlapped = lpOverlapped;
		stComplete.lpCompletion = lpCompletion;
		stComplete.pChild = pBasic->getChild();
		pBasic->m_fProcessLoop(&stComplete);
		//pBasic->processLoop(bSuccess, lpOverlapped, bytesTrans);

	}
	return 0;
}


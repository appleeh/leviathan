/******************************************************************************/
/*   by keh                                                                   */
/******************************************************************************/
#include "comTypes.h"
#include "NWSessionManager.h"
#include "NWGlobal.h"

//int g_nRecvBufSize;
CSessionManager * g_pSessMgr;
CSessionManager::CSessionManager()
{
	m_pDataMemMgr	= NULL;
	m_pSockMemMgr	= NULL;
}


CSessionManager::~CSessionManager(void)
{	
	if (!m_pSockMemMgr) return;

	CNWSocket::SOCKETDATA *pSockData;
	AllClose();
	int i = 0;

	delete m_pSockMemMgr;
	m_pSockMemMgr = NULL;
	if(m_pDataMemMgr) {
		while ((pSockData = m_pDataMemMgr->getNext(&i))) {
			if (pSockData->pData) {
				gs_pMMgr->delBuf(pSockData->pData, pSockData->nTotLen);
				pSockData->pData = NULL;
				pSockData->nTotLen = 0;
			}
			i++;
		}
        delete m_pDataMemMgr;
        m_pDataMemMgr = NULL;
    }
}

void CSessionManager::createMemoryPool(int nMaxCnt, int nSDataCnt)
{
	// ���� MAX_SESSION * CLASS SIZE ũ���� �޸𸮰� �Ҵ�ȴ�.
	// ������ MAX_SESSION ���� �����Ͱ� std::stack ���� �����ȴ�.
	int i;
	int nMax;

	if (m_pSockMemMgr) return;
	
	m_pSockMemMgr = new CMemPool<CNWSocket>();
	if(!m_pSockMemMgr->alloc(nMaxCnt)) {
		//printf("CSessionManager::createMemoryPool m_pSockMemMgr ERROR! [%s]\n", m_pSockMemMgr->getMessage());
	}
	nMax = nMaxCnt*(g_nSendDataCount + NR_RECV_SOCKETDATA);
	m_pDataMemMgr = new CMemPool<CNWSocket::SOCKETDATA>();
	if (!m_pDataMemMgr->alloc(nMax, eAlloc_Type_alloc)) {
		//printf("CSessionManager::createMemoryPool m_pDataMemMgr ERROR! [%s]\n", m_pDataMemMgr->getMessage());
	}

    //=== �޸� �Ҵ� �Ϸ� =====

	CNWSocket	*pSock;
	for(i = 0; i < nMaxCnt; i++)
	{
		pSock = m_pSockMemMgr->getMem(i);		// �̸� �Ҵ�� �޸� ��ȯ
        //recv data setting
        pSock->SETLPRECVDATA(m_pDataMemMgr->getMem(i));
		pSock->SETSESSIONIDX(i);
	}
}


void CSessionManager::prepareAccept(SOCKET serverSocket, int nMax) // ���� �ѹ���
{
	int i, nTot=0, nSIdx;
	CNWSocket *pSocket;

	for(i= 0; i < nMax; i++)
	{
		pSocket = newSocket(&nSIdx);
		pSocket->createSocket();
		if(pSocket->bindAccept(serverSocket) == CERROR_BINDACCEPT)
		{
			gs_cLogger.DebugLog(LEVEL_ERROR, "pSocket->bindAccept nSIdx[%d] nFD[%d]", pSocket->GETSESSIONIDX(), serverSocket);
			pSocket->closeSocket();
			m_pSockMemMgr->delMem(pSocket);
		}
		else {
			nTot++;
			pSocket->SETSOCKETSTATUS(SOCK_STATUS_ACCEPTED);
			gs_cLogger.PutLogQueue(LEVEL_TRACE, "Client Socket Create & AcceptEx nSIdx[%d] nFD[%d]", pSocket->GETSESSIONIDX(), serverSocket);
		}
	}
	gs_cLogger.PutLogQueue(LEVEL_TRACE, "BindAccept nTot[%d]", nTot);
}


/*
	����� ��� client ���� ������ �����Ѵ�.
	���α׷� ����� ���� ����� ��Ȳ �ƴϸ� ȣ����� �ʴ´�.
*/
void CSessionManager::AllClose()
{
	CNWSocket *pSock;

    int nCurIdx = 0, nCnt = 0;

	if(m_pSockMemMgr)
    {
		while ((pSock = m_pSockMemMgr->getNext(&nCurIdx)))
		{
			if (pSock->Connected()) {
				if (pSock->closeSocket()) nCnt++;
			}
			nCurIdx++;
		}
    }

    gs_cLogger.PutLogQueue(LEVEL_TRACE, "AllClose() nCnt[%lu]\n",nCnt);
}




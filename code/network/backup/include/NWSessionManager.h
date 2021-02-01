/******************************************************************************/
/*   by keh                                                                   */
/******************************************************************************/
#pragma once
#include "comMemPool.h"
#include "comBMemPool.h"
#include "comBufPool.h"
#include "comList.h"
#include "NWSocket.h"

//#include "NWMTSync.h"



enum {
    POP_WAY_STACK   = 1,
    POP_WAY_QUEUE,
    POP_WAY_INDEX,
    POP_WAY_FAST
};



enum {
	MEMTYPE_SESSION		= 1,
	MEMTYPE_NWSOCKET	,
	MEMTYPE_SOCKETDATA	,
	MEMTYPE_QUEUEDATA
};

enum {
    IS_MEMTYPE_CONNLIST     = 0x01
};


enum ENETFLAG {
    eNET_FLAG_BALANCE_NONE = 0,
    eNET_FLAG_BALANCE_CRITICAL = 1,
    eNET_FLAG_BALANCE_BAD,
    eNET_FLAG_BALANCE_WARNING,
    eNET_FLAG_BALANCE_SMOOTH,
    eNET_FLAG_BALANCE_LOW
};

class CSession
{
public:
	CSession():m_pSocket(0),m_pObject(0)	{}
	virtual ~CSession() {}
	inline void SETSOCKET(CNWSocket *pSockMgr)		{ m_pSocket = pSockMgr; }
	inline CNWSocket *	GetSocket()					{ return m_pSocket; }
	inline void setObject(void *pObj)				{ m_pObject = pObj; } // �پ��� Ÿ���� ��ü���� �ϳ��� ��ü�� ��� ���
	inline void * getObject()						{ return m_pObject; }

private:
	CNWSocket *m_pSocket;
	void *		m_pObject;		// �پ��� ������ ��ü�� �������� ��� �����ϵ��� �Ѵ�.
};

class CSessionManager
{
public:
//	typedef std::list<CNWSocket *> STCONNECTLIST;
	
	CSessionManager();
	~CSessionManager(void);

	void createMemoryPool(int nMaxCnt, char isMemType, int socketFlag);

	int getRemainCnt(int nType);			/* ANY Ÿ���� ���� ������ ����.*/
	int getMaxCnt(int nType);
	CNWSocket *getNewNWSocket(int nFlag, int idx=0);			// ���� ���ῡ �ʿ��� Ŭ���̾�Ʈ ���� �ν��Ͻ� ��ȯ
	bool addSession(CNWSocket *pClient);	// ���� ���� �Ϸ�
	void delSession(CNWSocket *pClient);	// ���� ���� ����
	StFuncData * newFuncMem();	
    CNWSocket::SOCKETDATA * newSockMem(); 


	void prepareAccept(SOCKET serverSocket, int nFlag = 1);


	/* �ΰ� ��� */
	bool isConnected(int nSIdx);
	int broadcastToAllSession(char *pBuf, int nLen, CRTSQueue *pRTSQueue);
    void sendToAllSession(CRTSQueue *pRTCQueue);
	void AllClose();
	int checkSocketTimetick(unsigned int nCheckTime,  CNWSocket **pDelList);

	inline CSession	* GetSession(int nSIdx)		{ return m_pSessionArray[nSIdx];	}				// ���� ��ü �ϳ��� ��� ������ �˼� �ִ�.
	inline CNWSocket * GetNWSocket(int nSIdx)	{ return m_pSessionArray[nSIdx]->GetSocket();	}	
    inline void returnSockData(CNWSocket::SOCKETDATA *pMem) { m_pDataMemMgr->delMem(pMem); }
    inline int getConnectListCount()            { return m_pConnectList->size(); }

private :
	CMemPool<CNWSocket>				* m_pSockMemMgr;
	CBMemPool<CSession>				* m_pSessMemMgr;
	CBMemPool<CNWSocket::SOCKETDATA>* m_pDataMemMgr;
	CSBufPool						* m_pRecvMemMgr;    
    /* 
		���������� �ε��� ���� -- 
		�ٸ� �޸𸮿��� �ش� ������ index ������ ������ ���� �����͸� ������ �ִ�.
	*/
	CSession** m_pSessionArray;	
	bool m_bCreateMemSession;
    CMemList<CNWSocket> * m_pConnectList;

};

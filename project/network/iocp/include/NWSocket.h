/******************************************************************************/
/*                                                                            */
/*   �� �ҽ� �ڵ��� �Ǹ��� ����ϴ� �ּ��� �������� ���ÿ�.                      */
/*   Copyright 2020 by keh													  */
/*                                                                            */
/*   by keh                                                                   */
/******************************************************************************/

/********************************************************************
	2012.05.03 by KEH
	-------------------------------------------------------------
	@ Ŭ���̾�Ʈ ���� ���� ó���� �����Ѵ�.

	@ ����
	1. createSocket
	2. bindAccept
	3. bindIOCP - accept �� ������ IOCP �� ����
	4. send / recv
	5. ���� ����
*********************************************************************/
#pragma once

#include "RTSQueue.h"



/* socket �� �پ��� ���� */
enum {
	SOCK_STATUS_SERVER			= 0x01000000,	// Listen socket
	SOCK_STATUS_TOSERVER		= 0x02000000,	// socket of connect to other server 
	SOCK_STATUS_ACCEPTED		= 0x04000000,	// accepted client socket
	SOCK_STATUS_SERVERTYPE		= 0x000000ff,	// 0x000000ff ���� (Ÿ�� �� ����) - server type , ex:A_Server, B_Server, C_Server type number
	SOCK_STATUS_CONNECTED		= 0x00100000,
	SOCK_STATUS_CREATE_ERROR	= 0x00200000,
	SOCK_STATUS_CONNECTED_ERROR = 0x00400000,
	SOCK_STATUS_DETAIL			= 0x0000ff00,	// 0x0000ff00 ���� (Ÿ�� �� ����) - client type (bind type )
	SOCK_STATUS_SEND			= 0x00010000,	// send ��
	SOCK_STATUS_RECV			= 0x00020000,	// recv ��
	SOCK_STATUS_CLOSE_TIMEOUT	= 0x00040000,	
	SOCK_STATUS_CLOSE_NORMAL	= 0x00080000,
};

#define SOCK_STATUS_SHIFT	8
#define MAX_IO_COUNT	    10000
#define NR_BUF              3
#define NR_SEND_SOCKETDATA  3
#define NR_RECV_SOCKETDATA  1


int winSocketInit();
void winSocketClean();

class CNWSocket
{
public:
	typedef struct SOCKETDATA {
		STOVERLAPPED	stCommon;
		CNWSocket	*   lpClient;
		DWORD			nTotLen;		// �� ���� ����
		DWORD			nCurLen;		// ���� ����/���� ����
		char *			pData;
		void *			pObj;
	} SOCKETDATA, *LPSOCKETDATA;


	CNWSocket(void);       // ��� ����
	~CNWSocket(void);      // ��� ����

    void clear();           // �Ҹ��� ��� ���

	int createSocket();
	int SockServerInit(const int nPort, const char* szIP= g_sAnyIp);

	/*********************************************************************
	*	Ŭ���̾�Ʈ accept ��� -> accept �Ϸ�
	**********************************************************************/
	int bindAccept(SOCKET serverSock);
	int doAccept(CRTSQueue *pRTSQueue);
	
	inline void SetConnectionIP(const char *szIP) { memcpy(m_szIP, szIP, MAX_IP_LEN); }
	inline char * GetConnectionIP() { return m_szIP; }

	/*********************************************************************
	*	Ŭ���̾�Ʈ Connect To Server
	**********************************************************************/

	inline bool	Connected()	{ if(m_sockStatus & SOCK_STATUS_CONNECTED) return true; return false; }
	bool ConnectTo(const char* szIP, int nPort, int miliseconds, CRTSQueue *pRTSQueue);
	/*********************************************************************
	*	accept �Ϸ��� IO ó�� ����
	**********************************************************************/
	// 
	int bindRecv();
	int RecvPacket(int nRecvSize);
	int Send(char *pBuf, int nLen, LPSOCKETDATA lpSendData);

	/*********************************************************************
	*	���� ���Ḧ ���� ����
	*---------------------------------------------------------------------
		1. ���۷��� ī��Ʈ üũ - ī��Ʈ �����ϸ�, ���� ����
		2. PostQueuedCompletionStatus �� ȣ���Ͽ� ProcessIOCP ť�� �۾��� �ѱ�
		3. ProcessThread ���� �̺�Ʈ ������ ó��
		 - closeSocket() ȣ�� : ���ð� �ʿ�
		 - ���� �޸� �ʱ�ȭ
		 - �ٽ� BindAccept ��� ��
	**********************************************************************/

	bool socketShutdown();	
	int closeSocket(bool bLog=true);	// ���α׷� ����� ȣ�� (closesocket -> create socket -> acceptEx)

	/*********************************************************************/

 
	inline int GETSESSIONIDX()				{ return m_nSIdx; }
	inline void SETSESSIONIDX(int nSIdx) {
		m_nSIdx = nSIdx; m_pObject = NULL;
	}
	inline SOCKET GETSOCKET()				{ return m_socket; }
	
	/*
		LPSOCKETDATA type�� �޸� �Ҵ��� sessionManager ���� �ϰ� �Ҵ�/���� ������ å������.
	*/
	void changeRecvBuf(int nNewSize);
	inline void SETLPRECVDATA(LPSOCKETDATA lpRecvData)          { m_lpRecvData = lpRecvData; m_lpRecvData->lpClient = this;	}

	inline void SETLPRECVDATA_BUF(char *pNewBuf, int nBufSize) {  // set Data in newBuf, so do not zero copy
		m_lpRecvData->pData = pNewBuf; m_lpRecvData->nTotLen = nBufSize;
	}
	inline void RETURN_DATABUF() {
		if (m_lpRecvData->pData) gs_pMMgr->delBuf(m_lpRecvData->pData, m_lpRecvData->nTotLen); 
		m_lpRecvData->pData = NULL; m_lpRecvData->nTotLen = 0;
	}
	inline void SETREMAINDATA(char *pData, int nSize) {
		memcpy(m_lpRecvData->pData, pData, nSize);
		memset(m_lpRecvData->pData + nSize, 0, m_lpRecvData->nTotLen - nSize);
		m_nAddLen = nSize;
	}

	inline LPSOCKETDATA GETRECVDATA()	{ return m_lpRecvData; }
	inline LPSOCKETDATA GETSENDDATA()	{ return m_lpSendData; }
	inline void INITSENDDATA() { m_lpSendData = NULL; }

    inline void SETSOCKETSTATUS(int nStatus)	{ m_sockStatus |= nStatus; }	
	inline void DELSOCKETSTATUS(int nStatus)	{ m_sockStatus &= ~nStatus; }	
	inline bool ISSOCKETSTATUS(int nStatus)		{ return m_sockStatus & nStatus ? true:false; }
	inline int  GETSOCKETSTATUS()				{ return m_sockStatus; }
	inline unsigned int GETTIMETICK()			{ return m_nAliveTick; }
	inline void SETTIMETICK(unsigned int nTime) { m_nAliveTick = nTime; }

	inline void setObject(void *pObj) { m_pObject = pObj; } // �پ��� Ÿ���� ��ü���� �ϳ��� ��ü�� ��� ���
	inline void * getObject() { return m_pObject; }

	bool	setOption(int level, int optname, const void *optval, int optlen);
	bool	getOption(int level, int optname, void *optval, int *optlen);

private:
	SOCKET	m_socket;
	unsigned int    m_nAliveTick;
	char	m_szAddressBuf[MAX_ADDRESS_LEN]; 
	char	m_szIP[MAX_IP_LEN];			
	int		m_sockStatus;
	int		m_nSIdx;					
	int		m_nPort;

	LPSOCKETDATA m_lpRecvData;
	LPSOCKETDATA m_lpSendData;
	int m_nAddLen;
	void * m_pObject;

};


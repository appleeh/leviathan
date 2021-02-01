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
	SOCK_STATUS_SERVER		= 0x10000000,	
	SOCK_STATUS_SERVERTYPE	= 0x000000ff,	// 0x000000ff ���� (Ÿ�� �� ����) - server type
	SOCK_STATUS_CONNECTED	= 0x01000000,
	SOCK_STATUS_DETAIL		= 0x0000ff00,	// 0x0000ff00 ���� (Ÿ�� �� ����) - client type
	SOCK_STATUS_SEND		= 0x00010000,	// send ��
	SOCK_STATUS_RECV		= 0x00020000,	// recv ��
	SOCK_STATUS_WRITEBUF	= 0x00040000,	// write buf ��
	SOCK_STATUS_SENDBUF	    = 0x00080000	// write buf Send ��
};

#define SOCK_STATUS_SHIFT	8
#define MAX_IO_COUNT	    10000
#define NR_BUF              3
#define NR_SEND_SOCKETDATA  3
#define NR_RECV_SOCKETDATA  1



// Ŀ�ο��� �޾ƿ��� recv Buf
//typedef struct {
//    char szBuf[RECV_BUFFER_SIZE];
//} StRecvBuf;

typedef struct {
	char *pRecvBuf;
	int nAddLen;
	int nBufSize;
} StRemain;

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
	} SOCKETDATA, *LPSOCKETDATA;


	CNWSocket(void);       // ��� ����
	~CNWSocket(void);      // ��� ����

    void createObject();    // ������ ��� ���
    void clear();           // �Ҹ��� ��� ���

	void initCNWSocket();
	int createSocket();

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
	int	ConnectTo(const char* szIP, int nPort, CRTSQueue *pRTSQueue);
	
	/*********************************************************************
	*	accept �Ϸ��� IO ó�� ����
	**********************************************************************/
	// 
	int bindRecv();
	int RecvPacket();
	int Send(char *pBuf, int nLen, CRTSQueue *pRTSQueue);

    int SendBuf(CRTSQueue *pRTSQueue);
    int WriteToBuffer(const char *pMsg, int nLen, CRTSQueue *pRTSQueue);

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
	
//	inline BOOL isPossibleDisconnect()	{if(0 < m_lpRecvData->nCount || 0 < m_lpSendData->nCount) return FALSE;	return TRUE;}
//	inline BOOL isPossibleDisconnect()	{if(0 < m_lpRecvData->nCount ) return FALSE;	return TRUE;}
	

	int closeSocket();	// ���α׷� ����� ȣ�� (closesocket -> create socket -> acceptEx)

	/*********************************************************************/

 
	inline int GETSESSIONIDX()				{ return m_nSIdx; }
	inline void SETSESSIONIDX(int nSIdx)	{ m_nSIdx = nSIdx; }
	inline SOCKET GETSOCKET()				{ return m_socket; }
	
	//inline char *getRecvBuf() { return m_pRecvBuf; }
	//inline int getAddLen() { return m_nAddLen; }
	//inline void setAddLen(int nLen) { m_nAddLen = nLen; }
	inline StRemain * getRemain() { return &m_stRemain; }

	/*
		LPSOCKETDATA type�� �޸� �Ҵ��� sessionManager ���� �ϰ� �Ҵ�/���� ������ å������.
	*/
	inline void SETLPRECVDATA(LPSOCKETDATA lpRecvData)          { m_lpRecvData          = lpRecvData; }
    inline void SETLPRECVDATA_BUF(char *pData)                  { m_lpRecvData->pData   = pData; }
	inline LPSOCKETDATA GETRECVDATA()							{ return m_lpRecvData; }
	inline void SETLPSENDDATA(LPSOCKETDATA lpSendData,int j)    { m_lpSendData[j]       = lpSendData; }
	//inline void SETLPSENDDATA_BUF(char *pData,int j)            { m_lpSendData[j]->pData = pData; }

    inline void SETSOCKETSTATUS(int nStatus)	{ m_sockStatus |= nStatus; }	
	inline void DELSOCKETSTATUS(int nStatus)	{ m_sockStatus &= ~nStatus; }	
	inline int  GETSOCKETSTATUS()				{ return m_sockStatus; }	
	inline unsigned int GETTIMETICK()			{ return m_nAliveTick; }
	inline void SETTIMETICK(unsigned int nTime) { m_nAliveTick = nTime; }

	int _SendPacket(LPSOCKETDATA lpSendData, CRTSQueue *pRTSQueue);

private:
	SOCKET	m_socket;
	unsigned int    m_nAliveTick;
	char	m_szAddressBuf[MAX_ADDRESS_LEN]; 
	char	m_szIP[MAX_IP_LEN];			
	int		m_sockStatus;
	int		m_nSIdx;					
	int		m_nPort;					

	LPSOCKETDATA m_lpRecvData;
	LPSOCKETDATA m_lpSendData[NR_SEND_SOCKETDATA];
    //atomic_nr m_nSendIdx;
	int m_nSendIdx;
	//COM_CRITICAL_SECTION	*m_CSWriteBufSocket;
	SPIN_LOCK m_spinLock;


	StRemain m_stRemain;	
	LPSOCKETDATA getSendLPData();

};


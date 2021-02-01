/******************************************************************************/
/*   by keh                                                                   */
/******************************************************************************/
#include "NWSocket.h"
#include "comLogger.h"
#include <mswsock.h>

//-----------------------------------------------------------------------------
// main ���� �ѹ����� ȣ��
int winSocketInit()
{
	WSADATA socket;
	if(WSAStartup(MAKEWORD(2,0), &socket) != 0) //winsock v2.0�� ����ϴ� socket
		return 0; //���� �� ����� ������ �����ڵ� ��ȯ

	return 1;
}

void winSocketClean()
{
	WSACleanup(); 
}
//-----------------------------------------------------------------------------

CNWSocket::CNWSocket(void)		
{
    m_CSWriteBufSocket = NULL;
    m_sockStatus = 0;

	m_socket = INVALID_SOCKET;
	m_nAliveTick =0 ;
	memset(m_szAddressBuf, 0, sizeof(m_szAddressBuf));
	memset(m_szIP, 0, sizeof(m_szIP));
	m_nSIdx = 0;				// session index	
	m_nPort = 0;

	m_lpRecvData = NULL;
	memset(m_lpSendData, 0, sizeof(m_lpSendData));
	SPIN_LOCK_INIT(&m_spinLock);
	m_nSendIdx = 0;

    //m_pRecvBuf = NULL;
    //m_nAddLen =0;
	memset(&m_stRemain, 0, sizeof(StRemain));

}

CNWSocket::~CNWSocket(void)
{
    if(m_CSWriteBufSocket)  delete m_CSWriteBufSocket ;
}


// ������ ��� ��� - ��ó�� �� �ѹ� ȣ��
void CNWSocket::createObject()
{


	initCNWSocket();
}

// �Ҹ��� ��� ���- ���α׷� �������� ��  �� �ѹ� ����
void CNWSocket::clear()
{
}

void CNWSocket::initCNWSocket()
{
    int nSize = sizeof(LPSOCKETDATA);
    m_socket = INVALID_SOCKET;
	memset(m_szAddressBuf, 0, MAX_ADDRESS_LEN);
	memset(m_szIP, 0, MAX_IP_LEN);
	m_sockStatus &= 0xf00000ff;
	m_lpRecvData->lpClient = this;
    for(int i = 0; i < NR_SEND_SOCKETDATA; i++) 
    {
	    m_lpSendData[i]->lpClient = this;
    }

	m_nSendIdx = 0;

    //memset(m_pRecvBuf, 0, REMAIN_BUFFER_SIZE);
    //m_nAddLen =0;
	memset(&m_stRemain, 0, sizeof(StRemain));


}


int CNWSocket::createSocket()
{
	if(m_socket != INVALID_SOCKET) return CERROR_NONE;

	initCNWSocket();
	m_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if(m_socket == INVALID_SOCKET) 
	{
		printf( "Cannot open socket WSAGetLastError[%d]\n", WSAGetLastError());
		return CERROR_CREATE;
	}


	return CERROR_NONE;
}

/*
WSA_IO_PENDING : �񵿱� �̹Ƿ� ���� ������ �����̶�� �ǹ�.
WSAWOULDBLOCK  : �ѹ��� ó������ �ʰ�, �����߻��ߴٴ� �ǹ�

*/

int CNWSocket::bindAccept(SOCKET serverSock)
{
	WSABUF sWsaBuf;
	sWsaBuf.buf =	m_szAddressBuf; 
	sWsaBuf.len =	0;
	m_lpRecvData->stCommon.opCode = OP_ACCEPT;

	if(AcceptEx(serverSock, m_socket, sWsaBuf.buf, 0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		&m_lpRecvData->nCurLen, reinterpret_cast<LPOVERLAPPED>(m_lpRecvData)))
	{
		// nRet == SOCKET_ERROR -1
		if(WSAGetLastError() != WSA_IO_PENDING && WSAGetLastError() != WSAEWOULDBLOCK) 
		{
			return CERROR_BINDACCEPT;
		}
	}

	return CERROR_NONE;
}

int CNWSocket::doAccept(CRTSQueue *pRTSQueue)
{
	SOCKADDR		*lpLocalSockAddr = NULL, *lpRemoteSockAddr = NULL;
	int				nLocalSockaddrLen=0, nRemoteSockaddrLen=0;


	m_lpRecvData->stCommon.opCode = 0;
	m_sockStatus |= SOCK_STATUS_CONNECTED;

	// get remote address
	GetAcceptExSockaddrs(m_szAddressBuf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, 
		&lpLocalSockAddr, &nLocalSockaddrLen, &lpRemoteSockAddr, &nRemoteSockaddrLen); 

	if(0 != nRemoteSockaddrLen) {
		SetConnectionIP( inet_ntoa( ((SOCKADDR_IN *)lpRemoteSockAddr)->sin_addr) );
	} else {
        gs_cLogger.PutLogQueue(LEVEL_ERROR, "nSIdx[%d] doAccept Error [nRemoteSockaddrLen is 0]",m_nSIdx);
		return CERROR_ACCEPT;
	}

	// bind completion key & connection context
	/*
		1��° ���� : ���� IO �ڵ��� �ǹ�, ���ϵ�ũ����, ���� ����ü
		2��° ���� : IOCP �� �̸�
		3��° ���� : ����� ��ü ������, ���� GetQueuedCompletonStatus ���� �������� �۾��� �����ϱ� ����
		4��° ���� : �Ϸ�� ����� ������ ��� �����带 ���� Ȱ��ȭ ��ų �������� �ִ� ���� 
		             -> �ʹ� ���� Ȱ��ȭ �Ǵ� ���� ���� ����
					 -> 0 : (CPU ���� * 2) + 2 �� �ڵ����� ������
	*/
	if(pRTSQueue->registerSocket((HANDLE)m_socket, this) == CERROR_REGIOCP) {
        gs_cLogger.PutLogQueue(LEVEL_ERROR, "nSIdx[%d] doAccept Error [registerIOCP Error]",m_nSIdx);
			return CERROR_ACCEPT;
	}
    
    return CERROR_NONE;

}

/*
	recv ����Ѵ�.
*/
int	CNWSocket::bindRecv()
{
	m_lpRecvData->nTotLen	= g_nRecvBufSize;		        // �ʱ� ������ MAX ��
	memset(m_lpRecvData->pData, 0, g_nRecvBufSize);
	memset(&m_lpRecvData->stCommon, 0, sizeof(STOVERLAPPED));
	m_lpRecvData->nCurLen = 0;
	return RecvPacket();
}

int CNWSocket::RecvPacket()
{
	WSABUF sWsaBuf;
	DWORD nFlags = 0;

	sWsaBuf.buf =	m_lpRecvData->pData; 
	sWsaBuf.len =	m_lpRecvData->nTotLen;
	m_lpRecvData->nCurLen = 0;

	m_lpRecvData->stCommon.opCode = OP_RECV;

	// WSARecv �� �Ϸ��ϸ� ��ȯ���� 0 �̴�.
	if(WSARecv(m_socket,  &sWsaBuf, 1, &m_lpRecvData->nCurLen, 
		&nFlags, reinterpret_cast<LPWSAOVERLAPPED>(m_lpRecvData), NULL) == SOCKET_ERROR) 
	{
		int nRet = WSAGetLastError();

		if(nRet != WSA_IO_PENDING && nRet != WSAEWOULDBLOCK)  { // �����߻�
            //gs_cLogger.PutLogQueue(LEVEL_ERROR, "nSIdx[%d] RecvPacket Error[%d] ",m_nSIdx, nRet);
			return CERROR_SENDRECV;
		}
	}
	return CERROR_NONE;
}



//#######################################################################################################################
    //���� ��Ŷ send

int CNWSocket::Send(char *pMsg, int nLen, CRTSQueue *pRTSQueue) // nLen : msg Len
{
	WSABUF sWsaBuf;
	DWORD nFlags = 0;

	sWsaBuf.buf = pMsg;
	sWsaBuf.len = nLen;
	LPSOCKETDATA lpSendData = getSendLPData();
	lpSendData->nCurLen = 0;
	lpSendData->stCommon.opCode = OP_SEND;

	// WSASend �� �Ϸ��ϸ� ��ȯ���� 0 �̴�.    
	int nRes = WSASend(m_socket, &sWsaBuf, 1, &lpSendData->nCurLen,
		nFlags, reinterpret_cast<LPWSAOVERLAPPED>(lpSendData), NULL);
	if (nRes == SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		if (errorCode == WSA_IO_PENDING || errorCode == WSAEWOULDBLOCK) {
			gs_cLogger.PutLogQueue(LEVEL_WARN, "nSIdx[%d] WSA_IO_PENDING nTotLen[%d]", m_nSIdx, sWsaBuf.len);
		}
		else { // ���� �߻�
			DELSOCKETSTATUS(SOCK_STATUS_SEND);
			socketShutdown();
			pRTSQueue->putRTSQueue(OP_CLOSE, this);
			gs_cLogger.PutLogQueue(LEVEL_ERROR, "_SendPacket errorCode[%d] res[%d] nCurLen[%d] nTotLen[%d]",
				errorCode, nRes, lpSendData->nCurLen, sWsaBuf.len);
			return CERROR_SENDRECV;
		}
	}
	gs_cLogger.PutLogQueue(LEVEL_TRACE, "nSIdx[%d] _SendPacket nTotLen[%d] nCurLen[%d]", m_nSIdx, sWsaBuf.len, lpSendData->nCurLen);
	return CERROR_NONE;
}

CNWSocket::LPSOCKETDATA CNWSocket::getSendLPData()
{ 
	SPIN_LOCK_ENTER(&m_spinLock);
	m_nSendIdx++;
	if (m_nSendIdx >= NR_SEND_SOCKETDATA) {
		m_nSendIdx = 0;
	}
	SPIN_LOCK_LEAVE(&m_spinLock);
	printf("getSendLPData() nRes[%d]\n", m_nSendIdx);
	return m_lpSendData[m_nSendIdx];
}

int CNWSocket::SendBuf(CRTSQueue *pRTSQueue) // nLen : msg Len
{
    if(!(m_sockStatus & SOCK_STATUS_CONNECTED)) return CERROR_NONE;
#ifdef USE_SENDRECV_BUF
    if(!m_CSWriteBufSocket->enter(false)) return CERROR_NONE;
    if(8 <= stBuf[m_nIndex].nAddLen)
    {
        int nIdx = m_nIndex++;
        if(m_nIndex==NR_BUF) m_nIndex = 0;
        m_CSWriteBufSocket->leave();
        LPSOCKETDATA lpSendData = getSendLPData();


        memset(lpSendData->pData, 0, MAX_PACKET_SIZE);
        memcpy(lpSendData->pData, stBuf[nIdx].szBuff, stBuf[nIdx].nAddLen);    
        lpSendData->nTotLen   = stBuf[nIdx].nAddLen;
        stBuf[nIdx].nAddLen     = 0;
        memset(stBuf[nIdx].szBuff, 0, MAX_PACKET_SIZE);

        lpSendData->nAddLen     = 0;
        lpSendData->nCount	    = 0;
        lpSendData->nAddCount	= 0;
        lpSendData->nFlags	    = 0;  

        return _SendPacket(lpSendData, pRTSQueue);

    }
    else {
        m_CSWriteBufSocket->leave();
        return CERROR_NONE; 
	}
#endif
	return CERROR_NONE; 
}

int CNWSocket::WriteToBuffer(const char *pMsg, int nLen, CRTSQueue *pRTSQueue) // nLen : msg Len
{
    if(!(m_sockStatus & SOCK_STATUS_CONNECTED)) return CERROR_NONE;
#ifdef USE_SENDRECV_BUF
    SETSOCKETSTATUS(SOCK_STATUS_WRITEBUF);

    char szSendBuf[MAX_PACKET_SIZE];
    memset(szSendBuf, 0, MAX_PACKET_SIZE);
	int nPacketLen = 0;	// packet header encoding, (encrypt)  -- �ӽ÷� �������� ����. Ŀ���͸���¡ �ؾ���
//	int nPacketLen = MakePacket(szSendBuf, pMsg, nLen);	// packet header encoding, (encrypt)  -- �ӽ÷� �������� ����. Ŀ���͸���¡ �ؾ���
	if(!nPacketLen) return CERROR_INSUFFICIENT_BUFFER;

    m_CSWriteBufSocket->enter();
    if(SEND_IO_SIZE <= stBuf[m_nIndex].nAddLen + nPacketLen)
    {
        int nIdx = m_nIndex++;
        if(m_nIndex==NR_BUF) m_nIndex = 0;
        memcpy(stBuf[m_nIndex].szBuff+stBuf[m_nIndex].nAddLen , szSendBuf, nPacketLen);
        stBuf[m_nIndex].nAddLen += nPacketLen;
        m_CSWriteBufSocket->leave();
        LPSOCKETDATA lpSendData = getSendLPData();

	    memset(lpSendData->pData, 0, MAX_PACKET_SIZE);
        memcpy(lpSendData->pData, stBuf[nIdx].szBuff, stBuf[nIdx].nAddLen);
        lpSendData->nTotLen = stBuf[nIdx].nAddLen;    
        stBuf[nIdx].nAddLen = 0;
        memset(stBuf[nIdx].szBuff, 0, MAX_PACKET_SIZE);

        lpSendData->nAddLen     = 0;
	    lpSendData->nCount	    = 0;
	    lpSendData->nAddCount	= 0;
	    lpSendData->nFlags	    = 0;         

        _SendPacket(lpSendData, pRTSQueue);

    }
    else
    {
        memcpy(stBuf[m_nIndex].szBuff+stBuf[m_nIndex].nAddLen , szSendBuf, nPacketLen);
        stBuf[m_nIndex].nAddLen += nPacketLen;
        m_CSWriteBufSocket->leave();
        gs_cLogger.DoWritePacket(LEVEL_DEBUG, SND_QUEUE, m_nSIdx, szSendBuf, nPacketLen);
    }

    DELSOCKETSTATUS(SOCK_STATUS_WRITEBUF);
#endif
    return CERROR_NONE;
}

int CNWSocket::_SendPacket(LPSOCKETDATA lpSendData, CRTSQueue *pRTSQueue)
{
	WSABUF sWsaBuf;
	DWORD nFlags = 0;

	sWsaBuf.buf =	lpSendData->pData; 
	sWsaBuf.len =	lpSendData->nTotLen;
	lpSendData->nCurLen = 0;
	lpSendData->stCommon.opCode = OP_SEND;

	// WSASend �� �Ϸ��ϸ� ��ȯ���� 0 �̴�.    
	int nRes = WSASend(m_socket,  &sWsaBuf, 1, &lpSendData->nCurLen, 
		nFlags, reinterpret_cast<LPWSAOVERLAPPED>(lpSendData), NULL);
	if(nRes == SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		if(errorCode == WSA_IO_PENDING || errorCode == WSAEWOULDBLOCK) {
			gs_cLogger.PutLogQueue(LEVEL_WARN, "nSIdx[%d] WSA_IO_PENDING nTotLen[%d]", m_nSIdx,sWsaBuf.len);
		}
		else  { // ���� �߻�
			DELSOCKETSTATUS(SOCK_STATUS_SEND);
			socketShutdown();
			pRTSQueue->putRTSQueue(OP_CLOSE, this);
				gs_cLogger.PutLogQueue(LEVEL_ERROR, "_SendPacket errorCode[%d] res[%d] nCurLen[%d] nTotLen[%d]", 
					errorCode, nRes, lpSendData->nCurLen, sWsaBuf.len);
 			return CERROR_SENDRECV;
		}	
	}
    gs_cLogger.PutLogQueue(LEVEL_TRACE, "nSIdx[%d] _SendPacket[%d]", m_nSIdx, sWsaBuf.len);
    return CERROR_NONE;
}

bool CNWSocket::socketShutdown()	// gracefull socketclose
{
    if(m_sockStatus & SOCK_STATUS_CONNECTED)
    {
	    shutdown(m_socket, SD_BOTH);
        m_sockStatus &= ~SOCK_STATUS_CONNECTED;     // �߰����� send ����
        return true;
    }
    return false;

}


 
int CNWSocket::closeSocket() // ���� ������� �ʿ���
{
	//struct linger li = {1,0};	// ��� ����
//	li.l_onoff	= 1;	// 1:��⿩�� ���� 0:��� ����
//	li.l_linger = 0;	// ��ٸ��� �ð��� ����, 0�̸� �۽Ź��ۿ� �����ִ� �����͸� �ı�
	// SO_LINGER : close() �� ȣ��������, Ŀ�ο��� ���� ���α׷����� �����ϴ� ������
	//			  �۽� ������ �ڷᰡ ��� ���۵� ���� Ȯ�ε� ������ ������ �� �ִ�. 

	int nOptValue = 1;

	if(m_socket !=  INVALID_SOCKET) {
//		setsockopt(m_socket, SOL_SOCKET, SO_LINGER | SO_REUSEADDR, (char *)&li, sizeof(li));
		setsockopt(m_socket, SOL_SOCKET, SO_LINGER|SO_REUSEADDR, (char *)&nOptValue, sizeof(nOptValue));
		closesocket(m_socket);  // IOCP ��� �ڵ� ����
		m_socket = INVALID_SOCKET;
		m_sockStatus &= 0xf00000ff;
		return 1;
	}	

    return 0;
}



int CNWSocket::ConnectTo(const char* szIP, int nPort, CRTSQueue *pRTSQueue)
{
	struct sockaddr_in sa;


	// �̹� ����� ����
	if(m_sockStatus & SOCK_STATUS_CONNECTED)
	{
		// ������ �ּҰ� ������, return
		if(strcmp(szIP, m_szIP) == 0 && nPort == m_nPort) {
			return 1;

		} else { // ������ �ּҰ� �ٸ���, ���� close
			closeSocket();
		}
	}

	if (m_socket == INVALID_SOCKET) 
	{
		if(createSocket() == CERROR_CREATE)
		{
			return 0;
		}
	}

	memset(&sa, 0x00, sizeof(sa));
	
	sa.sin_family = AF_INET;
	sa.sin_port = htons((short)nPort);
	sa.sin_addr.s_addr = inet_addr(szIP);

	int iResult;
	u_long iMode = 1;  // 0:blocking, 1:nonBlocking


	iResult = ioctlsocket(m_socket, FIONBIO, &iMode);
	if (iResult != NO_ERROR)
		printf("ioctlsocket failed with error: %ld\n", iResult);

	

	int nRet = connect(m_socket, (struct sockaddr*)&sa, sizeof(sockaddr_in));
	if(nRet == -1) {
		if (WSAGetLastError() != WSAEWOULDBLOCK) {
			gs_cLogger.PutLogQueue(LEVEL_ERROR, "Connect Fail nRet[%d] nSIdx[%d] [%s:%d]", nRet, m_nSIdx, szIP, nPort);//, socketErrorMsg());
			closeSocket();
			return 0;
		}
	}

	fd_set wrset;
	fd_set exset;
	FD_ZERO(&wrset);
	FD_ZERO(&exset);

	FD_SET(m_socket, &wrset);
	FD_SET(m_socket, &exset);

	//wait till connect finishes
	struct timeval tval;
	tval.tv_sec = 0;
	tval.tv_usec = 500000;
	nRet = select(0, 0, &wrset, &exset, &tval);

	if (!nRet || nRet == SOCKET_ERROR || FD_ISSET(m_socket, &exset)) {
		gs_cLogger.PutLogQueue(LEVEL_ERROR, "Connect select Fail nRet[%d] nSIdx[%d] [%s:%d]", nRet, m_nSIdx, szIP, nPort);//, socketErrorMsg());
		closeSocket();
		return 0;
	}
	else if (FD_ISSET(m_socket, &wrset))
	{

		m_sockStatus |= SOCK_STATUS_CONNECTED;
		strcpy_s(m_szIP, szIP);
		m_nPort = nPort;

		if (pRTSQueue->registerSocket((HANDLE)m_socket, this) == CERROR_REGIOCP) return 0;

		if (bindRecv() == CERROR_NONE) return 1;
	}

	return 0;

}


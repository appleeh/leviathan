#include "../../comSocket.h"
#include "NWCommunicator.h"
#include "comFileUtil.h"

char *g_pEndDelimeter = NULL;

/*
// http:
header �� ���� : HTTP method(GET, POST, HEAD, PUT, DELETE, TRACE, OPTION, CONNECT), header �� �� : ����
Content-Length : <length>
https://developer.mozilla.org/ko/docs/Web/HTTP/Headers/Content-Length
chunked : 16���� ���� ('\r\n') + chunk(body)
https://developer.mozilla.org/ko/docs/Web/HTTP/Headers/Transfer-Encoding
HTTP/1.1 200 OK
Content-Type: text/plain
Transfer-Encoding: chunked
*/
//-----------------------------------------------------------------------------------
//	DataSize, MsgIndex, body
//-----------------------------------------------------------------------------------
struct stPacketHeader
{
	unsigned int nDataSize;
};

struct STMessageHeader
{
	unsigned int nMsgIndex;
};

struct STComPacket
{
	E_HEADER_LENGTH_TYPE eLength_type;
	unsigned int nHeaderSize;
	unsigned int nBodySize;
	char *pHeader; // STMessageHeader *, char *, NULL
	char *pBody;
};

#define PACKET_HEADER_SIZE          sizeof(stPacketHeader)
#define PACKET_MSGHEADER_SIZE       sizeof(STMessageHeader)

int MakePacketHeader(char *pDest, const char *pMsg, int nMsgLen, int nBufSize);
void DecodePacketHeader(char *pDest, stPacketHeader *stHeader, const char *pPacket);


// recv header
// recv body
// recv process

void MakePacket(STComPacket *p)
{
	switch (p->eLength_type)
	{
	case ePacket_none			: break;
	case ePacket_numberLength	: break;
	case ePacket_stringLength	: break;
	case ePacket_http		: break;
	}
}

void makePacket_1(STComPacket *p)
{

}


int MakePacketHeader(char *pDest, const char *pMsg, int nMsgLen, int nBufSize)
{
	int nHeaderSize = PACKET_HEADER_SIZE;
	int nTotSize = nHeaderSize + nMsgLen;
	if (pDest == NULL || NULL == pMsg)
		return 0;

	if (nTotSize > nBufSize) {
		// todo error Log
		return 0;
	}

	stPacketHeader stPacketHeader;
	stPacketHeader.nDataSize = nMsgLen;

	// ��� �߰� (��ȣȭ ����)
	memcpy(pDest, (char *)&stPacketHeader, nHeaderSize);

	// Message �߰�
	memcpy(&pDest[nHeaderSize], pMsg, nMsgLen);

	return nTotSize;
}


void DecodePacketHeader(char *pDest, stPacketHeader *stHeader, const char *pPacket)
{
	// ��ȣȭ decoding  �� msg �� �����ؼ� pDest �� ����
	memcpy(pDest, pPacket + PACKET_HEADER_SIZE, stHeader->nDataSize);
	pDest[stHeader->nDataSize] = 0;
}

int getServiceCode(char *pPacket, int *pSize)
{
	STMessageHeader *pMsgHeader = (STMessageHeader*)pPacket;
	*pPacket += PACKET_MSGHEADER_SIZE;
	*pSize -= PACKET_MSGHEADER_SIZE;
	return pMsgHeader->nMsgIndex;
}

/*
��Ŷ����
|	4byte	|	4byte	|	byteData	|
|  length	| svc code	|	Data		|
*/

void recvProc_length(void *pSocketObj, fp_parsingProcess fpProcess)
{
	CNWSocket * pSocket = (CNWSocket *)pSocketObj;
	unsigned int  nProcCompleteLen=0, nPacketLen = 0, nRemainLen=0;
	CNWSocket::LPSOCKETDATA pSockData = pSocket->GETRECVDATA();
	stPacketHeader *pHeader;

	char *pPacket = pSockData->pData;
	pHeader = (stPacketHeader*)pSockData;
	nPacketLen = PACKET_HEADER_SIZE + pHeader->nDataSize;

	if (pSockData->nTotLen < nPacketLen) {
		pSocket->changeRecvBuf(nPacketLen + 1024);
		pSocket->RecvPacket(nPacketLen - pSockData->nCurLen);  // �ϼ��� ��Ŷ ������� ���ݱ��� ���� ������ ���̸� ���� �� �޴´�.
		return;
	}

	while (pSockData->nCurLen >= nProcCompleteLen + nPacketLen) { // �ϼ��� ��Ŷ�� ���� ó�� Loop
		nProcCompleteLen += nPacketLen; pPacket += PACKET_HEADER_SIZE;
		//----------------------------------------------
		if (fpProcess) fpProcess(pPacket, pHeader->nDataSize);
		//----------------------------------------------
		pPacket += pHeader->nDataSize;

		// nextPacket parsing
		if (pPacket) {
			pHeader = (stPacketHeader*)pPacket;
			nPacketLen = PACKET_HEADER_SIZE + pHeader->nDataSize;
		}
		else break;
	}

	// �����Ͱ� ó���ǰ� ���� �����Ͱ� ������ -- 
	nRemainLen = pSockData->nCurLen - nProcCompleteLen;
	if (nRemainLen) {
		pSocket->SETREMAINDATA(pSockData->pData + nProcCompleteLen, nRemainLen);
		pSocket->RecvPacket(pSockData->nTotLen - nRemainLen);
	}
	else pSocket->bindRecv();
}

/*
��Ŷ����
|	string	|separator  |	stringData	|
|  length	|,			|      Data		|
ex: "10,dataString"
*/
int parsing_stringSize(const char *pPacket, unsigned int *pPacketLen)
{
	char szSize[32];
	char *pPos = CFileUtil::ehstrstr(pPacket, g_pEndDelimeter);
	if (!pPos) return 0;
	int nLen = (int)(pPos - pPacket);
	strncpy(szSize, pPacket, nLen);
	szSize[nLen]=0;
	*pPacketLen = nLen + 1 + atoi(szSize);  //123,
	return nLen + 1;
}

void recvProc_string(void *pSocketObj, fp_parsingProcess fpProcess)
{
	CNWSocket * pSocket = (CNWSocket *)pSocketObj;
	unsigned int  nProcCompleteLen=0, nPacketLen=0, nRemainLen=0, headerLen=0, nDataSize=0;
	CNWSocket::LPSOCKETDATA pSockData = pSocket->GETRECVDATA();

	char *pPacket = NULL;
	headerLen = parsing_stringSize(pSockData->pData, &nPacketLen);
	if (!headerLen) goto RECV_END_PROC;

	if (pSockData->nTotLen < nPacketLen) {
		pSocket->changeRecvBuf(nPacketLen + 1024);
		pSocket->RecvPacket(nPacketLen - pSockData->nCurLen);  // �ϼ��� ��Ŷ ������� ���ݱ��� ���� ������ ���̸� ���� �� �޴´�.
		return;
	}

	while (pSockData->nCurLen >= nProcCompleteLen + nPacketLen) { // �ϼ��� ��Ŷ�� ���� ó�� Loop
		nProcCompleteLen += nPacketLen; pPacket += headerLen;
		nDataSize = nPacketLen - headerLen;
		//----------------------------------------------
		if(fpProcess) fpProcess(pPacket, nDataSize);
		//----------------------------------------------
		pPacket += nDataSize;

		// nextPacket parsing
		if (pPacket) {
			headerLen = parsing_stringSize(pSockData->pData, &nPacketLen);
			if (!headerLen) goto RECV_END_PROC;
		}
		else break;
	}

	// �����Ͱ� ó���ǰ� ���� �����Ͱ� ������ -- 
RECV_END_PROC:
	nRemainLen = pSockData->nCurLen - nProcCompleteLen;
	if (nRemainLen) {
		pSocket->SETREMAINDATA(pSockData->pData + nProcCompleteLen, nRemainLen);
		pSocket->RecvPacket(pSockData->nTotLen - nRemainLen);
	}
	else pSocket->bindRecv();
}

/*
��Ŷ����
http Header : 
*/
void parsingHeader_html(void *pData)
{
	CNWSocket * pSocket = (CNWSocket *)pData;
	//if (fpProcess) fpProcess(pPacket, nDataSize);
}

E_HEADER_LENGTH_TYPE CComSocket::initConfig_headerParsing(int nIdx)
{
	char szSect[16], szBuf[64];
	int nSizeByteLength;

	if (!g_pEndDelimeter) {
		nSizeByteLength = GetPrivateProfileString(_T("HEADER"), _T("SIZE_END_CHAR"), "", szBuf, sizeof(szBuf), g_pSystem);
		g_pEndDelimeter = (char *)malloc(nSizeByteLength + 1);
		strcpy(g_pEndDelimeter, szBuf);
	}

	sprintf(szSect, "HEADER_PARSING_%d", nIdx);
	int nType = GetPrivateProfileInt(szSect, "HEADER_LEN_TYPE", 0, g_pSystem);
	switch (nType)
	{
	case 0: return ePacket_none;
	case 1: return ePacket_numberLength;
	case 2: return ePacket_stringLength;
	case 3: return ePacket_http;
	}
	return ePacket_none;
}


bool CComSocket::startComunicator(int nIdx, fp_parsingProcess function, E_HEADER_LENGTH_TYPE type)
{
	CNWCommunicator * p = (CNWCommunicator *)g_pNWCList->getObj(nIdx);
	if (!p) return false;
	fp_recvProcess recvProc = NULL;
	switch (type)
	{
	case ePacket_none: break;
	case ePacket_numberLength: recvProc = recvProc_length; break;
	case ePacket_stringLength: recvProc = recvProc_string; break;
	case ePacket_http: return false;
	}

	return p->start(recvProc, function);
}


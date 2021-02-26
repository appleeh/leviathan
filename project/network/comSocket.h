/******************************************************************************/
/*                                                                            */
/*   �� �ҽ� �ڵ��� �Ǹ��� ����ϴ� �ּ��� �������� ���ÿ�.                      */
/*   Copyright 2020 by keh													  */
/*                                                                            */
/*   by keh                                                                   */
/******************************************************************************/

/********************************************************************
2020.05.13 by KEH
-------------------------------------------------------------
Common Interface,
Not Yet finished!!

Function to separate only data by extracting only the data length
*********************************************************************/

#pragma once

#include "types.h"

typedef void(*fp_parsingProcess) (char *pData, int nSize);
extern char *g_pEndDelimeter;
enum E_HEADER_LENGTH_TYPE
{
	ePacket_none = 0,
	ePacket_numberLength = 1,
	ePacket_stringLength,
	ePacket_http, // header �� ���� : HTTP method(GET, POST, HEAD, PUT, DELETE, TRACE, OPTION, CONNECT), header �� �� : ����
};


class CComSocket {
public:
	static E_HEADER_LENGTH_TYPE initConfig_headerParsing(int nIdx = 0);
	static bool startComunicator(int nIdx, fp_parsingProcess function, E_HEADER_LENGTH_TYPE type);
};

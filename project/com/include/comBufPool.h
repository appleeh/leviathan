/******************************************************************************/
/*                                                                            */
/*   �� �ҽ� �ڵ��� �Ǹ��� ����ϴ� �ּ��� �������� ���ÿ�.                   */
/*   Copyright 2020 by keh									  */
/*                                                                            */
/*   by keh                                                                   */
/******************************************************************************/

/********************************************************************
	2020.05.13 by KEH
	-------------------------------------------------------------
	FOR THREAD SAFTY!
	NOT USE THIS CLASS!!!!!!!!
	High Dependendy on CMemManager

	if Used, memory corruption will occur!
	2016.11.11 by KEH
*********************************************************************/
#pragma once


#include "comTypes.h"


enum E_IDX_BUF {
	IDX_BUF_2 = 1,
	IDX_BUF_4,
	IDX_BUF_8,
	IDX_BUF_16,
	IDX_BUF_32,
	IDX_BUF_64,   // -- Log start
	IDX_BUF_128,
	IDX_BUF_256,
	IDX_BUF_512,
	IDX_BUF_1k,
	IDX_BUF_2k,
	IDX_BUF_4k,
	IDX_BUF_8k,
	IDX_BUF_16k,
	IDX_BUF_32k,
	IDX_BUF_64k,
	IDX_BUF_128k,
	IDX_BUF_256k,  // -- Log End
	IDX_BUF_512k,
	IDX_BUF_1m,
	IDX_BUF_2m,
	IDX_BUF_MAX,
};


struct STBufConf { int nMaxList[IDX_BUF_MAX]; };
extern const int g_nSizeList[IDX_BUF_MAX];
inline int getIndex(int s) { int m; union { double x; int b[2]; }; x = s; m = (b[1] >> 20) - 1023; return m + ((s&(s - (1 << m))) ? 1 : 0); }
inline int getSizeByIndex(int nIdx) { return g_nSizeList[nIdx]; }

class CBufPool 
{
public:
    // �޸� ����
	CBufPool();
	~CBufPool(void);							
    void clear();
	bool alloc(int nShift, int nMaxCount);
	inline int alloc2(int nSize, int nMaxCount) { return alloc(getIndex(nSize), nMaxCount); }
	char *	newMem();
	void	delMem(char *p);
	bool  newMemList(int nCount);
	void	delMem(char *p, int nCount);
	inline char * getMem(int i) { return &m_pChunk[i << m_nShift]; } // ������ ����� ���¿��� �ϰ�ó�� ���ٽ� ���
	inline char * getUseMem(int i) { if (i >= m_nAllocCount || 0 > i || !m_nFlagArray[i].getCount()) return NULL; return &m_pChunk[i]; }
	inline char *getNext(int *i) {
		for (*i; *i < m_nAllocCount; (*i)++) { 
			if (m_nFlagArray[*i].getCount()) { char *p = &m_pChunk[*i << m_nShift]; return p; }
		} 
		return NULL;
	}

	// memory pool �����Ȳ �� ��Ÿ    
	inline int	GETUSECOUNT()		{ return m_nUseCnt.getCount(); }
	inline int	GETREMAINCOUNT()    { return m_nAllocCount-m_nUseCnt.getCount(); }
	inline int  GETMAXCOUNT()	    { return m_nAllocCount; }
	inline TCHAR * getMessage()	    { return g_szMessage;   }
	inline bool ISINCLUDE_ADDRESS(char *p) { if(m_pChunk <= p && p <= m_pEnd) return true; return false; }
	inline int GETOVERCNT() { return m_nOverCnt.getCount(); }
private:
    atomic_nr * m_nFlagArray;       // keh 20130305 add
    atomic_nr	m_nUseCnt;
	atomic_nr m_nOverCnt;
	//atomic_nr m_nOverMaxCnt;
	int	m_nLastCount;		// lazy  count. roughly
	int m_nAllocCount;
	int m_nShift;
	char * m_pChunk;
	char * m_pEnd;
};


/********************************************************************
2020.07.17 by KEH
-------------------------------------------------------------
FOR SINGLE THREAD !
*********************************************************************/
class CSBufPool
{
public:

	// �޸� ����
	CSBufPool(void);
	~CSBufPool(void);						// ��ü�� �Ҹ�ɶ� malloc �� ���� chunk �޸� �ڵ� free
	void clear();
	bool alloc(int nShift, int nMaxCount);	// �ѹ��� �޸𸮸� ��Ƴ��´�.
	inline int alloc2(int nSize, int nMaxCount) { return alloc(getIndex(nSize), nMaxCount); }
	void destroy();


	char *	newMem(int *pKey = NULL);
	bool	delMem(char *);
	bool	delMemByIdx(char *pUsedMem, int nKey);

	bool newMemList(int nCount);
	bool delMem(char *p, int nCount);

	// �޸� direct ����

	inline char * getMem(int i) { return &m_pChunk[i << m_nShift]; } // ������ ����� ���¿��� �ϰ�ó�� ���ٽ� ���
	inline char *getUseMem(int nKey) { if (ISINCLUDE(nKey, m_nAllocCount) && isBitSet_flag(nKey, m_nFlagArray)) return &m_pChunk[nKey << m_nShift]; return NULL;  }

	// memory pool �����Ȳ �� ��Ÿ    
	inline int	GETUSECOUNT() { return m_nUseCnt; }
	inline int	GETREMAINCOUNT() { return m_nAllocCount - m_nUseCnt; }
	inline int  GETMAXCOUNT() { return m_nAllocCount; }
	inline TCHAR * getMessage() { return g_szMessage; }
	inline bool ISINCLUDE_ADDRESS(char *p) { if (m_pChunk <= p && p <= m_pEnd) return true; return false; }
private:
	int m_nUseCnt;
	int m_nAllocCount;
	char *m_nFlagArray;
	int m_nShift;
	char * m_pChunk;
	char * m_pEnd;
};
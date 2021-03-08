/********************************************************************************/
/*																				*/
/*   �� �ҽ� �ڵ��� �Ǹ��� ����ϴ� �ּ��� �������� ���ÿ�.							*/
/*   Copyright 2020 by keh														*/
/*																				*/
/*   by keh																		*/
/********************************************************************************/

/********************************************************************
	2019.09.27 by KEH
	-------------------------------------------------------------
	ring queue : �ڵ� ����Ʈ �޸� ����
	���� ������ => push, �̱� ������ => pop ��
*********************************************************************/
#pragma once
#include "comTypes.h"


// TODO: complete !! 20190926
// m_nFlagArray �� ���ְ� ���ɶ����� ��ü
// void* ������ ����
// ����Ʈ size �ڵ� ������ ����



class CQueueS
{
public:

	CQueueS();
	~CQueueS(void);				// ��ü�� �Ҹ�ɶ� malloc �� ���� chunk �޸� �ڵ� free
	bool alloc(int nMaxCount, E_ALLOC_TYPE type = eAlloc_Type_new);	// �ѹ��� �޸𸮸� ��Ƴ��´�.
	bool realloc(int nMaxCount, bool bInit=false);

	bool push(void *pData); 
	void* pop();
	inline void* next(int *idx) { void *res = m_pArray[*idx]; *idx++; return res; }
	//inline void* circularNext(int *idx) { void *res = m_pArray[*idx]; *idx++; return res; }

	inline bool empty() {return (m_nLast - m_nFront) ? false : true;}

	inline TCHAR * getMessage()	{ return g_szMessage; }
    inline int  capacity()   { return m_nMax; }
	inline int  size() { int res = m_nLast - m_nFront; if (res < 0) return -res; return res; }

private:
	void**	m_pArray;
	int	m_nLast;
	int	m_nFront;
	int	m_nMax;
	E_ALLOC_TYPE m_nObjAllocType;
	SPIN_LOCK m_cLockPush; // for push (by realloc)
	COM_CRITICAL_SECTION m_CS;  // for realloc
	SPIN_LOCK  m_cLockPop; // for pop, for realloc
	int m_nOldMax;
	atomic_nr m_nRealloc;


};



/********************************************************************************/
/*																				*/
/*   �� �ҽ� �ڵ��� �Ǹ��� ����ϴ� �ּ��� �������� ���ÿ�.							*/
/*   Copyright 2020 by keh														*/
/*																				*/
/*   by keh																		*/
/********************************************************************************/
#pragma once
#include "comList.h"


struct STCOMQueue {
	int nLast;
	int nFront;
	int nMax;
	int nUse;
	void **pList;
	inline void init() { nLast = 0; nFront = 0; nMax = 0; nUse = 0; pList = NULL; }
	inline int size() {	return nUse; }
	inline void pop() { pList[nFront] = NULL; nFront++; if (nFront == nMax) nFront = 0; nUse--; }
	inline void push(void *pData) { pList[nLast] = pData; nLast++; if (nLast == nMax) nLast = 0; nUse++; }
	inline bool isFull() { return (nMax == nUse); }
	inline int  capacity() { return nMax; }
	inline void * front() { return pList[nFront]; }
	inline void printfInfo() { printf("nSize[%d] nMax[%d] nFront[%d] nLast[%d]\n", nUse, nMax, nFront, nLast); }
};

/********************************************************************
	2019.09.27 by KEH
	-------------------------------------------------------------
	ring queue : �ڵ� ����Ʈ �޸� ����
	���� ������ => push, �̱� ������ => pop ��
*********************************************************************/


// TODO: complete !! 20190926
// m_nFlagArray �� ���ְ� ���ɶ����� ��ü
// void* ������ ����
// ����Ʈ size �ڵ� ������ ����
// �����带 �����ִ� ����� ���� , �ʿ�� ���� ����



class CQueueS
{
public:

	CQueueS();
	~CQueueS(void);				// ��ü�� �Ҹ�ɶ� malloc �� ���� chunk �޸� �ڵ� free
	bool alloc(int nMaxCount, E_ALLOC_TYPE type = eAlloc_Type_none);	// push �ϴ� �޸� �Ҵ� �޽��� ALLOC_TYPE ���� ������, �Ҹ��ڿ��� DeepDelete �Ѵ�.
	inline bool realloc(int nMaxCount) {destroy(m_pQueue); m_pQueue=__realloc(nMaxCount); return m_pQueue?true:false;	}
	bool push(void *pData); 
	void* pop(); // single thread pop
	inline int  capacity() { return m_pQueue ? m_pQueue->capacity() : NULL; }
	inline int  size() { return m_pQueue ? m_pQueue->size() : NULL; }

private:
	E_ALLOC_TYPE m_nObjAllocType;
	SPIN_LOCK m_cLockPush;		// for push (by realloc)
	COM_CRITICAL_SECTION m_CS;  // for realloc
	atomic_nr m_nRealloc;

	STCOMQueue *m_pQueue;
	CSList* m_pQueueList;

	STCOMQueue * __realloc(int nMaxCount);
	void destroy(STCOMQueue *pQueue);
	inline void * __pop(STCOMQueue *p) { void* res; if(res = p->front()) p->pop(); return res;}
};



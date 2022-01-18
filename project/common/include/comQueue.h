/********************************************************************************/
/*																				*/
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.							*/
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
	ring queue : 자동 리스트 메모리 증가
	다중 쓰레드 => push, 싱글 쓰레드 => pop 모델
*********************************************************************/


// TODO: complete !! 20190926
// m_nFlagArray 를 없애고 스핀락으로 교체
// void* 형으로 변경
// 리스트 size 자동 증가로 수정
// 쓰레드를 깨워주는 기능은 없음 , 필요시 별도 연동



class CQueueS
{
public:

	CQueueS();
	~CQueueS(void);				// 객체가 소멸될때 malloc 해 놓은 chunk 메모리 자동 free
	bool alloc(int nMaxCount, E_ALLOC_TYPE type = eAlloc_Type_none);	// push 하는 메모리 할당 받식을 ALLOC_TYPE 으로 넣으면, 소멸자에서 DeepDelete 한다.
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



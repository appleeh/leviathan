#include "comLogger.h"
#include "comMemManager.h"
#include "comQueue.h"

CQueueS::CQueueS()
{
	SPIN_LOCK_INIT(&m_cLockPush);
	m_pQueue = NULL;
	m_pQueueList = NULL;
}

CQueueS::~CQueueS(void)
{
	STCOMQueue* pQueue;
	int i = 0;
	if (m_pQueueList) {
		pQueue = (STCOMQueue*)m_pQueueList->getNext(&i);
		while (pQueue) {
			destroy(pQueue);
			m_pQueueList->del(i,false);  // 두번째 인자 : deepDelete -> false, destory(pQueue) 에서 모두 파괴 완료
			i++;
			pQueue = (STCOMQueue*)m_pQueueList->getNext(&i);
		}
		delete m_pQueueList;
		m_pQueueList = NULL;
		m_pQueue = NULL;
	}
	
	SPIN_LOCK_DESTROY(&m_cLockPush);
}


void CQueueS::destroy(STCOMQueue *pQueue)
{
	void *p;
	if (pQueue)
	{
		if (pQueue->pList) 
		{
			if (ISABLETODELETE(m_nObjAllocType)) {
				while ((p = pQueue->front())) {
					switch (m_nObjAllocType) {
					case eAlloc_Type_new:	delete p; break;
					case eAlloc_Type_alloc:	free(p); break;
					case eAlloc_Type_BufPool:	gs_pMMgr->delBuf((char*)p, STRING_SIZE((TCHAR*)p)); break;
					case eAlloc_Type_newArray:	delete[] p; break;
					}
					pQueue->pop();
				}
			}
			free(pQueue->pList);
			pQueue->pList = NULL;
		}
		delete pQueue;
	}
}



bool CQueueS::alloc(int nMaxCount, E_ALLOC_TYPE type)
{
	if (!m_pQueueList)
	{
		m_pQueueList = new CSList();
		m_pQueueList->alloc(12);
	}
	if (m_pQueue) {
		destroy(m_pQueue);
	}
	m_nObjAllocType = type;
	m_pQueue = __realloc(nMaxCount);
	if(m_pQueue) return true;
	return false;
}


STCOMQueue * CQueueS::__realloc(int nMaxCount)
{
	STCOMQueue *pQueue = new (std::nothrow) STCOMQueue();
	if (!pQueue) return NULL;

	pQueue->init();
	pQueue->pList = (void **)calloc(nMaxCount, sizeof(void*));
	if (!pQueue->pList) {
		return NULL;
	}
	pQueue->nMax = nMaxCount;
	m_nRealloc.init();
	return pQueue;
}

bool CQueueS::push(void* pData) // multi thread (Lock required)
{
	if (m_nRealloc.getCount()) {
		m_CS.enter();
		m_CS.leave();
	}
	else if(m_pQueue->isFull()) {
		if (m_nRealloc.atomic_compare_exchange(1, 0)) {
			m_CS.enter();
			m_CS.leave();
		}
		else {
			m_CS.enter();
			m_pQueue = __realloc(m_pQueue->capacity() << 1);
			if (!m_pQueue) { m_CS.leave(); return false; }
			m_pQueueList->push_back(m_pQueue);
			m_CS.leave();
		}
	}

	SPIN_LOCK_ENTER(&m_cLockPush);
	m_pQueue->push(pData);
	SPIN_LOCK_LEAVE(&m_cLockPush);

	return true;
}


void* CQueueS::pop() // single thread 
{
	int nIdx = 0;
	STCOMQueue *pQ = (STCOMQueue *)m_pQueueList->getNext(&nIdx);

	if (!pQ) return NULL;
	
	void* res = __pop(pQ);
	if (!res) {
		if (1 < m_pQueueList->size()) {
			m_pQueueList->del(nIdx, true); // pQ deep destroy
		}
	}
	return res;
}
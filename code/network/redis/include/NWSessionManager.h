/********************************************************************************/
/*																				*/
/*   �� �ҽ� �ڵ��� �Ǹ��� ����ϴ� �ּ��� �������� ���ÿ�.							*/
/*   Copyright 2021 by KimEunHye												*/
/*																				*/
/*   by keh																		*/
/********************************************************************************/
#pragma once

#include "comTList.h"

// CRedisList �� ��Ƽ������� ������ �ƴ�, �̱۾����忡�� Send �ϱ� ���� �ϳ��� ������ �ڷᱸ����.
class CRedisList
{
public:

	CRedisList();
	~CRedisList();			// ��ü�� �Ҹ�ɶ� malloc �� ���� chunk �޸� �ڵ� free
						// �ѹ��� �޸𸮸� ��Ƴ��´�.
	void clear();			// list �� ���� ���� ���� �޸𸮴� ������ �κ��� �ı��Ѵ�.
	inline int alloc(int nMaxCount, E_ALLOC_TYPE type = eAlloc_Type_BufPool) { m_nObjAllocType = type;  m_nMax = __alloc(nMaxCount); return m_nMax; }

	bool add(char *p, int nLen, int *pIdx = NULL);
	bool new_push_back(char *p, int nLen=0, int *pIdx = NULL);
	bool push_back(char *p, int nLen, int *pIdx = NULL);
	inline void* getObj(int i) { if (ISINCLUDE(i, m_nMax)) return m_pArgv[i]; return NULL; }
	inline void* getNext(int *pIdx) {
		for (*pIdx; *pIdx < m_nMax; (*pIdx)++) {
			if (m_pArgv[*pIdx]) { return m_pArgv[*pIdx]; }
		}
		return NULL;
	}
	inline char** getList() { return m_pArgv;	}
	inline size_t *getSizeList() { return m_pArgvLen;	}
	inline int  size() { return m_nUse; }
	inline int  lastIdx() { return m_nLast; }
	inline int  capacity() { return m_nMax; }
	inline void setObjAllocType(E_ALLOC_TYPE type) { m_nObjAllocType = type; }

	int getRedisContents(STSringBuf *p);

private:
	char**		m_pArgv;
	size_t * m_pArgvLen;
	int	m_nUse;
	int	m_nLast;
	int	m_nMax;
	E_ALLOC_TYPE m_nObjAllocType;
	char *m_pRedisFullContents;
	int m_nContentsIdx;
	int __alloc(int nMaxCount);
	inline void _DEL(int i) { m_pArgv[i] = 0; m_nUse--;}
};


#define ADD_CLEINT_COUNT 20

class CSessionManager
{
public:
	
	CSessionManager();
	~CSessionManager();

	bool createMemoryPool(int nListCnt, int nRedisListCnt);

	CRedisList * getRedisList(); // ���������� pop ���� ó���Ѵ�
	inline void returnRedisList(CRedisList *pList) { pList->clear();  m_pList->add(pList); 	//printf("getRedisList add [cur/tot:%d/%d]\n", m_pList->size(), m_pList->capacity());
	} // add �� �޸� ��ȯ�̴�.

private :
	CMemList<CRedisList> *m_pList;  // ��Ƽ�����忡 ������ �ڷᱸ��
};
extern CSessionManager * g_pSessMgr;

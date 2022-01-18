/******************************************************************************/
/*                                                                            */
/*   �� �ҽ� �ڵ��� �Ǹ��� ����ϴ� �ּ��� �������� ���ÿ�.                   */
/*   Copyright 2020 by keh									  */
/*                                                                            */
/*   by keh                                                                   */
/******************************************************************************/
/********************************************************************
2020.07.10 by KEH
-------------------------------------------------------------
@ function
- single thread
- no sequntial
- realloc

single push, single pop
*********************************************************************/
#pragma once

#include "comTypes.h"

class CSList
{
public:

	CSList();
	~CSList();			
							
	void clear();			
	inline int alloc(int nMaxCount, E_ALLOC_TYPE type = eAlloc_Type_new) { m_nObjAllocType = type;  m_nMax = __alloc(nMaxCount); return m_nMax; }

	bool push_back(void*p, int *pIdx = NULL);
	bool del(void*p, bool bDeepDelete=true);
	bool del(int i, bool bDeepDelete = true);
	inline void* getObj(int i) { if (ISINCLUDE(i, m_nMax)) return m_pArray[i]; return NULL; }
	inline bool isObj(void*p) { int i; for (i = 0; i < m_nMax; i++) { if (m_pArray[i] == p) { return true; } } return false; }
	inline void* getNext(int *pIdx) {
		for (*pIdx; *pIdx < m_nMax; (*pIdx)++) {
			if (m_pArray[*pIdx]) { return m_pArray[*pIdx]; }
		}
		return NULL;
	}

	// �̹� Add �� ������ �����Ҷ��� ���
	inline bool setObj(int idx, void*p) { if (ISINCLUDE(idx, m_nMax)) { if (m_pArray[idx]) { if (!p) _DEL(idx); else m_pArray[idx] = p; 	return true; } } return false; }
	// Ư�� ��ġ�� �ű� Obj �߰�
	inline bool setNewObj(int idx, void*p) { if (ISINCLUDE(idx, m_nMax)) { if (!m_pArray[idx]) { m_pArray[idx] = p; m_nUse++; m_nLast++;  return true; } } return false; }

	inline int  size() { return m_nUse; }
	inline int  lastIdx() { return m_nLast; }
	inline int  capacity() { return m_nMax; }
	inline void setObjAllocType(E_ALLOC_TYPE type) { m_nObjAllocType = type; }

private:
	void**		m_pArray;
	int	m_nUse;
	int	m_nLast;
	int	m_nMax;
	E_ALLOC_TYPE m_nObjAllocType;
	int __alloc(int nMaxCount);
	inline void _DEL(int i) { m_pArray[i] = 0; m_nUse--;}
};
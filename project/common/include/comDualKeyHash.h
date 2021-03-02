/******************************************************************************/
/*                                                                            */
/*   �� �ҽ� �ڵ��� �Ǹ��� ����ϴ� �ּ��� �������� ���ÿ�.                      */
/*   Copyright 2020 by keh													  */
/*                                                                            */
/*   by keh                                                                   */
/******************************************************************************/

/********************************************************************
2019.01.13 by KEH
-------------------------------------------------------------
Dual Hansh Map Library
*********************************************************************/

#pragma once

#include "comHash.h"
#include "comBMemPool.h"
//#include "comMemPool.h"
#include "comMemManager.h"

// �� Ŭ������ ����Ҷ�, �޸� �Ҵ翡 ���� ���� ������� ���ƾ� �Ѵ�.
// write �� ���� �۾��� ���� �� Ŭ������ �̱� ������ �����̴�. ��Ƽ������ ���ٽ� ť�� �Ἥ �̱� ������� ó���ϵ���, ��, ��ȸ�� �������
struct STValue
{
	int nIdx; // �޸� �Ҵ� index, -1 �̸� �ܺ��Ҵ�
	void *pValue;
};



class CDKeyHashMap
{
public :
	CDKeyHashMap(int nKey1Max, int nKey2Max, bool bValInternal=false);					// �ؽ�Ű ������ �ʱ⿡ �����ؾ� �Ѵ�.
	~CDKeyHashMap();

	int addKey(TCHAR *pKey1, TCHAR *pKey2, CHash **pTarget=NULL, void *pHashValue=NULL);							// ���ο��� �޸� �ڵ� �Ҵ�

	// key1, key2 ���ο��� �޸� �ڵ� �Ҵ�
	int addKeyValue(TCHAR *pKey1, TCHAR *pKey2, void *pValue, void *pHashValue=NULL); 

	// �̹� key1,key2 �� ����� �Ǿ��־�߸� ����
	int setValue(TCHAR *pKey1, TCHAR *pKey2, void *pValue);
	bool deleteHash(TCHAR *pKey1);
	bool deleteNode(TCHAR *pKey1, TCHAR *pKey2);

	// pValue �� string �Ǵ� ����ü ����; �ش� ������ ũ���� �޸� �Ҵ��Ͽ� memcpy ��
	STValue * newValueBuf(int size, TCHAR *pValue);
	void delValueBuf(STValue *pValue);
	inline STValue * getValueBuf() { return m_pValue->newMem(); }

	bool isNode(TCHAR *pKey1, TCHAR *pKey2);
	CHash * getHashMap(TCHAR *pKey1);
	STHash_Node * getNode(TCHAR *pKey1, TCHAR *pKey2);

	// for Loop
	CHash *getHashGroup() { return m_pHashGroup; }

private :
	CBMemPool<SThash_next> *m_pHashNext;	
	CBMemPool<STValue> *m_pValue;
	CHash *m_pHashGroup;
	HASH_TYPE m_nMaxCount1, m_nMaxCount2;
	bool m_bValInternal;

	inline SThash_next *newNode() { return m_pHashNext->newMem();  }
	inline bool delNode(SThash_next *p) { return m_pHashNext->delMem(p); }
};
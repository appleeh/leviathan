

#include "comDualKeyHash.h"
#include "comLogger.h"

#define ALLOC_HAXH_NEXT_COUNT(max1, max2) (max1+max2+50)

CDKeyHashMap::CDKeyHashMap(int nKey1Max, int nKey2Max, bool bValInternal)
{
	m_nMaxCount1 = nKey1Max;
	m_nMaxCount2 = nKey2Max;
	m_pHashNext = new CBMemPool<SThash_next>(eAlloc_Type_alloc);
	m_pHashNext->alloc(nKey1Max + nKey2Max + 50);
	m_pHashGroup = new CHash(nKey1Max);

	int nBasicCount = nKey2Max << 1;	//(nKey2Max * 2)
	gs_pMMgr->init(IDX_BUF_4, nBasicCount);
	gs_pMMgr->init(IDX_BUF_8, nBasicCount + 50);
	gs_pMMgr->init(IDX_BUF_16, nBasicCount + 50);
	gs_pMMgr->init(IDX_BUF_32, nBasicCount + 50);
	gs_pMMgr->init(IDX_BUF_64, nBasicCount);
	gs_pMMgr->init(IDX_BUF_128, nBasicCount);
	gs_pMMgr->init(IDX_BUF_256, 10);
	gs_pMMgr->init(IDX_BUF_512, 10);

	m_bValInternal = bValInternal;

	if (m_bValInternal) {
		m_pValue = new CBMemPool<STValue>(eAlloc_Type_alloc);
		m_pValue->alloc(nKey2Max + 50);
	}
	else m_pValue = NULL;
}

// �ؽø��� �Ҹ��ڴ� �Ϲ�������
CDKeyHashMap::~CDKeyHashMap()
{
	SThash_next *pNext1, *pNext2, *pDelNode;
	CHash *pHash;
	unsigned int i,j;
	
	// �� ��ü�� ���μ����� �����ֱ⸦ ���� �Ѵٸ�, ���� �Ʒ��� ���� ���� �� �ʿ�� ����. OS ���� ó���� �ֱ� �����̴�.
	// �׷����� �ұ��ϰ� ������ �޸� ������ �ϴ� ������ �޸� ������ ��Ȯ�� �Ͽ� �� �ڽ��� �޸𸮸� �������� ���������ν� ������ ��ƴ�� ������ �����ϱ� �����̴�.
	// �ʱ� �Ҵ緮�� ������ �߰��� �����Ҵ��� �̷���� �޸𸮵��� ������ ���ؼ��� ������ �޸𸮸� �Ʒ��� ���� ��ȯ�ؾ��Ѵ�.
	for (i = 0; i < m_nMaxCount1; i++) {
		pNext1 = m_pHashGroup->getNext(i);
		while (pNext1) {
			pHash = (CHash *)pNext1->pNode.value;
			for (j = 0; j < m_nMaxCount2; j++) {
				pNext2 = pHash->getNext(j);
				while (pNext2) {
					gs_pMMgr->delBuf(pNext2->pNode.pKey, STRING_SIZE(pNext2->pNode.pKey));	// key2 �޸� ����
					if(m_bValInternal)delValueBuf((STValue *)pNext2->pNode.value);					// key2 �� value �޸� ����
					pDelNode = pNext2;
					pNext2 = pNext2->next;
					m_pHashNext->delMem(pDelNode);												// key2,value,next ������ node ����
				}
			}
			gs_pMMgr->delBuf(pNext1->pNode.pKey, STRING_SIZE(pNext1->pNode.pKey));	// key1 �޸� ����
			delete pHash;																	// key1 �� value �޸� ����
			pDelNode = pNext1;
			pNext1 = pNext1->next;
			m_pHashNext->delMem(pDelNode);												// key1,value,next ������ node ����
		}
	}

	// �Ʒ� ��ƾ�� �ʱ⿡ �Ҵ�� �޸� ������ ������ �����Ѵ�.
	delete m_pValue;
	delete m_pHashNext;
	delete m_pHashGroup;
}


int CDKeyHashMap::addKey(TCHAR *pKey1, TCHAR *pKey2, CHash **pTarget, void *pHashValue)
{
	SThash_next *pNext;
	CHash *pHash;
	TCHAR *pBuf;
	int nRes1=0, nRes2=0;


	STHash_Node *pNode = m_pHashGroup->hashLookup(pKey1);
	if (pNode) {
		pHash = (CHash *)pNode->value;
		if (pHash->hashLookup(pKey2)) {
			gs_cLogger.DebugLog(LEVEL_ERROR, "DUPLICATE_KEYS pKey1[%s] pHash->hashLookup(pKey2[%s])", pKey1, pKey2);
			return eHASH_RESULT_DUPLICATE_KEYS;
		}
	}
	else {
		pHash = new (std::nothrow)CHash(m_nMaxCount2);					// key1 �� value �޸� �Ҵ�
		if (!pHash) return eHASH_RESULT_FAIL_ALLOCATION;
		pNext = m_pHashNext->newMem();						// key1,value,next ������ node �Ҵ�
		if(!pNext) return eHASH_RESULT_FAIL_ALLOCATION;
		pBuf = (TCHAR*)gs_pMMgr->newBuf(STRING_SIZE(pKey1));				// key1 �޸� �Ҵ�
		if(!pBuf) return eHASH_RESULT_FAIL_ALLOCATION;
		_tcscpy(pBuf, pKey1);
		nRes1 = m_pHashGroup->addNode(pNext, pBuf, (void *)pHash);	// 1�� �ؽøʿ� �߰�
		if (nRes1 < 0) {
			gs_cLogger.DebugLog(LEVEL_ERROR, "errorNo[%d] new CHash(max:%d) key1:%s", nRes1, m_nMaxCount2, pBuf);
			return nRes1;
		}
		gs_cLogger.PutLogQueue(LEVEL_DEBUG, _T("new CHash(max:%d) key1:%s hashNo[%d]"), m_nMaxCount2, pBuf, nRes1);
	}

	pNext = m_pHashNext->newMem();						// key2, value, next ������ node �Ҵ�
	if (!pNext) return eHASH_RESULT_FAIL_ALLOCATION;
	pBuf = (TCHAR*)gs_pMMgr->newBuf(STRING_SIZE(pKey2));				// key2 �޸� �Ҵ�
	if (!pBuf) return eHASH_RESULT_FAIL_ALLOCATION;
	_tcscpy(pBuf, pKey2);
	if (pHashValue) pHash->setHashValue(pHashValue);
	if(pTarget) *pTarget = pHash;
	// 2�� �ؽøʿ� �߰� (key2 �� value �� �������� ����)
	nRes2 = pHash->addNode(pNext, pBuf); // 2�� �ؽøʿ� �߰� (key2 �� value �� �������� ����)
	if (nRes2 < 0) {
		gs_cLogger.PutQueue(LEVEL_ERROR, "errorNo[%d] pHash->addNode(pNext, %s)", nRes2, pBuf);
		return nRes2;
	}
	return nRes2;
}

int CDKeyHashMap::addKeyValue(TCHAR *pKey1, TCHAR *pKey2, void *pValue, void *pHashValue)	// key �ߺ��� return ERROR_CODE
{
	CHash *pHash;
	int nRes; // 1. key2's hashKey -> 2. nRes
	nRes = addKey(pKey1, pKey2, &pHash);
	if (pHash) {
		if(pHashValue) pHash->setHashValue(pHashValue);
		if (nRes >= 0) {
			nRes = pHash->setValue(nRes, pKey2, pValue);
		}
	}
	return nRes;
}

int CDKeyHashMap::setValue(TCHAR *pKey1, TCHAR *pKey2, void *pValue)
{
	CHash *pHash;
	STHash_Node *pNode = m_pHashGroup->hashLookup(pKey1);
	if (pNode) {
		pHash = (CHash *)pNode->value;
		pNode = pHash->hashLookup(pKey2);
		if (pNode) {
			if (pNode->value) { delValueBuf((STValue *)pNode->value); }
			pNode->value = pValue; 
			return eHASH_RESULT_SUCESS;
		}
	}
	return eHASH_RESULT_INVALID_KEY;
}


bool CDKeyHashMap::isNode(TCHAR *pKey1, TCHAR *pKey2)
{
	STHash_Node *pNode = m_pHashGroup->hashLookup(pKey1);
	if (pNode) {
		CHash *pHash = (CHash *)pNode->value;
		if (pHash->hashLookup(pKey2)) return true;
	}
	return false;
}

CHash * CDKeyHashMap::getHashMap(TCHAR *pKey1)
{
	STHash_Node *pNode = m_pHashGroup->hashLookup(pKey1);
	return (CHash *)pNode->value;		// return NULL ����
}

STHash_Node * CDKeyHashMap::getNode(TCHAR *pKey1, TCHAR *pKey2)
{
	STHash_Node *pNode = m_pHashGroup->hashLookup(pKey1);
	if (pNode) {
		CHash *pHash = (CHash *)pNode->value;
		return pHash->hashLookup(pKey2);
	}
	return NULL;
}

bool CDKeyHashMap::deleteHash(TCHAR *pKey1)
{
	SThash_next **ppPrev=NULL;
	SThash_next *pNext2;
	SThash_next *pCur = m_pHashGroup->getNode(pKey1, ppPrev);
	CHash *pHash;
	unsigned int j;

	if (pCur) {
		*ppPrev = pCur->next; // NULL ���� ����.

		pHash = (CHash *)pCur->pNode.value;
		for (j = 0; j < m_nMaxCount2; j++) {
			pNext2 = pHash->getNext(j);
			while (pNext2) {
				gs_pMMgr->delBuf(pNext2->pNode.pKey, STRING_SIZE(pNext2->pNode.pKey));	// key2 �޸� ����
				if (m_bValInternal) delValueBuf((STValue *)pNext2->pNode.value);	// key2 �� value �޸� ����
				m_pHashNext->delMem(pNext2);										// key2,value,next ������ node ����
				pNext2 = pNext2->next;
			}
		}
		gs_pMMgr->delBuf(pCur->pNode.pKey, STRING_SIZE(pCur->pNode.pKey));	// key1 �޸� ����
		delete pHash;															// key1 �� value �޸� ����
		m_pHashNext->delMem(pCur);												// key1,value,next ������ node ����

		return true;
	}		
	return false;
}



bool CDKeyHashMap::deleteNode(TCHAR *pKey1, TCHAR *pKey2)
{
	SThash_next **ppPrev=NULL;
	SThash_next *pNext1, *pCur2;

	pNext1 = m_pHashGroup->getNext(pKey1);

	if (pNext1) {
		CHash *pHash = (CHash *)pNext1->pNode.value;
		pCur2 = pHash->getNode(pKey2, ppPrev);
		if (pCur2) {
			*ppPrev = pCur2->next;  // NULL ���� ����.
			gs_pMMgr->delBuf(pCur2->pNode.pKey, STRING_SIZE(pCur2->pNode.pKey));	// key2 �޸� ����
			if (m_bValInternal)delValueBuf((STValue *)pCur2->pNode.value);	// key2 �� value �޸� ����
			m_pHashNext->delMem(pCur2);										// key2,value,next ������ node ����

			return true;
		}
	}
	return false;
}


STValue * CDKeyHashMap::newValueBuf(int size, TCHAR *pValue)			// pValue �� string �Ǵ� ����ü ����; �ش� ������ ũ���� �޸� �Ҵ��Ͽ� memcpy ��
{
	int nIdx;
	STValue *pSTValue = m_pValue->newMem();
	TCHAR *pValueBuf = gs_pMMgr->newBuf(size, &nIdx);
	c_memcpy(pValueBuf, pValue, size);
	pSTValue->nIdx = nIdx;
	pSTValue->pValue = pValueBuf;

	return pSTValue;
}


void CDKeyHashMap::delValueBuf(STValue *pValue) 
{ 
	if (m_bValInternal) {
		gs_pMMgr->delBufByIndex((TCHAR *)pValue->pValue, pValue->nIdx); m_pValue->delMem(pValue);
	}

	// �ܴ̿� �ܺ� �Ҵ�... �ܺο��� ��ȯó��
} // value �� ����� ���� �޸� ��ȯ

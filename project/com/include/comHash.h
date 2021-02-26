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
Hash API
/******************************************************************************/


#ifndef	__COMHASH_H__
#define	__COMHASH_H__



#include "types.h"


typedef unsigned int HASH_TYPE;
#define none  0xffffffff;

struct STHash_Node
{
	TCHAR *pKey;
	void *value;		// Data
};


struct SThash_next
{
	STHash_Node pNode;
	SThash_next *next;	// �ߺ��� key ���� ���� next data
};

struct SThash_value
{
	HASH_TYPE nHashKey;
	unsigned int nElementCnt;
	SThash_next *pNext;
};

// info: Thread safty ���� ���� Ŭ����. -- ���� ��Ƽ �����忡�� ����ϱ� ���ؼ��� add �Ҷ�, Lock �ʿ�
//		 Thread safty �� Ŭ������ �������, m_pHashData �� comList �� ����
// TODO Mempool ���� �Ͽ�, add �� ���� �޸� Pool ���� �޸� �Ҵ�

enum HASH_RETURN_CODE
{
	eHASH_RESULT_SUCESS = 0,
	eHASH_RESULT_DUPLICATE_KEYS=-1,
	eHASH_RESULT_NOT_FOUND=-2,
	eHASH_RESULT_INVALID_KEY = -3,
	eHASH_RESULT_EMPTY_NODE = -4,
	eHASH_RESULT_FAIL_ALLOCATION=-5
};


class CHash
{
public :
	CHash(HASH_TYPE max);
	~CHash();
	void clear();
	HASH_TYPE getHashNo(TCHAR *pKey);

	STHash_Node * hashLookup(TCHAR *pKey);
	STHash_Node * hashLookup(HASH_TYPE nHashNo, TCHAR *pStr);
	SThash_next * getNext(HASH_TYPE nHashKey);
	SThash_next * getNext(TCHAR *pKey);
	SThash_next * getNode(TCHAR *pStr, SThash_next **prevNext);
	//	bool addHash(SThash_next *pNext, TCHAR *pKey, void *pData);
	HASH_RETURN_CODE setValue(TCHAR *pKey, void *pData);								// only set
	HASH_RETURN_CODE setValue(HASH_TYPE nHashNo, TCHAR *pKey, void *pData);				// only set
	int addNode(SThash_next *pNext, TCHAR *pKey, void *pData=NULL);			// add key & value, �ߺ�Ű üũ ���� - return Code : hashKey
	int uniqueAddNode(SThash_next *pNext, TCHAR *pKey, void *pData=NULL);	// add key & value, �ߺ�Ű üũ , �ߺ�Ű �߰ߵǸ� ���� ���� - return Code : hashKey

	size_t getString(TCHAR *pTitle, TCHAR *pBuf, unsigned int nBufMax);
	inline HASH_TYPE getMaxCount() { return m_nMaxCount; }
	inline HASH_TYPE getTotCount() { return m_nElementCount; }
	inline SThash_value * getHashData() { return m_pHashData; }

	inline void setHashValue(void *p) { m_pHashValue = p; }
	inline void * getHashValue() { return m_pHashValue; }

private : 
	HASH_TYPE m_nMaxCount;
	unsigned int m_nElementCount;
	SThash_value *m_pHashData;
	void *m_pHashValue;
};



#endif	//__COMHASH_H__
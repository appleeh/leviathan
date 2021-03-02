/******************************************************************************/
/*                                                                            */
/*   �� �ҽ� �ڵ��� �Ǹ��� ����ϴ� �ּ��� �������� ���ÿ�.                   */
/*   Copyright 2012 by keh									  */
/*                                                                            */
/*   by keh                                                                   */
/******************************************************************************/

/********************************************************************
	2012.05.13 by KEH
	-------------------------------------------------------------

*********************************************************************/
#pragma once


#include "comTypes.h"
#define LEN_MEM_MESSAGE		128
#define MEMORY_ALLOCK_BLOCK	100

// MEMTYPE class ��������

template <typename MEMTYPE>
class CSharedPool
{
public:

    // �޸� ����
	CSharedPool();
	~CSharedPool(void);							// ��ü�� �Ҹ�ɶ� malloc �� ���� chunk �޸� �ڵ� free
    void clear();
	int alloc(int nMaxCount, void *pSharedChunk);	// �ѹ��� �޸𸮸� ��Ƴ��´�.

	MEMTYPE *	newMem(int *pKey=NULL);		// thread safty. ����� ���, ��ȯ�� �߻��Ҷ� ��� 
	bool		delMem(MEMTYPE *);			// thread safty. ����� ���, ��ȯ�� �߻��Ҷ� ���

	inline void memSetComplete(int nKey) { if(m_nFlagArray[nKey].getCount() == 1) m_nFlagArray[nKey].setCount(2);  }

    // �޸� direct ����
    inline MEMTYPE * getMem(int i)          { return &m_pChunk[i]; }  // ������ ����� ���¿��� �ϰ�ó�� ���ٽ� ���
    MEMTYPE *	getUseMem(int nKey);   // key ��ȿ��, ��뿩�� üũ - �⺻ �޸� ���ٽ� ���
	MEMTYPE *	getNext(int i, int *PNextIdx);


    // memory pool �����Ȳ �� ��Ÿ    
    inline int	GETUSECOUNT()		{ return m_nUseCnt; }	
	inline int	GETREMAINCOUNT()    { return m_nAllocCount-m_nUseCnt.getCount(); }	
	inline int  GETMAXCOUNT()	    { return m_nAllocCount; }
	inline char * getMessage()	    { return m_szMessage;   }

private:
    atomic_nr * m_nFlagArray;       // keh 20130305 add
    atomic_nr m_nUseCnt;
	int m_nAllocCount;
	char m_szMessage[LEN_MEM_MESSAGE];
    MEMTYPE *       m_pChunk;

};

template <typename MEMTYPE>
CSharedPool<MEMTYPE>::CSharedPool()
{
	m_nAllocCount = 0;
	memset(m_szMessage, 0 ,LEN_MEM_MESSAGE); 
	m_pChunk = NULL;
	m_nFlagArray = NULL;
}

template <typename MEMTYPE>
CSharedPool<MEMTYPE>::~CSharedPool(void)
{
    if(m_pChunk) {
        m_pChunk = NULL; // m_pChunk �� ���� �Ҵ��� Ŭ���� �Ҹ��ڿ��� �޸� �����Ѵ�. (���⼭�� ���ڷ� ���ԵǾ��� ������ �ʱ�ȭ�� ����)
    }
    if(m_nFlagArray) {
        delete [] m_nFlagArray;
        m_nFlagArray = NULL;
    }
	
}


template <typename MEMTYPE> 
int CSharedPool<MEMTYPE>::alloc(int nMaxCount, void *pSharedChunk)
{
	//int i;
	
	m_nFlagArray = new atomic_nr[nMaxCount];
	if(m_nFlagArray == NULL) {
	#ifdef USE_POSIX_LIB
			sprintf(m_szMessage, "CMemPool::m_nFlagArray allock ERROR");
	#else
			sprintf_s(m_szMessage, LEN_MEM_MESSAGE-1, "CMemPool::m_nFlagArray allock ERROR");
	#endif
		   return 0;
	}


	m_pChunk = (MEMTYPE *)pSharedChunk;

    if(m_pChunk == NULL) {
        
#ifdef USE_POSIX_LIB
		sprintf(m_szMessage, "CMemPool::m_pChunk allock ERROR");
#else
        sprintf_s(m_szMessage, LEN_MEM_MESSAGE-1, "CMemPool::m_pChunk allock ERROR");
#endif
	

		delete[] m_nFlagArray;
		m_nFlagArray = NULL;
		return 0;
    }

    m_nAllocCount   = nMaxCount;

#ifdef USE_POSIX_LIB
		sprintf(m_szMessage, "mem allock list m_nAllocCount[%d]", m_nAllocCount);
#else
        sprintf_s(m_szMessage, LEN_MEM_MESSAGE-1, "mem allock list m_nAllocCount[%d]", m_nAllocCount);
#endif

		

	return 1;
}

template <typename MEMTYPE>
void CSharedPool<MEMTYPE>::clear()
{
    memset(m_nFlagArray, 0, m_nAllocCount * sizeof(atomic_nr));
	m_nUseCnt.setCount(0);
	memset(m_szMessage, 0, sizeof(m_szMessage));
}



template <typename MEMTYPE> 
MEMTYPE * CSharedPool<MEMTYPE>::newMem(int *pKey)        // random pop
{
	int i= m_nUseCnt.getCount();
	
	
	while(m_nUseCnt.getCount() < m_nAllocCount)
    {
		if(i < m_nAllocCount) 
		{
			if(!m_nFlagArray[i].getCount()) 
			{
				if(!m_nFlagArray[i].atomic_compare_exchange(1,0)) 
				{
					m_nUseCnt.atomic_increment();
					if(pKey) 
					{ // NULL �� �ƴ� �� ���ڰ� �����ϸ�,
						*pKey = i;
					}
					return &m_pChunk[i];
				}
			}
			i++;
		}
		else i = 0;
    }    
    return NULL;
}


template <typename MEMTYPE> 
bool CSharedPool<MEMTYPE>::delMem(MEMTYPE *pUsedMem)	// random push
{
    int i;
    for(i = 0; i < m_nAllocCount; i++)
    {
        if(pUsedMem == &m_pChunk[i]) {
			m_nFlagArray[i].setCount(1);
			m_nFlagArray[i].atomic_exchange(0);
			m_nUseCnt.atomic_decrement();
            return true;
        }
    }
    return false;
}

template <typename MEMTYPE> // keh 20130305 add - 
MEMTYPE * CSharedPool<MEMTYPE>::getUseMem(int nKey)     
{
    if(0 <= nKey && nKey < m_nAllocCount)   // key ��ȿ�� üũ
    {
		if(m_nFlagArray[nKey].getCount() == 2) {            // ����� üũ
            return &m_pChunk[nKey];
        }
    }

    return NULL;                            
    
}

template <typename MEMTYPE>	// keh 20130430 add						
MEMTYPE *  CSharedPool<MEMTYPE>::getNext(int i, int *PNextIdx)
{
    int nLastCount = m_nAllocCount;
    if(i < nLastCount) 
    {
        if(m_nFlagArray[i].getCount() == 2) {
            *PNextIdx = i + 1;
            return &m_pChunk[i];	
        }
        else 
        {
            int idx = i+1;
            for(idx; idx < nLastCount; idx++)
            {
				if(m_nFlagArray[idx].getCount() == 2) {
                    *PNextIdx = idx+1;
                    return &m_pChunk[idx];	
                }
            }
        }
    }
    return NULL;
}
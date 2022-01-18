#include "Analyzer_Sequence.h"


CAnalyzer_Sequence::CAnalyzer_Sequence()
{
	m_pKeySeqList = NULL;
}
CAnalyzer_Sequence::~CAnalyzer_Sequence()
{
	int i = 0;
	STAnalyzer_Seq *pStruct;
	if (m_pKeySeqList) {
		pStruct = (STAnalyzer_Seq *)m_pKeySeqList->getObj(i++);
		while (pStruct) {
			if (pStruct->pKeyword) {
				gs_pMMgr->delString(pStruct->pKeyword);
			}
			if (pStruct->m_pErrorLineList) {
				delete pStruct->m_pErrorLineList;
				pStruct->m_pErrorLineList = NULL;
			}
			pStruct = (STAnalyzer_Seq *)m_pKeySeqList->getObj(i++);
		}
		delete m_pKeySeqList;
		m_pKeySeqList = NULL;
	}

}


bool CAnalyzer_Sequence::initConfig(char *pConfigFile, char *pSector)
{
	int nRes, nCount, i;
	char szBuf[128], szKey[24];
	STAnalyzer_Seq *pSeq;

	nCount = GetPrivateProfileInt(pSector, _T("KEYWORD_COUNT"), 0, pConfigFile);

	if (!nCount) return false;

	m_pKeySeqList = new (std::nothrow) CSList();
	m_pKeySeqList->alloc(nCount, eAlloc_Type_alloc);

	for (i = 0; i < nCount; i++) {
		sprintf(szKey, "KEYWORD_%d", i);
		nRes = GetPrivateProfileString(pSector, szKey, _T(""), szBuf, sizeof(szBuf), pConfigFile);
		if (nRes) {
			pSeq = (STAnalyzer_Seq *)calloc(1, sizeof(STAnalyzer_Seq));
			pSeq->pKeyword = gs_pMMgr->newString(szBuf);
			pSeq->m_pErrorLineList = new CStringList();
			pSeq->m_pErrorLineList->alloc(64);
			m_pKeySeqList->push_back(pSeq);
		}
	}
	CAnalyzer::setAvailable(true);
	return true;
}

bool CAnalyzer_Sequence::parsingLine(char *pLine)
{
	int i = 0, nValue;
	STAnalyzer_Seq *pStruct, *pRes = NULL;


	if (m_pKeySeqList) {
		pStruct = (STAnalyzer_Seq *)m_pKeySeqList->getObj(i);
		while (pStruct) {
			if (pStruct->pKeyword) {
				nValue = getIntValue(pLine, pStruct->pKeyword);
				if (nValue < 0) goto SEQ_NEXT_LOOP;
				if ((nValue - pStruct->nPrevSequenceNumber) != 1) {
					pStruct->m_pErrorLineList->push_back(pLine);
				}
				pStruct->nPrevSequenceNumber = nValue;
			}
		SEQ_NEXT_LOOP:
			i++;
			pStruct = (STAnalyzer_Seq *)m_pKeySeqList->getObj(i);
		}
	}
	return true;
}



void CAnalyzer_Sequence::reportErrorList(CLogger *pLogger, CStringList *pStringList)
{
	int i=0;
	char *pLine;
	while ((pLine = pStringList->getNext_str(&i))) {
		pLogger->LogPrint(pLine);
		i++;
	}
}


void CAnalyzer_Sequence::report(CLogger *pLogger)
{
	int i = 0, nPos = 0;
	STAnalyzer_Seq *pStruct;
	char szBuf[1024];

	if (m_pKeySeqList) {

		nPos += sprintf(szBuf + nPos, "\n\n################################################# \n");
		nPos += sprintf(szBuf + nPos, "  Sequence Report : \n");
		nPos += sprintf(szBuf + nPos, "################################################# \n");
		pLogger->LogPrint(szBuf); nPos = 0;

		pStruct = (STAnalyzer_Seq *)m_pKeySeqList->getObj(i);
		while (pStruct) {
			if (pStruct->pKeyword) {
				nPos += sprintf(szBuf, "\n[%d] key[%s] Sequence report :  ErrorCount:%d \n", i, pStruct->pKeyword, pStruct->m_pErrorLineList->GETUSECNT());
				pLogger->LogPrint(szBuf);
				if (pStruct->m_pErrorLineList) {
					reportErrorList(pLogger, pStruct->m_pErrorLineList);
				}
			}
			i++;
			pStruct = (STAnalyzer_Seq *)m_pKeySeqList->getObj(i);
		}
	}
}
#pragma once

#include "Analyzer.h"

struct STAnalyzer_Seq
{
	char *pKeyword;
	int nPrevSequenceNumber;
	CStringList *m_pErrorLineList;
};

class CAnalyzer_Sequence : public CAnalyzer
{
public:
	CAnalyzer_Sequence();
	~CAnalyzer_Sequence();

	bool initConfig(char *pConfigFile, char *pSector);
	bool parsingLine(char *p);
	void report(CLogger *pLogger);

private:
	CSList *m_pKeySeqList;  // specific Key's value average
	void reportErrorList(CLogger *pLogger, CStringList *pStringList);

};
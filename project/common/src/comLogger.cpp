/********************************************************************************/
/*																				*/
/*   이 소스 코드의 권리를 명시하는 주석을 제거하지 마시오.							*/
/*   Copyright 2012 by coms All Right Reserved									*/
/*																				*/
/*   by keh																		*/
/********************************************************************************/
/*	MBCS : English 2bytes, Korean 1byte											*/
/*	WBCS : English, Korean, NULL -> 2bytes										*/
/********************************************************************************/


#include "comLogger.h"
#include "comEnv.h"


#define MAX_MESSAGE_LEN	256
//static CLogger gs_cLogger;
//static CTimeExpManager gs_DTEManager;
CLogger gs_cLogger;
CTimeExpManager gs_DTEManager;

char g_szKeepDuratrion[8] = { "15D" };
char g_szLogExtension[8] = { "log" };

CTimeExpManager::CTimeExpManager()
{
	m_pTimeExpList = NULL;
}
CTimeExpManager::~CTimeExpManager()
{
	if (m_pTimeExpList) delete m_pTimeExpList; // 내부 malloc 까지 전부 destroy
}

STLogDateTimeExp *CTimeExpManager::getDTE(int nIdx) { if (!m_pTimeExpList) return NULL;  return (STLogDateTimeExp *)m_pTimeExpList->getObj(nIdx); }

bool CTimeExpManager::initConfig(TCHAR *pConfig)
{
	TCHAR szSection[64];
	STLogDateTimeExp *pDTE;
	size_t nDTESize = sizeof(STLogDateTimeExp);
	int nCount;


	nCount = GetPrivateProfileInt(_T("DATETIME_EXPRESSION"), _T("LIST_MAX"), 0, pConfig);
	int i;
	m_pTimeExpList = new CSList();
	if (!m_pTimeExpList) {
		printf("new CList() is NULL!\t%s%d\n", __FUNCTION__, __LINE__);
		return false;
	}
	if (!m_pTimeExpList->alloc(nCount + LIST_ADD_COUNT, eAlloc_Type_alloc)) {
		printf("m_pTimeExpList->alloc has Failed!\t%s%d\n", __FUNCTION__, __LINE__);
		return false;
	}
	
	// Default
	if (!nCount) {
		nCount = 1;
		pDTE = (STLogDateTimeExp *)malloc(nDTESize);
		pDTE->bTimeDelimeter = 1;
		pDTE->cDateDelimeter = '/';
		pDTE->bBracketEnclosed = 1;
		pDTE->nLastType = DT_MILLISEC;
		pDTE->nYearLen = 4;
		pDTE->nTZType = TZ_NONE;
		m_pTimeExpList->push_back(pDTE);
	}
	else {
		for (i = 0; i < nCount; i++) {
			pDTE = (STLogDateTimeExp *)malloc(nDTESize);
			_stprintf(szSection, _T("DATETIME_EXPRESSION_%d"), i);
			pDTE->bTimeDelimeter = GetPrivateProfileInt(szSection, _T("IS_TIME_DELIMETER"), 1, pConfig);
			pDTE->cDateDelimeter = (char)GetPrivateProfileInt(szSection, _T("DATE_DELIMETER"), '/', pConfig);
			pDTE->bBracketEnclosed = GetPrivateProfileInt(szSection, _T("IS_BRACKET_ENCLOSED"), 1, pConfig);
			pDTE->nLastType = (DT_TYPE)GetPrivateProfileInt(szSection, _T("LAST_TYPE"), DT_MILLISEC, pConfig);
			pDTE->nYearLen = GetPrivateProfileInt(szSection, _T("YEAR_LEN"), 4, pConfig);
			pDTE->nTZType = (TZ_TYPE)GetPrivateProfileInt(szSection, _T("TZ_TYPE"), TZ_NONE, pConfig);
			m_pTimeExpList->push_back(pDTE);
		}
	}
	return true;
}


CLogger::CLogger()
{
	m_hFile = NULL;

	m_nLogLevel = eLDOPT_LOGLEVEL;
	m_nLogSize = 0;
	m_nLogFlag = 0;
	m_nStlNo = eLDOPT_STL_NO;

	m_pLogDir = NULL;
	m_pLogName = NULL;
	strcpy(m_szFileExtension,g_szLogExtension);
	m_nSeq = 0;
	m_nSizeLimit = eLDOPT_SIZE_LIMIT;
	m_nLogSize = 0;
	m_nDateTimeFormat = 0;
	m_fpDataProc = NULL;

	m_szHeaderFormat[0] = 0;
	m_pObj = NULL;

	m_nDeleteInterval = 0;
	m_pLogFileListForDelete = NULL;
}



CLogger::~CLogger()
{
	if (isLogFlag(IS_LOG_ENABLE)) {
		clearLogFlag(IS_LOG_ENABLE);
	}
	if (m_hFile) { fclose(m_hFile); m_hFile = NULL;	}
	if (m_pLogDir) {
		delete m_pLogDir; m_pLogDir = NULL;
	}
	if (m_pLogName) {
		delete m_pLogName; m_pLogName = NULL;
	}

	if (m_pLogFileListForDelete) {
		CFileUtil::destroyFileList(m_pLogFileListForDelete);
		gs_pMMgr->delString(m_pLogFileListForDelete->root);
		free(m_pLogFileListForDelete); m_pLogFileListForDelete = NULL;
	}
}

bool CLogger::initDefault(TCHAR *pDir, TCHAR *pName, int nLogType, int nLogLevel, int nSizeLimit, TCHAR *pFileExtension)
{
	TCHAR szValue[128], szValue2[128], szBuf[12];

	if (!pDir) {
		comErrorPrint(_T("pDir is NULL"));
		return false;
	}
	if (pName) {
		if (pDir[0] == '~') {
			changeToAbsolutePath(szValue, pDir);
			pDir = szValue;
		}
	}
	else {
		detachFullPath(pDir, szValue, szValue2, szBuf);
		pDir = szValue; pName = szValue2;
	}

	if (!_initCom(pDir, pName, pFileExtension)) return false;

	if (!_setLoggerQueue(eLDOPT_LISTMAX)) {
		comErrorPrint(_T("_setLoggerQueue has Failed"));
		return false;
	}
	if (!_setHeaderFormat(eLDOPT_TIMESTEMP_NO)) {
		comErrorPrint(_T("_setHeaderFormat has Failed"));
		return false;
	}

	m_nStlNo = 0;
	m_nLogLevel = nLogLevel;
	_setLoggerBasic(nLogType, eLDOPT_IS_PRINT_LEVEL, nSizeLimit);
	setLogFlag(IS_LOG_SET_COMPLETE);
	return true;
}


E_LOG_INIT_RES CLogger::initLog(STLogInfo *pLogInfo)
{
	TCHAR szValue[128];
	if (pLogInfo->pLogDir[0] == '~') {
		changeToAbsolutePath(szValue, pLogInfo->pLogDir);
		pLogInfo->pLogDir = szValue;
	}
	if (!_initCom(pLogInfo->pLogDir, pLogInfo->pLogName, pLogInfo->pFileExtension, pLogInfo->szKeepDuration)) {
		return LOG_INIT_ERROR;
	}
	_setHeaderFormat(pLogInfo->nDateTimeFormat);
	_setLoggerBasic(pLogInfo->nLogType, pLogInfo->nLogLevel, pLogInfo->nSizeLimit);
	if (pLogInfo->nQueueListCount < 0) {
		pLogInfo->nQueueListCount = eLDOPT_LISTMAX;
	}
	_setLoggerQueue(pLogInfo->nQueueListCount);
	m_nStlNo = pLogInfo->nSTLNo;

	if (pLogInfo->bIsPrint) {
		m_nLogFlag |= IS_PRINT;
	}
	if (pLogInfo->bIsLevelPrint) {
		m_nLogFlag |= IS_PRINT_LEVEL;
	}


	setLogFlag(IS_LOG_SET_COMPLETE);
	return LOG_INIT_SUCCESS;
}

E_LOG_INIT_RES CLogger::initConfig(TCHAR *pConfig, TCHAR *pSection)
{
	int nMax, nRes, nVal;
	TCHAR szValue[128], szValue2[128], szValue3[8], szValue4[8];
	E_LOG_INIT_RES eRes = LOG_INIT_SUCCESS;

	nRes = GetPrivateProfileString(pSection, "LOGNAME", "", szValue2, 128, pConfig);
	if (!nRes) {
		if (&gs_cLogger == this) { eRes = LOG_INIT_NO_FILE; goto SET_DISABLE; }
		else {	comErrorPrint(_T("LOGNAME is NULL!"));	return LOG_INIT_ERROR;}
	}
	nRes = GetPrivateProfileInt(pSection, _T("ISENABLE"), 0, pConfig);
	if (!nRes) { eRes = LOG_INIT_SUCCESS; goto SET_DISABLE; }

	nRes = GetProfilePath(pSection, _T("LOGDIR"), _T("Log"), szValue, pConfig);
	GetPrivateProfileString(pSection, _T("EXTENSION"), g_szLogExtension, szValue3, sizeof(szValue3), pConfig);
	GetPrivateProfileString(pSection, _T("KEEP_DURATION"), g_szKeepDuratrion, szValue4, sizeof(szValue4), pConfig);
	if (!_initCom(szValue, szValue2, szValue3, szValue4)) return LOG_INIT_ERROR;

	nMax = GetPrivateProfileInt(pSection, _T("LIST_MAX"), eLDOPT_LISTMAX, pConfig);
	if (!_setLoggerQueue(nMax)) {
		comErrorPrint(_T("_setLoggerQueue has Failed"));
		return LOG_INIT_ERROR;
	}
	nRes = GetPrivateProfileInt(pSection, _T("TIMESTEMP_NO"), eLDOPT_TIMESTEMP_NO, pConfig);
	if (nRes < 0) {
		_stprintf(g_szMessage, _T("invalid value[%d]! pSection[%s]"), nRes, pSection);
		comErrorPrint(g_szMessage);
		return LOG_INIT_ERROR;
	}
	if (!_setHeaderFormat(nRes)) {
		comErrorPrint(_T("_setHeaderFormat has Failed"));
		return LOG_INIT_ERROR;
	}
	m_nStlNo = GetPrivateProfileInt(pSection, _T("STL_NO"), eLDOPT_STL_NO, pConfig);

	nRes = GetPrivateProfileInt(pSection, _T("LOGTYPE"), eLDOPT_LOGTYPE, pConfig);
	m_nSizeLimit = GetPrivateProfileInt(pSection, _T("SIZE_LIMIT"), eLDOPT_SIZE_LIMIT, pConfig);
	nVal = GetPrivateProfileInt(pSection, _T("IS_PRINT_LEVEL"), eLDOPT_IS_PRINT_LEVEL, pConfig);
	_setLoggerBasic(nRes, nVal, m_nSizeLimit);

	nRes = GetPrivateProfileInt(pSection, _T("IS_PRINT"), eLDOPT_ISPRINTF, pConfig);
	if (nRes) m_nLogFlag |= IS_PRINT;
	m_nLogLevel = GetPrivateProfileInt(pSection, _T("LOGLEVEL"), eLDOPT_LOGLEVEL, pConfig);

	if (isLogFlag(IS_LOG_ENABLE)) return LOG_INIT_CHANGED_SUCCESS;

	setLogFlag(IS_LOG_SET_COMPLETE);
	return LOG_INIT_SUCCESS;

SET_DISABLE : 
	if (isLogFlag(IS_LOG_ENABLE)) {
		clearLogFlag(IS_LOG_ENABLE);
	}
	return eRes;
}

int CLogger::getString(TCHAR *pDest)
{
	int nLen;
	if (!isLogFlag(IS_LOG_SET_COMPLETE)) {
		_tcscpy(pDest, _T("Logger is not Initialized!"));
	}

	TCHAR szBuf[64];
	CDateTime cDateTime(DT_NONE);

	if (m_nLogFlag & LOG_FLAG_DAILY) {
		_tcscpy(szBuf, _T("TYPE_DAILY"));
	}
	else if (m_nLogFlag & LOG_FLAG_HOURLY) {
		_tcscpy(szBuf, _T("TYPE_HOURLY"));
	}
	else if (m_nLogFlag & LOG_FLAG_FILE) {
		_tcscpy(szBuf, _T("TYPE_FILE"));
	}

	nLen = _stprintf(pDest, _T("================================================\n"));
	nLen += _stprintf(pDest + nLen, _T("Logger Path[%s] Prefix[%s] Extension[%s]\n"), m_pLogDir, m_pLogName, m_szFileExtension);
	nLen += _stprintf(pDest + nLen, _T("Logger SizeLimit[%d] LogLevel[%d] LogType[%s]\n"), m_nSizeLimit, m_nLogLevel, szBuf);

	cDateTime.SetSecondsValue(m_nDeleteInterval);
	cDateTime.setString(szBuf, _T("KeepDuration : %02d days %02d:%02d:%02d"), LOG_DATE_DDHHMMSS);

	nLen += _stprintf(pDest + nLen, _T("Logger KeepDuration[%s]\n"), szBuf);
	nLen += _stprintf(pDest + nLen, _T("================================================\n"));
	return nLen;
}

bool CLogger::_initCom(TCHAR *pDir, TCHAR *pName, TCHAR *pFileExtension, TCHAR *pKeepDuration)
{
	CFileUtil::CheckDirName(pDir);

	size_t nStrLen = _tcslen(pDir);
	bool bSet = true;
	if (m_pLogDir) {
		if (_tcscmp(pDir, m_pLogDir)) {
			delete[] m_pLogDir;
		}
		else bSet = false;
	}

	if (bSet) {
		m_pLogDir = new TCHAR[nStrLen + 1];
		_tcscpy(m_pLogDir, pDir);
	}

	bSet = true;
	if (m_pLogName) {
		if (_tcscmp(pDir, m_pLogName)) {
			delete[] m_pLogName;
		}
		else bSet = false;
	}

	if (bSet) {
		nStrLen = _tcslen(pName);
		m_pLogName = new TCHAR[nStrLen + 1];
		_tcscpy(m_pLogName, pName);
	}

	if (_tcscmp(pFileExtension, m_szFileExtension)) {
		_tcscpy(m_szFileExtension, pFileExtension);
	}

	if (pKeepDuration[0]) {
		if (GetKeepInfo(pKeepDuration)) {
			m_pLogFileListForDelete->root = gs_pMMgr->newString(m_pLogDir);
			return true;
		} 
		else return false;
	}
	return true;
}
bool CLogger::_setLoggerQueue(int nMax)
{
	int nQueueMax = m_sLogQueue.capacity();
	if (!nQueueMax) { // empty
		if (!m_sLogQueue.alloc(nMax, eAlloc_Type_BufPool)) {
			_stprintf(g_szMessage, _T("m_sLogQueue.alloc(20, eAlloc_Type_BufPool) has Failed"));
			comErrorPrint(g_szMessage);
			return false;
		}
	}
	else if (nQueueMax < nMax) {
		if (!m_sLogQueue.realloc(nMax, true)) {
			_stprintf(g_szMessage, _T("m_sLogQueue.realloc(%d,true) has Failed"), nMax);
			comErrorPrint(g_szMessage);
			return false;
		}
	}
	return true;
}
void CLogger::_setLoggerBasic(int nLogType, int nPrintLevel, int nSizeLimit)
{
	int nFlag = SAVE_FULLPATH;
	switch (nLogType){
		case LOG_TYPE_DAILY: {
			m_nLogFlag |= LOG_FLAG_DAILY; m_nLogFlag &= ~(LOG_FLAG_HOURLY | LOG_FLAG_FILE);
		} break;
		case LOG_TYPE_HOURLY: {
			m_nLogFlag |= LOG_FLAG_HOURLY; m_nLogFlag &= ~(LOG_FLAG_DAILY | LOG_FLAG_FILE);
			nFlag |= (HAS_DIRNAME | DIR_RECURSIVE);
		}  break;
		case LOG_TYPE_FILE: {
			m_nLogFlag |= LOG_FLAG_FILE; m_nLogFlag &= ~(LOG_FLAG_DAILY | LOG_FLAG_HOURLY);
		} break;
	}
	if (nPrintLevel) m_nLogFlag |= IS_PRINT_LEVEL;
	else m_nLogFlag &= ~IS_PRINT_LEVEL;

	if (nSizeLimit) m_nLogFlag |= LOG_FLAG_SIZEOVER;
	else m_nLogFlag &= ~LOG_FLAG_SIZEOVER;
	m_nSizeLimit = nSizeLimit;

	if (m_pLogFileListForDelete) {
		m_pLogFileListForDelete->nFlag = nFlag;
	}
}
bool CLogger::_setHeaderFormat(int nTimeStempNo)
{
	if (nTimeStempNo < 0) {
		m_pDTE = NULL;
		m_szHeaderFormat[0] = 0;
		return true;
	}

	STLogDateTimeExp *pDTE = gs_DTEManager.getDTE(nTimeStempNo);
	if (!pDTE) {
		_stprintf(g_szMessage, _T("m_pDTE is NULL! nTimeStempNo[%d]"), nTimeStempNo);
		comErrorPrint(g_szMessage);
		return false;
	}
	else if (m_pDTE != pDTE) {
		setDateTimeFormat(pDTE);
		m_pDTE = pDTE;
	}
	return true;
}

void CLogger::OpenLogger()
{
	TCHAR szBuf[128];
	if (m_nLogFlag & LOG_FLAG_DAILY) {
		_stprintf(szBuf, _T("%s%d_%s.%s"), m_pLogDir, gs_today_time.date, m_pLogName, m_szFileExtension);
	}
	else if (m_nLogFlag & LOG_FLAG_HOURLY) {
		_stprintf(szBuf, _T("%s%d%c%s_%02d.%s"), m_pLogDir, gs_today_time.date, g_s, m_pLogName, gs_today_time.hour, m_szFileExtension);
	}
	else if (m_nLogFlag & LOG_FLAG_FILE) {
		_stprintf(szBuf, _T("%s%s.%s"), m_pLogDir, m_pLogName, m_szFileExtension);
	}

	__openLogger(szBuf);
}

void CLogger::setDateTimeFormat(STLogDateTimeExp *pLogDTE)
{
	int nRes = 0;

	if (!pLogDTE) {
		m_szHeaderFormat[0] = 0;
		return;
	}

	if (pLogDTE->bBracketEnclosed) m_szHeaderFormat[nRes++] = _T('[');

	if (pLogDTE->nYearLen == 4) {
		_tcscpy(m_szHeaderFormat + nRes, _T("%04d")); nRes += 4;
		switch (pLogDTE->nLastType) {
		case DT_SEC: m_nDateTimeFormat = LOG_DATE_YYYYMMDDHHMMSS; break;
		case DT_MILLISEC: m_nDateTimeFormat = LOG_DATE_YYYYMMDDHHMMSS3; break;
		case DT_MICROSEC: m_nDateTimeFormat = LOG_DATE_YYYYMMDDHHMMSS6; break;
		case DT_NANOSEC: m_nDateTimeFormat = LOG_DATE_YYYYMMDDHHMMSS9; break;
		}
	}
	else if (pLogDTE->nYearLen == 2) {
		_tcscpy(m_szHeaderFormat + nRes, _T("%s")); nRes += 2;
		switch (pLogDTE->nLastType) {
		case DT_SEC: m_nDateTimeFormat = LOG_DATE_YYMMDDHHMMSS; break;
		case DT_MILLISEC: m_nDateTimeFormat = LOG_DATE_YYMMDDHHMMSS3; break;
		case DT_MICROSEC: m_nDateTimeFormat = LOG_DATE_YYMMDDHHMMSS6; break;
		case DT_NANOSEC: m_nDateTimeFormat = LOG_DATE_YYMMDDHHMMSS9; break;
		}
	}
	else {
		m_nDateTimeFormat = LOG_DATE_NONE;
		m_szHeaderFormat[0] = 0;
	}

	if (pLogDTE->cDateDelimeter) nRes += _stprintf(m_szHeaderFormat + nRes, "%c", pLogDTE->cDateDelimeter);
	_tcscpy(m_szHeaderFormat + nRes, _T("%02d")); nRes += 4;
	if (pLogDTE->cDateDelimeter) nRes += _stprintf(m_szHeaderFormat + nRes, "%c", pLogDTE->cDateDelimeter);
	_tcscpy(m_szHeaderFormat + nRes, _T("%02d ")); nRes += 5;

	if (pLogDTE->bTimeDelimeter) { _tcscpy(m_szHeaderFormat + nRes, _T("%02d:%02d:%02d")); nRes += 14; }
	else { _tcscpy(m_szHeaderFormat + nRes, _T("%02d%02d%02d")); nRes += 12; }

	switch (pLogDTE->nLastType) {
	case DT_MILLISEC: _tcscpy(m_szHeaderFormat + nRes, _T(".%03d")); nRes += 5; break;
	case DT_MICROSEC: _tcscpy(m_szHeaderFormat + nRes, _T(".%06d")); nRes += 5; break;
	case DT_NANOSEC: _tcscpy(m_szHeaderFormat + nRes, _T(".%09d")); nRes += 5; break;
	}

	if (pLogDTE->bBracketEnclosed) m_szHeaderFormat[nRes++] = _T(']');
	m_szHeaderFormat[nRes] = 0;
}


int CLogger::setHeader(TCHAR *pHeader, int nLevel)
{
	CDateTime	cDateTime(m_pDTE->nLastType, m_pDTE->nTZType);
	int nLen = 0;
	nLen = cDateTime.setString(pHeader, m_szHeaderFormat, (DT_FORMAT)m_nDateTimeFormat);
	if (m_pDTE->nTZType) {
		nLen += _stprintf(pHeader + nLen, _T("%s"), cDateTime.TimezoneString());
	}
	if (m_nLogFlag & IS_PRINT_LEVEL)
	{
		nLen += _stprintf(pHeader + nLen,_T(" %s"), gs_fmtStr[nLevel]);
	}
	return nLen;
}

void CLogger::__openLogger(const TCHAR *pFileName, int nSeq)
{
	// 기본적으로 현 파일 닫음
	bool bres;

	//m_cLock.enter();

	if (m_hFile) {
		clearLogFlag(IS_LOG_OPEN);
		fclose(m_hFile); m_hFile = NULL;
	}

	if (nSeq) {
		CFileUtil::renameFile(m_szFName, pFileName);
		pFileName = m_szFName;
	}

	bres = CFileUtil::MakeDirectory(pFileName);

	if (bres) {
		m_hFile = _tfopen(pFileName, _T("a+"));
		if (m_hFile == NULL) {
			_tprintf(_T("Can't create log file [%s]\n"), pFileName);
			//m_cLock.leave();
			return;
		}
		if (!nSeq) strcpy(m_szFName, pFileName);
		m_nLogSize = 0;
		setLogFlag(IS_LOG_OPEN);
	}
	//m_cLock.leave();
}

void CLogger::disable()
{
	clearLogFlag(IS_LOG_ENABLE);
	if (m_hFile) {
		clearLogFlag(IS_LOG_OPEN);
		fclose(m_hFile); m_hFile = NULL;
	}
}

void CLogger::LogPrint(TCHAR *pLog)
{
	if (m_fpDataProc) m_fpDataProc(pLog, m_pObj);
	if (m_hFile) {
		if (m_nLogFlag & IS_PRINT) _tprintf(pLog);
		if (isLogFlag(IS_LOG_OPEN)) {
			_ftprintf(m_hFile, pLog);
			fflush(m_hFile);
		}
		//else {
		//	//m_cLock.enter();
		//	_ftprintf(m_hFile, pLog);
		//	fflush(m_hFile);
		//	//m_cLock.leave();
		//}

	}
}

void CLogger::Log(int level, const char *body, const char *header, const char *tail, bool bToFile)
{
	if (!isLogFlag(IS_LOG_ENABLE)) return;
	if (level < m_nLogLevel) return;

	int headerLen, bodyLen=0, totlen=0;

	TCHAR    headerBuf[MAX_LOGBUF_LEN];
	TCHAR *pBuf;

	headerLen = setHeader(headerBuf, level);

	// calculate total length ----------------
	if (header) {
		_tcscpy(headerBuf + headerLen, header); headerLen += (int)_tcslen(header);;
	}
	bodyLen = (int)_tcslen(body);
	if (tail) { 
		totlen = headerLen + bodyLen + (int)_tcslen(tail);
		pBuf = (TCHAR *)gs_pMMgr->newBuf(totlen + 2);
		if (!pBuf) {
			printf("pBuf is NULL nSize[%d]\n", totlen + 2);
			return;
		}
		_tcscpy(pBuf, headerBuf);
		_tcscpy(pBuf + headerLen, body);
		_tcscpy(pBuf + headerLen + bodyLen, tail);
	}
	else {
		totlen = headerLen + bodyLen;
		pBuf = (TCHAR *)gs_pMMgr->newBuf(totlen + 2);
		if (!pBuf) {
			printf("pBuf is NULL nSize[%d]\n", totlen + 2);
			return;
		}
		_tcscpy(pBuf, headerBuf);
		_tcscpy(pBuf + headerLen, body);
	}
	// calcuate end -------------------------

	pBuf[totlen] = '\n';
	pBuf[totlen+1] = 0;
	if (bToFile) {
		LogPrint(pBuf); return;
	}
	putQueue(pBuf);
}

void CLogger::LogPrint(int level, const TCHAR *_szFormat, ...)
{
	if (!isLogFlag(IS_LOG_ENABLE)) return;
	if (level < m_nLogLevel) return;

//	char tmpBuf[512];
	int nLen;

	TCHAR	msgBuf[MAX_LOGBUF_LEN];
	TCHAR	header[512];
	va_list	ap;

	nLen = setHeader(header, level);
	sprintf(header + nLen, " %s ", _szFormat);
	va_start(ap, _szFormat);
	nLen = vsnprintf(msgBuf, MAX_LOGBUF_LEN - 1, header, ap);
	va_end(ap);

	TCHAR *pBuf = (TCHAR *)gs_pMMgr->newBuf(nLen + 2);
	if (!pBuf) {
		printf("pBuf is NULL nSize[%d]\n", nLen + 2);
		return;
	}
	_tcscpy(pBuf, msgBuf);
	pBuf[nLen] = '\n';
	pBuf[nLen + 1] = 0;

	LogPrint(pBuf);
}


void CLogger::WritePacket(int nLevel, int nKind, int nSocket, TCHAR *szBuffer, int nPacketSize)
{
	if (!isLogFlag(IS_LOG_ENABLE)) return;
	if (nLevel < m_nLogLevel) return;

	TCHAR header[LEN_LOG_HEADER];
	const TCHAR	*fmtStr[5] = {
		_T(" nSIdx[%d] Send [%d] PKIdx[%d] "),
		_T(" nSIdx[%d] Recv [%d] PKIdx[%d] "),
		_T(" nSIdx[%d] send Queue [%d] PKIdx[%d] "),
		_T(" nSIdx[%d] recv Queue [%d] PKIdx[%d] "),
		_T(" Undefind nkind[%d] [%d] PKIdx[%d] ")
	};

	if (nKind < 0 || nKind > 4) nKind = 4;

	int	i=0, nLCompare, nLen, nTotLen;
	nLen = setHeader(header, nLevel);


	nLen += _stprintf(header +nLen, fmtStr[nKind], nSocket, nPacketSize, nKind);
	nLCompare = (int)_tcslen(szBuffer);
	
	nTotLen = nLen + nLCompare;
	TCHAR *pBuf = (TCHAR *)gs_pMMgr->newBuf(nTotLen + 2);
	if (!pBuf) {
		printf("pBuf is NULL nSize[%d]\n", nLen + 2);
		return;
	}

	_tcscpy(pBuf, header);

	while (nPacketSize--) {
		_stprintf(pBuf+nLen, _T("%02x "), szBuffer[i] & 0x0ff);
		nLen += 3; i++;
	}
	pBuf[nTotLen] = '\n';
	pBuf[nTotLen + 1] = 0;

	putQueue(pBuf);
}


void CLogger::__debugLog(int level, const char *_szFunc, const int _nLine, const char *_szFormat, ...)
{
	if (!isLogFlag(IS_LOG_ENABLE)) return;
	if (level < m_nLogLevel) return;

//	TCHAR tmpBuf[512];
	int nLen, nLen2;

	TCHAR	msgBuf[MAX_LOGBUF_LEN];
	TCHAR	header[512];
	va_list	ap;

	nLen2 = setHeader(header, level);
	sprintf(header+nLen2, " %s, %d, %s ", _szFunc, _nLine, _szFormat);
	va_start(ap, _szFormat);
	nLen = vsnprintf(msgBuf, MAX_LOGBUF_LEN - 1, header, ap);
	va_end(ap);

	TCHAR *pBuf = (TCHAR *)gs_pMMgr->newBuf(nLen + 2);
	if (!pBuf) {
		printf("pBuf is NULL nSize[%d]\n", nLen + 2);
		return;
	}
	_tcscpy(pBuf, msgBuf);
	pBuf[nLen] = '\n';
	pBuf[nLen + 1] = 0;

	putQueue(pBuf);
}

//void CLogger::PutLogQueue(int level, const char *_szFormat, ...)
//{
//	TCHAR	msgBuf[MAX_LOGBUF_LEN];
//	va_list pArg;
//	va_start(pArg, _szFormat);
//	vsnprintf(msgBuf, MAX_LOGBUF_LEN, _szFormat, pArg);
//	vfprintf(m_hFile, _szFormat, pArg);
//	va_end(pArg);
//}


void CLogger::PutLogQueue(int level, const char *_szFormat, ...)
{
	if (!isLogFlag(IS_LOG_ENABLE)) return;
	if (level < m_nLogLevel) return;

	int nLen;

	TCHAR	msgBuf[MAX_LOGBUF_LEN];
	TCHAR	header[512];
	va_list	ap;

	nLen = setHeader(header, level);
	sprintf(header + nLen, " %s ", _szFormat);
	va_start(ap, _szFormat);
	nLen = _vsntprintf(msgBuf, MAX_LOGBUF_LEN - 1, header, ap);
	va_end(ap);

	TCHAR *pBuf = (TCHAR *)gs_pMMgr->newBuf(nLen + 2);
	if (!pBuf) {
		printf("pBuf is NULL nSize[%d]\n", nLen + 2);
		return;
	}
	_tcscpy(pBuf, msgBuf);
	pBuf[nLen] = '\n';
	pBuf[nLen + 1] = 0;
	putQueue(pBuf);
}

void CLogger::PutLogQueue(int level, TCHAR *pBody)
{
	if (!isLogFlag(IS_LOG_ENABLE)) return;
	if (level < m_nLogLevel) return;
	
	TCHAR	header[512];

	int nLen2	= setHeader(header, level);
	int nLen = (int)_tcslen(pBody) + nLen2;

	TCHAR *pBuf = (TCHAR *)gs_pMMgr->newBuf(nLen+2);
	if (!pBuf) {
		printf("pBuf is NULL nSize[%d]\n", nLen + 2);
		return;
	}
	_tcscpy(pBuf, header);
	_tcscpy(pBuf + nLen2, pBody);
	pBuf[nLen] = '\n';
	pBuf[nLen + 1] = 0;
	putQueue(pBuf);
}

void CLogger::PutLogQueue(TCHAR *pData)
{
	if (!isLogFlag(IS_LOG_ENABLE)) return;

	int nLen = (int)_tcslen(pData);

	TCHAR *pBuf = (TCHAR *)gs_pMMgr->newBuf(nLen + 1);
	if (!pBuf) {
		printf("pBuf is NULL nSize[%d]\n", nLen + 1);
		return;
	}
	_tcscpy(pBuf, pData);
	pBuf[nLen + 1] = 0;
	putQueue(pBuf);
}

void CLogger::checkChangeProc(int nFlag) // at least min changed
{
	TCHAR szFile[512];


	if (nFlag & LOG_FLAG_DAYCHANGE) {
		if (m_nLogFlag & LOG_FLAG_DAILY) { 
			_stprintf(szFile, _T("%s%d_%s.%s"), m_pLogDir,  gs_today_time.date, m_pLogName, m_szFileExtension);
			__openLogger(szFile);
			m_nSeq = 0; 
			return;
		}
	}
	if (nFlag & LOG_FLAG_HOURCHANGE) {
		if (m_nLogFlag & LOG_FLAG_HOURLY) {
			_stprintf(szFile, _T("%s%d%c%s_%02d.%s"), m_pLogDir, gs_today_time.date, g_s, m_pLogName, gs_today_time.hour, m_szFileExtension);
			__openLogger(szFile);
			m_nSeq = 0; 
			return;
		}
	}
	if(m_nSizeLimit) {
		if (m_nSizeLimit < m_nLogSize) {
			procFileSizeOver();
		}
	}

	// LOG_FLAG_FILE
	// no need to open a new file


	// delete proc
	if (m_nDeleteInterval) checkDeleteProc();

}

void CLogger::procFileSizeOver()
{
	TCHAR szFile[128];
	m_nSeq++;
	if (m_nLogFlag & LOG_FLAG_DAILY) {
		_stprintf(szFile, _T("%s%d_%s_%d.%s"), m_pLogDir, gs_today_time.date, m_pLogName, m_nSeq, m_szFileExtension);
	}
	else if (m_nLogFlag & LOG_FLAG_HOURLY) {
		_stprintf(szFile, _T("%s%d%c%s_%02d_%d.%s"), m_pLogDir, gs_today_time.date, g_s, m_pLogName, gs_today_time.hour, m_nSeq,m_szFileExtension);
	}
	else if (m_nLogFlag & LOG_FLAG_FILE) {
		_stprintf(szFile, _T("%s%s_%d.%s"), m_pLogDir, m_pLogName, m_nSeq, m_szFileExtension);
	}
	__openLogger(szFile, m_nSeq);
}

void CLogger::checkDeleteProc()
{
	time_t t, basic;
	CFileTime cFileTime;
	STFilterData stFilter;

	stFilter.eOP = eOperator_NSUBSET;
	stFilter.pData = m_szFName; // current log file full path & file name ;;; current directory, current file exclude
	stFilter.pFunc = filter_stringCompare;
	if (!m_pLogFileListForDelete->pList) {
		if (!CFileUtil::GetFileList(m_pLogFileListForDelete, &stFilter)) return;
	}

	t = time(NULL);
	basic = t - m_nDeleteInterval;
	cFileTime.setFileTime(&basic);
	stFilter.eOP = eOperator_LE;
	stFilter.pData = &cFileTime;
	stFilter.pFunc = filter_dateTimeCompare;
	CFileUtil::delDirectory(m_pLogFileListForDelete, &stFilter);

}

void CLogger::setLogName(TCHAR *pTarget)
{
	TCHAR szBuf[128];
	if (m_nLogFlag & LOG_FLAG_DAILY) {
		_stprintf(szBuf, _T("%s%d_%s"), m_pLogDir, gs_today_time.date, m_pLogName);
	}
	else if (m_nLogFlag & LOG_FLAG_HOURLY) {
		_stprintf(szBuf, _T("%s%d%c%s_%02d.%s"), m_pLogDir, gs_today_time.date, g_s, m_pLogName, gs_today_time.hour, m_szFileExtension);
	}
	else if (m_nLogFlag & LOG_FLAG_FILE) {
		_stprintf(szBuf, _T("%s%s"), m_pLogDir, m_pLogName);
	}
}


bool CLogger::GetKeepInfo(const char *pValue)
{
	char szBuf[32];
	char chKeepFileUnit;
	int nStrLen;
	int nUnitSecond, nNum;

	strcpy(szBuf, pValue);
	nStrLen = (int)strlen(szBuf);
	if (nStrLen < 2)
	{
		comErrorPrint("GetKeepInfo nStrLen < 2");
		return false;
	}

	nStrLen = nStrLen - 1;
	chKeepFileUnit = szBuf[nStrLen];
	szBuf[nStrLen] = '\0';

	int i;
	for (i = 0; i < nStrLen; i++) {
		if ((szBuf[i] < '0') || (szBuf[i] > '9')) {
			sprintf(g_szMessage, "pValue:%s", pValue);
			comErrorPrint(g_szMessage);
			return false;
		}
	}

	nNum = atoi(szBuf);

	if (nNum < 0) {
		sprintf(g_szMessage, "pValue:%s", pValue);
		comErrorPrint(g_szMessage);
		return false;
	}

	switch (chKeepFileUnit) {
	case 's': case 'S': nUnitSecond = 1; break;
	case 'm': case 'M': nUnitSecond = 60; break;
	case 'h': case 'H': nUnitSecond = 60 * 60; break;
	case 'd': case 'D': nUnitSecond = 60 * 60 * 24; break;
	default: {
			sprintf(g_szMessage, "pValue:%s", pValue);
			comErrorPrint(g_szMessage);
			return false;
		}
	}
	m_pLogFileListForDelete = (STFileList *)calloc(1, sizeof(STFileList));
	m_nDeleteInterval = nNum * nUnitSecond;
	return true;
}

// path : ~/path1/path2/aaa.log
// result : /root/home/exceutionDir/path1/path2/aaa.log
void changeToAbsolutePathFile(TCHAR *ret, const TCHAR *path)
{
	TCHAR szBuf[1024];
	TCHAR *pBuf = (TCHAR *)szBuf;
	_tcscpy(szBuf, path);
	int nRes = 0;
	CEnv::initSystem();
	if (pBuf[0] == '~') {
		pBuf++;
		while (*pBuf == '\\' || *pBuf == '/') {
			*pBuf = g_s;
			pBuf++;
		}
		nRes = _stprintf(ret, _T("%s%s"), g_pWorkDir, pBuf);
		ret[nRes] = 0;
	}
}

// path : ~/path1/path2
// ret : /root/home/exceutionDir/path1/path2/
void changeToAbsolutePath(TCHAR *ret, const TCHAR *path)
{
	TCHAR szBuf[1024];
	TCHAR *pBuf = (TCHAR *)szBuf;
	_tcscpy(szBuf, path);
	int nRes=0;
	CEnv::initSystem();
	if (pBuf[0] == '~') {
		pBuf++;
		while (*pBuf == '\\' || *pBuf == '/') {
			*pBuf = g_s;
			pBuf++;
		}
		nRes = _stprintf(ret, _T("%s%s"), g_pWorkDir, pBuf);
		pBuf = ret+(nRes - 1);
		if(*pBuf == '\\' || *pBuf == '/') *pBuf = g_s;
		else { pBuf++; *pBuf = g_s;	}
		*(pBuf + 1) = 0;
	}
}

// fName : configFilePath
// ret : /root/dir/path1/path2/
int	GetProfilePath(const TCHAR *section, const TCHAR *key, const TCHAR * initVal, TCHAR *ret, const TCHAR *fName)
{
	TCHAR szDir[256];
	int nRes = GetPrivateProfileString(section, key, _T(""), szDir, sizeof(szDir), fName);

	if (!nRes) {
		if (initVal) {
			nRes = _stprintf(ret, "%s%s%c", g_pWorkDir, initVal, g_s);
		}
		else {
			nRes = _stprintf(ret, "%s%c", g_pWorkDir, g_s);
		}
	}
	else if (szDir[0] == '~') {
		changeToAbsolutePath(ret, szDir);
	}
	return nRes;
}


bool detachFullPath(const TCHAR *fullpath, TCHAR *dir, TCHAR *name, TCHAR *Extension)
{
	size_t nLen;
	TCHAR *pFullPath, *pBuf;
	TCHAR *pRear;

	if (fullpath[0] == '~') {
		TCHAR szDir[256];
		changeToAbsolutePathFile(szDir, fullpath);
		pFullPath = szDir;
	}
	else {
		pFullPath = (TCHAR *)fullpath;
		CFileUtil::CheckDirName(pFullPath);
	}


	// detach Directory
	pRear = _tcsrchr(pFullPath, g_s);
	if (pRear) {
		nLen = pRear - pFullPath + 1;
		if (nLen) {
			_tcsncpy(dir, pFullPath, nLen);
			dir[nLen] = 0;
		}
		else goto DETACHPAHT_ERROR;
	}
	else goto DETACHPAHT_ERROR;


	pBuf = pRear + 1; // start file name position
	if (pBuf) {
		pRear = _tcsrchr(pFullPath, '.');
		nLen = 0;
		if (pRear) {
			*pRear = 0;
			pRear++;
			nLen = strlen(pRear);			
		}

		if (nLen) _tcscpy(Extension, pRear);
		else _tcscpy(Extension, g_szLogExtension);
		
		nLen = strlen(pBuf);
		if (nLen) _tcscpy(name, pBuf);
		else goto DETACHPAHT_ERROR;
		
	}

	return true;

DETACHPAHT_ERROR:
	sprintf(g_szMessage, "fullpath[%s] is not correct", fullpath);
	comErrorPrint(g_szMessage);
	return false;
}



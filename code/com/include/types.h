/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2010 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: unicode.h
**
**/
 
#ifndef __types___
#define __types___

// Undef UNICODE_UCS2 and UNICODE_UCS4 if they are defined to 0
#if !UNICODE_UCS2
#undef UNICODE_UCS2
#endif
#if !UNICODE_UCS4
#undef UNICODE_UCS4
#endif
 
#if defined(WIN32)
 
// Ensure that both UNICODE and _UNICODE are defined

#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE
#endif
#endif
 
#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#endif
   

 
#ifdef UNICODE
#include <tchar.h>
 
 // Windows always use UCS-2
#define UNICODE_UCS2				1
 
#define ICONV_DEFAULT_CODEPAGE    "ACP"

#define _tcstoll  wcstoll
#define _tcstoull wcstoull
 
#define _ERR_error_tstring		ERR_error_string_W
 
#else	/* !UNICODE */  // multibytes & windows
#include <tchar.h>
#define _tcstoll  strtoll
#define _tcstoull strtoull
 
#define _ERR_error_tstring		ERR_error_string


#endif	/* UNICODE */
 
#define ucs2_tcslen		wcslen
#define ucs2_strdup		wcsdup
#define ucs2_strncpy	wcsncpy
 
#define ucs2_to_mb(wstr, wlen, mstr, mlen)	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, wstr, wlen, mstr, mlen, NULL, NULL)
#define mb_to_ucs2(mstr, mlen, wstr, wlen)	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mstr, mlen, wstr, wlen)
 


#elif defined(__SYMBIAN32__)
 
// Ensure that both UNICODE and _UNICODE are defined

#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE
#endif
#endif
 
#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#endif

// Symbian always use UCS-2
#ifdef UNICODE
#define UNICODE_UCS2				1
#endif 

#define TCHAR     TText
#define WCHAR     TText16
#define UCS2CHAR  TText16
 
#ifdef UNICODE
#define _T(x)     L##x
#else
#define _T(x)     x
#endif
 
#else    /* not _WIN32 and __SYMBIAN32__ */
 
#if defined(_NETWARE) && defined(__GNUC__) && defined(__cplusplus)
#define _WCHAR_T
#define _WCHAR_T_DEFINED
#define wchar_t unsigned short
#endif
 
 
#if HAVE_WCHAR_H
#include <wchar.h>
#endif
 
#if HAVE_STRING_H
#include <string.h>
#endif
 
#if HAVE_WCHAR_T

#define WCHAR     wchar_t
#if UNICODE_UCS2
#define UCS2CHAR  wchar_t
#else
#define UCS2CHAR  unsigned short
#endif
 
 #else	/* wchar_t not presented */
 
#define WCHAR     unsigned short
#define UCS2CHAR  unsigned short
#undef UNICODE_UCS2
#undef UNICODE_UCS4
#define UNICODE_UCS2 1
 
#endif
 
// Use system wide character functions if system's wchar_t is 2 bytes long
#if UNICODE_UCS2
 
#if HAVE_WCSLEN
#define ucs2_tcslen	wcslen
#else
#define wcslen          ucs2_tcslen
#endif
 
#if HAVE_WCSDUP
#define ucs2_strdup	wcsdup
#else
#define wcsdup          ucs2_strdup
#endif
 
#if HAVE_WCSNCPY
#define ucs2_strncpy	wcsncpy
#else
#define wcsncpy         ucs2_strncpy
#endif
 
#endif

// On some old systems, ctype.h defines _T macro, so we include it
// before our definition and do an undef
#include <ctype.h>
#undef _T
 
#ifdef UNICODE
 
#define _T(x)     L##x
#define TCHAR     wchar_t
#define _TINT     int
 
#define _tcscpy   wcscpy
#define _tcsncpy  wcsncpy
#define _tcslen   wcslen
#define _tcsnlen  wcsnlen
#define _tcschr   wcschr
#define _tcsrchr  wcsrchr
#define _tcscmp   wcscmp
#define _tcsicmp  wcsicmp
#define _tcsncmp  wcsncmp
#define _tcsnicmp wcsnicmp
#define _tprintf  nx_wprintf
#define _ftprintf nx_fwprintf
#define _sntprintf nx_swprintf
#define _vtprintf nx_vwprintf
#define _vftprintf nx_vfwprintf
#define _vsntprintf nx_vswprintf
#define _tscanf  nx_wscanf
#define _ftscanf nx_fwscanf
#define _stscanf nx_swscanf
#define _vtscanf nx_vwscanf
#define _vftscanf nx_vfwscanf
#define _vstscanf nx_vswscanf
#define _tfopen   wfopen
#define _tfopen64 wfopen64
#define _tpopen   wpopen
#define _fgetts   fgetws
#define _fputts   fputws
#define _putts    putws
#define _puttc    putwc
#define _tgetenv  _wgetenv
#define _tputenv  _wputenv
#define _tcstol   wcstol
#define _tcstoul  wcstoul
#define _tcstoll  wcstoll
#define _tcstoull wcstoull
#define _tcstod   wcstod
#define _tcsdup   wcsdup
#define _tcsupr   wcsupr
#define _tcsspn   wcsspn
#define _tcscspn  wcscspn
#define _tcsstr   wcsstr
#define _tcscat   wcscat
#define _tcscat_s wcscat_s
#define _tcsncat  wcsncat
#define _tcspbrk  wcspbrk
#define _tcstok   wcstok
#define _topen    wopen
#define _taccess  waccess
#define _tstat    wstat
#define _tchmod   wchmod
#define _tunlink  wunlink
#define _trename  wrename
#define _tremove  wremove
#define _tcsftime wcsftime
#define _tctime   wctime
#define _istspace iswspace
#define _istdigit iswdigit
#define _istxdigit iswxdigit
#define _istalpha iswalpha
#define _istupper iswupper
#define _istprint iswprint
#define _tgetenv  wgetenv
#define _tmkdir   wmkdir
#define _tchdir   wchdir
#define _trmdir   wrmdir
#define _tcserror wcserror
#define _tcserror_r wcserror_r
#define _tsystem  wsystem
#define _topendir wopendir
#define _treaddir wreaddir
#define _tclosedir wclosedir
#define _tmkstemp wmkstemp
#define _ERR_error_tstring ERR_error_string_W
 
#define _TDIR     DIRW
#define _tdirent  dirent_w
 
#else

 
#define _T(x)     x
#define TCHAR     char
#define _TINT     int
 
#define _tcscpy   strcpy
#define _tcsncpy  strncpy
#define _tcslen   strlen
#define _tcsnlen  strnlen
#define _tcschr   strchr
#define _tcsrchr  strrchr
#define _tcscmp   strcmp
#define _tcsicmp  stricmp
#define _tcsncmp  strncmp
#define _tcsnicmp strnicmp
#define _tprintf  printf
#define _stprintf sprintf
#define _ftprintf fprintf
#define _sntprintf snprintf
#define _vtprintf vprintf
#define _vstprintf vsprintf
#define _vsntprintf vsnprintf
#define _tscanf   scanf
#define _ftscanf  fscanf
#define _stscanf  sscanf
#define _vtscanf  vscanf
#define _vftscanf vfscanf
#define _vstscanf vsscanf
#define _tfopen   fopen
#define _tfopen64 fopen64
#define _tpopen   popen
#define _fgetts   fgets
#define _fputts   fputs
#define _putts    puts
#define _puttc    putc
#define _tgetenv  getenv
#define _tputenv  putenv
#define _tcstol   strtol
#define _tcstoul  strtoul
#define _tcstoll  strtoll
#define _tcstoull strtoull
#define _tcstod   strtod
#define _tcsdup   strdup
#define _tcsupr   strupr
#define _tcsspn   strspn
#define _tcscspn  strcspn
#define _tcsstr   strstr
#define _tcscat   strcat
#define _tcscat_s strcat_s
#define _tcsncat  strncat
#define _tcspbrk  strpbrk
#define _tcstok   strtok
#define _topen    open
#define _taccess  access
#define _tstat    stat
#define _tchmod   chmod
#define _tunlink  unlink
#define _trename  rename
#define _tremove  remove
#define _tcsftime strftime
#define _tctime   ctime
#define _istspace isspace
#define _istdigit isdigit
#define _istxdigit isxdigit
#define _istalpha isalpha
#define _istupper isupper
#define _istprint isprint
#define _tgetenv  getenv
#define _tmkdir   mkdir
#define _tchdir   chdir
#define _trmdir   rmdir
#define _tcserror strerror
#define _tcserror_r strerror_r
#define _tsystem  system
#define _topendir opendir
#define _treaddir readdir
#define _tclosedir closedir
#define _ttoi		atoi
#define _tioa		itoa
#define _ttol		atol
#define _tloa		ltoa


#define _tmkstemp mkstemp
#define _ERR_error_tstring ERR_error_string
 
#define _TDIR     DIR
#define _tdirent  dirent
 
#endif
 
#define CP_ACP             0
#define CP_UTF8				65001
#define MB_PRECOMPOSED     0x00000001
#define WC_COMPOSITECHECK  0x00000002
#define WC_DEFAULTCHAR     0x00000004
 
// Default codepage for iconv()
#define ICONV_DEFAULT_CODEPAGE "ASCII"


#endif	/* _WIN32 */
 
// Define UCS2-specific helper functions as define to wide string helper functions if possible
#if UNICODE_UCS2
#define UCS2StringFromMBString	WideStringFromMBString
#define MBStringFromUCS2String	MBStringFromWideString
#endif
 
 
#ifdef UNICODE
#define _t_inet_addr    inet_addr_w
#else
#define _t_inet_addr    inet_addr
#endif
 
 
// Check that either UNICODE_UCS2 or UNICODE_UCS4 are defined
//#if !defined(UNICODE_UCS2) && !defined(UNICODE_UCS4)
//#error Neither UNICODE_UCS2 nor UNICODE_UCS4 are defined
//#endif

enum E_OPERATOR
{
	eOperator_NONE = 0,
	eOperator_EQ,
	eOperator_NE,
	eOperator_GT,
	eOperator_LT,
	eOperator_GE,
	eOperator_LE,
	eOperator_MAX
};



#endif   /* __types___ */



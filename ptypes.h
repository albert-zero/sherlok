// -----------------------------------------------------------------
//
// Author: Albert Rossmann
// File  : ptypes.h
// Date  : 27.08.2008
// Abstract:
//    compile with SAPon<platform> using sap C++ runtime sapcpp47.dll
//    compile with PROFILE_CPP using sap C++ runtime + thrlib

// Copyright (C) 2015  Albert Zedlitz
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// -----------------------------------------------------------------
#ifndef PTYPES_H
#define PTYPES_H

#include "jni.h"
#include "jvmti.h"

// standard includes
#include <sys/types.h>
#include <assert.h>
#include <ctype.h> 
#include <time.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <wchar.h>
#include <algorithm>
using namespace std;

// different systems
#if defined   (SAPonNT)
#  define _WINDOWS
#endif

#if defined (_WINDOWS)
#  pragma warning( disable : 4311 4312 )
#  include <Ws2tcpip.h>
// #  include <Windows.h>
#  include <fstream>
#  include <iostream>
#  include <windows.h>
#  include <winbase.h>
#  include <io.h>
#  include <direct.h>

#  define GET_SYSTIME(p) GetSystemTime(p)
#  define CLOCK()        clock()
#  define CLOSESOCKET(s) closesocket(s)
#  define FILESEPARATOR cU('\\')
#  define SLEEP(n) Sleep(n * 1000);
   typedef int socklen_t;
#  define SAPSOCKLEN_T socklen_t
#  define USE_SECURE_STR
#  define ACCESS(x,y)      _access    ((x),(y))

#else
#  include <sys/times.h>
#  include <netdb.h>
#  include <unistd.h>
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <signal.h>
#  include <pthread.h>
#  include <sys/socket.h>
#  include <dirent.h>

#  define GET_SYSTIME(p)
#  define CLOCK()        clock()
#  define DWORD          unsigned long
#  define WORD           unsigned short
#  define LPVOID         void*
#  define FILESEPARATOR cU('/')
#  define SLEEP(n) sleep(n);
   typedef int SOCKET;
/* #  define NAME_MAX _POSIX_NAME_MAX */
#  define ACCESS(x,y)    access    ((x),(y))
#  define CLOSESOCKET(s) ::shutdown(s, 2)
#endif

#if defined   (SAPonNT)
#  include "saptype.h"
#  include "sapuc.h"
#  include "sapuc2.h"
#  include "sapthr.h"
#  include "dlxx.h"
#  include "sapfstream.hpp"
#  include "sapstring.hpp"
#  include "sapsstream.hpp"
#  include "sapiostrm.hpp"
#  include "sapsstream.hpp"
#  define SAPSOCKLEN_T socklen_t

#elif defined (SAPonLIN)
#  include "saptype.h"
#  include "sapuc.h"
#  include "sapuc2.h"
#  include "sapthr.h"
#  include "dlxx.h"
#  include "sapfstream.hpp"
#  include "sapstring.hpp"
#  include "sapsstream.hpp"
#  include "sapiostrm.hpp"
#  include "sapsstream.hpp"
#  include <sys/dir.h>
#  include <sys/time.h>
#  define SAPSOCKLEN_T socklen_t

#elif defined (SAPonRS6000)
#  include "saptype.h"
#  include "sapuc.h"
#  include "sapuc2.h"
#  include "sapthr.h"
#  include "dlxx.h"
#  include "sapfstream.hpp"
#  include "sapstring.hpp"
#  include "sapsstream.hpp"
#  include "sapiostrm.hpp"
#  include "sapsstream.hpp"
#  include <string.h>
#  include <limits.h>
#  include <sys/types.h>
#  include <sys/dir.h>
#  define SAPSOCKLEN_T socklen_t

#elif defined (SAPonHPPA)
#  include "saptype.h"
#  include "sapuc.h"
#  include "sapuc2.h"
#  include "sapthr.h"
#  include "dlxx.h"
#  include "sapfstream.hpp"
#  include "sapstring.hpp"
#  include "sapsstream.hpp"
#  include "sapiostrm.hpp"
#  include "sapsstream.hpp"
#  include <sys/dirent.h>
#  include <sys/types.h>
   typedef int SOCKET;
#  define abs(x) (x < 0 ? -x : x)

#  define SAPSOCKLEN_T int

#elif defined (SAPonSUN)
#  include "saptype.h"
#  include "sapuc.h"
#  include "sapuc2.h"
#  include "sapthr.h"
#  include "dlxx.h"
#  include "sapfstream.hpp"
#  include "sapstring.hpp"
#  include "sapsstream.hpp"
#  include "sapiostrm.hpp"
#  include "sapsstream.hpp"
#  include <limits.h>
#  include <sys/types.h>
   typedef int SOCKET;
#  define SAPSOCKLEN_T socklen_t

#else

#   define WCHAR            wchar_t
#   define cU_HELP(par)     L##par
#   define cU(s)            cU_HELP(s)
#   define cR(s)            s
#   define SAP_UC           WCHAR

#   define mainU            main
#   define SAP_BOOL         bool
#   define SAP_INT          int
#   define SAP_CHAR         char
#   define SAP_RAW          jbyte
#   define SAP_UINT         unsigned int
#   define SAPRETURN        int
#   define SAP_A7           char
#   define SAP_UTF16        char

#   define sizeofR          sizeof

#   define strcpyU          wcscpy
#   define strcpyR          strcpy

#   define strncpyU         wcsncpy
#   define strncpyR         strncpy

#   define strcatU          wcscat
#   define strcatR          strcat

#   define strncatU         wcsncat
#   define strncatR         strncat

#   define strcmpU          wcscmp

#   define strncmpU         wcsncmp

#   define strlenU          wcslen
#   define strlenR          strlen

#   define SAP_cout         wcout
#   define SAP_cerr         wcerr

#   define A7sLen(a)        strlen(a)


#   define memsetR          memset
#   define memsetU          wmemset

#   define memcpyR          memcpy

#   define getcwdU          _wgetcwd
#   define chdirU           _wchdir

#   define SAP_ostringstream wostringstream
#   define SAP_stringstream  wstringstream
#   define SAP_ifstream      wifstream
#   define SAP_ofstream      wofstream
#   define BEGIN_externC extern "C" {
#   define END_externC   }

#ifdef USE_SECURE_STR
#    define strcpy_sU(x,l,y)       wcscpy_s((x),(l),(y))
#    define strncpy_sU(x,l,y,z)    wcsncpy_s((x),(l),(y),(z))

#    define strcat_sU(x,l,y)       wcscat_s((x),(l),(y)) 
#    define strncat_sU(x,l,y,z)    wcsncat_s((x),(l),(y),(z))
#else
#    define strcpy_sU(x,l,y)       wcscpy((x),(y))
#    define strncpy_sU(x,l,y,z)    wcsncpy((x),(y),(z))
#    define strcat_sU(x,l,y)       wcscat((x),(y)) 
#    define strncat_sU(x,l,y,z)    wcsncat((x),(y),(z))
#endif

#endif

#define strcmpR             strcmp
#define strncmpR            strncmp

#define STRCPY(x,y,l)       strcpy_sU(x,l,y)
#define STRNCPY(x,y,z,l)    strncpy_sU(x,l,y,z)
#define STRCAT(x,y,l)       strcat_sU(x,l,y) 
#define STRNCAT(x,y,z,l)    strncat_sU(x,l,y,z)
#define STRLEN(x)           (int)strlenU(x)
#define STRCMP              strcmpU
#define STRNCMP             strncmpU
#define STRLEN_A7(x)        (int)strlenR ((x))
#define SAP_BOOL            bool
#define ERROR_OUT(s, e)  /*SAPUNICODEOK_STRINGCONST*/ \
                 SAP_cerr << cU("-sherlok message- ") \
                          << cU(" file:")  \
                          << __FILE__      \
                          << cU(" line:")  \
                          << __LINE__      \
                          << cU(" code:")  \
                          << (int)e        \
                          << cU(" ") << s  \
                          << endl 

#define ERROR_STR      /*SAPUNICODEOK_STRINGCONST*/ \
    SAP_cerr << cU("-sherlok message- ") \
    << cU(" file:")  \
    << __FILE__      \
    << cU(" line:")  \
    << __LINE__      

#if   (JVMTI_VERSION | 0x33)==0x33
#   define JVMTInterface    jvmtiInterface_1
#elif (JVMTI_VERSION | 0x37)==0x37
#   define JVMTInterface    jvmtiNativeInterface
#else
#   define JVMTInterface    jvmtiNativeInterface
#endif

// ----------------------------------------------------------------
// error output
// ----------------------------------------------------------------
#ifdef _DEBUG
#  define ASSERT(expr) assert(expr)
#  define CHECK_POINTER(x) /* -- */
    //#ifdef SAPonNT
    //#define CHECK_POINTER(x) (assert(reinterpret_cast<unsigned long>(x) < 0xCDCDCDCD))
    //#else
    //#endif
#else
#  define ASSERT(expr)     /* -- */
#  define CHECK_POINTER(x) /* -- */
#endif

#if !defined(DECL_EXP)
# if defined(SAPonLIN) && defined(__GNUC__) && defined(GCC_HIDDEN_VISIBILITY)
#   define DECL_EXP __attribute__((visibility("default")))
# else
#   define DECL_EXP
# endif
#endif

// Used to caculate consistent timestamp on all platforms
#define OFFSET_1600 11644473573578LL

#endif


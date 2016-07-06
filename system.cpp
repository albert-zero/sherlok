// -----------------------------------------------------------------
//
// Author: Albert Rossmann
// File  : system
// Date  : 14.04.2003
// Abstract:
//    System implementation
// 
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
// Java native interface
#include "ccqcovrun.h"
#include "ptypes.h"
#include "standard.h"
#include "extended.h"
#include "console.h"
#include "tracer.h"
#include "profiler.h"
#include "monitor.h"
#include "javapi.h"

// ----------------------------------------------------------------
// TSystem::getTimestamp
// ----------------------------------------------------------------
jlong TSystem::getTimestamp() {
#   ifdef _WINDOWS
        if (mHasHpcTimer) {
#           if defined(USE_RDTSC) && defined(_X86_)
                LARGE_INTEGER *aPtr = &mHpcTime;
                __asm MOV   EBX, aPtr
                __asm RDTSC          
                __asm MOV   [EBX]LARGE_INTEGER.LowPart,  EAX
                __asm MOV   [EBX]LARGE_INTEGER.HighPart, EDX
#           else
                QueryPerformanceCounter(&mHpcTime);                
#           endif
            mOffset = calculateOffset(false);
            return (jlong)floor(mHpcTime.QuadPart * mScale) + mOffset;
        }
        else {
            // The FILETIME represents the time in units of "10**-7 sec" since 1/1/1601
            // Conversion to UNIX timestamps: OFFSET_1600 = 1/1/1970 - 1/1/1601
            FILETIME      tFt;
            LARGE_INTEGER aFt;
            GetSystemTimeAsFileTime(&tFt); 
            
            aFt.HighPart = tFt.dwHighDateTime;
            aFt.LowPart  = tFt.dwLowDateTime;

            jlong  aTime = ((*(jlong*)&aFt) / 10000LL) - OFFSET_1600;
            return aTime;
        }
#   else
        // gettimeofday returns the time in units of "10**-6 sec" stating from 1/1/1970
        // The update of the timestamp is hardware dependant
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (jlong)floor((double)(1000000LL * tv.tv_sec + tv.tv_usec)/(double)1000);
#   endif
};
// ----------------------------------------------------------------
// TSystem::getTimestamp
// ----------------------------------------------------------------
jlong TSystem::getTimestampHp() {
#   ifdef _WINDOWS
        if (mHasHpcTimer) {
#           if defined(USE_RDTSC) && defined(_X86_)
                LARGE_INTEGER *aPtr = &mHpcTime;
                __asm MOV   EBX, aPtr
                __asm RDTSC          
                __asm MOV   [EBX]LARGE_INTEGER.LowPart,  EAX
                __asm MOV   [EBX]LARGE_INTEGER.HighPart, EDX
#           else
                QueryPerformanceCounter(&mHpcTime);                
#           endif
            return (jlong)mHpcTime.QuadPart;
        }
        else {
            // The FILETIME represents the time in units of "10**-7 sec" since 1/1/1601
            FILETIME      tFt;
            LARGE_INTEGER aFt;
            GetSystemTimeAsFileTime(&tFt); 
            
            aFt.HighPart = tFt.dwHighDateTime;
            aFt.LowPart  = tFt.dwLowDateTime;

            return (*(jlong*)&aFt);
        }
#   else
        // gettimeofday returns the time in units of "10**-6 sec" stating from 1/1/1970
        // The update of the timestamp is hardware dependant
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (jlong)(1000000LL * tv.tv_sec + tv.tv_usec);
#   endif
};
// ----------------------------------------------------
// TSystem::getHpDiff
// ----------------------------------------------------
jlong TSystem::getDiffHp(jlong aHpTime) {
    jlong aDiffTime = 0;

    if (aHpTime == 0) {
        return 0;
    }
    
    aDiffTime = getTimestampHp();
    if (aDiffTime < aHpTime) {
        return 0;
    }

    // Output in microseconds
#ifdef _WINDOWS
    if (mHasHpcTimer) {
        aDiffTime = (jlong)floor((aDiffTime - aHpTime) * mScale);
    }
    else {
        aDiffTime = (aDiffTime - aHpTime)/10;
    }
#else 
    aDiffTime = (aDiffTime - aHpTime);
#endif
    return aDiffTime;

};

// ----------------------------------------------------------------
// TSystem::setHpcTimer
// ----------------------------------------------------------------
bool TSystem::setHpcTimer() {
    mHasHpcTimer = true;
#   ifdef _WINDOWS
        mHasHpcTimer = 
            QueryPerformanceFrequency(&mHpcFrequence) &&
            QueryPerformanceCounter(&mHpcStartTime);

        if (!mHasHpcTimer) {
            return mHasHpcTimer;    
        }        

        mScale          = (long double)(1000.0 / (jdouble)mHpcFrequence.QuadPart);
        mHpcStartTime   = mHpcTime;

#       if defined(USE_RDTSC) && defined(_X86_)
            LARGE_INTEGER *aPtr = &mHpcTime;
            __asm MOV   EBX, aPtr
            __asm RDTSC          
            __asm MOV   [EBX]LARGE_INTEGER.LowPart,  EAX
            __asm MOV   [EBX]LARGE_INTEGER.HighPart, EDX

            mScale *= (long double)((jdouble)mHpcStartTime.QuadPart / (jdouble)mHpcTime.QuadPart);
#       endif
        calculateOffset(true);
#   endif
    return mHasHpcTimer;
}
// ----------------------------------------------------
// TSystem::calculateOffset
// ----------------------------------------------------
jlong TSystem::calculateOffset(bool aForce) {
#   ifdef _WINDOWS
        if (!mHasHpcTimer) {
            return mOffset;
        }

        if (!aForce) {
            if (mHpcStartTime.QuadPart <= mHpcTime.QuadPart) {
                return mOffset;
            }
            mHpcStartTime = mHpcTime;
        }
        FILETIME tFt;
        LARGE_INTEGER aTimeFt;
        GetSystemTimeAsFileTime(&tFt); 
        aTimeFt.LowPart     = tFt.dwLowDateTime;
        aTimeFt.HighPart    = tFt.dwHighDateTime;
        mOffset             = ((*(jlong*)&aTimeFt / 10000LL) - OFFSET_1600)
                               - (jlong)floor((long double)mHpcStartTime.QuadPart * mScale);
#   endif    
    return mOffset;
}

// ----------------------------------------------------
// TSystem::hasHpc
// ----------------------------------------------------
bool TSystem::hasHpc() {
    return mHasHpcTimer;
}
// ----------------------------------------------------
// TSystem::getSystemTime
// ----------------------------------------------------
SAP_UC *TSystem::getSystemTime() {
    SAP_ostringstream aStream;
    memsetU(mBuffer, cU('\0'), 127);
#   ifdef _WINDOWS
        GET_SYSTIME(&mSystemTime);
        aStream << mSystemTime.wDay    << cU(".")
                << mSystemTime.wMonth  << cU(".")
                << mSystemTime.wYear   << cU(" ")
                << mSystemTime.wHour   << cU(":")
                << mSystemTime.wMinute << cU(":")
                << mSystemTime.wSecond << ends;
        STRNCPY(mBuffer, aStream.str().c_str(), 127, 128);
#   else
        time_t      iTime = time(NULL);
        /*CCQ_CLIB_LOCTIME_OK*//*SAPUNICODEOK_CHARTYPE*//*SAPUNICODEOK_LIBFCT*/
        const char *aTime = ctime(&iTime);
        for (int i = 0; i < 24; i++) {
           mBuffer[i] = SAP_UC(aTime[i]);
        }
        mBuffer[24] = cU('\0');
#   endif
    return mBuffer;
}
// ----------------------------------------------------
// TSystem::getDiff
// ----------------------------------------------------
jlong TSystem::getDiff(jlong aTime) {
    if (aTime == 0) {
        return 0;
    }
    jlong aActualTime = getTimestamp();
    if (aActualTime > aTime) {
        return (jlong)((unsigned long)aActualTime - (unsigned long)aTime);
    }
    else {
        return 0;
    }
}
// ----------------------------------------------------
// TSystem::GetCurrentThreadCpuTime
// ----------------------------------------------------
jlong TSystem::GetCurrentThreadCpuTime() {
    jlong aTime = 0;
#ifdef _WINDOWS
    HANDLE          hThread = GetCurrentThread();
    FILETIME        aCreationTime;
    FILETIME        aExitTime;
    FILETIME        aKernelTime;
    FILETIME        aUserTime;

    LARGE_INTEGER   aUserClock;
    LARGE_INTEGER   aKernClock;

    BOOL aDone = GetThreadTimes(hThread, &aCreationTime, &aExitTime, &aKernelTime, &aUserTime);
    if (!aDone) {
        // not supported
    }
    aUserClock.HighPart = aUserTime.dwHighDateTime;
    aUserClock.LowPart  = aUserTime.dwLowDateTime;
    
    aKernClock.HighPart = aKernelTime.dwHighDateTime;
    aKernClock.LowPart  = aKernelTime.dwLowDateTime;

    aTime = (*(jlong *)&aUserClock + *(jlong *)&aKernClock)/ 10LL;          
#else
#   if defined(_POSIX_TIMERS) && defined(_POSIX_THREAD_CPUTIME) && _POSIX_THREAD_CPUIME > 0
        struct timespec aTimeSpec;
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &aTimeSpec);
        aTime = (jlong)floor((double)(1000000000LL * aTimeSpec.tv_sec + aTimeSpec.tv_nsec)/(double)1000);
#   else 
        aTime = (jlong)floor((double)(1000000 * CLOCK())/(double)CLOCKS_PER_SEC);
#   endif
#endif
    return aTime;
}

// ----------------------------------------------------
// TSystem::Execute a program and wait
// ----------------------------------------------------
long TSystem::Execute(const SAP_UC *aCmdLine) {
    long aResult = -1;

#ifdef _WINDOWS
    BOOL        aRetCreate;
    DWORD       aRetCode       = 0;
    DWORD       aCreationFlags = 0;
    STARTUPINFO aStartupInfo;
    PROCESS_INFORMATION aProcInfo;   

    memsetR(&aStartupInfo, 0, sizeofR(STARTUPINFO));
    aStartupInfo.cb = sizeofR(STARTUPINFO);
    SAP_UC  aBuf[1024];
    SAP_UC *aCwd;
    aCwd = getcwdU(aBuf, 1024);
    TString aPath( TProperties::getInstance()->getPath(), 1024 );
    chdirU(aPath.str());
    aCwd = getcwdU(aBuf, 1024);

    aRetCreate = CreateProcess(NULL, const_cast<LPTSTR> (aCmdLine), NULL, NULL, 1, aCreationFlags, NULL, NULL, &aStartupInfo, &aProcInfo);
    aResult    = (aRetCreate == TRUE) ? 0: -1;

    if (aResult == 0) {
        WaitForSingleObject(aProcInfo.hProcess, INFINITE);
        GetExitCodeProcess(aProcInfo.hProcess, &aRetCode);
        aResult = aRetCode;
    }

#endif
    return aResult;
}


// ----------------------------------------------------
// TString::parseInt
// ----------------------------------------------------
SAP_UC *TString::parseInt(
        jlong   aInt, 
        SAP_UC *aBuffer, 
        int     aRight, 
        bool    aSigned) {

    jlong  i        = 0;
    jlong  j        = 0;
    bool   bSign    = false;
    jlong  aHigh    = 0;
    jlong  aHigh1   = 0;
    jlong  aTmp     = aInt;

    if (aInt < 0 && aSigned) {
        bSign = true;
    }

    if (aInt < 0) {
#       ifdef _WINDOWS
            LARGE_INTEGER aLarge;

            aLarge.LowPart  = 0;
            aLarge.HighPart = 1 << (((sizeofR(aLarge) * 4) - 1)); 
            aHigh  = aLarge.QuadPart;

            aLarge.LowPart  = 0;
            aLarge.HighPart = 1 << (((sizeofR(aLarge) * 4) - 2)); 
            aHigh1 = aLarge.QuadPart;
#       else
            aHigh  = (jlong)1 << (((sizeofR(aHigh) * 8) - 1));
            aHigh1 = (jlong)1 << (((sizeofR(aHigh) * 8) - 2));
#       endif 

        // Representation of negative number m: 
        // 2**sizeof(integer) - m
        // For large ingeger we know: sizeof(integer)    mod  4 = 0  
        // Form this it follow      : 2**sizeof(integer) mod 10 = 6
        // From modul calculus      : (a + b) mod 10 <==> ((a mod 10) + (b mod 10)) mod 10
        // To avoid negative numbers: (10 + (a mod 10) + (b mod  10)) mod 10
        aBuffer[i++] = (SAP_UC)(48 + ((16 +              (aInt % 10)) %   10));

        // devide by 10
        aTmp = aInt ^ aHigh;    // clear sign bit
        aTmp = aTmp / 2;        // devide by 2     <=> shift right
        aTmp = aTmp | aHigh1;   // set highest bit <=> restore the sign bit
        aInt = aTmp / 5;        // devide by 5
        j++;
    }

    while(aInt > 0) {
        aBuffer[i++] = (SAP_UC)(48 + aInt % 10);
        aInt = aInt / 10;
        if (((++j % 3) == 0) && (aInt > 0)) {
            aBuffer[i++] = cU('.');
        }
    }

    if (i == 0) {
        aBuffer[i++] = (SAP_UC)48;
    }
    if (bSign) {
        aBuffer[i++] = cU('-');
    }
    while (i < aRight) {
        aBuffer[i++] = cU(' ');
    }
    aBuffer[i] = 0;
    return reverse(aBuffer);
}
// -----------------------------------------------------------------
// System::openSocket
//! \brief Establish socket administration to accept clients
//! \return \c TRUE if socket listener established successfully
// -----------------------------------------------------------------
bool TSystem::openSocket(SOCKET *aHostSocket, unsigned short aPort, const SAP_UC *aHostName) {
    int     aResult;
    TString alHostName(aHostName, 1024);

    /*CCQ_IPV6_SUPPORT_OK*/
    struct sockaddr_in aServerAddr;
    // create socket
    /*CCQ_IPV6_SUPPORT_OK*/
    *aHostSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if ((jlong)*aHostSocket <= 0) {            
        ERROR_OUT(cU("create inet socket"), 0);
        return false;
    }
    // bind to port
    /*CCQ_IPV6_SUPPORT_OK*/
    aServerAddr.sin_family = AF_INET;
    /*CCQ_IPV6_SUPPORT_OK*/
    aServerAddr.sin_port   = htons(aPort);

    aResult = 0;
    /*CCQ_IPV6_SUPPORT_OK*//*SAPUNICODEOK_LIBSTRUCT*//*SAPUNICODEOK_LIBFCT*/
    struct hostent *aHost = gethostbyname(alHostName.a7_str());
    if (aHost == NULL) {
#       if defined (_WINDOWS) 
            aResult = WSAGetLastError();
#       endif
        ERROR_OUT(cU("gethostbyname"), aResult);
        return false;
    }
    
    /*SAPUNICODEOK_CHARTYPE*/
    char    aInetAddr[32];
    memsetR(aInetAddr, 0, 32);

#   if defined (_WINDOWS) 
       /*SAPUNICODEOK_CHARTYPE*//*SAPUNICODEOK_LIBFCT*//*SAPUNICODEOK_SIZEOF*/
       sprintf_s(aInetAddr, sizeof(aInetAddr), cR("%d.%d.%d.%d"),
           /*SAPUNICODEOK_CHARTYPE*/(unsigned char)aHost->h_addr_list[0][0],
           /*SAPUNICODEOK_CHARTYPE*/(unsigned char)aHost->h_addr_list[0][1],
           /*SAPUNICODEOK_CHARTYPE*/(unsigned char)aHost->h_addr_list[0][2],
           /*SAPUNICODEOK_CHARTYPE*/(unsigned char)aHost->h_addr_list[0][3]);
#   else
       /*SAPUNICODEOK_CHARTYPE*//*SAPUNICODEOK_LIBFCT*//*SAPUNICODEOK_SIZEOF*/
       sprintf(aInetAddr, cR("%d.%d.%d.%d"),
           /*SAPUNICODEOK_CHARTYPE*/(unsigned char)aHost->h_addr_list[0][0],
           /*SAPUNICODEOK_CHARTYPE*/(unsigned char)aHost->h_addr_list[0][1],
           /*SAPUNICODEOK_CHARTYPE*/(unsigned char)aHost->h_addr_list[0][2],
           /*SAPUNICODEOK_CHARTYPE*/(unsigned char)aHost->h_addr_list[0][3]);
#   endif

#   if defined (_WINDOWS) || defined (_HPPA) 
       /*CCQ_IPV6_SUPPORT_OK*/
       aServerAddr.sin_addr.s_addr = inet_addr(aInetAddr);
       memsetR(&(aServerAddr.sin_zero), 0, 8);
#   else
       /*CCQ_IPV6_SUPPORT_OK*//*SAPUNICODEOK_CHARTYPE*/
       aResult = inet_pton(aServerAddr.sin_family, aInetAddr, reinterpret_cast<char *>(&(aServerAddr.sin_addr)));
       if (aResult != 1) {
           ERROR_OUT(cU("inet_pton"), (int)aResult);
           return false;
       }
       memsetR(&(aServerAddr.sin_zero), 0, sizeofR(aServerAddr.sin_zero));
#   endif
    
    /*CCQ_IPV6_SUPPORT_OK*/
    aResult = bind(*aHostSocket, reinterpret_cast<struct sockaddr *>(&aServerAddr), (int)sizeofR(struct sockaddr_in));
    if (aResult != 0) {
        ERROR_OUT(cU("bind socket to port"), (int)aResult);
        return false;
    }
    // start listener
    /*CCQ_IPV6_SUPPORT_OK*/
    aResult = listen(*aHostSocket, 2);
    if (aResult != 0) {
        ERROR_OUT(cU("listen"), (int)aResult);
        return false;
    }
    return true;
 }
// ----------------------------------------------------
// System::acceptClient
// ----------------------------------------------------
bool TSystem::acceptClient(SOCKET aHostSocket, SOCKET *aClientSocket) {
    int aIntRes = 0;
    
    /*CCQ_IPV6_SUPPORT_OK*/
    struct sockaddr_in aClientAddr;
    /*CCQ_IPV6_SUPPORT_OK*/
    SAPSOCKLEN_T aSize = sizeofR(struct sockaddr);

    /*CCQ_IPV6_SUPPORT_OK*/
    aIntRes = listen(aHostSocket, 2);
    if (aIntRes != 0) {
         ERROR_OUT(cU("Cannot start listener on port"), -1);
         return false;
     }
    
    /*CCQ_IPV6_SUPPORT_OK*/
    (*aClientSocket) = accept(aHostSocket, reinterpret_cast<struct sockaddr *>(&aClientAddr), &aSize);
    if ((*aClientSocket) <= 0) {
        ERROR_OUT(cU("accept failed"), -1);
        return false;
    }       
    return true;
}

// ----------------------------------------------------
// ----------------------------------------------------
void TSystem::startup() {
#   if defined (_WINDOWS)
        int aResult;
        mVersionRequested = MAKEWORD(2, 2);
        aResult = WSAStartup(mVersionRequested, &mWsaData);

        if (aResult != 0) {
            ERROR_OUT(cU("WSAStartup()"), aResult);
        }   
#   endif
}

// ----------------------------------------------------
// TString::parseHex
// ----------------------------------------------------
SAP_UC *TString::parseHex(
        jlong   aInt, 
        SAP_UC *aBuffer) {

    jlong i      = 0;
    jlong aDigit = 0;
    jlong aSign  = 0;
    /*SAPUNICODEOK_SIZEOF*/
    jlong aSize  = 2 * sizeof(jlong);

    if (aInt < 0) {
        aSign = 1;
    }

    while (i < aSize) {
        aDigit = aInt & 0xF;
        if (aDigit < 10)    
            aBuffer[i++] = (SAP_UC)(cR('0') + aDigit);
        else
            aBuffer[i++] = (SAP_UC)(cR('A') + aDigit - 10);
        aInt = aInt / 0x10;
        aInt = aInt - aSign;
    }
    if (i == 0) {
        aBuffer[i++] = cU('0');
    }
    aBuffer[i++] = cU('x');
    aBuffer[i++] = cU('0');
    aBuffer[i] = 0;
    return reverse(aBuffer);
}
// ----------------------------------------------------------------
// TString::parseBool
// ----------------------------------------------------------------
SAP_UC *TString::parseBool(bool bValue, SAP_UC *aBuffer) {
    if (bValue) {
        STRCPY(aBuffer, cU("true"),  6);
    } 
    else {
        STRCPY(aBuffer, cU("false"), 6);
    }
    return aBuffer;
}
// ----------------------------------------------------------------
// TString::reverse
// ----------------------------------------------------------------
SAP_UC *TString::reverse(SAP_UC *aBuffer) {
    SAP_UC aTmp;
    size_t  i;
    size_t  aLen = STRLEN(aBuffer);
    size_t  aRev = aLen-1;

    for (i = 0; i < aLen/2; i++) {
        aTmp            = aBuffer[i];
        aBuffer[i]      = aBuffer[aRev-i];
        aBuffer[aRev-i] = aTmp;
    }
    return aBuffer;
}
// ----------------------------------------------------------------
// Implementation for static members 
// ----------------------------------------------------------------
#ifdef _WINDOWS
    SYSTEMTIME    TSystem::mSystemTime;
    LARGE_INTEGER TSystem::mHpcTime;
    LARGE_INTEGER TSystem::mHpcStartTime;
    LARGE_INTEGER TSystem::mHpcFrequence;
    WSADATA       TSystem::mWsaData;
    WORD          TSystem::mVersionRequested;
#endif

jlong       TSystem::mOffset            = 0;
bool        TSystem::mHasHpcTimer       = false;
long double TSystem::mScale             = (double)1000000 / (double)CLOCKS_PER_SEC;
SAP_UC      TSystem::mBuffer[128];

TMonitor    *TMonitor::mInstance        = NULL;
TTracer     *TTracer::mInstance         = NULL;
TProperties *TProperties::mInstance     = NULL;
TLogger     *TLogger::mInstance         = NULL;
TConsole    *TConsole::mInstance        = NULL;
TWriter     *TWriter::mInstance         = NULL;
TSecurity   *TSecurity::mInstance       = NULL;
TCallstack  *TMonitorThread::mGCallstack = new TCallstack(1024);
jint         TMonitorThread::mGlobalHash = 1;
TCommand    *TCommand::mInstance        = NULL;

TThreadList  TMonitorThread::mThreads;
unsigned int TMonitor::gTransaction     = 1;

// THttpServer  *THttpServer::mInstance    = NULL;
// TAgentServer *TAgentServer::mInstance   = NULL;
// TAgent       *TAgent::mInstance         = NULL;
// THiBroker    *THiBroker::mInstance      = NULL;

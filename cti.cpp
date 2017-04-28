// -----------------------------------------------------------------
//
// Author: Albert Rossmann
// File  : cti.cpp
// Date  : 27.08.2008
// Abstract:
//    Profiler interface for C,C++
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
#include "cti.h"
#include "ptypes.h"

#include <iostream>
#include <new>
#include <string>

#if defined(_WINDOWS) || defined(SAPonNT)
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" {
    typedef JNIEXPORT jint(JNICALL *TAgentOnLoad)( /*SAPUNICODEOK_CHARTYPE*/
        const char      *aOptions,
        TCtiInterface  **CtiEnv,
        jint             aVersion);

    typedef JNIEXPORT jint(JNICALL *TTypesOnLoad)( /*SAPUNICODEOK_CHARTYPE*/
        TCtiInterface  **CtiEnv,
        jint             aVersion);
}
extern "C" {
    TCtiInterface& TCtiInterface::operator=(const TCtiInterface &aCti) {
        if (this == &aCti) {
            return *this;
        }
        this->mVersion          = aCti.mVersion;;

        this->doCommand         = aCti.doCommand;

        this->registerMethod    = aCti.registerMethod;
        this->registerField     = aCti.registerField;
        this->onEnterMethod     = aCti.onEnterMethod;
        this->onExitMethod      = aCti.onExitMethod;
        this->onException       = aCti.onException;
        this->onExceptionCatch  = aCti.onExceptionCatch;

        this->onObjectAlloc     = aCti.onObjectAlloc;
        this->onObjectFree      = aCti.onObjectFree;
        this->onObjectRealloc   = aCti.onObjectRealloc;

        this->onVMDeath         = aCti.onVMDeath;
        this->toString          = NULL;
        return *this;
    };

    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    TCtiProfiler::TCtiProfiler(
        jmethodID   *jMethod,           /*SAPUNICODEOK_CHARTYPE*/
        const char  *aPackage,          /*SAPUNICODEOK_CHARTYPE*/
        const char  *aClass,            /*SAPUNICODEOK_CHARTYPE*/
        const char  *aMethod,           /*SAPUNICODEOK_CHARTYPE*/
        const char  *aSignature,...) {

        if (mCti != NULL) {
            if (*jMethod == NULL) {
                *jMethod = mCti->registerMethod(aPackage, aClass, aMethod, aSignature);
            }
            mMethod = *jMethod;

            //va_list  aVarArgs;
            //va_start(aVarArgs, aSignature);
            mCti->onEnterMethod(mMethod);
            //va_end  (aVarArgs);
        }
    };

    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    TCtiProfiler::TCtiProfiler(         /*SAPUNICODEOK_CHARTYPE*/
        const char *aPackage,           /*SAPUNICODEOK_CHARTYPE*/
        const char *aClass,             /*SAPUNICODEOK_CHARTYPE*/
        int        *argc,
        wchar_t    *argv[]) {

        int            aNewCnt = 0;
        wchar_t       *aNewArgv[64];

        jint           jResult = JNI_OK;
        TAgentOnLoad   aFncAgentLoad;
        bool           aRunSherlok = false;
        /*SAPUNICODEOK_CHARTYPE*/
        char aOptions[1024];
        /*SAPUNICODEOK_CHARTYPE*//*SAPUNICODEOK_LIBFCT*//*SAPUNICODEOK_STRINGCONST*/
		strcpy(aOptions, (const char*)"ConfigPath=.");

        mIsMain = true;

        for (int i = 0; i < *argc; i++) {
            std::wstring aArg(argv[i]);
            
            if (aArg.compare(0, 17, L"-agentlib:sherlok") == 0) {
                aRunSherlok = true;

                if (argv[i][17] != L'=') {
                    break;
                }
                wchar_t *aSrc = argv[i] + 18;
                /*SAPUNICODEOK_CHARTYPE*/
                char    *aDst = aOptions;
                /*SAPUNICODEOK_CHARTYPE*/
                while  ((*aDst++ = (char)(*aSrc++)) != 0);
                argv[i][0] = L'\0';
                break;
            }
            else {
                aNewArgv[aNewCnt++] = argv[*argc];
            }
        }

        // Remove the sherlok specific arguments to prevent program from 
        // thowing errors for unkown parameters
        if (aRunSherlok) {
            for (int i = 0; i < aNewCnt; i++) {
                argv[i] = aNewArgv[i];
            }
            *argc = aNewCnt;
        }
        else {
            /*SAPUNICODEOK_CHARTYPE*//*SAPUNICODEOK_STRINGCONST*/
            strcpy(aOptions, "ConfigPath=.");
        }


#if defined(_WINDOWS) || defined(SAPonNT)
        wchar_t *aStrLibname  = (wchar_t *)L"sherlok.dll";
        HMODULE  aHandleLib   = LoadLibrary(aStrLibname);

        wchar_t *aTypeLibname = (wchar_t *)L"shertype.dll";
        HMODULE  aHandleTypes = LoadLibrary(aTypeLibname);
#else
        /*SAPUNICODEOK_CHARTYPE*//*SAPUNICODEOK_STRINGCONST*/
        char    *aStrLibname  = (char *)"libsherlok.so";
        void    *aHandleLib   = dlopen(aStrLibname, RTLD_LAZY | RTLD_GLOBAL);

        char    *aTypeLibname = (char *)"libshertype.so";
        void    *aHandleTypes = dlopen(aStrLibname, RTLD_LAZY | RTLD_GLOBAL);
#endif

        if (aHandleLib == NULL) {
            std::wcerr << L"Error loading library sherlok library" << std::endl;
            return;
        }

#if defined (_WINDOWS) || defined(SAPonNT)
        aFncAgentLoad = (TAgentOnLoad)GetProcAddress(aHandleLib, (LPCSTR)L"CtiAgentOnLoad");
#else
        dlerror();
        /*SAPUNICODEOK_CHARTYPE*//*SAPUNICODEOK_STRINGCONST*/
        aFncAgentLoad = (TAgentOnLoad)dlsym(aHandleLib, "CtiAgentOnLoad");
#endif

        if (aFncAgentLoad == NULL) {
            std::wcerr << L"Error loading method CtiAgentOnLoad" << std::endl;
            return;
        }

        jResult = aFncAgentLoad(aOptions, &mCti, CTI_VERSION_1);

#if defined (_WINDOWS) || defined(SAPonNT)
        GetProcAddress(aHandleLib, (LPCSTR)L"CtiTypesOnLoad");
#else
        dlerror();
        /*SAPUNICODEOK_CHARTYPE*//*SAPUNICODEOK_STRINGCONST*/
        dlsym(aHandleLib, "CtiTypesOnLoad");
#endif

        if (mCti != NULL) {
            /*SAPUNICODEOK_CHARTYPE*//*SAPUNICODEOK_STRINGCONST*/
            mMethod = mCti->registerMethod(aPackage, aClass, (const char*)"main", (const char*)"int argc, SAP_UC** argv");
        }
    };

    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    TCtiProfiler::~TCtiProfiler() {
        if (mCti != NULL) {
            mCti->onExitMethod(mMethod);
            
            if (mIsMain) {
                mCti->onVMDeath();
            }
        }
    };

    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    TCtiInterface *TCtiProfiler::getCti() {
        return mCti;
    };

    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    void *TCtiProfiler::ctiAlloc(size_t aSize, jmethodID jMethod, int aLineNo) {
        /*SAPUNICODEOK_CHARTYPE*/
        unsigned char *aMem;

        if (mCti != NULL) {
            mCti->onObjectAlloc(aSize, jMethod, aLineNo, &aMem);
        }
        else {
            #undef malloc
            /*SAPUNICODEOK_CHARTYPE*//*SAPUNICODEOK_LIBFCT*/
            aMem = (unsigned char *)malloc(aSize);
        }
        return aMem;
    }

    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    void * TCtiProfiler::ctiRealloc(void *aMem, size_t aSize, jmethodID jMethod, int aLine) {
        if (mCti != NULL) {
            /*SAPUNICODEOK_CHARTYPE*/
            mCti->onObjectRealloc(aSize, jMethod, aLine, reinterpret_cast<unsigned char **>(&aMem));
        }
        else {
            #undef realloc
            /*SAPUNICODEOK_LIBFCT*/
            return realloc(aMem, aSize);
        }
        return aMem;
    }

    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    void TCtiProfiler::ctiDelete(void *aMem) {
        if (mCti != NULL) { /*SAPUNICODEOK_CHARTYPE*/
            mCti->onObjectFree(reinterpret_cast<unsigned char *>(aMem));
        }
        else  {
            #undef free
            free(aMem);
        }
    }
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
TCtiInterface *TCtiProfiler::mCti = NULL;


#ifdef PROFILE_MEM
// 
// usage:
// TObject *anObject = cti::new(jMethod, aLine) TObject()
//
namespace cti {
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    void *operator new (size_t aSize, jmethodID jMethod, int aLineNo)  throw(){
        return TCtiProfiler::ctiAlloc(aSize, jMethod, aLineNo);
    };

    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    void *operator new [](size_t aSize, jmethodID jMethod, int aLineNo) throw() {
        return TCtiProfiler::ctiAlloc(aSize, jMethod, aLineNo);
    };

    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    void operator delete (void *aMem) throw() {
        TCtiProfiler::ctiDelete(aMem);
    };

    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    void operator delete[](void *aMem) throw() {
        TCtiProfiler::ctiDelete(aMem);
    };

#endif







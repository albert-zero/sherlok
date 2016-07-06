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
#include <iostream>
#include <new>

#if defined(PROFILE_CPP)
#   include "sapiostrm.hpp"
#   include "saptype.h"
#   include "dlxx.h"
#   include "sapthr.h"
#endif

#include "ptypes.h"
#include "cti.h"

#if !defined(SAPwithTHREADS) && defined(PROFILE_CPP)
#    error PROFILE_CPP needs SAPwithTHREADS
#endif

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
        return *this;
    };

    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    TCtiProfiler::TCtiProfiler(
        jmethodID   *jMethod,           /*SAPUNICODEOK_CHARTYPE*/
        const char  *aPackage,          /*SAPUNICODEOK_CHARTYPE*/
        const char  *aClass,            /*SAPUNICODEOK_CHARTYPE*/
        const char  *aMethod,           /*SAPUNICODEOK_CHARTYPE*/
        const char  *aSignature) {

        if (mCti != NULL) {
            if (*jMethod == NULL) {
                *jMethod = mCti->registerMethod(aPackage, aClass, aMethod, aSignature);
            }
            mMethod = *jMethod;

            //va_list  aVarArgs;
            //va_start(aVarArgs, aMethod);
            mCti->onEnterMethod(mMethod);
            //va_end  (aVarArgs);
        }
    };
    
    TCtiProfiler::TCtiProfiler(int argc, /*SAPUNICODEOK_CHARTYPE*/ char * argv[]) {
        mCti = new TCtiInterface;
    };
    
    // 
    // -----------------------------------------------------------------
    TCtiProfiler::~TCtiProfiler() {
        if (mCti != NULL) {
            mCti->onExitMethod(mMethod);
        }
    };

    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    TCtiInterface *TCtiProfiler::getCti() {
        if (mCti == NULL) {
            mCti = new TCtiInterface;
        }
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
/*
namespace cti {
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    void *operator new (size_t aSize, jmethodID jMethod, int aLineNo)  throw() {
        return TCtiProfiler::ctiAlloc(aSize, jMethod, aLineNo);
    };

    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    void *operator new [] (size_t aSize, jmethodID jMethod, int aLineNo) throw() {
        return TCtiProfiler::ctiAlloc(aSize, jMethod, aLineNo);
    };

    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    void operator delete (void *aMem) throw() {
        TCtiProfiler::ctiDelete(aMem);
    };

    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    void operator delete[] (void *aMem) throw() {
        TCtiProfiler::ctiDelete(aMem);
    };
}*/
#endif

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" {
    JNIEXPORT jint (JNICALL *AgentOnLoad)( /*SAPUNICODEOK_CHARTYPE*/
        const char      *aOptions, 
        TCtiInterface  **CtiEnv,
        jint             aVersion) = NULL;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" JNIEXPORT jint JNICALL CtiInit(        /*SAPUNICODEOK_CHARTYPE*/
        const char      *aOptions) { 

    jint   jResult = JNI_OK;
    
#   if defined(PROFILE_CPP)
        SAP_UC            aProfilerName[128];
        DL_HDL            aProfilerHdl;
        SAPRETURN            aResult;

        TCtiInterface  *aCti = TCtiProfiler::getCti();

        strcpyU(aProfilerName, cU(DL_DEF_LIB_PREFIX));
        strcatU(aProfilerName, cU("sherlok"));
        strcatU(aProfilerName, cU(DL_DEF_LIB_POSTFIX));
    
        aResult = DlLoadLib2((const SAP_UC *)aProfilerName, &aProfilerHdl, DL_RTLD_LAZY | DL_RTLD_GLOBAL);
        if (aResult != 0) {
            SAP_cerr << cU("Error loading library sherlok rc=") << aResult << std::endl;
            return JNI_ERR;
        }
        aResult = DlLoadFunc(aProfilerHdl,  cU("CtiAgentOnLoad"), 0, (DL_ADR*)&AgentOnLoad);
        if (aResult != 0) {
            SAP_cerr << cU("Error loading function pointer CtiAgentOnLoad, rc=") << aResult << std::endl;
            return JNI_ERR;
        }
        jResult = AgentOnLoad(aOptions, &aCti, CTI_VERSION_1);
#   endif 

    return jResult;
};

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" void ccQCovInitSherlok (int argc, /*SAPUNICODEOK_CHARTYPE*/ char *argv []) {
    /*SAPUNICODEOK_CHARTYPE*/ char  aOptions[1024];
    /*SAPUNICODEOK_CHARTYPE*/ char *aPtr = aOptions;

    for (int i = 0; i < argc; i++) {
        SAP_UC *aLocArg = (SAP_UC *)argv[i];

        if (!STRNCMP(aLocArg, cU("-agentlib:sherlok"), 17)) {
            if (aLocArg[17] == cU('=')) {
                for (aLocArg = aLocArg + 18; *aLocArg != cU('\0'); aLocArg++) {
                    /*SAPUNICODEOK_CHARTYPE*/ 
                    *aPtr = (char )(*aLocArg);
                    aPtr ++;
                }
            }
            *argv[i]    = cU('\0');
            *aPtr       = cR('\0');
            CtiInit(aOptions);
            break;
        }
            
        if (!STRNCMP(aLocArg, cU("pf="), 3)) {
            /*SAPUNICODEOK_STRINGCONST*/
            CtiInit(cR("ConfigPath=."));
            break;
        }
    }
}

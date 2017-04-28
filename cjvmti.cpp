// -----------------------------------------------------------------
//
// Author: Albert Rossmann
// File  : cjvmti.cpp
// Date  : 27.08.2008
// Abstract:
//    Profiler interface for C,C++
//    Options on Linux -pthread -std=gnu++11 -lstdc++ 
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
#include <memory>
#include "ptypes.h"
#include "cjvmti.h"
#include "monitor.h"
#include "command.h"
#include  <thread>
		
#if defined (PROFILE_SAP_CPP)
#   include "sapthr.h"
#elif defined (PROFILE_STD_CPP)
#   include <thread>
#   include <mutex>
#   include <condition_variable>
#endif
// -----------------------------------------------------------------
// CtiMonitor::CtiMonitor
//! CtiMontior replaces JVMTI monitor in case CTI is active 
// -----------------------------------------------------------------
CtiMonitor::CtiMonitor(/*SAPUNICODEOK_CHARTYPE*/ const char *aName) {
#if defined (PROFILE_SAP_CPP)
    THR_ERR_TYPE    cResult;
    TString         aLockName;        

    aLockName.assignR(aName, STRLEN_A7(aName));
    cResult = ThrRecMtxInit(&mMonitor, const_cast<SAP_UC*>(aLockName.str()));
    cResult = ThrEvtInit(&mEvent);
#elif defined (PROFILE_STD_CPP)
    mMonitor  = new std::recursive_mutex();
    mEventMtx = new std::mutex();
    mEvent    = new std::condition_variable();
#endif
};

// -----------------------------------------------------------------
// CtiMonitor::~CtiMonitor
//! \brief destructor
// -----------------------------------------------------------------
CtiMonitor::~CtiMonitor() {
#if defined(PROFILE_SAP_CPP)
    THR_ERR_TYPE    cResult = THR_ERR_OK;
    cResult = ThrRecMtxDelete(&mMonitor);
    cResult = ThrEvtDelete(&mEvent);
#elif defined (PROFILE_STD_CPP)
    delete mMonitor;
    delete mEventMtx;
    delete mEvent;
#endif
};

// -----------------------------------------------------------------
// -----------------------------------------------------------------
jvmtiError CtiMonitor::enter(bool aExclusive) {
#if defined(PROFILE_SAP_CPP)
    THR_ERR_TYPE    cResult = THR_ERR_OK;
    cResult = ThrRecMtxLock(&mMonitor);
#elif defined (PROFILE_STD_CPP)
    mMonitor->lock();
#endif
	return JVMTI_ERROR_NONE;
};

// -----------------------------------------------------------------
// -----------------------------------------------------------------
jvmtiError CtiMonitor::exit() {
#if defined(PROFILE_SAP_CPP)
    THR_ERR_TYPE cResult = ThrRecMtxUnlock(&mMonitor);
#elif defined (PROFILE_STD_CPP)
    mMonitor->unlock();
#endif
	return JVMTI_ERROR_NONE;
};

// -----------------------------------------------------------------
// -----------------------------------------------------------------
jvmtiError CtiMonitor::wait(jlong aTime) {
#if defined(PROFILE_SAP_CPP)
    THR_ERR_TYPE cResult = THR_ERR_OK;
    cResult = ThrEvtWait(&mEvent, (int)aTime);
    cResult = ThrEvtReset(&mEvent);
#elif defined (PROFILE_STD_CPP)
    std::unique_lock<std::mutex> aUniqueLock(*mEventMtx);
    
    if (aTime < 0) {
        mEvent->wait(aUniqueLock);
    }
    else {
        std::chrono::milliseconds aTimeSpan(aTime);
        auto aTimeInterval = std::chrono::system_clock::now() + aTimeSpan;
        mEvent->wait_until(aUniqueLock, aTimeInterval);
    }
#endif
	return JVMTI_ERROR_NONE;
};

// -----------------------------------------------------------------
// -----------------------------------------------------------------
jvmtiError CtiMonitor::notify() {
#if defined(PROFILE_SAP_CPP)
    cResult = ThrEvtSet(&mEvent);
#elif defined (PROFILE_STD_CPP)
    mEvent->notify_all();
#endif
	return JVMTI_ERROR_NONE;
};


// -----------------------------------------------------------------
// -----------------------------------------------------------------
TObject::TObject(jclass jClass) {
    setClass(jClass);
};
// -----------------------------------------------------------------
// -----------------------------------------------------------------
void TObject::deallocate(jlong aSize) {
    delete this;
};

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiCallocate(
        jvmtiEnv        *aJvmti, 
        jlong            aCount,
        jlong            aSize,             /*SAPUNICODEOK_CHARTYPE*/
        unsigned char  **aMemPtr) {

    if (aMemPtr == NULL) {
        return JVMTI_ERROR_NULL_POINTER;
    }

    /*SAPUNICODEOK_CHARTYPE*//*SAPUNICODEOK_LIBFCT*/
    *aMemPtr = (unsigned char  *)calloc((size_t)aCount, (size_t)aSize);
    /*SAPUNICODEOK_CHARTYPE*/
    return (*aMemPtr == NULL) ? JVMTI_ERROR_OUT_OF_MEMORY: JVMTI_ERROR_NONE;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiAllocate(
        jvmtiEnv        *aJvmti, 
        jlong            aSize,             /*SAPUNICODEOK_CHARTYPE*/
        unsigned char  **aMemPtr) {

    if (aMemPtr == NULL) {
        return JVMTI_ERROR_NULL_POINTER;
    }

    /*SAPUNICODEOK_CHARTYPE*//*SAPUNICODEOK_LIBFCT*/
    *aMemPtr = (unsigned char  *)malloc((size_t)aSize);
    /*SAPUNICODEOK_CHARTYPE*/
    return (*aMemPtr == NULL) ? JVMTI_ERROR_OUT_OF_MEMORY: JVMTI_ERROR_NONE;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiDeallocate(
        jvmtiEnv        *aJvmti,            /*SAPUNICODEOK_CHARTYPE*/
        unsigned char   *aMemPtr) {
    
    free(aMemPtr);
    return JVMTI_ERROR_NONE;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiSetThreadLocalStorage(
        jvmtiEnv    *aJvmti, 
        jthread      jThread, 
        const void  *aData) {

    THR_ID_TYPE cThread;

#if defined(PROFILE_SAP_CPP)
    cThread = (long)ThrGetCurrentId();
#elif defined(PROFILE_STD_CPP)
    std::hash<std::thread::id> aHashId;
    cThread = (THR_ID_TYPE)aHashId(std::this_thread::get_id());
#endif

    THashThreads::iterator aPtr;
    void            *pData   = const_cast<void*>(aData);
    TJvmtiEnv       *aCtiJti = TJvmtiEnv::getInstance();
    THR_ERR_TYPE     cResult;
    TMonitorThread  *aThread = NULL;

    if (jThread != NULL) {
        cThread = reinterpret_cast<THR_ID_TYPE>(jThread);
    }

    aPtr = aCtiJti->mThreads.find(cThread);

    if (aPtr == aCtiJti->mThreads.end()) {
        aThread = reinterpret_cast<TMonitorThread*>(pData);    
        aPtr    = aCtiJti->mThreads.insert((jlong)cThread, aThread);
    }
    return JVMTI_ERROR_NONE;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiGetThreadLocalStorage(
        jvmtiEnv    *aJvmti, 
        jthread      jThread, 
        void       **aData) {
    
    THR_ID_TYPE cThread;
    if (aData == NULL) {
        return JVMTI_ERROR_NULL_POINTER;
    }
    *aData = NULL;

#if defined(PROFILE_SAP_CPP)
    cThread = (long)ThrGetCurrentId();
#elif defined(PROFILE_STD_CPP)
    std::hash<std::thread::id> aHashId;
    cThread = (THR_ID_TYPE)aHashId(std::this_thread::get_id());
#endif

    TJvmtiEnv       *aCtiJti    = TJvmtiEnv::getInstance();
    TMonitorThread  *aThread    = NULL;
    THashThreads::iterator aPtr;
       
    if (jThread != NULL) {
        cThread = reinterpret_cast<THR_ID_TYPE>(jThread);
    }

    aPtr = aCtiJti->mThreads.find((jlong)cThread);

    if (aPtr != aCtiJti->mThreads.end()) {
        aThread = aPtr->aValue;
        *aData  = reinterpret_cast<void *>(aThread);
    }
    return JVMTI_ERROR_NONE;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiGetThreadInfo(
        jvmtiEnv        *aJvmti, 
        jthread          jThread, 
        jvmtiThreadInfo *aInfo) {

    THR_ID_TYPE cThread;
#if defined(PROFILE_SAP_CPP)
    cThread = (long)ThrGetCurrentId();
#elif defined(PROFILE_STD_CPP)
    std::hash<std::thread::id> aHashId;
    cThread = (THR_ID_TYPE)aHashId(std::this_thread::get_id());
#endif

    if (aInfo == NULL) {
        return JVMTI_ERROR_NULL_POINTER;
    }
    aInfo->context_class_loader = NULL;
    aInfo->name                 = NULL;    
    aInfo->priority             = 0;
    aInfo->is_daemon            = 0;
    aInfo->thread_group         = NULL;
    aInfo->context_class_loader = NULL;

    TString          aThreadName(cU("NativeThread-"));
    SAP_UC           aBuffer[128];

    if (jThread != NULL) {
        cThread = reinterpret_cast<THR_ID_TYPE>(jThread);
    }
    aThreadName.concat(TString::parseInt((jlong)cThread, aBuffer));
    aInfo->name = const_cast<SAP_A7 *>(aThreadName.a7_str(true));
    return JVMTI_ERROR_NONE;
}
// -----------------------------------------------------------------
// Register method
// -----------------------------------------------------------------
extern "C" jmethodID JNICALL CtiRegisterMethod( /*SAPUNICODEOK_CHARTYPE*/
        const char      *jPackageName,          /*SAPUNICODEOK_CHARTYPE*/
        const char      *jClassName,            /*SAPUNICODEOK_CHARTYPE*/
        const char      *jMethodName,           /*SAPUNICODEOK_CHARTYPE*/
        const char      *jMethodSign) {

    TJvmtiEnv       *aCtiJti    = TJvmtiEnv::getInstance();

    TMonitorClass   *aClass     = NULL;
    TMonitorMethod  *aMethod    = NULL;
    TMemoryBit      *aMemBit    = NULL;
    jmethodID        jMethod    = NULL;
    // jclass           jClass     = NULL;

    TString          aStrPckge;
    TString          aStrClass;
    TString          aStrMethd;
    TString          aStrSigna;
    

    if (jPackageName != NULL) aStrPckge.assignR(jPackageName,     STRLEN_A7(jPackageName));
    if (jClassName   != NULL) aStrClass.assignR(jClassName,       STRLEN_A7(jClassName));
    if (jMethodName  != NULL) aStrMethd.assignR(jMethodName,      STRLEN_A7(jMethodName));
    if (jMethodSign  != NULL) aStrSigna.assignR(jMethodSign,      STRLEN_A7(jMethodSign));

    TString aStrFullClass (aStrPckge.str(), aStrClass.str());
    // TString aStrFullMethod(aStrMethd.str(), aStrSigna.str());

    THashString::iterator aStrClassPtr;

    aStrClassPtr = aCtiJti->mClassTags.find(aStrFullClass.getHash());
    if (aStrClassPtr == aCtiJti->mClassTags.end()) {
        aClass          = new TMonitorClass(aStrFullClass.str());
        aMemBit         = new TMemoryBit(aClass, sizeofR(aClass), 0);
        aStrClassPtr    = aCtiJti->mClassTags.insert(aStrFullClass.getHash(), aMemBit);
    }
    aMemBit = aStrClassPtr->aValue;        
    aClass  = aMemBit->mCtx;
    // jClass  = reinterpret_cast<jclass>(aClass);

    aMethod = new TMonitorMethod(aStrMethd.str(), aStrSigna.str(), aClass, aClass->getName());
    jMethod = reinterpret_cast<jmethodID>(aMethod);

    TMonitor::getInstance()->onClassRegister(aCtiJti->mCtiJvmti, aClass, aMethod, aMemBit);
    return jMethod;
}

// -----------------------------------------------------------------
// Register method arguments
// -----------------------------------------------------------------
extern "C" jfieldID JNICALL CtiRegisterField(
        jmethodID        jMethod,               /*SAPUNICODEOK_CHARTYPE*/
        const char      *jFieldName,            /*SAPUNICODEOK_CHARTYPE*/
        const char      *jFieldSign) {
    jfieldID jFieldID = NULL;
    return jFieldID;
}

// -----------------------------------------------------------------
// Enter method event
// -----------------------------------------------------------------
extern "C" jint JNICALL CtiOnEnterMethod(
    jmethodID jMethod) {

    TJvmtiEnv       *aCtiJti = TJvmtiEnv::getInstance();
    jvmtiError       jResult = JVMTI_ERROR_NONE;
    TMonitorThread  *aThread = NULL;
    // jthread          jThread = NULL;

    jResult = aCtiJti->mCtiJvmti->GetThreadLocalStorage(NULL, (void **)&aThread);
    if (aThread == NULL) {
        SAP_UC      aBuffer[128];
        TString     aThreadName(cU("NativeThread-"));
        
        THR_ID_TYPE cThread = 0;

#ifdef PROILE_SAP_CPP        
        cThread = ThrGetCurrentId();
#endif
        aThreadName.concat(TString::parseInt((int)cThread, aBuffer));
        aThread = new TMonitorThread(aCtiJti->mCtiJvmti, aCtiJti->mCtiJni, NULL, aThreadName.str());
        jResult = aCtiJti->mCtiJvmti->SetThreadLocalStorage(NULL, (void *)aThread);
    }
    TMonitorMethod *aMethod = reinterpret_cast<TMonitorMethod *>(jMethod);
    TMonitor::getInstance()->onMethodEnter(aCtiJti->mCtiJvmti, aCtiJti->mCtiJni, NULL, jMethod, aMethod, aThread);

    return JNI_OK;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CtiOnExitMethod(
        jmethodID    jMethod) {

    jvmtiError       jResult;

    TJvmtiEnv       *aCtiJti      = TJvmtiEnv::getInstance();
    TMonitorThread  *aThread;

    jResult = aCtiJti->mCtiJvmti->GetThreadLocalStorage(NULL, (void **)&aThread);
    //-- TMonitorMethod *aMethod = reinterpret_cast<TMonitorMethod *>(jMethod);
    TMonitor::getInstance()->onMethodExit(aCtiJti->mCtiJvmti, aCtiJti->mCtiJni, NULL, jMethod, aThread);

    return JNI_OK;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiGetTag(
        jvmtiEnv        *aJvmti, 
        jobject          jObject, 
        jlong           *jTag) {

    THashObj *aObject = reinterpret_cast<THashObj *>(jObject);
    *jTag = aObject->getTag();
    return JVMTI_ERROR_NONE;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiSetTag(
        jvmtiEnv        *aJvmti, 
        jobject          jObject, 
        jlong            jTag) {
    THashObj *aObject = reinterpret_cast<THashObj *>(jObject);
    aObject->setTag(jTag);
    return JVMTI_ERROR_NONE;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CtiRegisterObject(/*SAPUNICODEOK_CHARTYPE*/
        void            *aMem,
        jlong           aSize, 
        jmethodID       jMethod, 
        jlong           jLocation) {

    THashObjects::iterator aObjPtr  = NULL;
    TJvmtiEnv       *aCtiJti        = TJvmtiEnv::getInstance();
    TObject         *aObject;
    TMonitorClass   *aClass;
    TMonitor        *aMonitor       = TMonitor::getInstance();
    TMonitorMethod  *aMethod        = aMonitor->findMethod(aCtiJti->mCtiJvmti, jMethod);
    jvmtiError       jResult        = JVMTI_ERROR_NONE;
    TMemoryBit      *aMemBit;
    THashString::iterator aStrClassPtr;

    // Create a class for each location
    TString aClassName(cU("alloc"), aMethod->getFullName(), jLocation);

    aCtiJti->mLockAccess.enter();
    aStrClassPtr = aCtiJti->mClassTags.find(aClassName.getHash());
    if (aStrClassPtr == aCtiJti->mClassTags.end()) {
        aClass          = new TMonitorClass(aClassName.str());
        aMemBit         = new TMemoryBit(aClass, 0, 0);
        aStrClassPtr    = aCtiJti->mClassTags.insert(aClassName.getHash(), aMemBit);
        
        /*SAPUNICODEOK_CHARTYPE*/
        aMethod = new TMonitorMethod(cU("<init>"), cU("()V"), aClass, aClass->getName());
        aClass->setConstructor(reinterpret_cast<jmethodID>(aMethod));
        aMonitor->onClassRegister(aCtiJti->mCtiJvmti, aClass, aMethod, aMemBit);

        /*SAPUNICODEOK_CHARTYPE*/
        aMethod = new TMonitorMethod(cU("finalize"), cU("()V"), aClass, aClass->getName());
        aClass->setFinalizer(reinterpret_cast<jmethodID>(aMethod));
        aMonitor->onClassRegister(aCtiJti->mCtiJvmti, aClass, aMethod, aMemBit);
    }

    // Create an instance for a class
    aObjPtr  = aCtiJti->mObjects.find((jlong)(aMem));
    if (aObjPtr != aCtiJti->mObjects.end()) {
        aObject = aObjPtr->aValue;
        CtiGetTag(aCtiJti->mCtiJvmti, (jobject)aObject, (jlong*)&aMemBit);

        ERROR_OUT(cU("CtiOnObjectAlloc: instrumentation error"), 0);
        aCtiJti->setEventMode(JVMTI_DISABLE, JVMTI_EVENT_VM_OBJECT_ALLOC);
        aMonitor->onObjectDelete(aCtiJti->mCtiJvmti, (jlong)aMemBit);        
        delete aObject;
    }
    aClass   = aStrClassPtr->aValue->mCtx;
    aObject  = new TObject((jclass)aClass);
    aObjPtr  = aCtiJti->mObjects.insert((jlong)(aMem), aObject);
    aCtiJti->mLockAccess.exit();

    aMethod  = reinterpret_cast<TMonitorMethod *>(aClass->getConstructor());
    aMonitor->onMethodEnter(aCtiJti->mCtiJvmti, aCtiJti->mCtiJni, NULL, NULL, aMethod, NULL);
    aMonitor->onObjectAlloc(
                aCtiJti->mCtiJvmti,
                aCtiJti->mCtiJni,
                NULL,                               // Thread
                reinterpret_cast<jobject>(aObject), // Object
                NULL,                               // Class 
                aSize);
    aMonitor->onMethodExit(aCtiJti->mCtiJvmti, aCtiJti->mCtiJni, NULL, aMethod->getID());
    return JNI_OK;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CtiOnObjectCalloc(
        jlong           aCount,
        jlong           aSize, 
        jmethodID       jMethod, 
        jlong           jLocation,     /*SAPUNICODEOK_CHARTYPE*/
        unsigned char **aMem) {

    TJvmtiEnv       *aCtiJti        = TJvmtiEnv::getInstance();
    jvmtiError       jResult        = JVMTI_ERROR_NONE;
  
    if ((jResult = CtiCallocate(aCtiJti->mCtiJvmti, aCount, aSize, aMem)) != JVMTI_ERROR_NONE) {
        return jResult;
    }
    
    if (aCtiJti->getEventMode(JVMTI_EVENT_VM_OBJECT_ALLOC) == JVMTI_DISABLE) {
        return jResult;
    }
    return CtiRegisterObject(aMem, aSize, jMethod, jLocation);
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CtiOnObjectAlloc(
        jlong           aSize, 
        jmethodID       jMethod, 
        jlong           jLocation,     /*SAPUNICODEOK_CHARTYPE*/
        unsigned char **aMem) {

    TJvmtiEnv       *aCtiJti        = TJvmtiEnv::getInstance();
    jvmtiError       jResult        = JVMTI_ERROR_NONE;
  
    if ((jResult = CtiAllocate(aCtiJti->mCtiJvmti, aSize, aMem)) != JVMTI_ERROR_NONE) {
        return jResult;
    }
    
    if (aCtiJti->getEventMode(JVMTI_EVENT_VM_OBJECT_ALLOC) == JVMTI_DISABLE) {
        return jResult;
    }
    return CtiRegisterObject(aMem, aSize, jMethod, jLocation);
}


// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CtiOnObjectRealloc(
        jlong           aSize, 
        jmethodID       jMethod, 
        jlong           jLocation,     /*SAPUNICODEOK_CHARTYPE*/
        unsigned char **aMem) {

    /*SAPUNICODEOK_CHARTYPE*/
    unsigned char   *aOldMem    = *aMem;
    TJvmtiEnv       *aCtiJti    = TJvmtiEnv::getInstance();
    TMonitor        *aMonitor   = TMonitor::getInstance();
    TObject         *aObject    = NULL;
    jvmtiError       jResult    = JVMTI_ERROR_NONE;
    THashObjects::iterator aPtr;

    if (aCtiJti->getEventMode(JVMTI_EVENT_VM_OBJECT_ALLOC) == JVMTI_DISABLE) {
        /*SAPUNICODEOK_CHARTYPE*//*SAPUNICODEOK_LIBFCT*/
        *aMem   = (unsigned char *)realloc(*aMem, aSize);
        return jResult;
    }

    if (*aMem == NULL) {
        return CtiOnObjectAlloc(aSize, jMethod, jLocation, aMem);
    }
    /*SAPUNICODEOK_CHARTYPE*//*SAPUNICODEOK_LIBFCT*/
    *aMem   = (unsigned char *)realloc(*aMem, aSize);

    aCtiJti->mLockAccess.enter();
    if ((aPtr = aCtiJti->mObjects.move((jlong)aOldMem, (jlong)*aMem, 0)) != aCtiJti->mObjects.end()) {
        aObject = aPtr->aValue;
    }
    aCtiJti->mLockAccess.exit();

    if (aObject != NULL &&  aCtiJti->getEventMode(JVMTI_EVENT_VM_OBJECT_ALLOC) == JVMTI_ENABLE) {
        aMonitor->doObjectRealloc(aCtiJti->mCtiJvmti, aCtiJti->mCtiJni, NULL, (jobject)aObject, aSize);        
    }
    return JNI_OK;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CtiUnregisterObject(
        void *aMem) {
    TJvmtiEnv       *aCtiJti   = TJvmtiEnv::getInstance();
    TMonitor        *aMonitor  = TMonitor::getInstance();
    TMonitorMethod  *aMethod   = NULL;
    TMemoryBit      *aMemBit;
    TObject         *aObject   = NULL;
    jvmtiError       jResult   = JVMTI_ERROR_NONE;
    THashObjects::iterator aPtr;
    

    if (aCtiJti->getEventMode(JVMTI_EVENT_VM_OBJECT_ALLOC) == JVMTI_DISABLE) {
        return jResult;
    }

    aCtiJti->mLockAccess.enter();
    if ((aPtr = aCtiJti->mObjects.remove((jlong)aMem)) != aCtiJti->mObjects.end()) {
        aObject = aPtr->aValue;
    }
    aCtiJti->mLockAccess.exit();

    if (aObject != NULL) {
        CtiGetTag(aCtiJti->mCtiJvmti, (jobject)aObject, (jlong*)&aMemBit);

        aMethod  = reinterpret_cast<TMonitorMethod *>(((TMonitorClass *)aObject->getClass())->getFinalizer());
        aMonitor->onMethodEnter(aCtiJti->mCtiJvmti, aCtiJti->mCtiJni, NULL, NULL, aMethod);
        aMonitor->onObjectDelete(aCtiJti->mCtiJvmti, (jlong)aMemBit);        
        aMonitor->onMethodExit(aCtiJti->mCtiJvmti, aCtiJti->mCtiJni, NULL, aMethod->getID());
        delete aObject;
    }
    return JNI_OK;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CtiOnObjectFree(
        /*SAPUNICODEOK_CHARTYPE*/unsigned char *aMem) {

    CtiUnregisterObject(aMem);
    free(aMem);
    return JNI_OK;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CtiGetEnv(
        JavaVM      *aJvm, 
        void       **aEnv, 
        jint         aVersion) {
    
    TJvmtiEnv  *aCtiJti = TJvmtiEnv::getInstance();

    *aEnv = NULL;

    switch (aVersion) {
        case JVMTI_VERSION_1_0:
            *aEnv = aCtiJti->mCtiJvmti;
            return JNI_OK;
        case JNI_VERSION_1_2:
            *aEnv = aCtiJti->mCtiJni;
            return JNI_OK;
    }
    return JNI_EVERSION;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiGetObjectHashCode(
        jvmtiEnv    *jJvmti, 
        jobject      jObject, 
        jint        *aHash) {
    *aHash = (jint)((jlong)(jObject) | 0xFF);
    return JVMTI_ERROR_NONE;
}
// -----------------------------------------------------------------
// Wrapper to translate JVM thread function call to Thr syntax
// including a call to ThrExit at the end of an thread.
// -----------------------------------------------------------------
extern "C" unsigned long CtiThreadFunction(
        void    *aThreadFunction) {

    unsigned long        aExitCode  = 0;
    jthread              jThread    = NULL;
    
#ifdef PROFILE_SAP_CPP
    THR_ID_TYPE          cThread    = GetCurrentThreadId();
    TJvmtiEnv           *aCtiJti    = TJvmtiEnv::getInstance();
    jvmtiStartFunction  jThreadFunction;
    TMonitor            *aMonitor   = TMonitor::getInstance();

    jThreadFunction = (jvmtiStartFunction)(aThreadFunction);
    jThread         = reinterpret_cast<jthread>(cThread);

    // aMonitor->onThreadStart(aCtiJti->mCtiJvmti, aCtiJti->mCtiJni, jThread);
    jThreadFunction(aCtiJti->mCtiJvmti, aCtiJti->mCtiJni, NULL);
    // aMonitor->onThreadEnd(aCtiJti->mCtiJvmti, aCtiJti->mCtiJni, jThread);
#endif
    return 0;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError CtiRunAgentThread(
        jthread              jThread,
        jvmtiStartFunction   jThreadFunction,
        const void          *aArgs,
        jint                 aPriority) {

#   if defined(PROFILE_SAP_CPP)
        THR_ERR_TYPE aResult;
        THR_ATTRIB   aThrAttr;
        THR_ID_TYPE  cThread;

        aThrAttr.scope              = THR_SCOPE_DEFAULT;
        aThrAttr.detachedstate      = THR_DETACHEDSTATE_DEFAULT;
        aThrAttr.stacksize          = THR_STACKSIZE_DEFAULT;
        aResult                     = ThrCreate2(&aThrAttr, CtiThreadFunction, (void*)jThreadFunction);
#   else if defined(PROFILE_STD_CPP)
        jThread = NULL;
        TJvmtiEnv  *aCtiJti = TJvmtiEnv::getInstance();
        std::thread aThread(CtiThreadFunction, (void*)jThreadFunction);
#   endif    
    return JVMTI_ERROR_NONE;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiAddCapabilities(
        jvmtiEnv                *aEnv, 
        const jvmtiCapabilities *aCapabilities) {
    return JVMTI_ERROR_NONE;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CtiGetJavaVm(JNIEnv *aJni, JavaVM **aJvm) {
    *aJvm = TJvmtiEnv::getInstance()->mCtiJavaVM;
    return JNI_OK;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiCreateRawMonitor(
        jvmtiEnv        *aJvmti, /*SAPUNICODEOK_CHARTYPE*/
        const char      *aName,
        jrawMonitorID   *jMonitor) {

    CtiMonitor *aMonitor = new CtiMonitor(aName);
   *jMonitor = reinterpret_cast<jrawMonitorID>(aMonitor);

    return JVMTI_ERROR_NONE;  
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiDestroyRawMonitor(
        jvmtiEnv        *aJvmti, 
        jrawMonitorID    jMonitor) {
    
    CtiMonitor *aMonitor = reinterpret_cast<CtiMonitor *>(jMonitor);
    delete aMonitor;
    return JVMTI_ERROR_NONE;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiRawMonitorEnter(
        jvmtiEnv        *aJvmti,
        jrawMonitorID    jMonitor) {

    CtiMonitor *aMonitor = reinterpret_cast<CtiMonitor *>(jMonitor);
    aMonitor->enter();
    return JVMTI_ERROR_NONE;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiRawMonitorExit(
        jvmtiEnv        *aJvmti,
        jrawMonitorID    jMonitor) {

    CtiMonitor *aMonitor = reinterpret_cast<CtiMonitor *>(jMonitor);
    aMonitor->exit();
    return JVMTI_ERROR_NONE;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiRawMonitorWait(
        jvmtiEnv        *aJvmti, 
        jrawMonitorID    jMonitor, 
        jlong            aTime) {

    CtiMonitor *aMonitor = reinterpret_cast<CtiMonitor *>(jMonitor);
    aMonitor->wait(aTime);
    return JVMTI_ERROR_NONE;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiRawMonitorNotifyAll(
        jvmtiEnv        *aJvmti, 
        jrawMonitorID    jMonitor) {

    CtiMonitor *aMonitor = reinterpret_cast<CtiMonitor *>(jMonitor);
    aMonitor->notify();
    return JVMTI_ERROR_NONE;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CtiAttachCurrentThread(
        JavaVM      *aJvm, 
        void       **aJniEnv, 
        void        *args) {
    return JNI_OK;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CtiDetachCurrentThread(
        JavaVM      *aJavaVM) {
    return JNI_OK;    
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiGetCurrentThreadCpuTime(
        jvmtiEnv    *aJvmti, 
        jlong       *aTime) {
    return JVMTI_ERROR_ABSENT_INFORMATION;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiGetAllThreads(
        jvmtiEnv    *aJvmti, 
        jint        *aCnt, 
        jthread    **jThreads) {

    jint        i;
    TJvmtiEnv  *aCtiJti    = TJvmtiEnv::getInstance();
    THashThreads::iterator aPtr;

    *aCnt = (jint)aCtiJti->mThreads.getSize();
    if (*aCnt == 0) {
        *jThreads = NULL;
        return JVMTI_ERROR_NONE;
    }

    /*SAPUNICODEOK_CHARTYPE*/
    CtiAllocate(aJvmti, (*aCnt) * sizeofR(jthread), (unsigned char **)jThreads);
    
    for (aPtr  = aCtiJti->mThreads.begin(), i = 0;
         aPtr != aCtiJti->mThreads.end();
         aPtr  = aCtiJti->mThreads.next(),  i ++) {
             (*jThreads)[i] = reinterpret_cast<jthread>(aPtr->aKey);
    }
    return JVMTI_ERROR_NONE;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiSetEventCallbacks(
        jvmtiEnv    *aJvmti, 
        const jvmtiEventCallbacks* aCallbacks, 
        jint         aSize) {

    TJvmtiEnv  *aCtiJti = TJvmtiEnv::getInstance();
    aCtiJti->mEventCallbacks = const_cast<jvmtiEventCallbacks *>(aCallbacks);
    return JVMTI_ERROR_NONE;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiSetEventNotificationMode(
        jvmtiEnv        *aJvmti,
        jvmtiEventMode   aMode, 
        jvmtiEvent       aEvent, 
        jthread          jThread, 
        ...) {
    
    TJvmtiEnv  *aCtiJti = TJvmtiEnv::getInstance();
    aCtiJti->setEventMode(aMode,aEvent);
    return JVMTI_ERROR_NONE;
}

extern "C" void JNICALL onGcStart (jvmtiEnv *);
extern "C" void JNICALL onGcFinish(jvmtiEnv *);
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiForceGarbageCollection(
        jvmtiEnv        *aJvmti) {
    onGcStart(aJvmti);
    onGcFinish(aJvmti);
    return JVMTI_ERROR_NONE;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiCommand(
    const SAP_UC    *aCommand,
    SAP_UC          *aXmlResult) {
    // TCommand *aCmdInterf = TCommand::getInstance();

    //aCmdInterf->parse(aCommand);
    //aCmdInterf->execute(aJvmti, aJni, NULL);
    return JVMTI_ERROR_NONE;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiIterateOverHeap(
    jvmtiEnv                *aEnv, 
    jvmtiHeapObjectFilter    aObjectFilter, 
    jvmtiHeapObjectCallback  aHeapObjectCallback, 
    const void              *aUserData) {

    TJvmtiEnv               *aCtiJti = TJvmtiEnv::getInstance();
    THashObjects::iterator   aPtr;
    jvmtiIterationControl    aCtrl;
    TMonitorClass           *aClass;
    TObject                 *aObject;
    TMemoryBit              *MemBit;

    if (aHeapObjectCallback == NULL) {
        return JVMTI_ERROR_NONE;
    }

    for (aPtr  = aCtiJti->mObjects.begin(); 
         aPtr != aCtiJti->mObjects.end();
         aPtr  = aCtiJti->mObjects.next()) {

        aObject = aPtr->aValue;

        aClass  = (TMonitorClass *)aObject->getClass();
        CtiGetTag(aEnv, (jobject)aObject, (jlong *)&MemBit);

        aCtrl   = aHeapObjectCallback((jlong)aClass->getTag(), MemBit->mSize, (jlong*)&MemBit, (void*)aUserData);

        if (aCtrl != JVMTI_ITERATION_CONTINUE) {
            break;
        }
    }
    return JVMTI_ERROR_NONE;
}

// ---------------------------------------------------------
// TMonitor:HeapCallback
//! \brief Callback for the heap runner
//! \param  aClassTag The hash to the class
//! \param  aSize The size of the class
//! \param  aTag  The hash to the associated memory
//! \param  aUserData Optional user data of heap runner
//! \return Operation control: JVMTI_ITERATION_CONTINUE
// ---------------------------------------------------------
extern "C" jvmtiIterationControl JNICALL TMonitorHeapCallback(
        jlong        aClassTag, 
        jlong        aSize, 
        jlong       *aTag, 
        void        *aUserData)  {

    TMemoryBit     *aMemBit;
    TMonitor       *aMonitor = TMonitor::getInstance();
    THashClasses::iterator aPtr;

    if (aTag == NULL || *aTag == 0) {
        return JVMTI_ITERATION_CONTINUE;
    }

    aMemBit = (TMemoryBit *)(*aTag);
    if (aUserData == NULL || (jlong)aUserData == aMemBit->mCtx->getID()) {
        
        // TMonitorLock aLockAccess(aMonitor->mRawMonitorAccess, true, false);
        if (aMonitor->getState() == MONITOR_ACTIVE && aMemBit->mTID != aMonitor->getTransaction()) {
            return JVMTI_ITERATION_CONTINUE;
        }

        aPtr = aMonitor->mClasses.find(aClassTag);
        if (aPtr != aMonitor->mClasses.end()) {
            aPtr->aValue->incHeapCount(aMemBit->mSize);
        }
    }
    return JVMTI_ITERATION_CONTINUE;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jclass JNICALL CtiGetObjectClass(
    JNIEnv                  *aEnv, 
    jobject                  jObject) {

    THashObj  *aObject;    
    aObject = reinterpret_cast<THashObj *>(jObject);
    return aObject->getClass();
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jvmtiError JNICALL CtiGetFrameCount(
    jvmtiEnv                *aEnv, 
    jthread                  jThread,
    jint                    *jCounter) {

    return JVMTI_ERROR_NONE;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jclass JNICALL CtiFindClass(
    JNIEnv                  *aEnv,          /*SAPUNICODEOK_CHARTYPE*/
    const char              *name) {

    return NULL;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jthrowable JNICALL CtiExceptionOccurred(
    JNIEnv                  *aEnv) {
    return NULL;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" void JNICALL CtiExceptionDescribe(
    JNIEnv                  *aEnv) {
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" void JNICALL CtiExceptionClear(
    JNIEnv                  *aEnv) {
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CtiRegisterCallback(TCtiCallback aCallback) {
    TConsole::getInstance()->setTraceCallback(aCallback);
    return JVMTI_ERROR_NONE;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
TJvmtiEnv *TJvmtiEnv::mInstance = NULL;
extern "C" void JNICALL onVmInit (jvmtiEnv *, JNIEnv *, jthread);
extern "C" void JNICALL onVmDeath(jvmtiEnv *, JNIEnv *);
extern "C" void JNICALL doTelnetThread (jvmtiEnv *, JNIEnv *, void *);
extern "C" void JNICALL doRepeatThread (jvmtiEnv *, JNIEnv *, void *);

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CtiVmDeath() {
    TJvmtiEnv       *aCtiJti = TJvmtiEnv::getInstance();
    onVmDeath(aCtiJti->mCtiJvmti, aCtiJti->mCtiJni);
    return JVMTI_ERROR_NONE;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" JNIEXPORT jint JNICALL CtiAgentOnLoad(     /*SAPUNICODEOK_CHARTYPE*/
        const char      *aOptions, 
        TCtiInterface  **pCtiEnv, 
        jint             aVersion)  {

    TJvmtiEnv *aCtiJvti = TJvmtiEnv::getInstance();    

    if (aCtiJvti->mCtiJvmti == NULL) {
#       ifdef PROFILE_CPP
            THR_ERR_TYPE cResult = ThrProcInit();
            if (cResult != THR_ERR_OK) {
                ERROR_OUT(cU("CtiAgentOnLoad: Thread-Library"), 0);
                return JNI_OK;
            }
#       endif
        // Check versions, if there is more than one interface ...
        if (aVersion != CTI_VERSION_1) {
            return JNI_ERR;
        }
        *pCtiEnv = new TCtiInterface;

        JVMTInterface       *aJtiFunctions      = new JVMTInterface;
        JNIInvokeInterface_ *aJvmFunctions      = new JNIInvokeInterface_;
        JNINativeInterface_ *aJniFunctions      = new JNINativeInterface_;

        aCtiJvti->mCtiJavaVM                    = new JavaVM;
        aCtiJvti->mCtiJvmti                     = new jvmtiEnv;
        aCtiJvti->mCtiJni                       = new JNIEnv;

        aCtiJvti->mCtiJavaVM->functions         = aJvmFunctions;
        aCtiJvti->mCtiJvmti->functions          = aJtiFunctions;
        aCtiJvti->mCtiJni->functions            = aJniFunctions;

        aJvmFunctions->AttachCurrentThread      = CtiAttachCurrentThread;
        aJvmFunctions->DetachCurrentThread      = CtiDetachCurrentThread;
        aJvmFunctions->GetEnv                   = CtiGetEnv;

        aJtiFunctions->AddCapabilities          = CtiAddCapabilities;
        aJtiFunctions->SetEventCallbacks        = CtiSetEventCallbacks;
        aJtiFunctions->GetObjectHashCode        = CtiGetObjectHashCode;

        aJtiFunctions->CreateRawMonitor         = CtiCreateRawMonitor;
        aJtiFunctions->DestroyRawMonitor        = CtiDestroyRawMonitor;
        aJtiFunctions->RawMonitorEnter          = CtiRawMonitorEnter;
        aJtiFunctions->RawMonitorExit           = CtiRawMonitorExit;
        aJtiFunctions->RawMonitorWait           = CtiRawMonitorWait;
        aJtiFunctions->RawMonitorNotifyAll      = CtiRawMonitorNotifyAll;
        
        aJtiFunctions->Allocate                 = CtiAllocate;
        aJtiFunctions->Deallocate               = CtiDeallocate;

        aJtiFunctions->GetTag                   = CtiGetTag;
        aJtiFunctions->SetTag                   = CtiSetTag;

        aJtiFunctions->GetThreadInfo            = CtiGetThreadInfo;
        aJtiFunctions->GetAllThreads            = CtiGetAllThreads;
        aJtiFunctions->GetThreadLocalStorage    = CtiGetThreadLocalStorage;
        aJtiFunctions->SetThreadLocalStorage    = CtiSetThreadLocalStorage;
        aJtiFunctions->GetCurrentThreadCpuTime  = CtiGetCurrentThreadCpuTime;
        aJtiFunctions->SetEventNotificationMode = CtiSetEventNotificationMode;
        aJtiFunctions->ForceGarbageCollection   = CtiForceGarbageCollection;
        aJtiFunctions->IterateOverHeap          = CtiIterateOverHeap;
        aJtiFunctions->GetFrameCount            = CtiGetFrameCount;

        aJniFunctions->GetObjectClass           = CtiGetObjectClass;
        aJniFunctions->FindClass                = CtiFindClass;
        aJniFunctions->ExceptionDescribe        = CtiExceptionDescribe;
        aJniFunctions->ExceptionClear           = CtiExceptionClear;
        aJniFunctions->ExceptionOccurred        = CtiExceptionOccurred;
        

        /*SAPUNICODEOK_CHARTYPE*/
        Agent_OnLoad(aCtiJvti->mCtiJavaVM, const_cast<char*>(aOptions), NULL);

        onVmInit(aCtiJvti->mCtiJvmti, NULL, NULL);

        CtiRunAgentThread(NULL, doTelnetThread, NULL, 0);
        CtiRunAgentThread(NULL, doRepeatThread, NULL, 0);
    }

    (*pCtiEnv)->mVersion           = aVersion;
    (*pCtiEnv)->doCommand          = aCtiJvti->mCtiEnv->doCommand;
    (*pCtiEnv)->registerMethod     = aCtiJvti->mCtiEnv->registerMethod;
    (*pCtiEnv)->registerField      = aCtiJvti->mCtiEnv->registerField;
    (*pCtiEnv)->onEnterMethod      = aCtiJvti->mCtiEnv->onEnterMethod;
    (*pCtiEnv)->onExitMethod       = aCtiJvti->mCtiEnv->onExitMethod;
    (*pCtiEnv)->onException        = aCtiJvti->mCtiEnv->onException;
    (*pCtiEnv)->onExceptionCatch   = aCtiJvti->mCtiEnv->onExceptionCatch;
    (*pCtiEnv)->onObjectAlloc      = aCtiJvti->mCtiEnv->onObjectAlloc;
    (*pCtiEnv)->onObjectFree       = aCtiJvti->mCtiEnv->onObjectFree;
    (*pCtiEnv)->onObjectRealloc    = aCtiJvti->mCtiEnv->onObjectRealloc;
    (*pCtiEnv)->onVMDeath          = aCtiJvti->mCtiEnv->onVMDeath;

    return JNI_OK;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
jint TJvmtiEnv::onAgentLoad(
        JavaVM      *aJavaVM, 
        jvmtiEnv    *aJvmti) {

    jint jResult = JNI_OK;
    JVMTInterface       *aJtiFunctions;
    JNIInvokeInterface_ *aJvmFunctions;

    if (mCtiJavaVM == NULL) {
        mCtiJavaVM  = aJavaVM;
        mCtiJvmti   = aJvmti;
    }
    else {
        aJvmFunctions = const_cast<JNIInvokeInterface_ *>(mCtiJavaVM->functions);
        //aJvmFunctions->AttachCurrentThread     = aJavaVM->functions->AttachCurrentThread;
        //aJvmFunctions->DetachCurrentThread     = aJavaVM->functions->DetachCurrentThread;
        //aJvmFunctions->GetEnv                  = aJavaVM->functions->GetEnv;

        aJtiFunctions = const_cast<JVMTInterface *>(aJvmti->functions);
        aJtiFunctions->CreateRawMonitor        = mCtiJvmti->functions->CreateRawMonitor;
        aJtiFunctions->DestroyRawMonitor       = mCtiJvmti->functions->DestroyRawMonitor;
        aJtiFunctions->RawMonitorEnter         = mCtiJvmti->functions->RawMonitorEnter;
        aJtiFunctions->RawMonitorExit          = mCtiJvmti->functions->RawMonitorExit;
        aJtiFunctions->RawMonitorWait          = mCtiJvmti->functions->RawMonitorWait;        
        aJtiFunctions->RawMonitorNotifyAll     = mCtiJvmti->functions->RawMonitorNotifyAll; 
    }
    mCtiEnv->onEnterMethod    = CtiOnEnterMethod;
    mCtiEnv->onExitMethod     = CtiOnExitMethod;
    mCtiEnv->onObjectAlloc    = CtiOnObjectAlloc;
    mCtiEnv->onObjectCalloc   = CtiOnObjectCalloc;
    mCtiEnv->onObjectRealloc  = CtiOnObjectRealloc;
    mCtiEnv->onObjectFree     = CtiOnObjectFree;
    mCtiEnv->registerObject   = CtiRegisterObject;
    mCtiEnv->unregisterObject = CtiUnregisterObject;
    mCtiEnv->registerMethod   = CtiRegisterMethod;
    mCtiEnv->registerField    = CtiRegisterField;
    mCtiEnv->registerCallback = CtiRegisterCallback;
    mCtiEnv->onVMDeath        = CtiVmDeath;

    if (mCtiJvmti != aJvmti) {
        TProperties::getInstance()->setJvmti(aJvmti);
    }
    return jResult;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
TJvmtiEnv::TJvmtiEnv(): 
        mThreads(1023),
        /*SAPUNICODEOK_STRINGCONST*/
        mLockAccess("CtiLock")
{
    mCtiJavaVM      = NULL;
    mCtiJvmti       = NULL;
    mCtiJni         = NULL;
    mCtiEnv         = new TCtiInterface();
    mEventCallbacks = NULL;
    /*SAPUNICODEOK_SIZEOF*/
    memsetR(mEventSettings, 0, sizeof(mEventSettings));
};

// -----------------------------------------------------------------
// -----------------------------------------------------------------
void TJvmtiEnv::setEventMode(jvmtiEventMode aMode, jvmtiEvent aEvent) {
    if (aEvent > 99) 
      return;

    mEventSettings[aEvent] = aMode;

    if (aEvent == JVMTI_EVENT_VM_OBJECT_ALLOC && aMode == JVMTI_ENABLE) {
        mObjects.reset();
    }
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
jvmtiEventMode TJvmtiEnv::getEventMode(jvmtiEvent aEvent) {
    if (aEvent > 99) 
        return (jvmtiEventMode)0;

    return mEventSettings[aEvent];
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
TJvmtiEnv *TJvmtiEnv::getInstance() {
    if (mInstance == NULL) {
        mInstance = new TJvmtiEnv;        
    }
    return mInstance;
}



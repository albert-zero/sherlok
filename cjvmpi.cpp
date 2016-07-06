// -----------------------------------------------------------------
//
// Author: Albert Rossmann
// File  : cjvmdi.cpp
// Date  : 27.08.2008
// Abstract:
//    Profiler debug interface for C,C++
// -----------------------------------------------------------------
#include "jni.h"

#ifndef JNI_VERSION_1_6
#include "cti.h"
#include "cjvmpi.h"
#include "standard.h"

// -----------------------------------------------------------------
// -----------------------------------------------------------------
typedef THash <THR_ID_TYPE, THR_KEY_TYPE> THashThreads;

static JNIEnv                CTIJniEnv;
static JNINativeInterface_   CTIJniEnvFunctions;

#ifndef JNI_VERSION_1_6
static JVMPI_Interface      *CTIJvmpi   = NULL;
#endif 

static JavaVM               *CTIJavaVm  = NULL;
static JNIInvokeInterface_   CTIJavaVmFunctions;

static THashThreads          CTIHashThreads(1023);
static jint                  CTIRequestEvent = 0;
static TCtiInterface         CTIEnv;
// static CTICallback           CTIRegisterEnv  = NULL;

#ifdef SAPonNT
    SYSTEMTIME      TSystem::mSystemTime;
    LARGE_INTEGER   TSystem::mHpcTime;
    LARGE_INTEGER   TSystem::mHpcStartTime;
    LARGE_INTEGER   TSystem::mHpcFrequence;
#endif

bool            TSystem::mHasHpcTimer = false;
long double     TSystem::mScale       = (double)1000000 / (double)CLOCKS_PER_SEC;
SAP_UC          TSystem::mBuffer[128];
jlong           TSystem::mOffset      = 0;

#ifndef PROFILE_CPP
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" JNIEXPORT jint JNICALL JvmtiInit(
        SAP_BOOL         aRunModeJni,
        CTICallback      aSetEnv,
        SAP_UC          *aOptions) {
    return JNI_OK;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CTIOnLoad(
        JavaVM          *aJavaVM,
        JVMPI_Interface *aJvmpi) {
    return JNI_OK;
}
#else
struct _jmethodID {};
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL JVM_OnLoad(
        JavaVM      *aJvm, 
        SAP_A7      *aOptions, 
        void        *);

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" unsigned long THR_API CTIWrapper(
        void        *aFunc) {

    JVMPI_Event     jEvent;
    JNIEnv         *jEnv;
    THR_ERR_TYPE    aReturn;
    unsigned long   aExitCode;
    THR_ID_TYPE     aThr                = ThrGetCurrentId();
    void            (*jFunc)(void *)    = (void (*)(void *))aFunc;
    TString         aThrName;
    SAP_UC          aBuffer[THR_ID_STRMAXLEN];

    aThrName.concat(ThrId2Str(aBuffer, aThr));
    CTIJavaVm->AttachCurrentThread(reinterpret_cast<void **>(&jEnv), NULL);

    jEvent.env_id                       = jEnv;
    jEvent.event_type                   = JVMPI_EVENT_THREAD_START;
    jEvent.u.thread_start.thread_name   = const_cast<SAP_A7*>(aThrName.a7_str());
    jEvent.u.thread_start.group_name    = const_cast<SAP_A7*>(cR(""));
    jEvent.u.thread_start.parent_name   = const_cast<SAP_A7*>(cR(""));
    jEvent.u.thread_start.thread_id     = reinterpret_cast<jobjectID>(aThr);
    jEvent.u.thread_start.thread_env_id = jEnv;
    CTIJvmpi->NotifyEvent(&jEvent);

    jFunc(NULL);

    jEvent.env_id     = jEnv;
    jEvent.event_type = JVMPI_EVENT_THREAD_END;
    CTIJvmpi->NotifyEvent(&jEvent);

    aReturn = ThrExitCode(aThr, &aExitCode);
    ThrExit(aExitCode);
    return 0;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint CTIEnableEvent(
        jint         aEventType, 
        void        *aArg) {

    CTIRequestEvent |= (1 << (aEventType-1));
    return JVMPI_SUCCESS;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint CTIDisableEvent(
        jint         aEventType, 
        void        *aArg) {

    CTIRequestEvent &= ~(1 << (aEventType-1));
    return JVMPI_SUCCESS;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CTIDestroyJavaVM(
        JavaVM      *vm) {

    ERROR_OUT(cU("not implemented"), -1);
    return JNI_OK;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jlong CTIGetCurrentThreadCpuTime(void) {
    return 0;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" JNIEXPORT jint JNICALL CTIGetEnv(
        JavaVM      *aJavaVm, 
        void       **penv, 
        jint         version) {

    switch (version) { 
        case JVMPI_VERSION_1:
            *penv =  CTIJvmpi;
            break;
        case JNI_VERSION_1_2:
            *penv = &CTIJniEnv;
            break;
        case CTI_VERSION_1:
            *penv = &CTIRemoteEnv;
            break;
        default:
            return JNI_EVERSION;
    }
    return JNI_OK;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CTIAttachCurrentThread(
        JavaVM      *aJavaVm,
        void       **aEnv, 
        void        *args) {
    *aEnv = &CTIJniEnv;
    return JNI_OK;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CTIDetachCurrentThread(
        JavaVM *vm) {

    ERROR_OUT(cU("not implemented"), -1);
    return JNI_OK;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CTIAttachCurrentThreadAsDaemon(
        JavaVM   *aJavaVm,
        void    **penv, 
        void    *args) {

    ERROR_OUT(cU("not implemented"), -1);
    return JNI_OK;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" JVMPI_RawMonitor CTIRawMonitorCreate(
        jchar *lock_name) {

    THR_ERR_TYPE     aResult;
    JVMPI_RawMonitor aMonitor;
    THR_RECMTX_TYPE *aMutex   = new THR_RECMTX_TYPE;
    TString          aLockName;

    aLockName.assign(lock_name, strlenR((char*)lock_name));

    aResult  = ThrRecMtxInit(aMutex, const_cast<SAP_UC *>(aLockName.str()));
    aMonitor = reinterpret_cast<JVMPI_RawMonitor>(aMutex);
    return aMonitor;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" void CTIRawMonitorEnter(
        JVMPI_RawMonitor lock_id) {

    THR_RECMTX_TYPE *rmtx = (THR_RECMTX_TYPE *)lock_id;
    ThrRecMtxLock(rmtx);
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" void CTIRawMonitorExit(
        JVMPI_RawMonitor lock_id) {

    THR_RECMTX_TYPE * rmtx = (THR_RECMTX_TYPE *)lock_id;
    ThrRecMtxUnlock(rmtx);
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" void CTIRawMonitorWait(
        JVMPI_RawMonitor lock_id, 
        jlong            ms) {

    THR_EVT_TYPE aEvent;
    THR_ERR_TYPE aResult;

    aResult = ThrEvtInit(&aEvent);
    aResult = ThrEvtWaitReset(&aEvent, (int)ms);
    aResult = ThrEvtDelete(&aEvent);
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" void CTIRawMonitorNotifyAll(
        JVMPI_RawMonitor lock_id) {
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" void CTIRawMonitorDestroy(
        JVMPI_RawMonitor lock_id) {

    THR_RECMTX_TYPE * rmtx = (THR_RECMTX_TYPE *)lock_id;
    ThrRecMtxDelete(rmtx);
    delete rmtx;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" void THR_API CTIThreadExit(void *aData) {
    THashThreads::iterator  aPtr;
    THR_ID_TYPE             aThr = ThrGetCurrentId();
    THR_KEY_TYPE            aKey = THR_INVALID_KEY;

    aPtr = CTIHashThreads.find(aThr);
    if (aPtr != CTIHashThreads.end()) {
        aKey = aPtr->aValue;
        CTIHashThreads.remove(aThr);
    }
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" void CTISetThreadLocalStorage(
        JNIEnv      *jJniEnv, 
        void        *aData) {

    THashThreads::iterator  aPtr;
    THR_ID_TYPE             aThr = ThrGetCurrentId();
    THR_KEY_TYPE            aKey = THR_INVALID_KEY;
    THR_ERR_TYPE            aResult;

    aPtr = CTIHashThreads.find(aThr);
    if (aPtr != CTIHashThreads.end()) {
        aKey = aPtr->aValue;
        if (aData == NULL) {
            CTIHashThreads.remove(aThr);
        }
        ThrKeyVarSet(&aKey, aData);
        return;
    }
    
    if (aData == NULL) {
        return;
    }
    aResult = ThrKeyGet(&aKey, CTIThreadExit);
    CTIHashThreads.insert(aThr, aKey);
    ThrKeyVarSet(&aKey, aData);
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" void CTIDisableGC(void) {}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" void CTIEnableGC(void) {}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" void CTIRunGC(void)    {}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint CTICreateSystemThread(
        SAP_A7      *aName, 
        jint         jPriority, 
        void        (*jFunc)(void *)) {

    THR_ERR_TYPE aResult;
    THR_ATTRIB   aThrAttr;
    THR_ID_TYPE  aThr;

    aThrAttr.scope                    = THR_SCOPE_DEFAULT;
    aThrAttr.detachedstate          = THR_DETACHEDSTATE_DEFAULT;
    aThrAttr.stacksize                    = THR_STACKSIZE_DEFAULT;
    aResult                         = ThrCreate2(&aThrAttr, CTIWrapper, jFunc, &aThr);

    return JVMPI_SUCCESS;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" void * CTIGetThreadLocalStorage(
        JNIEnv      *jJniEnv) {

    THashThreads::iterator aPtr;
    void         *aData = NULL;
    THR_ID_TYPE   aThr  = ThrGetCurrentId();
    THR_KEY_TYPE  aKey  = THR_INVALID_KEY;

    aPtr = CTIHashThreads.find(aThr);
    if (aPtr != CTIHashThreads.end()) {
        aKey  = aPtr->aValue;
        aData = ThrKeyVarGet(&aKey);
    }
    return aData;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jmethodID JNICALL CTIRegisterMethod(
                TCtiEnv                        *aEnv,
        const SAP_UC    *jPackageName,
        const SAP_UC    *jClassName,
        const SAP_UC    *jMethodName,
        const SAP_UC    *jMethodSign) {

    JVMPI_Event   jEvent;
    JVMPI_Method  jMethods[1];
    jmethodID     jMethod;
    TString       aClassName( jPackageName == NULL ? cU("package"): jPackageName, 
                              jClassName   == NULL ? cU("class")  : jClassName);
    TString       aMethodName(jMethodName);
    TString       aMethodSign(jMethodSign);

    jMethod                         = new struct _jmethodID;

    jEvent.env_id                   = &CTIJniEnv;
    jEvent.event_type               = JVMPI_EVENT_CLASS_LOAD;
    jEvent.u.class_load.class_id    = reinterpret_cast<jobjectID>(aClassName.getHash());
    jEvent.u.class_load.class_name  = aClassName.a7_str(); 
    jEvent.u.class_load.num_methods = 1;
    jEvent.u.class_load.methods     = jMethods;

    jMethods[0].method_id           = jMethod;
    jMethods[0].method_name         = const_cast<char *>(aMethodName.a7_str());
    jMethods[0].method_signature    = const_cast<char *>(aMethodSign.a7_str());
    jMethods[0].start_lineno        = 0;
    jMethods[0].end_lineno          = 0;

    CTIJvmpi->NotifyEvent(&jEvent);
    return jMethod;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CTIEnterMethod(
        TCtiEnv         *aEnv,
        jmethodID        jMethodID,...) {

    JVMPI_Event  jEvent;
    JNIEnv      *jEnv;
    jint         jResult;

    jResult = CTIJavaVm->AttachCurrentThread(reinterpret_cast<void **>(&jEnv), NULL);

    if ((CTIRequestEvent & (1 << (JVMPI_EVENT_METHOD_ENTRY-1))) == 0) {
        return JVMPI_SUCCESS;
    }

    jEvent.event_type               = JVMPI_EVENT_METHOD_ENTRY;
    jEvent.env_id                   = jEnv;
    jEvent.u.method.method_id       = reinterpret_cast<jmethodID>(jMethodID);

    CTIJvmpi->NotifyEvent(&jEvent);
    return JVMPI_SUCCESS;
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CTIExitMethod(
                TCtiEnv                *aEnv,
        jmethodID    jMethodID) {

    JVMPI_Event  jEvent;
    JNIEnv      *jEnv;
    jint         jResult;

    jResult = CTIJavaVm->AttachCurrentThread(reinterpret_cast<void **>(&jEnv), NULL);

    if ((CTIRequestEvent & (1 << (JVMPI_EVENT_METHOD_EXIT-1))) == 0) {
        return JVMPI_SUCCESS;
    }

    jEvent.event_type               = JVMPI_EVENT_METHOD_EXIT;
    jEvent.env_id                   = jEnv;
    jEvent.u.method.method_id       = reinterpret_cast<jmethodID>(jMethodID);
    
    CTIJvmpi->NotifyEvent(&jEvent);
    return JVMPI_SUCCESS;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CTIOnLoad(
        JavaVM          *aJavaVM,
        JVMPI_Interface *aJvmpi) {

    if (aJavaVM != NULL) {
        CTIJvmpi  = aJvmpi;
        CTIJavaVm = aJavaVM;

        //if (CTIRegisterEnv != NULL) {
        //    CTIEnv.onEnterMethod  = CTIEnterMethod;
        //    CTIEnv.onExitMethod   = CTIExitMethod;
        //    CTIEnv.registerMethod = CTIRegisterMethod;
        //    CTIRegisterEnv(&CTIEnv);
        //}
    }
    return JNI_OK;
}
// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNIEXPORT JNICALL JvmtiInit(
        SAP_BOOL         aRunModeJni,
        CTICallback      aSetEnv,
        SAP_UC          *aOptions) {

    void        *aReserved = NULL;
    jint         jResult   = JNI_ERR;
    JVMPI_Event  jEvent;
    THR_ERR_TYPE aResult;
    TString      jOptions(aOptions);

    CTIRegisterEnv = aSetEnv;

    if (aRunModeJni) {
        jResult = CTIOnLoad(CTIJavaVm, CTIJvmpi);
        return jResult;
    }

    CTIJvmpi  = new JVMPI_Interface;
    CTIJavaVm = new JavaVM;

    aResult   = ThrProcInit();
    if (aResult != THR_ERR_OK) {
        
        ERROR_OUT(cU("CPIInit"), aResult);
        return jResult;
    }
    CTIJniEnv.functions                            = &CTIJniEnvFunctions;

    CTIJavaVmFunctions.DestroyJavaVM               = CTIDestroyJavaVM;
    CTIJavaVmFunctions.AttachCurrentThread         = CTIAttachCurrentThread;
    CTIJavaVmFunctions.DetachCurrentThread         = CTIDetachCurrentThread;
    CTIJavaVmFunctions.AttachCurrentThreadAsDaemon = CTIAttachCurrentThreadAsDaemon;
    CTIJavaVmFunctions.GetEnv                      = CTIGetEnv;
    CTIJavaVm->functions                           = &CTIJavaVmFunctions;

    CTIJvmpi->EnableEvent                          = CTIEnableEvent;
    CTIJvmpi->DisableEvent                         = CTIDisableEvent;
    CTIJvmpi->RawMonitorCreate                     = CTIRawMonitorCreate;
    CTIJvmpi->RawMonitorDestroy                    = CTIRawMonitorDestroy;
    CTIJvmpi->RawMonitorEnter                      = CTIRawMonitorEnter;
    CTIJvmpi->RawMonitorExit                       = CTIRawMonitorExit;
    CTIJvmpi->SetThreadLocalStorage                = CTISetThreadLocalStorage;
    CTIJvmpi->GetThreadLocalStorage                = CTIGetThreadLocalStorage;
    CTIJvmpi->CreateSystemThread                   = CTICreateSystemThread;
    CTIJvmpi->RawMonitorNotifyAll                  = CTIRawMonitorNotifyAll;
    CTIJvmpi->RawMonitorWait                       = CTIRawMonitorWait;
    CTIJvmpi->DisableGC                            = CTIDisableGC;
    CTIJvmpi->EnableGC                             = CTIEnableGC;
    CTIJvmpi->RunGC                                = CTIRunGC;
    CTIJvmpi->GetCurrentThreadCpuTime              = CTIGetCurrentThreadCpuTime;

    jResult = JVM_OnLoad(CTIJavaVm, const_cast<SAP_A7*>(jOptions.a7_str()), aReserved);
    if (jResult != JNI_OK) {
        return jResult;
    }
    jEvent.env_id     = &CTIJniEnv;
    jEvent.event_type = JVMPI_EVENT_JVM_INIT_DONE;
    CTIJvmpi->NotifyEvent(&jEvent);
    return jResult;
}
#endif
#endif



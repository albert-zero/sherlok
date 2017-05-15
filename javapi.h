// -----------------------------------------------------------------
//
// Author: Albert Rossmann
// Date  : 14.04.2003
//! \file  javapi.h
//! \brief Interface for JNI, JVMPI and JVMDI
//!
//! \mainpage Sherlok Application Monitor
//! Sherlok exists in the following versions 
//! <ul>
//!     <li> <b>V8 1.4.x</b> is using the interface JVMPI and JVMDI (until JDK 1.5).<br>
//!         It is attached using the following JVM arguments:<br><code>java -Xdebug -Xrunsherlok[:options]</code>
//!     </li>
//!     <li> <b>V9 1.5.x</b> is using interface is JVMTI (JDK 1.5 and later).<br>
//!         It is attached using  the following JVM arguments:<br><code>java -agentlib:sherlok[=options]</code>
//!     </li>
//! </ul>
//! Sherlok knows a set of configuration parameters, which can also be 
//! passed as arguments to the options above.
//! Sherlok was developed before standard template library was available for all platforms. 
//! The result is the implemntation of classes in standard.h file.
// -----------------------------------------------------------------
#ifndef JAVAPI_H
#define JAVAPI_H

#include "cjvmti.h"
#include "command.h"

typedef TList <TMonitorClass*> TListClasses;        //!< List of classes
jrawMonitorID     mRawMonitorSync    = NULL; //!< Sync monitor
jrawMonitorID     mRawMonitorJni     = NULL; //!< Sync monitor
TMonitor         *gMonitor;                  //!< Global monitor
TMonitorMutex    *mMonitorJni        = NULL;
bool              gInitialized       = false;

#ifdef SAPonNT
// ------------------------------------------------------------
//! NT specific exception handling
// ------------------------------------------------------------
extern "C" int filterException (struct _EXCEPTION_POINTERS *ep)
{
    PEXCEPTION_RECORD er = ep->ExceptionRecord;
     switch (er->ExceptionCode) {
        case EXCEPTION_ACCESS_VIOLATION:        ERROR_OUT(cU("Access Violation"),    er->ExceptionCode); break;
        case EXCEPTION_DATATYPE_MISALIGNMENT:   ERROR_OUT(cU("Alignment"),           er->ExceptionCode); break;
        case EXCEPTION_ILLEGAL_INSTRUCTION:     ERROR_OUT(cU("Illegal Instruction"), er->ExceptionCode); break;
        case EXCEPTION_IN_PAGE_ERROR:           ERROR_OUT(cU("Page Error"),          er->ExceptionCode); break;
        case EXCEPTION_PRIV_INSTRUCTION:        ERROR_OUT(cU("Priv Instruction"),    er->ExceptionCode); break;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:      ERROR_OUT(cU("Devide by Zero"),      er->ExceptionCode); break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:      ERROR_OUT(cU("Devide by Zero"),      er->ExceptionCode); break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:   ERROR_OUT(cU("Array Bound Exceed"),  er->ExceptionCode); break;
        default: break;
     }     
     DebugBreak();
     return EXCEPTION_EXECUTE_HANDLER;
     //return EXCEPTION_CONTINUE_SEARCH;
 }
#define JNI_TRY   __try
#define JNI_CATCH __except (filterException(GetExceptionInformation()))
#else
#define JNI_TRY     try
#define JNI_CATCH   catch(...)
#endif

// -----------------------------------------------------------------
// doRepeat: JAVA Thread to repeat last command
//! Repeater thread task
// -----------------------------------------------------------------
extern "C" void JNICALL doRepeatThread (
        jvmtiEnv        *aJvmti,
        JNIEnv          *aJni,
        void            *aArg) {

    TCommand        *aCommand = TCommand::getInstance();
    jvmtiError       aResult;
    TMonitorThread  *aThread;

    aResult = aJvmti->GetThreadLocalStorage(NULL, (void **)&aThread);
    
    for (;;) {
        aJvmti->RawMonitorEnter(mRawMonitorSync);
        aJvmti->RawMonitorWait(mRawMonitorSync, aCommand->getSleepTime());
        aJvmti->RawMonitorExit(mRawMonitorSync);
        
        // Synchronize command execution
        mMonitorJni->enter();
            aThread->attach(&aJni); 

            aCommand->executeStackCmd(aJvmti, aJni);

            if (aCommand->repeat()) {
                aCommand->execute(aJvmti, aJni, NULL);
            }
        mMonitorJni->exit();
    }
}
// -----------------------------------------------------------------
// doTelnetThread: JAVA Thread for command line application
//! Telnet thread task
// -----------------------------------------------------------------
extern "C" void JNICALL doTelnetThread (
        jvmtiEnv    *aJvmti, 
        JNIEnv      *aJni, 
        void        *aArg) {

    jvmtiError       aResult;
    TCommand        *aCommand    = TCommand::getInstance();
    TConsole        *aConsole    = TConsole::getInstance();
    TSecurity       *aSecurity   = TSecurity::getInstance();
    TProperties     *aProperties = TProperties::getInstance();
    TMonitor        *aMonitor    = TMonitor::getInstance();
    TMonitorThread  *aThread;
    TXmlWriter       aWriter(XMLWRITER_TYPE_ASCII);
    TXmlTag          aRootTag(cU("Message"), XMLTAG_TYPE_NODE);

    aRootTag.addAttribute(cU("Type"), cU("Command"));
    aRootTag.addAttribute(cU("Info"),  cU("unknown command"));

    if (!aConsole->openPort()) {
        ERROR_OUT(cU("open port"), aProperties->getTelnetPort());
        exit(1);
    }   

    aResult = aJvmti->GetThreadLocalStorage(NULL, (void **)&aThread);
    if (aResult == 0) {
        aThread = new TMonitorThread(aJvmti, aJni, NULL, cU("TelnetThread"));
        aResult = aJvmti->SetThreadLocalStorage(NULL, aThread);
    }

    for (;;) {
        if (!aConsole->checkState()) {
            while (aConsole->open()) {
                if (aSecurity->login()) {
					aConsole->login();
                    break;
                }
                SLEEP(1);
            }
            if (aProperties->getConsoleWriterType() == XMLWRITER_TYPE_XML) {
                aMonitor->syncOutput(
                    cU("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
                    cU("<sherlok>\n")
                    cU("<Message Info=\"Connected\"/>\n"));
            }
        }

        if (aConsole->errorState()) {
            break;
        }
        aConsole->prompt();

        if (!aCommand->read()) {
            if (aCommand->getCmd() != COMMAND_CONTINUE) {
                aWriter.print(&aRootTag);
            }
            continue;
        }

        // Synchronize command execution
        mMonitorJni->enter();
            aJvmti = aProperties->getJvmti();
            aThread->attach(&aJni); 

            if (aCommand->getCmd() == COMMAND_GC) {
                aJvmti->ForceGarbageCollection();
            }
            else {
                aCommand->execute(aJvmti, aJni, NULL);
            }
        mMonitorJni->exit();
    }
    aConsole->close();
}

// ------------------------------------------------------------------------------------
//! Callback VMDeath
// ------------------------------------------------------------------------------------
extern "C" void JNICALL onVmDeath(
        jvmtiEnv   *aJvmti, 
        JNIEnv     *aJni) {
    
    JNI_TRY {
        TCommand    *aCmd        = TCommand::getInstance();
        TProperties *aProperties = TProperties::getInstance();

        if (aProperties->getDumpOnExit()) {
            aCmd->parse(cU("lsc -m1"));
            aCmd->execute(aJvmti, aJni, NULL);
        }
    } 
    JNI_CATCH {
        ERROR_OUT(cU("onVmDeath"), 0);
    }
    ERROR_OUT(cU("Terminate JVM"), 0);
}
// ------------------------------------------------------------------------------------
//! Callback VMInit
// ------------------------------------------------------------------------------------
extern "C" void JNICALL onVmInit(
        jvmtiEnv   *aJvmti, 
        JNIEnv     *aJni, 
        jthread     jThread) {

    jint                 aResult;
    jint                 i;
    TProperties         *aProperties;
    TMonitor            *aMonitor;
    TTracer             *aTracer;
    jint                 aCnt;
    jint                 aStatus;
    jclass              *aClassPtr;
    TLogger             *aLogger;

    // evaluate options
    aProperties = TProperties::getInstance();
    aMonitor    = TMonitor::getInstance();
    aTracer     = TTracer::getInstance();
    aLogger     = TLogger::getInstance();

    TXmlWriter aWriter(XMLWRITER_TYPE_ASCII);
    TXmlTag aRootTag(cU("List"), XMLTAG_TYPE_NODE);
    aRootTag.addAttribute(cU("Type"), cU("Properties"));
    
    if (aProperties->doTrace()) {
    TCommand::getInstance()->setTraceOptions(aJvmti);
        aLogger->start();
    }
    
    if (!gInitialized) {
        aProperties->dump(&aRootTag);
        aWriter.print(&aRootTag);
    }

    if (aJni != NULL && !gInitialized) {
        jstring   jStrName[2];
        jobject   jObjThr [2];
        jclass    jClsThread;
        jmethodID jIniThread;

        jStrName[0] = aJni->NewStringUTF(cR("_Sherlok"));
        jStrName[1] = aJni->NewStringUTF(cR("_Repeate"));

        jClsThread  = aJni->FindClass(cR("java/lang/Thread")); 
        jIniThread  = aJni->GetMethodID(jClsThread, cR("<init>"), cR("(Ljava/lang/String;)V")); 

        jObjThr[0]  = aJni->NewObject(jClsThread, jIniThread, jStrName[0]); 
        jObjThr[1]  = aJni->NewObject(jClsThread, jIniThread, jStrName[1]); 

        aResult     = aJvmti->RunAgentThread((jthread)jObjThr[0], doTelnetThread, NULL, JVMTI_THREAD_MAX_PRIORITY);
        aResult     = aJvmti->RunAgentThread((jthread)jObjThr[1], doRepeatThread, NULL, JVMTI_THREAD_MAX_PRIORITY);

        // register all classes loaded so far
        aJvmti->GetLoadedClasses(&aCnt, &aClassPtr);
        for (i = 0; i < aCnt; i++) {
            aJvmti->GetClassStatus(aClassPtr[i], &aStatus);
            if ((aStatus & JVMTI_CLASS_STATUS_PREPARED) != 0) {
                aMonitor->onClassPrepare(aJvmti, aJni, jThread, aClassPtr[i]);
            }
        }
        
        /*SAPUNICODEOK_CHARTYPE*/
        aJvmti->Deallocate((unsigned char*)aClassPtr);
    }

    aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_PREPARE,             NULL);   
    aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_OBJECT_FREE,               NULL);
    aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_START,  NULL);
    aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_FINISH, NULL);


    if (!gInitialized) {
        // TCommand::getInstance()->setTraceOptions(aJvmti);
    }

    if (aProperties->doTrace()) {
        aTracer->start();

        if (aTracer->doTraceException()) {
            aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION, NULL);
        }
    }

    if (aProperties->doMonitoring()) {
        aMonitor->start(aJvmti, NULL, true);
    }
	else {
		aMonitor->stop(aJvmti, NULL);
	}
    gInitialized = true;
    mMonitorJni->exit();
}
// ------------------------------------------------------------------------------------
//! Callback MethodEntry
// ------------------------------------------------------------------------------------
extern "C" void JNICALL onMethodEntry(
        jvmtiEnv   *aJvmti, 
        JNIEnv     *aJni, 
        jthread     jThread, 
        jmethodID   jMethod) {

    JNI_TRY {
        gMonitor->onMethodEnter(aJvmti, aJni, jThread, jMethod);
    } 
    JNI_CATCH {}
}
// ------------------------------------------------------------------------------------
//! Callback MethodExit
// ------------------------------------------------------------------------------------
extern "C" void JNICALL onMethodExit(
        jvmtiEnv   *aJvmti, 
        JNIEnv     *aJni, 
        jthread     aThread, 
        jmethodID   aMethod,
        jboolean    aPopped,
        jvalue      aResult) {
    JNI_TRY {
        gMonitor->onMethodExit(aJvmti, aJni, aThread, aMethod);
    } 
    JNI_CATCH {}
}
// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------
extern "C" void JNICALL onFieldModification(
        jvmtiEnv    *aJvmti,
        JNIEnv      *aJni,
        jthread      aThread,
        jmethodID    jMethod,
        jlocation    jLocation,
        jclass       jClass,
        jobject      jObject,
        jfieldID     jField,
        char         aSignaturType,
        jvalue       jValue) {
    JNI_TRY{
        gMonitor->onFieldModification(aJvmti, aJni, aThread, jClass, jMethod, jField, aSignaturType, jValue);
    }
    JNI_CATCH{}

}

// ------------------------------------------------------------------------------------
//! Callback ClassPrepare
// ------------------------------------------------------------------------------------
extern "C" void JNICALL onClassPrepare(
            jvmtiEnv   *aJvmti,
            JNIEnv     *aJni,
            jthread     aThread,
            jclass      jClass) {
    JNI_TRY {
        gMonitor->onClassPrepare(aJvmti, aJni, aThread, jClass);
    } 
    JNI_CATCH {}
}
// ------------------------------------------------------------------------------------
//! Callback Breakpoint
// ------------------------------------------------------------------------------------
extern "C" void JNICALL onBreakpoint(
            jvmtiEnv   *aJvmti,
            JNIEnv     *aJni,
            jthread     aThread,
            jmethodID   aMethod,
            jlocation   aLocation) {
    JNI_TRY {
        gMonitor->onBreakpoint(aJvmti, aJni, aThread, aMethod, aLocation);
    } 
    JNI_CATCH {}
}
// ------------------------------------------------------------------------------------
//! Callback VMObjectAlloc
// ------------------------------------------------------------------------------------
extern "C" void JNICALL onObjectAlloc(
            jvmtiEnv    *aJvmti,
            JNIEnv      *aJni,
            jthread      jThread,
            jobject      jObject,
            jclass       jClass,
            jlong        aSize) {
    JNI_TRY {
        TMonitor::getInstance()->onObjectAlloc(aJvmti, aJni, jThread, jObject, jClass, aSize);
    }
    JNI_CATCH {
        ERROR_OUT(cU("onObjectAlloc"), 0);
    }
}
// ------------------------------------------------------------------------------------
//! Callback ObjectFree
// ------------------------------------------------------------------------------------
extern "C" void JNICALL onObjectFree(
            jvmtiEnv   *aJvmti,
            jlong       aTag) {
    JNI_TRY {
        TMonitor::getInstance()->onObjectDelete(aJvmti, aTag);
    }
    JNI_CATCH {
        ERROR_OUT(cU("onObjectFree"), 0);
    }
}
// ------------------------------------------------------------------------------------
//! Callback ThreadStart
// ------------------------------------------------------------------------------------
extern "C" void JNICALL onThreadStart(
            jvmtiEnv   *aJvmti,
            JNIEnv     *aJni,
            jthread     jThread) {

    JNI_TRY {
        TMonitor::getInstance()->onThreadStart(aJvmti, aJni, jThread);
    } 
    JNI_CATCH {}
}
// ------------------------------------------------------------------------------------
//! Callback ThreadEnd
// ------------------------------------------------------------------------------------
extern "C" void JNICALL onThreadEnd(
            jvmtiEnv   *aJvmti,
            JNIEnv     *aJni,
            jthread     aThread) {
    JNI_TRY {
        TMonitor::getInstance()->onThreadEnd(aJvmti, aJni, aThread);
    } 
    JNI_CATCH {}
}
// ------------------------------------------------------------------------------------
//! Callback GarbageCollectionStart
// ------------------------------------------------------------------------------------
extern "C" void JNICALL onGcStart(
            jvmtiEnv   *aJvmti) {
    TMonitor::getInstance()->setGCTime();
}
// ------------------------------------------------------------------------------------
//! Callback GarbageCollectionFinish
// ------------------------------------------------------------------------------------
extern "C" void JNICALL onGcFinish(
            jvmtiEnv   *aJvmti) {

    TCommand *aCmd = TCommand::getInstance();
    aCmd->pushStackCmd(COMMAND_GC);

    aJvmti->RawMonitorEnter(mRawMonitorSync);
    aJvmti->RawMonitorNotifyAll(mRawMonitorSync);
    aJvmti->RawMonitorExit(mRawMonitorSync);

}
// ------------------------------------------------------------------------------------
//! Callback MonitorContendedEnter
// ------------------------------------------------------------------------------------
extern "C" void JNICALL onContentionEnter(
            jvmtiEnv   *aJvmti,
            JNIEnv     *aJni,
            jthread     aThread,
            jobject     aObject) {

    JNI_TRY {
        jvmtiEvent aEvent = JVMTI_EVENT_MONITOR_CONTENDED_ENTER;
        TMonitor::getInstance()->setThreadStatus(aJvmti, aJni, aThread, aObject, aEvent);
    } 
    JNI_CATCH {}
}
// ------------------------------------------------------------------------------------
//! Callback MonitorContendedEntered
// ------------------------------------------------------------------------------------
extern "C" void JNICALL onContentionExit(
            jvmtiEnv   *aJvmti,
            JNIEnv     *aJni,
            jthread     aThread,
            jobject     aObject) {

    JNI_TRY {
        jvmtiEvent aEvent = JVMTI_EVENT_MONITOR_CONTENDED_ENTERED;
        TMonitor::getInstance()->setThreadStatus(aJvmti, aJni, aThread, aObject, aEvent);
    } 
    JNI_CATCH {}
}
// ------------------------------------------------------------------------------------
//! Callback MonitorWait
// ------------------------------------------------------------------------------------
extern "C" void JNICALL onMonitorEnter(
            jvmtiEnv   *aJvmti,
            JNIEnv     *aJni,
            jthread     aThread,
            jobject     aObject,
            jlong       aTimeout) {

    JNI_TRY {
        jvmtiEvent aEvent = JVMTI_EVENT_MONITOR_WAIT;
        TMonitor::getInstance()->setThreadStatus(aJvmti, aJni, aThread, aObject, aEvent);
    } 
    JNI_CATCH {}
}
// ------------------------------------------------------------------------------------
//! Callback MonitorWaited
// ------------------------------------------------------------------------------------
extern "C" void JNICALL onMonitorExit(
            jvmtiEnv   *aJvmti,
            JNIEnv     *aJni,
            jthread     aThread,
            jobject     aObject,
            jboolean    aTimeout) {

    JNI_TRY {
        jvmtiEvent aEvent = JVMTI_EVENT_MONITOR_WAITED;
        TMonitor::getInstance()->setThreadStatus(aJvmti, aJni, aThread, aObject, aEvent);
    } 
    JNI_CATCH {}
}
// ------------------------------------------------------------------------------------
//! Callback Exception 
// ------------------------------------------------------------------------------------
extern "C" void JNICALL onException(
            jvmtiEnv   *aJvmti,
            JNIEnv     *aJni,
            jthread     jThread,
            jmethodID   jTMethod,
            jlocation   jTLocation,
            jobject     jException,
            jmethodID   jCMethod,
            jlocation   jCLocation) {
    JNI_TRY {
        gMonitor->onException(aJvmti, aJni, jThread, jTMethod, jTLocation, jException, jCMethod, jCLocation);
    } JNI_CATCH {}
}
// ------------------------------------------------------------------------------------
//! Callback ExceptionCatch
// ------------------------------------------------------------------------------------
extern "C" void JNICALL onExceptionCatch(
            jvmtiEnv   *aJvmti,
            JNIEnv     *aJni,
            jthread     jThread,
            jmethodID   jCMethod,
            jlocation   jCLocation,
            jobject     jException) {
    JNI_TRY {
        gMonitor->onExceptionCatch(aJvmti, aJni, jThread, jException, jCMethod, jCLocation);
    } 
    JNI_CATCH {}
}
// ------------------------------------------------------------------------------------
// Global members for table functions
// ------------------------------------------------------------------------------------
#define MAX_INX_TABLES 18               //!< Tag table dimension
TXmlTag mExtTable[MAX_INX_TABLES];      //!< Output tag tables for external reference
// ------------------------------------------------------------
// JVM_OnLoad profiler agent entry point
//! \brief JVMTI Agent interface
//! \param aJvm         The Java virtual machine
//! \param aOptions     The command line options
//! \param aReserved    Future use
//! \return JVMTI_ERROR_OK if initialization was successfull
// ------------------------------------------------------------
extern "C" JNIEXPORT jint JNICALL Agent_OnLoad(
        JavaVM *aJvm,      /*SAPUNICODEOK_CHARTYPE*/ 
        char   *aOptions, 
        void   *aReserved) {

    jvmtiEnv            *aJvmti;
    jint                 aResult;
    TProperties         *aProperties;
    TCommand            *aCommand;
    TTracer             *aTracer;
    jvmtiEventCallbacks *aCallbacks;
    jvmtiCapabilities   *aCapa;

    // evaluate options
    // get interfaces
    aResult  = aJvm->GetEnv((void **)&aJvmti, JVMTI_VERSION_1_0);

    if (mMonitorJni == NULL) {    
        aResult     = aJvmti->CreateRawMonitor(/*SAPUNICODEOK_CHARTYPE*/(char*)cR("_MonitorJni"),  &mRawMonitorJni);
        aResult     = aJvmti->CreateRawMonitor(/*SAPUNICODEOK_CHARTYPE*/(char*)cR("_MonitorSync"), &mRawMonitorSync);
        mMonitorJni = new TMonitorMutex(aJvmti, cU("_C++Java"));
    }
    // block until successfull call to "onVmInit"
    mMonitorJni->enter();
    TJvmtiEnv::getInstance()->onAgentLoad(aJvm, aJvmti);

    aProperties = TProperties::getInstance();
    aProperties->initialize(aJvm, aJvmti);
    aProperties->parseOptions(aOptions);

    gMonitor = TMonitor::getInstance();
    gMonitor->initialize(aJvmti);
    gMonitor->stop(aJvmti, NULL);
    
    aCommand = TCommand::getInstance();
    aCommand->initialize();

    aTracer = TTracer::getInstance();
    aTracer->initialize();

    // get capabilities
    aCapa = new jvmtiCapabilities;

    // set capabilities
    (void)memsetR(aCapa, 0, sizeofR(jvmtiCapabilities));
    aCapa->can_generate_method_entry_events       = 1;
    aCapa->can_generate_method_exit_events        = 1;
    aCapa->can_generate_all_class_hook_events     = 1;
    aCapa->can_generate_vm_object_alloc_events    = 1;
    aCapa->can_generate_object_free_events        = 1;
    aCapa->can_generate_breakpoint_events         = 1;
    aCapa->can_access_local_variables             = 1;
    aCapa->can_generate_garbage_collection_events = 1;
    aCapa->can_get_current_thread_cpu_time        = 1;
    aCapa->can_tag_objects                        = 1;
    aCapa->can_maintain_original_method_order     = 1;
    aCapa->can_generate_monitor_events            = 1;
    aCapa->can_generate_exception_events          = 1;
    aCapa->can_suspend                            = 1;
    aCapa->can_get_current_thread_cpu_time        = 1;
    aCapa->can_signal_thread                      = 1;
    aCapa->can_get_synthetic_attribute            = 1;
    aCapa->can_get_line_numbers                   = 1;
    aCapa->can_generate_field_modification_events = 1;
    aCapa->can_generate_field_access_events       = 1;

    aResult = aJvmti->AddCapabilities(aCapa);
    if (aResult != JVMTI_ERROR_NONE) {
        ERROR_OUT(cU("capa "), aResult);
    }
    // set callbacks
    aCallbacks = new jvmtiEventCallbacks;
    (void)memsetR(aCallbacks, 0, sizeofR(jvmtiEventCallbacks));
    aCallbacks->VMInit                   = &onVmInit;   
    aCallbacks->VMDeath                  = &onVmDeath;
    aCallbacks->MethodEntry              = &onMethodEntry;  
    aCallbacks->MethodExit               = &onMethodExit;  
    aCallbacks->ClassPrepare             = &onClassPrepare; 
    aCallbacks->VMObjectAlloc            = &onObjectAlloc;
    aCallbacks->Breakpoint               = &onBreakpoint;
    aCallbacks->ObjectFree               = &onObjectFree;
    aCallbacks->ThreadStart              = &onThreadStart;
    aCallbacks->ThreadEnd                = &onThreadEnd;
    aCallbacks->GarbageCollectionStart   = &onGcStart;
    aCallbacks->GarbageCollectionFinish  = &onGcFinish;
    aCallbacks->MonitorContendedEnter    = &onContentionEnter;
    aCallbacks->MonitorContendedEntered  = &onContentionExit;
    aCallbacks->MonitorWait              = &onMonitorEnter;
    aCallbacks->MonitorWaited            = &onMonitorExit;
    aCallbacks->Exception                = &onException;
    aCallbacks->ExceptionCatch           = &onExceptionCatch;
    aCallbacks->FieldModification        = &onFieldModification;

    // activate callbacks
    aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT,         NULL);
    aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_DEATH,        NULL);
    aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_THREAD_START,    NULL);
    aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_THREAD_END,      NULL);

    aResult = aJvmti->SetEventCallbacks(aCallbacks, (jint)sizeofR(jvmtiEventCallbacks));
    return JNI_OK;
}
// ------------------------------------------------------------------------------------
// JNI::enterContext wrapper to JARM interface
//! JNI Set enter context
// ------------------------------------------------------------------------------------
extern "C" JNIEXPORT
    void JNICALL Java_com_sap_util_monitor_jarm_Sherlok_enterContext(
        JNIEnv  *jEnv, 
        jclass   , 
        jstring  jRequest,
        jstring  jComponent) {

    if (mRawMonitorJni == NULL) {
        return;
    }
    TString aRequest(jEnv, jRequest);
    TString aContext(jEnv, jComponent);
    TMonitor *aMonitor = TMonitor::getInstance();

    aMonitor->onContextEnter(
        TProperties::getInstance()->getJvmti(),
        jEnv, 
        aRequest.str(), 
        aContext.str());
}
// ------------------------------------------------------------------------------------
// JNI::exitContext wrapper to JARM interface
//! JNI Set exit context
// ------------------------------------------------------------------------------------
extern "C" JNIEXPORT
    jlong JNICALL Java_com_sap_util_monitor_jarm_Sherlok_exitContext(
        JNIEnv  *aJni, 
        jclass   , 
        jstring  jRequest,
        jstring  jComponent) {
    
    if (mRawMonitorJni == NULL) {
        return 0;
    }
    jlong     aCpuTime = 0;
    TString   aRequest(aJni, jRequest);
    TString   aContext(aJni, jComponent);
    TMonitor *aMonitor = TMonitor::getInstance();

    aCpuTime = aMonitor->onContextExit(
        TProperties::getInstance()->getJvmti(),
        aJni, 
        aRequest.str(), 
        aContext.str());

    return aCpuTime;
}
#endif

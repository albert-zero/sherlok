// -----------------------------------------------------------------
//
// Author: Albert Rossmann
// Date  : 14.04.2003
//! \file  monitor.h
//! \brief TMonitor is the C++ interface to JVMPI 
//!
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
#ifndef MONITOR_H
#define MONITOR_H
#include <jvmti.h>

// ----------------------------------------------------
//! \class TException
//! \brief Implements hash object for Java exceptions
//!
// ----------------------------------------------------
class TException: public THashObj {
public:
    TString  mName;                 //!< The name of the exception
    unsigned mCnt;                  //!< Number of instances
    //! Constructor
    //! \param aName    The name of the exception
    TException(const SAP_UC *aName) {
        mName = aName;
        mCnt  = 1;
    }
    //! Delallocator
    virtual void deallocate(jlong) {
    }
};
//! Hash Java exceptions
typedef THash  <unsigned long, TException *> THashExceptions;                       

// ----------------------------------------------------
//! \class TMonitorMutex
//! \brief Managing critical sections
//!
//! This class can run read/write MUTEX implementation 
// ----------------------------------------------------
class TMonitorMutex {
    jvmtiEnv        *mJvmti;    //!< The tool interface for Java-Mutex support
    jrawMonitorID    mMonitor;  //!< The mutex handle for Java-Mutex support

public:
    // ----------------------------------------------------
    //! Constructor
    //! \param aJvmti    The tool interface 
    //! \param aName     The name 
    // ----------------------------------------------------
    TMonitorMutex (
            jvmtiEnv        *aJvmti,
            const SAP_UC    *aName) {

        jvmtiError  jResult;
        TString     jName(aName);
        mJvmti      = aJvmti;
        jResult     = mJvmti->CreateRawMonitor(jName.a7_str(), (jrawMonitorID *)&mMonitor);
    }
    // ----------------------------------------------------
    //! Destructor
    // ----------------------------------------------------
    ~TMonitorMutex() {
        jvmtiError  jResult;
        jResult = mJvmti->DestroyRawMonitor(mMonitor);
    }
    // ----------------------------------------------------
    //! Create lock on monitor
    //! \param aExclusive could be used to implement read/write mutex 
    // ----------------------------------------------------
    void enter(bool aExclusive = true) {
        jvmtiError jResult;
        jResult = mJvmti->RawMonitorEnter(mMonitor);
    }
    // ----------------------------------------------------
    //! Unlock mutex
    // ----------------------------------------------------
    void exit() {
        jvmtiError jResult;
        jResult = mJvmti->RawMonitorExit(mMonitor);
    }
    // ----------------------------------------------------
    //! Wait for timeout or notify event
    // ----------------------------------------------------
    void wait(jlong aTime) {
        jvmtiError jResult;
        jResult = mJvmti->RawMonitorWait(mMonitor, aTime);
    }
    // ----------------------------------------------------
    //! Send notify event for all threads in wait
    // ----------------------------------------------------
    void notify() {
        jvmtiError jResult;
        jResult = mJvmti->RawMonitorNotifyAll(mMonitor);
    }
};

// ----------------------------------------------------
//! \class TMonitorLock
//! \brief Managing critical sections
//!
//! Using C++ block automatic destructor, the 
//! usage of locking monitors is save
// ----------------------------------------------------
class TMonitorLock {
private:
    int              mLocked;
    TMonitorMutex   *mMutex;
    TMonitorThread  *mThread;
public:
    // ----------------------------------------------------
    //! \brief Constructor
    //
    //! \param  aJvmti      The Java environment
    //! \param  aRawMonitor The monitor to lock
    //! \param  aLocked     If \c TRUE create a lock on monitor
    // ----------------------------------------------------
    TMonitorLock(
        TMonitorMutex   *aRawMonitor,
        bool             aLocked    = true,
        bool             aExclusive = true) {
    
        mThread = NULL;
        mMutex  = aRawMonitor;
        mLocked = 0;
        
        if (aLocked) {
            enter(aExclusive);
       }
    };
    // ----------------------------------------------------
    // ----------------------------------------------------
    TMonitorLock(
        TMonitorMutex   *aRawMonitor,
        TMonitorThread  *aThread)  {

        mMutex  = aRawMonitor;
        mLocked = 0;
        mThread = aThread;

        enter(true);
        
        if (mThread != NULL) {
            mThread->setProcessJni(true);
        }
    };
    // ----------------------------------------------------
    //! \brief Destructor
    //! 
    //! Destroy lock on monitor
    // ----------------------------------------------------
    ~TMonitorLock() {
        while (mLocked > 0) {
            exit();
        }
        if (mThread != NULL) {
            mThread->setProcessJni(false);
        }
    };
    // ----------------------------------------------------
    //! Create lock on monitor
    // ----------------------------------------------------
    void enter(bool aExclusive = true) {
        mMutex->enter(aExclusive);
        mLocked ++;
    }
    // ----------------------------------------------------
    //! Destroy lock on monitor
    // ----------------------------------------------------
    void exit() {
        if (mLocked > 0) {
            mLocked --;
            mMutex->exit();
        }
    }
};
    
// ---------------------------------------------------------
// TMonitor::heapCallback
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
            void        *aUserData);


// ----------------------------------------------------
//! \class TMonitor
//! \brief Profiler called by JVMTI
//!
// ----------------------------------------------------
class TMonitor {
    friend jvmtiIterationControl JNICALL TMonitorHeapCallback(jlong, jlong, jlong*, void*);
private:
    THashClasses     mClasses;          //!< Hash table for java classes
    TListClasses     mDelClasses;       //!< List for deleted classes
    THashClasses     mMemoryLeaks;      //!< Hash table for java classes
    THashClasses     mContextClasses;   // 
    THashMethods     mContextMethods;   // 
    THashMethods     mMethods;          //!< Hash table for java methods  
    THashExceptions  mHashExceptions;

    TMonitorMutex   *mRawMonitorOutput; //!< Mutex for output operations
    TMonitorMutex   *mRawMonitorThreads;
    TMonitorMutex   *mRawMonitorMemory;
    TMonitorMutex   *mRawMonitorAccess;
    TProperties     *mProperties;        //!< Properties reader
    TMonitorMethod  *mTriggerMethod;     //!< Method to trigger
    TTracer         *mTracer; 
    TMonitorClass   *mRefClass;    
    TXmlWriter       mWriter;
    jint             mNrMethods;
    int              mSocket;
    jlong            mNrCallsFkt;
    jlong            mNrCallsTrace;
    jlong            mGlobalRest;
    int              mFktCounter;
    TCallstack      *mCallstack;
    jlong            mGCTime;
    jint             mGCNr;
    jint             mGCUsageStart;
    jlong            mFrequency;
    jclass           mMxFact;
    jobject          mMxBean;
    jmethodID        mUsage;
    jlong            mNewAllocation;
    jlong            mNewObjects;
    TXmlTag          mTraceTag;
    TXmlTag         *mTraceEvent;
    bool             mInitialized;
    bool             mHandleException;
    static unsigned int gTransaction;
    static TMonitor     *mInstance;

    // ----------------------------------------------------
    // TMonitor::~TMonitor
    //! Destructor
    // ----------------------------------------------------
    virtual ~TMonitor() {        
        if (mCallstack != NULL) {
            delete mCallstack;
        }

        if (mRawMonitorThreads != NULL) {
            delete mRawMonitorThreads;
        }
        
        if (mRawMonitorMemory  != NULL) {
            delete mRawMonitorMemory;
        }
        
        if (mRawMonitorAccess  != NULL) {
            delete mRawMonitorAccess;
        }
        
        if (mRawMonitorOutput  != NULL) {
            delete mRawMonitorOutput;
        }
    }
    // ----------------------------------------------------
    // TMonitor::TMonitor
    //! Constructor
    // ----------------------------------------------------
    TMonitor()
        : 
          mMethods(237951, true),
          mHashExceptions(1024),
          mWriter(),
          mTraceTag(cU("Traces"), XMLTAG_TYPE_NODE)  {

        mNewAllocation      = 0;
        mNewObjects         = 0;
        mNrCallsFkt         = 0;
        mNrCallsTrace       = 0;
        mFktCounter         = 0;
        mNrMethods          = 0;
        mSocket             = 0;
        mGCTime             = 0;
        mGCNr               = 0;
        mGlobalRest         = 0;
        mInitialized        = false;
        mRefClass           = NULL;
        mTriggerMethod      = NULL;
        mProperties         = TProperties::getInstance();
        mTracer             = TTracer::getInstance();
        mCallstack          = NULL;
        mMxFact             = NULL;
        mMxBean             = NULL;
        mUsage              = NULL;

        mTraceTag.addTag(cU(""));
        mContextClasses.reset();
        mContextMethods.reset();

        if (mProperties->getProfilerMode() == PROFILER_MODE_ATS) {   
            mCallstack = new TCallstack(512);
        }
    }
public:
    // ----------------------------------------------------
    // ----------------------------------------------------
    unsigned int getTransaction() {
        return gTransaction;
    }

    // ----------------------------------------------------
    // TMonitor::getInstance
    //! Singelton constructor
    // ----------------------------------------------------
    static TMonitor *getInstance() {
        if (mInstance == NULL) {
            mInstance = new TMonitor();
        }
        return mInstance;
    }
    // ----------------------------------------------------
    // TMonitor::initialize
    //! \brief Initialize monitors
    //! \param  aJvmti The Java tool interface
    // ----------------------------------------------------
    void initialize(
        jvmtiEnv *aJvmti) {

        if (!mInitialized) {
            mInitialized =true;
            mRawMonitorThreads  = new TMonitorMutex(aJvmti, cU("_MonitorThread"));
            mRawMonitorMemory   = new TMonitorMutex(aJvmti, cU("_MonitorMemory"));
            mRawMonitorAccess   = new TMonitorMutex(aJvmti, cU("_MonitorAccess"));
            mRawMonitorOutput   = new TMonitorMutex(aJvmti, cU("_MonitorOutput"));
        }
    }
    // ----------------------------------------------------
    // TMonitor::getCallstack
    //! \return The callstack for ATS mode
    // ----------------------------------------------------
    TCallstack *getCallstack() {
        return mCallstack;
    }
    // ----------------------------------------------------
    // TMonitor::getCallstack
    //!\brief Synchronize output
    //!
    // ----------------------------------------------------
    void syncOutput(TXmlTag *aRootTag, int aRequestType = -1) {
        mRawMonitorOutput->enter();
        mWriter.print(aRootTag, aRequestType);
        mRawMonitorOutput->exit();
    }
    // ----------------------------------------------------
    // ----------------------------------------------------
    void syncOutput(const SAP_UC *aText) {
        mRawMonitorOutput->enter();
        mWriter.printLine(aText);
        mRawMonitorOutput->exit();
    }
    // ----------------------------------------------------
    // TMonitor::onClassRegister
    //! \brief  Register class and methods to profiler
    // ----------------------------------------------------
    void JNICALL onClassRegister(
            jvmtiEnv        *aJvmti,
            TMonitorClass   *aClass,
            TMonitorMethod  *aMethod,
            TMemoryBit      *aMemBit) {

        jmethodID    jMethod;
        THashClasses::iterator   aItClass;
        THashMethods::iterator   aItMethod;
        jvmtiError               aResult;

        TMonitorLock aLockAccess(mRawMonitorAccess);
        aItClass = mClasses.find((long)aMemBit);
        
        if (aItClass == mClasses.end()) {
            // Trace ClassLoad events
            aItClass = mClasses.insert((long)aMemBit, aClass, 0, 0, 2);
            aMemBit->mTID = gTransaction;
            aMemBit->mCtx = aClass;
            aClass->setID((jlong)aMemBit);

            aResult = aJvmti->SetTag((jobject)aClass, (jlong )aMemBit);
            resetClass(aClass);
        }        

        if (aMethod != NULL) {
            jMethod     = reinterpret_cast<jmethodID>(aMethod);
            aItMethod   = mMethods.findInsert(jMethod, aMethod, aClass);

            aClass->registerMethod(aMethod);
            resetMethod(aMethod);
        }
    }   
    // ----------------------------------------------------
    // TMonitor::onClassPrepare
    //! \brief  Register class and methods to profiler
    //! \param  aJni   The Java native interface
    //! \param  jThread The current thread
    //! \param  jClass  The class to register
    // ----------------------------------------------------
    void JNICALL onClassPrepare(
            jvmtiEnv    *aJvmti,
            JNIEnv      *aJni,
            jthread      jThread,
            jclass       jClass) {

        int i;
        jlong                    jSize;
        THashClasses::iterator   aItClass;
        THashMethods::iterator   aItMethod;
        jboolean                 jIsInterface;
        TMemoryBit              *aMemBit;
        TMonitorClass           *aClass     = NULL;
        TMonitorMethod          *aMethod;
        TMonitorThread          *aThreadObj = NULL;
        bool                     aActivate  = false;
        jint                     aCntMethod = 0;
        jvmtiError               aResult;
        jmethodID               *aPtrMethod;

        TMonitorLock aLockAccess(mRawMonitorAccess);
        aResult = aJvmti->GetTag(jClass, (jlong *)&aMemBit);
        aResult = aJvmti->GetThreadLocalStorage(jThread, (void**)&aThreadObj);

        if (aMemBit == NULL) {
            jclass         jSuper       = NULL;
            TMonitorClass *aSuper       = NULL;
            jlong          aSuperTag    = 0;

            aJvmti->GetObjectSize(jClass, &jSize);
            jSuper  = aJni->GetSuperclass(jClass);

            if (jSuper != NULL) {
                aJvmti->GetTag(jSuper, &aSuperTag);
                aSuper  = findClass(aJvmti, aSuperTag);
            }
            aClass  = new TMonitorClass(aJvmti, jClass, aSuper);
            aMemBit = new TMemoryBit(aClass, jSize, gTransaction);
            aResult = aJvmti->SetTag(jClass, (jlong)aMemBit);
            aClass->setID((jlong)aMemBit);
            aJni->ExceptionClear();

            resetClass(aClass);

            // Trace ClassLoad events
            if (mTracer->doTraceClass()) {
                mRawMonitorOutput->enter();

                if (mTracer->doTraceStack()) {
                    TXmlTag aTagStack(cU("Traces"), XMLTAG_TYPE_NODE);
                    aTagStack.addAttribute(cU("Type"),      cU("TraceTrigger"));
                    aTagStack.addAttribute(cU("ClassName"), aClass->getName());
                    
                    int aLevel;
                    if (dumpSingleStack(aJvmti, aJni, &aTagStack, &aLevel, cU("ClassLoad"), aClass->getName(), true, 0, true,jThread) != NULL) {
                        mTracer->printTrace(&aTagStack, aLevel);
                    }
                }

                TXmlTag  aTagClass(cU("Trace"));
                traceClass(&aTagClass, aClass, cU("ClassLoad"), TSystem::getTimestamp());
                mTracer->print(&aTagClass, XMLWRITER_TYPE_LINE);
                mRawMonitorOutput->exit();
            }
        }
        else {
            aClass = aMemBit->mCtx;
        }
        aItClass = mClasses.findInsert((jlong)aMemBit, aClass, 0, 0, 2);        

        // Do not process JNI calls relating to registration
        if (aThreadObj != NULL) {
            aThreadObj->setProcessJni(true);
        }        
        aResult = aJvmti->IsInterface(jClass, &jIsInterface);
        aResult = aJvmti->GetClassMethods(jClass, &aCntMethod, &aPtrMethod);
        
        for (i = 0; i < aCntMethod; i++) {
            aActivate = false;
            aItMethod = mMethods.find(aPtrMethod[i]);
            if (aItMethod == mMethods.end()) {
                aMethod = new TMonitorMethod(aJvmti, aJni, aPtrMethod[i], jIsInterface, aClass, aClass->getName());
                mMethods.insert(aPtrMethod[i], aMethod, aClass);
            }
            else {
                aMethod = aItMethod->aValue;
            }
            aClass->registerMethod(aMethod);
            resetMethod(aMethod);
        }
        aJvmti->Deallocate((unsigned char*)aPtrMethod);
        aClass->registerFields(aJvmti, aJni, jClass);
        // Reset thread object for normal processing
        if (aThreadObj != NULL) {
            aThreadObj->setProcessJni(false);
        }
    }
    // ----------------------------------------------------
    // TMonitor::onObjectDelete
    //! \brief Remove class registration
    //! \param aTag   The hash to the associated memory
    // ----------------------------------------------------
    void JNICALL onObjectDelete(
            jvmtiEnv   *aJvmti,
            jlong       aTag) {

        THashClasses::iterator aItClass;
        TMonitorClass  *aClass;
        TMemoryBit     *aMemBit = (TMemoryBit *)aTag;

        if (aMemBit == NULL) {
            return;
        }

        if (aMemBit->mIsClass) {
            aItClass = mClasses.remove(aTag);

            if (aItClass != mClasses.end()) {
                aClass = aItClass->aValue;
                if (mTracer->doTraceClass()) {
                    mRawMonitorOutput->enter();
                    TXmlTag  aTagClass(cU("Trace"));
                    traceClass(&aTagClass, aClass, cU("ClassUnload"), TSystem::getTimestamp());
                    mTracer->print(&aTagClass);
                    mRawMonitorOutput->exit();
                }
                mMethods.deleteArena(aClass);
                aClass->setDeleteFlag(true);

                if (aClass->deleteClass()) {
                    delete aClass;
                    aClass = NULL;
                }
                else {
                    mDelClasses.push_back(aClass);
                }
            }
            delete aMemBit;
        }
        else {
            if (aMemBit->mTID == gTransaction) {
                aClass = aMemBit->mCtx;
                if (aClass != NULL) {
                    aClass->deallocate(aMemBit->mSize);
                    mNewAllocation -= aMemBit->mSize;
                    mNewObjects--;
                }
            }
            delete aMemBit;
        }
    }
    // ----------------------------------------------------
    // TMonitor::findClass
    //! \brief Find a class using its hash value
    //! \param aTagValue The hash to the class
    // ----------------------------------------------------
    TMonitorClass *findClass(
            jvmtiEnv        *aJvmti, 
            jlong            aTagValue) {

        THashClasses::iterator aItClass;
        
        TMonitorLock aLockAccess(mRawMonitorAccess, true, false);
        aItClass = mClasses.find(aTagValue);

        if (aItClass != mClasses.end()) {
            return aItClass->aValue;
        }
        else {
            return NULL;
        }
    }
    // ---------------------------------------------------------
    // TMonitor::findSuperClass
    // ---------------------------------------------------------
    TMonitorClass *findClass(
            jvmtiEnv        *aJvmti,
            const SAP_UC    *aClassName) {

        THashClasses::iterator aPtr;
        TMonitorClass   *aClass = NULL;
        bool             aFound = false;

        if (STRLEN(aClassName) == 0) {
            return NULL;
        }
        TMonitorLock aLockAccess(mRawMonitorAccess, true, false);
        for (aPtr  = mClasses.begin();
             aPtr != mClasses.end() && !aFound;
             aPtr  = mClasses.next()) {
            aClass = aPtr->aValue;
            aFound = aClass->filterName(aClassName);
        }
        if (!aFound) {
            return NULL;
        }
        return aClass;
    }
    // ----------------------------------------------------
    // TMonitor::onObjectAlloc
    //! \brief JVMTI callback
    //! \param  aJni    The java native interface
    //! \param  jThread The current thread
    //! \param  jObject The calling object
    //! \param  jClass  The class of the calling object
    //! \param  aSize   The requested size for the object
    // ----------------------------------------------------
    inline bool onObjectAlloc(
            jvmtiEnv    *aJvmti,
            JNIEnv      *aJni,
            jthread      jThread,
            jobject      jObject,
            jclass       jClass,
            jlong        aSize) {

        jvmtiError      aResult;
        TMonitorThread *aThreadObj;
        TMonitorClass  *aCtxClass;
		
		if (!mProperties->doMonitorMemoryOn()) {
			return false;
		}

        aResult = aJvmti->GetThreadLocalStorage(jThread, (void**)&aThreadObj);
        if (aResult    != JVMTI_ERROR_NONE ||
            aThreadObj == 0                ||
           !aThreadObj->doCheck(false)) {
            return false;
        }

        aCtxClass = aThreadObj->getCallstack()->top()->getMethod()->getClass();
        doObjectAlloc(aJvmti, aJni, aThreadObj, jClass, jObject, aSize, NULL, aCtxClass);
        return true;
    }

    // ----------------------------------------------------
    // TMonitor::doObjectRealloc
    // ----------------------------------------------------
    void doObjectRealloc(
            jvmtiEnv        *aJvmti,
            JNIEnv          *aJni,
            TMonitorThread  *aThread,
            jobject          aObject,
            jlong            aNewSize) {
        
        jint             aResult;
        jlong            aObjTag     = 0;
        TMemoryBit      *aMemBit;
        TMonitorClass   *aContext;
        jlong            aOldSize;

        aResult = aJvmti->GetTag(aObject, &aObjTag);
        if (aObjTag != 0) {
            aMemBit  = (TMemoryBit *)aObjTag;
            aOldSize = aMemBit->mSize;

            if (aMemBit->mTID == gTransaction) {
                aMemBit->mSize  = aNewSize;
            
                aContext = aMemBit->mCtx;
                aContext->deallocate(aOldSize);
                aContext->allocate(aNewSize, mGCTime, mGCNr);

                mNewAllocation -= aOldSize;
                mNewAllocation += aNewSize;
            }
        }
    }

    // ----------------------------------------------------
    // TMonitor::doObjectAlloc
    //! \see TMonitor::onObjectAlloc
    // ----------------------------------------------------
    inline void doObjectAlloc(
            jvmtiEnv        *aJvmti,
            JNIEnv          *aJni,
            TMonitorThread  *aThread,
		    jclass           jClass,
            jobject          aObject,
            jlong            aSize,
            TMonitorClass   *aMemCls,
            TMonitorClass   *aContext) {

        TMemoryBit     *aMemBit     = NULL;
        TMemoryBit     *aClsBit     = NULL;
        jint            aResult;
        TXmlTag        *aTag        = NULL;
        jlong           aObjTag     = 0;
        jlong           aClassTag   = 0;
        jclass          jStrClz     = NULL;
        
        aResult = aJvmti->GetTag(aObject, &aObjTag);
		
        if (aObjTag != 0) {
            aMemBit = (TMemoryBit *)aObjTag;
            aMemBit->mCtx->deallocate(aMemBit->mSize, 0);
			aMemBit->mSize = aSize;
            aMemBit->mCtx  = aContext;
            aMemBit->mCtx->allocate(aMemBit->mSize, mGCTime, mGCNr);
            return;
        }
        else {
            aMemBit = new TMemoryBit(aContext, 0, gTransaction, false);
            aResult = aJvmti->SetTag(aObject, (jlong)aMemBit);
            mNewObjects++;
        }

        if (aSize == 0) {
            return;
        }

		
		if (aMemCls == NULL) {
			if (jClass == NULL) {
				jClass = aJni->GetObjectClass(aObject);
			}
			aResult = aJvmti->GetTag(jClass, (jlong *)&aClsBit);

			if (aClsBit == NULL) {
				jlong xSize;
				aResult = aJvmti->GetObjectSize(jClass, &xSize);
				TMonitorClass *xNewClass = new TMonitorClass(aJvmti, jClass, NULL);
				aClsBit = new TMemoryBit(xNewClass, xSize, gTransaction);
				aResult = aJvmti->SetTag(jClass, (jlong)aClsBit);
				xNewClass->setID((jlong)aClsBit);
				resetClass(xNewClass);
				mClasses.insert((long)aClsBit, xNewClass);
				aJni->ExceptionClear();

				//---aThread->setProcessJni(true);
				//---jmethodID mid = aJni->GetMethodID(jClass, "toString", "()Ljava/lang/String;");
				//---jstring strObj = (jstring)aJni->CallObjectMethod(jClass, mid);
				//---const char *str = aJni->GetStringUTFChars(strObj, NULL);
				//---//aMemBit = new TMemoryBit(aPtr->aValue, 0, gTransaction);
				//---//aResult = aJvmti->SetTag(jClass, (jlong)aMemBit);
				//---aThread->setProcessJni(false);
			}
		}
		

        mNewAllocation  += aSize;
        aMemBit->mSize   = aSize;        
        aContext->allocate(aSize, mGCTime, mGCNr);

        if (mProperties->doHistoryAlert() &&
            aContext->getAlert()) {

            SAP_UC   aBuffer[128];
            TXmlTag  aRootTag(cU("Traces"), XMLTAG_TYPE_NODE);

            aRootTag.addAttribute(cU("Type"),  cU("Leak"));
            aRootTag.addAttribute(cU("Class"), aContext->getName());

            TXmlTag *aTagClass = aRootTag.addTag(cU("Class"), XMLTAG_TYPE_NODE);
			aContext->dump(aTagClass);

            aTag = aTagClass->addTag(cU("List"), XMLTAG_TYPE_NODE);
            aTag->addAttribute(cU("Detail"), cU("History"));
			aTag->addAttribute(cU("ID"),     TString::parseHex((jlong)aContext->getID(), aBuffer));
			aContext->dumpHistory(aTag);

            aTag = aTagClass->addTag(cU("List"), XMLTAG_TYPE_NODE);
            aTag->addAttribute(cU("Detail"), cU("Heap"));
			aTag->addAttribute(cU("ID"),     TString::parseHex((jlong)aContext->getID(), aBuffer));
			dumpHeap(aJvmti, aTag, aContext->getID(), NULL);

			mMemoryLeaks.insert(aContext->getID(), aContext);
            syncOutput(&aRootTag);            
            aContext->resetAlert();
        }
    }
    // ----------------------------------------------------
    // TMonitor::deleteThreadObj
    //! \brief  JVMTI callback
    //! \param  jThread The current thread
    // ----------------------------------------------------
    void onThreadEnd(
            jvmtiEnv    *aJvmti,
            JNIEnv      *aJni,
            jthread      jThread) {

        jvmtiError      aResult;
        TMonitorThread *aThreadObj;
        TMonitorLock    aLock(mRawMonitorThreads);

        aResult = aJvmti->GetThreadLocalStorage(jThread, (void**)&aThreadObj);
        if (aResult == JVMTI_ERROR_THREAD_NOT_ALIVE) {
            return;
        }

        aResult = aJvmti->SetThreadLocalStorage(jThread, NULL);
        if (aThreadObj == 0) {
            return;
        }
        delete aThreadObj;
    }
    // ----------------------------------------------------
    // TMonitor::onBreakpoint
    //! \brief JVMTI callback
    //! \param  aJni    The Java native interface
    //! \param  jThread The current thread
    //! \param  jMethod The current method
    //! \param  jLocation The source code location
    // ----------------------------------------------------
    inline void onBreakpoint(
            jvmtiEnv   *aJvmti,
            JNIEnv     *aJni,
            jthread     jThread,
            jmethodID   jMethod,
            jlocation   jLocation) {

        THashMethods::iterator aPtr;
        jvmtiError      aResult;
        TMonitorThread *aThread = NULL;
        TMonitorMethod *aMethod;
        TMonitorTimer  *aTimer;
        TCallstack     *aCallstack;
        jint            aCount;
        bool            aFound;

        aResult = aJvmti->GetThreadLocalStorage(jThread, (void**)&aThread);
        if (aThread == NULL) {
            aThread = new TMonitorThread(aJvmti, aJni, jThread);
            aResult    = aJvmti->SetThreadLocalStorage(jThread, (void *)aThread);
        }

        if (aThread->getProcessJni()) {
            return;
        }
        mRawMonitorAccess->enter(false);
            aPtr    = mMethods.find(jMethod);
            aMethod = aPtr->aValue;
            aFound  = (aPtr != mMethods.end());
        mRawMonitorAccess->exit();

        if (!aFound) {
            return;
        }
        
        if (jLocation == aMethod->getStartLocation()) {
            onMethodEnter(aJvmti, aJni, jThread, jMethod, aMethod, aThread);
        }

        if (jLocation == aMethod->getEndLocation()) {  
            aResult    = aJvmti->GetFrameCount(jThread, &aCount);
            aCallstack = aThread->getCallstack();
            
            while (!aCallstack->empty()) {
                aTimer = aCallstack->top();
                                
                if (aCount > aTimer->getCount()) {
                    onMethodExit(aJvmti, aJni, jThread, jMethod, aThread, aMethod);
                    break;
                }
                else {
                    onMethodExit(aJvmti, aJni, jThread, aTimer->getMethod()->getID(), aThread);
                }
            }
        }
    }
    // ----------------------------------------------------
    // TMonitor::onMethodEnter
    //! \brief JVMTI callback
    //! \param  aJni   The Java native interface
    //! \param  jThread The current thread
    //! \param  jMethod The current method
    //! \param  aMethod The profiler method: Faster access if not NULL.
    //! \param  aThread The profiler thread: Faster access if not NULL.
    //! \param  aCount  The stack frame count, used to synchronize call stacks.
    // ----------------------------------------------------
    inline void onMethodEnter(
        jvmtiEnv        *aJvmti, 
        JNIEnv          *aJni,
        jthread          jThread,
        jmethodID        jMethod,
        TMonitorMethod  *aMethod = NULL,
        TMonitorThread  *aThread = NULL) {

        THashMethods::iterator aPtr;
		TMonitorMethod *xMethod     = aMethod;
        jvmtiError      aResult;
        bool            aFound      = (xMethod != NULL);
        jlong           aCpuTime    = 0;
        TCallstack     *aCallstack  = NULL;
        TCallstack     *aDebugStack = NULL;
        TMonitorTimer  *aTimer      = NULL;
        jint            aCount      = 1;
        
        if (aThread == NULL) {
            aResult = aJvmti->GetThreadLocalStorage(jThread, (void**)&aThread);
            if (aThread == NULL) {
                aThread = new TMonitorThread(aJvmti, aJni, jThread);
                aResult = aJvmti->SetThreadLocalStorage(jThread, (void *)aThread);
            }
        }
        
        if (aThread->getProcessJni()) {
            return;
        }
        mNrCallsFkt ++;

        // Find method in hash table
        if (!aFound) {
            mRawMonitorAccess->enter(false);
                aPtr    = mMethods.find(jMethod);
                xMethod = aPtr->aValue;
                aFound  = (aPtr != mMethods.end());
            mRawMonitorAccess->exit();
        }

        if (!aFound) {
            return;
        }

        if (xMethod->getStatus()) {
            aCallstack = aThread->getCallstack();
            aTimer     = aCallstack->push();
            aTimer->set(xMethod);

            // ------- Enter ATS -------------------------------------
            if (mProperties->getProfilerMode() == PROFILER_MODE_ATS) {
                //mJvmti->RawMonitorEnter(mRawMonitorThreads);
            }

            if (xMethod->checkContext(aCallstack, false)) {
                if (xMethod->getTimer()) {
                    aCpuTime = aThread->getCurrentCpuTime();
                }

                if (mProperties->getProfilerMode() == PROFILER_MODE_TRIGGER) {
                    aJvmti->GetFrameCount(jThread, &aCount);
                }
                else {
                    aCount = aCallstack->getDepth();
                }
                aTimer->set(xMethod, aCpuTime, aCount, aCallstack->getHighMemMark());
                xMethod->enter();

                // the trigger method
                if (xMethod == mTriggerMethod) {
                    mTracer->startTrigger(aThread->getName());
                }
            }
            
            // ------- Exit ATS -------------------------------------
            if (mProperties->getProfilerMode() == PROFILER_MODE_ATS) {   
                // mJvmti->RawMonitorExit(mRawMonitorThreads);
                return;
            }        
        }
        
        if (mTracer->doTraceMethod() && xMethod->getDebug()) {
            // trace enter and exit events
            aCallstack = aThread->getCallstack();

            if (xMethod->checkContext(aCallstack)) {
                mRawMonitorOutput->enter();
                    aCpuTime = aThread->getCurrentCpuTime();                    
                    TXmlTag  aRootTag(cU("Trace"));
                    TXmlTag *aParTag;
                    SAP_UC   aBuffer[128];

                    aDebugStack   = aThread->getDebugstack();
                    aTimer        = aDebugStack->push();
                    aTimer->set(xMethod, aCpuTime, aCount);
                    traceMethod(&aRootTag, xMethod, aThread->getID(), aTimer->getTimeStamp(), cU("Enter"), aDebugStack->getDepth(), cU(""));

                    if (mTracer->doTraceParameter()) {
                        aParTag = aRootTag.addTag(cU("Traces"), XMLTAG_TYPE_NODE);
                        aParTag->addAttribute(cU("Type"),       cU("Variables"));
                        aParTag->addAttribute(cU("MethodName"), xMethod->getName());
                        aParTag->addAttribute(cU("MethodId"),   TString::parseHex((jlong)xMethod->getID(), aBuffer));
                        aParTag->addAttribute(cU("ThreadId"),   TString::parseHex(aThread->getID(), aBuffer));
                        aParTag->addAttribute(cU("Info"),       cU("Arguments(enter)"));

                        dumpParameter(aJvmti, aJni, jThread, aParTag, xMethod);
                    }
                    mTracer->printTrace(&aRootTag);
                mRawMonitorOutput->exit();
            }
        }
        return;
    }
    // -------------------------------------------------------------------------------------
    // TMonitor::onMethodExit
    //! \brief JVMTI callback
    //! \param  aJvmti      The Java tool interface
    //! \param  aJni        The Java native interface
    //! \param  jThread     The current thread
    //! \param  jMethod     The current method
    //! \param  aThread     The profiler thread: Faster access if not NULL.
    //! \param  aCount      The stack frame count, used to synchronize call stacks.
    // -------------------------------------------------------------------------------------
    inline jmethodID onMethodExit(
            jvmtiEnv        *aJvmti, 
            JNIEnv          *aJni,
            jthread          jThread,
            jmethodID        jMethod,
            TMonitorThread  *aThread = NULL,
            TMonitorMethod  *xMethod = NULL) {

        jlong           aCpuTime    = 0;
        jlong           aElapsed    = 0;
        jlong           aMemory     = 0;
        jmethodID       jExitMethod = NULL;
        TMonitorMethod *aMethod     = NULL;
        TMonitorTimer  *aTimer      = NULL;
        TCallstack     *aCallstack  = NULL;
        jvmtiError      aResult;
        bool            aDebugTrc   = false;
        bool            aMemoryTrc  = false;


        if (aThread == NULL) {
            aResult = aJvmti->GetThreadLocalStorage(jThread, (void**)&aThread);
        }
        
        if (aThread == NULL || aThread->getProcessJni()) {
            return NULL;
        }

        aDebugTrc  = !aThread->getCallstack()->empty();

        if (aDebugTrc) {
            aCallstack = aThread->getCallstack();
            aTimer     = aCallstack->top();
            aMethod    = aTimer->getMethod();  
            aDebugTrc  = (aMethod->getID() == jMethod);
			aMemoryTrc = mProperties->doMonitorMemoryOn();
		}
        
        // Calculate memory
        while (aMemoryTrc) {
            THashMethods::iterator aPtr;
            TMonitorMethod *aMemMethod;
            jobject         jObject;
            jlong           aSize;

            // propagate memory info through stack:
            // find <init> or <cinit> on the stack and estimate the object size 
            aMemoryTrc = false;
            aMemMethod = aMethod;
            
            if (aMemMethod->getID() != jMethod) {
                if (xMethod != NULL) {
                    aMemMethod = xMethod;
                }
                else {
                    mRawMonitorAccess->enter(false);
                    aPtr = mMethods.find(jMethod);
                    if (aPtr != mMethods.end()) {
                        aMemMethod = aPtr->aValue;
                    }
                    mRawMonitorAccess->exit();
                }
            }

            // calculate memory for profile points
            if (aMemMethod != NULL && aMemMethod->isProfPointMem()) {
                aResult = aJvmti->GetLocalObject(jThread, 0, 0, &jObject);
                if (aResult != JVMTI_ERROR_NONE) {
                    break;
                }
                aResult = aJvmti->GetObjectSize(jObject, &aSize);
                if (aResult != JVMTI_ERROR_NONE) {
                    break;
                }
				
                doObjectAlloc(aJvmti, aJni, aThread, NULL, jObject, aSize, aMemMethod->getClass(), aMethod->getClass());
                
                aCallstack->incHighMemMark(aSize);
                aMemory = max(0, (int)(aCallstack->getHighMemMark() - aTimer->getMemory()));
            }
            break;
        }

        // Calculate time
        if (aDebugTrc) {
            if (aMethod->getTimer()) {
                aCpuTime = max(0, (int)(aThread->getCurrentCpuTime() - aTimer->getTime()));
                aElapsed = max(0, (int)aTimer->getElapsed());
                aThread->setTimer(aCpuTime);
            }
        
            mRawMonitorAccess->enter(false);
            aMethod->exit(aCpuTime, aElapsed);
            mRawMonitorAccess->exit();

            jlong aTraceType;
            jlong aTraceInfo;
        
            mNrCallsTrace += max(0, aCallstack->getDepth() - aCallstack->getSequence());

            if (aCallstack->beginSequence() != aCallstack->end() &&
                mTracer->doTraceEvent(
                    aThread->getName(), 
                    aMethod->getDebug(), 
                    aElapsed, 
                    aMemory, 
                    &mNrCallsTrace, 
                    &aTraceType, 
                    &aTraceInfo)) {

                int      aLevel;
                bool     aFound;
                TXmlTag  aRootTag(cU("Traces"), XMLTAG_TYPE_NODE);
                TXmlTag *aTag;
                SAP_UC   aBuffer[32];

                aRootTag.addAttribute(cU("Type"), cU("TraceTrigger"));
                
                if (mProperties->getProfilerMode() == PROFILER_MODE_TRIGGER) {
                    aLevel = 0;
                    aFound = true;
                    dumpSingleStack(aJvmti, aJni, &aRootTag, &aLevel, mTracer->getTraceEvent(aTraceType), TString::parseInt(aTraceInfo, aBuffer), true, 1, true, jThread, aThread);
                }
                else {
                    aTag = aRootTag.addTag(cU("Traces"), XMLTAG_TYPE_NODE);
                    aTag->addAttribute(cU("Type"), cU("Callstack"));
                    aThread->dump(aTag, aThread->getCallstack()->getDepth());                
                    aLevel = traceStackEvent(aTag, mTracer->getTraceEvent(aTraceType), TString::parseInt(aTraceInfo, aBuffer), &aFound, aThread, aThread->getCallstack());
                }

                if (aFound) {
                    TMonitorLock aLockOutput(mRawMonitorOutput);
                    mTracer->printTrace(&aRootTag, aLevel);
                }
            }

            if (aMethod == mTriggerMethod) {
                mTracer->stopTrigger();
            }
            jExitMethod = jMethod;
            aCallstack->pop();
        }

        // debug output methods
        aDebugTrc = !aThread->getDebugstack()->empty();
        if (aDebugTrc) {
            aCallstack = aThread->getDebugstack();
            aTimer     = aCallstack->top();
            aMethod    = aTimer->getMethod();  
            aDebugTrc  = (aMethod->getID() == jMethod);
        }

        if (aDebugTrc) {
            TMonitorLock  aLockOutput(mRawMonitorOutput);
            TXmlTag      *aTag;
            SAP_UC        aBuffer[32];

            aCallstack = aThread->getDebugstack();
            aCpuTime   = max(0, (int)(aThread->getCurrentCpuTime() - aTimer->getTime()));
            aTag       = mTraceTag.getLast(cU("Trace"));

            mNrCallsTrace++;

            traceMethod(aTag, aMethod, aThread->getID(), aTimer->getTimeStamp(), cU("Exit"), aCallstack->getDepth(), TString::parseInt(aCpuTime, aBuffer));
            mTracer->printTrace(aTag);
            aCallstack->pop();
        }
        return jExitMethod;
    }
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    inline void onFieldModification(
                jvmtiEnv        *aJvmti,
                JNIEnv          *aJni,
                jthread          jThread,
                jclass           jClass,
                jmethodID        jMethod,
                jfieldID         jField,   /*SAPUNICODEOK_CHARTYPE*/
                char             aType,
                jvalue           aValue) {

		THashMethods::iterator aPtrMethod;
        TMonitorThread *aThreadObj;
        TMonitorClass  *aCtxClass;
        TMonitorMethod *aMethod;
        TMonitorField  *aField;
        jvmtiError      aResult;
        jlong           aSize = 0;

		if (!mProperties->doMonitorMemoryOn()) {
			return;
		}
        
		aResult = aJvmti->GetThreadLocalStorage(jThread, (void**)&aThreadObj);
        if (aResult != JVMTI_ERROR_NONE ||
            aThreadObj == 0 ||
            !aThreadObj->doCheck(false)) {
            return;
        }

        if (aType != cR('L')) {
            return;
        }
        aCtxClass   = aThreadObj->getCallstack()->top()->getMethod()->getClass();
        aPtrMethod  = mMethods.find( jMethod );
		if (aPtrMethod == mMethods.end()) {
			return;
		}
		aMethod = aPtrMethod->aValue;
        aField  = aMethod->getClass()->getField( jField );

        if (aValue.l != NULL && aField != NULL) {
            aSize = aField->getArraySize(aJvmti, aJni, (jarray)aValue.l);
            doObjectAlloc(aJvmti, aJni, aThreadObj, jClass, aValue.l, aSize, aMethod->getClass(), aCtxClass);
        }
    }
    // -----------------------------------------------------------------
    // TMomitor::dumpParameter
    //! \brief Dump calling parameter
    //! \param  jJvmti      The Java tool interface
    //! \param  aJni        The Java native interface
    //! \param  jThread     The current thread
    //! \param  aRootTag    The output tag list
    //! \param  aMethod     The profiler method
    // -----------------------------------------------------------------
    void dumpParameter(
                jvmtiEnv        *aJvmti,
                JNIEnv          *aJni,
                jthread          jThread,                 
                TXmlTag         *aRootTag,
                TMonitorMethod  *aMethod) {

        int                      i;
        bool                     aFound = false;
        jint                     aResult;
        jint                     aNrArguments;
        jint                     aNrVariables;
        TString                  aName;
        TValues                 *aDebugAttributes; 
        TMonitorThread          *aThread;
        TValues::iterator        aPtr;
        jvmtiLocalVariableEntry *aTable;

        aResult = aMethod->getVariableTable(&aTable, &aNrVariables);
        if (aResult != 0) {
            return;
        }
        aResult = aJvmti->GetThreadLocalStorage(jThread, (void**)&aThread);
        aDebugAttributes = aMethod->getDebugAttributes();
        // Dump the parameter values according to their type
        aJvmti->GetArgumentsSize(aMethod->getID(), &aNrArguments);
        for (i = 0; i < aNrVariables; i++) {

            TString asExpression;
            TString asVarName;
            TString asVarSign;

            aFound = true;

            if (aDebugAttributes != NULL && 
                aDebugAttributes->getDepth() > 0) {

                for (aPtr  = aDebugAttributes->begin();
                     aPtr != aDebugAttributes->end();
                     aPtr ++) {

                    asExpression = *aPtr;
                    asVarName.assignR(aTable[i].name, STRLEN_A7(aTable[i].name));
                    if (!STRNCMP(asExpression.str(), asVarName.str(), asVarName.pcount())) {
                        dumpVariable(aJvmti, aJni, jThread, aRootTag, asExpression.str(), aTable + i);
                    }   
                }
            }
            else {
                if (aTable[i].slot< aNrArguments) {
                    asExpression.assignR(aTable[i].name, STRLEN_A7(aTable[i].name));
                    dumpVariable(aJvmti, aJni, jThread, aRootTag, asExpression.str(), aTable + i);
                }
            }
        }
    }
    // -----------------------------------------------------------------
    //! \brief Dump a variable
    //!
    //! \param aJvmti       The Java tool interface
    //! \param aJni         The Java native interface
    //! \param jThread      The current thread
    //! \param aRootTag     The result tag list 
    //! \param aExpression  The variable expression as specified in *.skp
    //! \param aVariable    The java variable description
    // -----------------------------------------------------------------
    void dumpVariable(
                jvmtiEnv                *aJvmti,
                JNIEnv                  *aJni,
                jthread                  jThread,
                TXmlTag                 *aRootTag,
                const SAP_UC            *aExpression,
                jvmtiLocalVariableEntry *aVariable) {

        TXmlTag     *aTag;
        jvmtiError   aResult;
        TString      asName;
        TString      asResult;
        TString      asType;
        SAP_UC       aBuffer[128];
        jobject      jObject;
        jsize        aArrLen = 0;

        switch (aVariable->signature[0]) {
            case cR('L'): {
                jObject = NULL;

                if ((aResult = aJvmti->GetLocalObject(jThread, 0, aVariable->slot, &jObject)) != JVMTI_ERROR_NONE) {
                    ERROR_OUT(cU("TMonitor::dumpVariable"), aResult);
                    break;
                }
                
                if (jObject == NULL) {
                    aJni->ExceptionClear(); 
                    break;
                }
                dumpObject(aJvmti, aJni, aRootTag, jObject, aExpression);
                break;
            }
            case cR('I'):   /* integer */
            case cR('B'):   /* byte */
            case cR('C'):   /* SAP_UC */
            case cR('S'):   /* short */
            case cR('Z'): { /* boolean */ 
                jint jValue;
                if ((aResult = aJvmti->GetLocalInt(jThread, 0, aVariable->slot, &jValue)) != JVMTI_ERROR_NONE) {
                    ERROR_OUT(cU("TDebug::dumpBreakpoint"), aResult);
                    break;
                }
                asName.assignR(aVariable->name, STRLEN_A7(aVariable->name));
                aTag = aRootTag->addTag(cU("Trace"));
                aTag->addAttribute(cU("Type"),  cU("Variable"));
                aTag->addAttribute(cU("Name"),  asName.str());
                aTag->addAttribute(cU("Info"),  cU("int"));
                aTag->addAttribute(cU("Value"), TString::parseInt(jValue, aBuffer));
                break;
            }
            case cR('J'): { /* long */ 
                jlong jValue;
                if ((aResult = aJvmti->GetLocalLong(jThread, 0, aVariable->slot, &jValue)) != JVMTI_ERROR_NONE) {
                    ERROR_OUT(cU("TDebug::dumpBreakpoint"), aResult);
                    break;
                }
                asName.assignR(aVariable->name, STRLEN_A7(aVariable->name));
                aTag = aRootTag->addTag(cU("Trace"));
                aTag->addAttribute(cU("Type"),  cU("Variable"));
                aTag->addAttribute(cU("Name"),  asName.str());
                aTag->addAttribute(cU("Info"),  cU("long"));
                aTag->addAttribute(cU("Value"), TString::parseInt(jValue, aBuffer));
                break;
            }
            case cR('F'): { /* float */
                jfloat jValue;
                if ((aResult = aJvmti->GetLocalFloat(jThread, 0, aVariable->slot, &jValue)) != JVMTI_ERROR_NONE) {
                    ERROR_OUT(cU("TDebug::dumpBreakpoint"), aResult);
                    break;
                }
                asName.assignR(aVariable->name, STRLEN_A7(aVariable->name));
                SAP_stringstream aStr;
                aStr    << jValue;
                asResult = aStr.str().c_str();

                aTag = aRootTag->addTag(cU("Trace"));
                aTag->addAttribute(cU("Type"),  cU("Variable"));
                aTag->addAttribute(cU("Name"),  asName.str());
                aTag->addAttribute(cU("Info"),  cU("float"));
                aTag->addAttribute(cU("Value"), asResult.str());
                break;
            }
            case cR('D'): { /* double */
                jdouble jValue;
                if ((aResult = aJvmti->GetLocalDouble(jThread, 0, aVariable->slot, &jValue)) != JVMTI_ERROR_NONE) {
                    ERROR_OUT(cU("TDebug::dumpBreakpoint"), aResult);
                    break;
                }
                asName.assignR(aVariable->name, STRLEN_A7(aVariable->name));
                SAP_stringstream aStr;
                aStr    << jValue;
                asResult = aStr.str().c_str();

                aTag = aRootTag->addTag(cU("Trace"));
                aTag->addAttribute(cU("Type"),  cU("Variable"));
                aTag->addAttribute(cU("Name"),  asName.str());
                aTag->addAttribute(cU("Info"),  cU("double"));
                aTag->addAttribute(cU("Value"), asResult.str());
                break;
            }
            case cR('['): {
                dumpArray(aJvmti, aJni, jThread, aRootTag, NULL, aVariable);
                break;
            }
            default: {
                break;
            }

        }        
    }
    // -----------------------------------------------------------------
    //! \brief Dump a array
    //!
    //! \param aJvmti       The Java tool interface
    //! \param aJni         The Java native interface
    //! \param jThread      The current thread
    //! \param aRootTag     The result tag list 
    //! \param aExpression  The variable expression as specified in *.skp
    //! \param aVariable    The java variable description
    // -----------------------------------------------------------------
    void dumpArray(
                jvmtiEnv                *aJvmti,
                JNIEnv                  *aJni,
                jthread                  jThread,
                TXmlTag                 *aRootTag,   
                const SAP_UC            *aExpression,
                jvmtiLocalVariableEntry *aVariable) {

        TString      asName;
        TString      asResult;
        TXmlTag     *aTag;
        jobject      jObject;

        switch (aVariable->signature[1]) {
            case cR('L'): {
                jobjectArray jArray;
                jobject      jElement;
                jsize        jSize;

                aJvmti->GetLocalObject(jThread, 0, aVariable->slot, &jObject);
                jArray    = (jobjectArray)jObject;
                jSize     = aJni->GetArrayLength(jArray);

                for (int i = 0; i < max((int)jSize, 64); i++) {
                    jElement  = aJni->GetObjectArrayElement(jArray, i);
                    dumpObject(aJvmti, aJni, aRootTag, jElement, aExpression);
                }
                break;
            }
            case cR('I'): { /* integer  */
                jintArray  jArray;
                jint      *jElements;
                jsize      jSize;
                SAP_stringstream aStr;

                aJvmti->GetLocalObject(jThread, 0, aVariable->slot, &jObject);
                jArray    = (jintArray)jObject;
                jSize     = aJni->GetArrayLength(jArray);
                jElements = aJni->GetIntArrayElements(jArray, NULL);
                
                for (int i = 0; i < max((int)jSize, 64); i++) {
                    aStr << jElements[i];  
                    if (i < jSize - 1) {
                        aStr << cU(',');
                    }
                }
                asName.assignR(aVariable->name, STRLEN_A7(aVariable->name));
                asResult = aStr.str().c_str();

                aTag = aRootTag->addTag(cU("Trace"));
                aTag->addAttribute(cU("Type"),  cU("Variable"));
                aTag->addAttribute(cU("Name"),  asName.str());
                aTag->addAttribute(cU("Info"),  cU("int[]"));
                aTag->addAttribute(cU("Value"), asResult.str());
                aJni->ReleaseIntArrayElements(jArray, jElements, 0);

                break;
            }
            case cR('B'): { /* byte     */
                jbyteArray jArray;
                jbyte     *jElements;
                jsize      jSize;

                aJvmti->GetLocalObject(jThread, 0, aVariable->slot, &jObject);
                jArray    = (jbyteArray)jObject;
                jSize     = aJni->GetArrayLength(jArray);
                jElements = aJni->GetByteArrayElements(jArray, NULL);

                asName.assignR(aVariable->name, STRLEN_A7(aVariable->name));
                asResult.assignBytes(jElements, jSize);

                aTag = aRootTag->addTag(cU("Trace"));
                aTag->addAttribute(cU("Type"),  cU("Variable"));
                aTag->addAttribute(cU("Name"),  asName.str());
                aTag->addAttribute(cU("Info"),  cU("byte[]"));
                aTag->addAttribute(cU("Value"), asResult.str());

                aJni->ReleaseByteArrayElements(jArray, jElements, 0);
                break;
            }
            case cR('C'): { /* SAP_UC   */
                jcharArray jArray;
                jchar     *jElements;
                jsize      jSize;

                aJvmti->GetLocalObject(jThread, 0, aVariable->slot, &jObject);
                jArray    = (jcharArray)jObject;
                jSize     = aJni->GetArrayLength(jArray);
                jElements = aJni->GetCharArrayElements(jArray, NULL);

                asName.assignR(aVariable->name, STRLEN_A7(aVariable->name));
                asResult.assign(jElements, jSize);

                aTag = aRootTag->addTag(cU("Trace"));
                aTag->addAttribute(cU("Type"),  cU("Variable"));
                aTag->addAttribute(cU("Name"),  asName.str());
                aTag->addAttribute(cU("Info"),  cU("char[]"));
                aTag->addAttribute(cU("Value"), asResult.str());

                aJni->ReleaseCharArrayElements(jArray, jElements, 0);
                break;
            }
            case cR('S'): { /* short    */
                break;
            }
            case cR('Z'): { /* boolean  */ 
                break;
            }
        }        
    }
    // -----------------------------------------------------------------
    // TMonitor::traceMethod
    //! \brief Trace method call 
    //! \param  aTag        The output tag list
    //! \param  aMethod     The profiler method
    //! \param  aLevel      The output level for tree view
    //! \param  aThreadID   The hash of the current thread
    //! \param  aTimeStamp  Timestap on method enter
    //! \param  aInfo       Either time or memory
    // -----------------------------------------------------------------
    void traceMethod(
            TXmlTag         *aRootTag,
            TMonitorMethod  *aMethod,
            jlong            aThreadID,
            jlong            aTimeStamp,
            const SAP_UC    *aTraceEvent,
            jlong            aDepth,
            const SAP_UC    *aInfoText) {

        SAP_UC  aBuffer[128];
        int     aInx = 0;

        aRootTag->setAttribute(aInx++, cU("Type"),       cU("Method"));
        aRootTag->setAttribute(aInx++, cU("Event"),      aTraceEvent);
        aRootTag->setAttribute(aInx++, cU("MethodName"), aMethod->getName());
        aRootTag->setAttribute(aInx++, cU("ClassName"),  aMethod->getClass()->getName());
        aRootTag->setAttribute(aInx++, cU("CpuTime"),    TString::parseInt(aMethod->getCpuTime(), aBuffer));
        aRootTag->setAttribute(aInx++, cU("NrCalls"),    TString::parseInt(aMethod->getNrCalls(), aBuffer),     PROPERTY_TYPE_INT); 
        aRootTag->setAttribute(aInx++, cU("Depth"),      TString::parseInt(aDepth,    aBuffer),                 PROPERTY_TYPE_INT); 
        aRootTag->setAttribute(aInx++, cU("ID"),         TString::parseHex((jlong)aMethod->getID(), aBuffer),   PROPERTY_TYPE_HIDDEN);
        aRootTag->setAttribute(aInx++, cU("ThreadId"),   TString::parseHex(aThreadID,  aBuffer));
        aRootTag->setAttribute(aInx++, cU("Info"),       aInfoText);
        
        if (mTracer->doTraceContention(-1)) {
            aRootTag->setAttribute(aInx++, cU("ContElapsed"), TString::parseInt(aMethod->getContention(), aBuffer),   PROPERTY_TYPE_INT | PROPERTY_TYPE_MICROSEC);
            aRootTag->setAttribute(aInx++, cU("ContNrCalls"), TString::parseInt(aMethod->getNrContention(), aBuffer), PROPERTY_TYPE_INT);
        }
    }
    // -----------------------------------------------------------------
    // TMonitor::traceClass
    //! \brief Trace a class
    //! \param  aClass The class to trace
    //! \param  aTraceType Info for trace output
    //! \param  aTimeStamp The time of the trace request
    // -----------------------------------------------------------------
    void traceClass(
        TXmlTag       *aTag,
        TMonitorClass *aClass,
        const SAP_UC  *aTraceType,
        jlong          aTimeStamp) {

        SAP_UC   aBuffer[128];

        aTag->addAttribute(cU("Type"),      cU("Class"));
        aTag->addAttribute(cU("Event"),     (SAP_UC*)aTraceType);
        aTag->addAttribute(cU("ClassName"), aClass->getName());
        aTag->addAttribute(cU("Timestamp"), TString::parseInt(aTimeStamp, aBuffer));    
        aTag->addAttribute(cU("ID"),        TString::parseHex(aClass->getID(), aBuffer), PROPERTY_TYPE_HIDDEN);
    }
    // ----------------------------------------------------
    // TMonitor::dumpFullStack
    //! \brief Dump threads
    //! \param aJvmti       The Java tool interface
    //! \param aJni         The Java native interface
    //! \param aRootTag     The output tag list
    //! \param aOptions     The dump options
    // ----------------------------------------------------
    void dumpFullStack(
            jvmtiEnv    *aJvmti,
            JNIEnv      *aJni,
            TXmlTag     *aRootTag, 
            TValues     *aOptions) {

        int i;
        jint                 aID      = 0;
        jvmtiError           aResult;
        jlong                aMinSize = 0;
        TXmlWriter           aWriter;
        bool                 aNativeStack   = false;
        bool                 aDumpCallstack = false;
        bool                 aUseHash       = false;
        TString              aNameFilter;
        TString              aSortCol;
        TMonitorThread      *aThread        = NULL;
        TValues::iterator    aPtrAttr;
        TString              aThrName;
        jint                 aThreadCount;
        jthread             *jThreads;
            
        aNameFilter = cU(".");
        aSortCol    = cU("Clock");

        if (aOptions != NULL) {
            for (aPtrAttr  = aOptions->begin();
                 aPtrAttr != aOptions->end();
                 aPtrAttr  = aOptions->next()) {

                if (!STRNCMP(*aPtrAttr, cU("-c"), 2)) {
                    aDumpCallstack = true;
                }
                else if (!STRNCMP(*aPtrAttr, cU("-s"), 2)) {
                    aSortCol  = (*aPtrAttr) + 2;
                }
                else if (!STRNCMP(*aPtrAttr, cU("-j"), 3)) {
                    aNativeStack = true;
                }
                else if (!STRNCMP(*aPtrAttr, cU("-a"), 3)) {
                    aNativeStack = true;
                }
                else if (!STRNCMP(*aPtrAttr, cU("-k"), 3)) {
#if defined (_WINDOWS) 
                    if (!GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, GetCurrentProcessId())) {
                         GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, 0);
                    }
#else
                    kill(getpid(), SIGQUIT);
#endif
                    return;
                }
                else if (!STRNCMP(*aPtrAttr, cU("-m"), 2)) {
                    aMinSize = TString::toInteger(*aPtrAttr + 2);
                }
                else if (!STRNCMP(*aPtrAttr, cU("-x"), 2)) {
                    aID         = (jint)TString::toInteger(*aPtrAttr + 2);
                    aUseHash    = true;
                }
                else if (!STRNCMP(*aPtrAttr, cU("-n"), 2)) {
                    aNameFilter = *aPtrAttr + 2;
                    if (aNameFilter[0] == cU('0')) {
                        aID      = (jint)aNameFilter.toInteger();
                        aUseHash = true;
                    }
                }
            }
        }

        TMonitorLock aLockThread(mRawMonitorThreads);
        aResult = aJvmti->GetAllThreads(&aThreadCount, &jThreads);

        for (i = 0; i < aThreadCount; i++) {
            aResult = aJvmti->GetThreadLocalStorage(jThreads[i], (void**)&aThread);
            if (aThread == NULL) {
                aThread = new TMonitorThread(aJvmti, aJni, jThreads[i]);
                aResult = aJvmti->SetThreadLocalStorage(jThreads[i], (void *)aThread);
            }

            if (aUseHash) {
                if (aThread->getID() != aID) {
                    continue;
                }
            }
            else {
                aThrName = aThread->getName(jThreads[i]);    
                if (aThrName.findWithWildcard(aNameFilter.str(), cU('.')) == -1) {
                    continue;
                }
            }
            int aLevel = 0;
            aThread->getVirtualStack()->reset();
            dumpSingleStack(aJvmti, aJni, aRootTag, &aLevel, mTracer->getTraceEvent(EVENT_TRIGGER), cU(""), aNativeStack, aMinSize, aDumpCallstack, jThreads[i], aThread);
        }
        aResult = aJvmti->Deallocate((unsigned char *)jThreads);
        aRootTag->qsort(aSortCol.str());
    }
    // -----------------------------------------------------
    // TMonitorThread::dumpSingleStack
    //! \brief Dump the current thread calling stack
    //! \param  aJvmti          The Java tool interface
    //! \param  aJni            The Java native interface
    //! \param  aRootTag        The output tag list
    //! \param  aDoNativeTrace  Handle native/sherlok thread
    //! \param  aMinSize        Minimal stack depth
    //! \param  aDumpCallStack  Do native/sherlok stack trace
    //! \param  aTraceEvent     The event which causes a call to stack trace
    //! \param  aContTime       Contention time
    //! \param  jThread         The current native thread
    //! \param  aThread         The current sherlok thread
    //! \return The method on top of the stack
    // -----------------------------------------------------
    TMonitorMethod *dumpSingleStack(
            jvmtiEnv        *aJvmti,
            JNIEnv          *aJni,
            TXmlTag         *aRootTag,
            int             *aLevel,
            const SAP_UC    *aTraceType,
            const SAP_UC    *aTraceText,
            bool             aDoNativeTrace,
            jlong            aMinSize,
            bool             aDumpCallStack,
            jthread          jThread,
            TMonitorThread  *aThread = NULL) {

        jvmtiFrameInfo  aFrames[256];
        jint            aCount;
        jint            aDepth;
        jint            i;
        jint            aStackCnt = 0;
        bool            aFound;
        jvmtiError      aResult;
        jmethodID       jMethod;
        // jlong           aTime = 0;
        // jmethodID       jTopMethod  = NULL;
        TXmlTag        *aNodeTag = NULL;
        TMonitorTimer  *aTimer;
        TCallstack     *aPtrCallstack;
        TMonitorMethod *aMethod     = NULL;
        THashMethods::iterator aPtrMethod;
        
        if (jThread == NULL) {
            return NULL;
        }

        if (aThread == NULL) {
            aResult = aJvmti->GetThreadLocalStorage(jThread, (void**)&aThread);
            if (aThread == NULL) {
                aThread = new TMonitorThread(aJvmti, aJni, jThread);
                aResult = aJvmti->SetThreadLocalStorage(jThread, (void *)aThread);
            }
        }

        if (aDoNativeTrace) {
            aPtrCallstack = aThread->getVirtualStack();
            aResult = aJvmti->GetStackTrace(jThread, 0, 256, aFrames, &aCount);            
            aDepth  = aCount;

            if ((aMinSize > 0 && aDepth >= aMinSize) || aMinSize == 0) {
                aNodeTag = aRootTag->addTag(cU("Traces"), XMLTAG_TYPE_NODE);
                aNodeTag->addAttribute(cU("Type"), cU("Callstack"));
                aThread->dump(aNodeTag, aThread->getCallstack()->getDepth());
            }
            else {
                aDumpCallStack = false;
            }

            if (!aDumpCallStack || aDepth == 0) {
                return NULL;
            }
            
            mRawMonitorAccess->enter();
            aStackCnt = 0;
            bool aFoundMethod = true;

            for (i = aCount-1; i >= 0; i--) {
                jMethod = aFrames[i].method;
                
                aPtrMethod  = mMethods.find(jMethod);           
                aMethod     = aPtrMethod->aValue;
                
                // display methods with monitoring info only
                if (aPtrMethod == mMethods.end()) {
                    continue;
                }
                // check if method already on stack
                aTimer = (*aPtrCallstack)[aStackCnt];
                if (aFoundMethod && 
                    aTimer != aPtrCallstack->end() && 
                    aTimer->getMethod() == aMethod) {
                    aStackCnt++;
                    continue;
                }
                // put new methods on stack
                if (aFoundMethod) {
                    aFoundMethod = false;
                    aPtrCallstack->reset(aStackCnt);
                    aPtrCallstack->resetSequence(aStackCnt);
                }
                aTimer = aPtrCallstack->push();
                aTimer->set(aMethod, 0, 0, 0, aFrames[i].location);
                aStackCnt++;
            }
            aPtrCallstack->reset(aStackCnt);
            mRawMonitorAccess->exit();
        }
        else {
            aPtrCallstack = aThread->getCallstack();
            aDepth  = aPtrCallstack->getSize();

            if ((aMinSize > 0 && aDepth >= aMinSize) || aMinSize == 0) {
                aNodeTag = aRootTag->addTag(cU("Traces"), XMLTAG_TYPE_NODE);
                aNodeTag->addAttribute(cU("Type"), cU("Callstack"));
                aThread->dump(aNodeTag, aDepth);
            }
            else {
                aDumpCallStack = false;
            }
        }

        if (aDumpCallStack &&  
            aPtrCallstack->getDepth() > aPtrCallstack->getSequence() &&
            aNodeTag != NULL) {
            
            *aLevel = traceStackEvent(aNodeTag, aTraceType, aTraceText, &aFound, aThread, aPtrCallstack, true);             
            return aMethod;
        }
        else {
            return NULL;
        }

    }
    // -----------------------------------------------------------------
    // TMonitor::traceStackEvent
    //! \brief Trace stack on TIME or MEMORY event
    //! \param  aRootTag    The output tag list
    //! \param  aTraceType  The trace type info for output
    //! \param  aTraceEvent The trace event info for output
    //! \param  aThread     The current thread
    //! \param  aInfo       Either time or memory
    // -----------------------------------------------------------------
    int traceStackEvent(
            TXmlTag         *aRootTag,
            const SAP_UC    *aTraceType,
            const SAP_UC    *aTraceText,
            bool            *aFoundSequence,
            TMonitorThread  *aThread, 
            TCallstack      *aStack,
            bool             aForce = false) {

        TCallstack::iterator aPtr;
        TXmlTag         *aTag    = NULL;
        TMonitorMethod  *aMethod = NULL;
        TMonitorTimer   *aTimer  = NULL; 
        SAP_UC           aBuffer[32];
        int              aLevel  = aStack->getDepth();
        int              aStart  = aStack->getDepth();
        
        *aFoundSequence = false;

        if (aForce) {
            aPtr   = aStack->begin();
            aStart = 0;
            aLevel = 0;
        }
        else {
            aPtr = aStack->beginSequence();
        }
        
        while (aPtr != aStack->end()) {            
            if (!*aFoundSequence) {
                 *aFoundSequence = true;

                 if (!aForce) {
                    aLevel  = aStack->getSequence();
                    aStart  = aStack->getSequence();
                 }
            }

            if (aTimer != NULL) {
                aTag = aRootTag->addTag(cU("Trace"));
                traceMethod(aTag, aMethod, aThread->getID(), aTimer->getTimeStamp(), cU("Call"), aLevel+1, TString::parseInt(aTimer->getLocation(), aBuffer));
                aLevel ++;
            }
            aTimer  = aPtr;   
            aMethod = aTimer->getMethod();

            if (aForce) {
                aPtr = aStack->next();
            }
            else {
                aPtr  = aStack->nextSequence();
            }
        }
        
        if (*aFoundSequence) {
            aTag = aRootTag->addTag(cU("Trace"));
            traceMethod(aTag, aMethod, aThread->getID(), aTimer->getTimeStamp(), aTraceType, aLevel+1, aTraceText);
        }
        //aRootTag->addAttribute(cU("Depth"), TString::parseInt(aStack->getDepth(), aBuffer), PROPERTY_TYPE_INT);
        return aStart;
    }
    // -----------------------------------------------------------------
    // TMonitor::onThreadStart
    //! \brief Callback on start of a java thread
    //! \param jThread The current thread to start
    // -----------------------------------------------------------------
    void onThreadStart(      
            jvmtiEnv   *aJvmti,
            JNIEnv     *aJni,
            jthread     jThread) {

        jvmtiError       aResult;
        TMonitorThread  *aThread;
        TMonitorLock     aLock(mRawMonitorThreads);

        aThread = new TMonitorThread(aJvmti, aJni, jThread);
        aResult = aJvmti->SetThreadLocalStorage(jThread, (void *)aThread);
    }
    // -----------------------------------------------------------------
    // TMonitor::onExceptionCatch
    // -----------------------------------------------------------------
    void onExceptionCatch(
            jvmtiEnv   *aJvmti,
            JNIEnv     *aJni,
            jthread     jThread,
            jobject     jException,
            jmethodID   jCatchMethod,
            jlocation   jCatchLocation) {

        jint            aCount;
        jvmtiError      aResult;
        TMonitorThread *aThreadObj;
        TMonitorTimer  *aTimer;
        aResult = aJvmti->GetThreadLocalStorage(jThread, (void**)&aThreadObj);
        
        if (aThreadObj == NULL || !aThreadObj->hasCallstack()) {
            return;
        }

        if (mProperties->getProfilerMode() != PROFILER_MODE_TRIGGER) {
            return;
        }
        aResult = aJvmti->GetFrameCount(jThread, &aCount);

        while (!aThreadObj->getCallstack()->empty()) {
            aTimer = aThreadObj->getCallstack()->top();

            if (aTimer->getCount() <= aCount) {
                break;
            }
            onMethodExit(aJvmti, aJni, jThread, aTimer->getMethod()->getID(), aThreadObj);
        }
    }
    // -----------------------------------------------------------------
    // TMonitor::onException
    //! \brief JVMTI callback
    //! \param  aJvmti          The Java tool interface
    //! \param  aJni            The Java native interface
    //! \param  jThread         The current thread
    //! \param  jThrowMethod    The method which throws the exception
    //! \param  jThrowLocation  The source code location of the thowing method
    //! \param  jException      The exception 
    //! \param  jCatchMethod    The method which catches the exception
    //! \param  jCatchLocation  The source code location of the catching method
    //! \param  aCount          The stack depth
    // -----------------------------------------------------------------
    void onException(
            jvmtiEnv   *aJvmti,
            JNIEnv     *aJni,
            jthread     jThread,
            jmethodID   jThrowMethod,
            jlocation   jThrowLocation,
            jobject     jException,
            jmethodID   jCatchMethod,
            jlocation   jCatchLocation) {

        THashMethods::iterator    aPtr;
        THashExceptions::iterator aPtrException;
        TMonitorThread *aThreadObj;
        TMonitorMethod *aMethod;
        TXmlTag        *aRootTag = NULL;
        bool            doTrace  = false;
        jclass          jClass;

        /*SAPUNICODEOK_STRINGCONST*/
        char           *aSignature = NULL;
        char           *aGeneric   = NULL;
        
        SAP_UC          aBuffer[256];
        jvmtiError      aResult;

        aResult = aJvmti->GetThreadLocalStorage(jThread, (void**)&aThreadObj);
        
        jClass  = aJni->GetObjectClass(jException);
        aResult = aJvmti->GetClassSignature(jClass, &aSignature, &aGeneric);

        TString asSigException;
        TString aExceptionOOME;

        asSigException.assignR(aSignature, STRLEN_A7(aSignature));
        asSigException.replace(cU('/'), cU('.'));
        asSigException.cut(1, asSigException.pcount() - 1);

        aExceptionOOME = cU("java.lang.OutOfMemoryError");

        unsigned long aKey = asSigException.getHash();
        aPtrException      = mHashExceptions.find(aKey);
        if (aPtrException != mHashExceptions.end()) {
            aPtrException->aValue->mCnt++;
        }
        else {
            TException *aEx = new TException(asSigException.str());
            mHashExceptions.insert(aKey, aEx);
        }

        doTrace = aExceptionOOME.compareSignatur(aSignature) || 
                    mProperties->doTraceException(asSigException.str());

        aJvmti->Deallocate((unsigned char*)aSignature);
        aJvmti->Deallocate((unsigned char*)aGeneric);

        if (!doTrace) {
            return;
        }

        aRootTag = new TXmlTag(cU("Trace"));
        aRootTag->addAttribute(cU("Type"), cU("Exception"));
        aRootTag->addAttribute(cU("Timestamp"), TString::parseInt(TSystem::getTimestamp(), aBuffer));
        aRootTag->addAttribute(cU("Info"), asSigException.str());

        if (aThreadObj != NULL)  {
            aRootTag->addAttribute(cU("ThreadName"), aThreadObj->getName());
        }
        else {
            aRootTag->addAttribute(cU("ThreadName"), cU("Unknown"));
        }
    

        if (jThrowMethod != NULL) {
            TMonitorLock aLockAccess(mRawMonitorAccess);
            aPtr = mMethods.find(jThrowMethod);
            if (aPtr != mMethods.end()) {
                aMethod = aPtr->aValue;
                aRootTag->addAttribute(cU("ThrowMethod"),   aPtr->aValue->getName());
                aRootTag->addAttribute(cU("ThrowClass"),    aPtr->aValue->getClass()->getName());
                aRootTag->addAttribute(cU("ThrowLocation"), TString::parseInt(jThrowLocation, aBuffer));
            }
        }

        if (jCatchMethod != NULL) {
            TMonitorLock aLockAccess( mRawMonitorAccess);
            aPtr = mMethods.find(jCatchMethod);

            if (aPtr != mMethods.end()) {
                aMethod = aPtr->aValue;
                aRootTag->addAttribute(cU("CatchMethod"),   aPtr->aValue->getName());
                aRootTag->addAttribute(cU("CatchClass"),    aPtr->aValue->getClass()->getName());
                aRootTag->addAttribute(cU("CatchLocation"), TString::parseInt(jCatchLocation, aBuffer));
            }              
        }
        syncOutput(aRootTag);

        if (aThreadObj != NULL)  {
            aThreadObj->setProcessJni(true);
        }

        if (aExceptionOOME.compareSignatur(aSignature)) {

            TValues aOptionsThread(4);
            TString aStrOptionThread(cU("-m1,-a,-c"));
            aStrOptionThread.split(&aOptionsThread, cU(','));
            TXmlTag aThreadTag(cU("List"), XMLTAG_TYPE_NODE);
            aThreadTag.addAttribute(cU("Type"), cU("Threads"));
            dumpFullStack(aJvmti, aJni, &aThreadTag, &aOptionsThread);
            syncOutput(&aThreadTag);

            TValues aOptionsClass(4);
            TString aStrOptionClass (cU("-m10000"));
            aStrOptionClass.split(&aOptionsClass, cU(','));
            TXmlTag aClassTag(cU("List"), XMLTAG_TYPE_NODE);
            aClassTag.addAttribute(cU("Type"), cU("Classes"));
            dumpClasses(aJvmti, &aClassTag, &aOptionsClass);
            syncOutput(&aClassTag);

            TXmlTag aStatistic(cU("List"), XMLTAG_TYPE_NODE);
            aStatistic.addAttribute(cU("Type"), cU("Statistic"));
            dumpStatistic(aJvmti, &aStatistic);
            syncOutput(&aStatistic);
        }

        if (aThreadObj != NULL)  {
            aThreadObj->setProcessJni(false);
        }

        delete aRootTag;
    }

    // -----------------------------------------------------------------
    // TMonitor::dumpExceptions
    //! \brief Dump registered exception
    //! \param aRootTag The output tag list
    // -----------------------------------------------------------------
    void dumpExceptions(TXmlTag *aRootTag) {
        THashExceptions::iterator aPtr;
        TXmlTag *aTag;
        SAP_UC     aBuffer[64];

        for (aPtr  = mHashExceptions.begin();
             aPtr != mHashExceptions.end();
             aPtr  = mHashExceptions.next()) {
             
            aTag = aRootTag->addTag(cU("Exception"));
            aTag->addAttribute(cU("Name"),   aPtr->aValue->mName.str());
            aTag->addAttribute(cU("Count"),  TString::parseInt(aPtr->aValue->mCnt, aBuffer), PROPERTY_TYPE_INT);
        }
        aRootTag->qsort(cU("Count"));
    }
    // -----------------------------------------------------------------
    // TMonitor::dumpObject
    //! \brief Dumps an parameter object
    //! \param aJvmti       The Java tool interface
    //! \param aJni         The Java native interface
    //! \param aRootTag     The output tag list
    //! \param jObject      The object to dump
    //! \param aExpression  The debug expression to evaluate
    //! 
    //! The dump tries evaluate an expression of the form "parameter.a.b.c".
    //! The dot seprarted entries could be 
    //!     - a method with signatur "()L<object name>;"
    //!     - a field with signatur "L<object name>;"
    //!     - any field at the end of the expression
    //!
    //! The paramter is the name of any method parameter, "this" or any 
    //! method local variable.
    // -----------------------------------------------------------------
    void dumpObject(
                jvmtiEnv        *aJvmti,
                JNIEnv          *jEnv, 
                TXmlTag         *aRootTag, 
                jobject          jObject,
                const SAP_UC    *aExpression) {

        jvmtiError           aResult;
        jint                 i;
        SAP_A7              *aName;
        SAP_A7              *aSignature;
        SAP_A7              *aGeneric;
        bool                 aFound;
        jint                 aNrMethods;
        jmethodID           *jMethods;
        jmethodID            jMethod;
        TXmlTag             *aTag;
        jstring              jString;
        jclass               jClass     = NULL;
        jclass               jVarCls    = NULL;
        TString              asName;
        TString              asSign;
        TValues::iterator    aPtr;
        TValues              aExpressionStack(10);
        TString              asExpression;
        jint                 aFieldCount;
        jfieldID            *aFields;
        jfieldID             jField = NULL;
        SAP_UC               aBuffer[128];
        TString              asTagName;
        TString              asTagType;
        TString              asTagResult;
        TString              asClass;

        asTagName    = aExpression;
        asTagType    = cU("");
        asTagResult  = cU("");
        asExpression = aExpression;

        asExpression.split(&aExpressionStack, cU('.'));

        // Call expression stack
        aFound = true;
        if (jObject != NULL) {
            jClass  = jEnv->GetObjectClass(jObject);
            jVarCls = jClass;
            jMethod = jEnv->GetMethodID(jVarCls, cR("toString"), cR("()Ljava/lang/String;"));
            jString = (jstring)jEnv->CallObjectMethod(jVarCls, jMethod);
            asClass.assign(jEnv, jString);
            asClass.cut(asClass.findFirstOf(cU(' ')) + 1);
        }

        // Call all elements of the expression stack
        for (aPtr  = aExpressionStack.begin();
             aPtr != aExpressionStack.end() && (jObject != NULL) && aFound;
             aPtr ++)  {

            asExpression = *aPtr;
            aFound       = false;
            jField       = NULL;

            if (aPtr == aExpressionStack.begin()) {
                asName = asExpression.str();
                aFound = true;
                continue;
            }

            if (!STRCMP(asExpression.str(), cU("super"))) {
                jClass = jEnv->GetSuperclass(jClass);
                aFound = (jClass != NULL);
                continue;
            }

            if (!STRCMP(asExpression.str(), cU("getClass"))) {
                asName    = asExpression.str();
                jMethod   = jEnv->GetMethodID(jClass, cR("getClass"), cR("()Ljava/lang/Class;"));
                jObject   = jEnv->CallObjectMethod(jObject, jMethod);
                if (jObject != NULL) {
                    jClass    = jEnv->GetObjectClass(jObject);
                    aFound    = true;
                }
                continue;
            }

            // Find field by name
            aResult = aJvmti->GetClassFields(jClass, &aFieldCount, &aFields);
            for (i = 0; i < aFieldCount && !aFound; i++) {
                jField  = aFields[i];
                aResult = aJvmti->GetFieldName(jClass, jField, &aName, &aSignature, &aGeneric); 
                asName.assignR(aName, STRLEN_A7(aName));
                asSign.assignR(aSignature, STRLEN_A7(aSignature));
                aFound  = STRCMP(asName.str(), asExpression.str()) == 0;
                aJvmti->Deallocate((unsigned char *)aName);
                aJvmti->Deallocate((unsigned char *)aSignature);
                aJvmti->Deallocate((unsigned char *)aGeneric);
            }
            aJvmti->Deallocate((unsigned char *)aFields);

            if (aFound) {
                if (asSign[0] == cU('L')) {
                    jObject = jEnv->GetObjectField(jObject, jField);
                    jClass  = jEnv->GetObjectClass(jObject);
                    jField  = NULL;
                    continue;
                }
                else {
                    break;
                }
            }
            jField = NULL;

            // Find method by name
            aResult = aJvmti->GetClassMethods(jClass, &aNrMethods, &jMethods);
            for (i = 0; i < aNrMethods && !aFound; i++) {
                aJvmti->GetMethodName(jMethods[i], &aName, &aSignature, &aGeneric);
                asName.assignR(aName, STRLEN_A7(aName));
                asSign.assignR(aSignature, STRLEN_A7(aSignature));

                aJvmti->Deallocate((unsigned char *)aName);
                aJvmti->Deallocate((unsigned char *)aSignature);
                aJvmti->Deallocate((unsigned char *)aGeneric);
                
                if (STRCMP(asName.str(), asExpression.str()) != 0) {
                    continue;
                }

                if (STRNCMP(asSign.str(), cU("()L"), 3) == 0) {
                    aFound  = true;
                    jMethod = jMethods[i];
                    jObject = jEnv->CallObjectMethod(jObject, jMethod);
                    if (jObject != NULL) {
                        jClass  = jEnv->GetObjectClass(jObject);
                    }
                }

                if (STRNCMP(asSign.str(), cU("()[L"), 3) == 0) {
                    jMethod = jMethods[i];
                    jObject = jEnv->CallObjectMethod(jObject, jMethod);
                    jobjectArray jArrObj  = (jobjectArray)jObject;
                    jsize        jArrSize = jEnv->GetArrayLength(jArrObj);

                    for (int j = 0; j < jArrSize; j++) {
                        jclass    jArrClass  = jEnv->GetObjectClass(jArrObj);
                        jmethodID jArrToStr  = jEnv->GetMethodID(jArrClass, cR("toString"), cR("()Ljava/lang/String;"));
                        jobject   jArrObjStr = jEnv->CallObjectMethod(jArrObj, jArrToStr);
                        asTagResult.assign(jEnv, (jstring)jArrObjStr);

                        asTagType = cU("toString[");
                        asTagType.concat(TString::parseInt(i, aBuffer));
                        asTagType.concat(cU("]"));

                        aTag = aRootTag->addTag(cU("Trace"));
                        aTag->addAttribute(cU("Type"),  cU("Array"));
                        aTag->addAttribute(cU("Name"),  asTagName.str());
                        aTag->addAttribute(cU("Info"),  asTagType.str());
                        aTag->addAttribute(cU("Value"), asTagResult.str());
                    }
                }
            }
            aJvmti->Deallocate((unsigned char *)jMethods);
            jEnv->ExceptionClear(); 
        }

        jEnv->ExceptionClear(); 
        if (!aFound || jObject == NULL) {
            return;
        }        
        aFound = false;

        // Evaluate the display string
        if (jField != NULL) {
            aFound = true;

            switch (asSign[0]) {
                case cR('I'):   /* integer */
                case cR('B'):   /* byte */
                case cR('C'):   /* SAP_UC */
                case cR('S'):   /* short */
                case cR('Z'): { /* boolean */ 
                    jint jValue;
                    asTagType   = cU("int"); 
                    jValue      = jEnv->GetIntField(jObject, jField);
                    asTagResult = TString::parseInt(jValue, aBuffer);
                    break;
                }
                case cR('J'): { /* long */ 
                    jlong jValue;
                    asTagType   = cU("long"); 
                    jValue      = jEnv->GetLongField(jObject, jField);
                    asTagResult = TString::parseInt(jValue, aBuffer);
                    break;
                }
                case cR('F'): { /* float */
                    SAP_stringstream aStr;
                    jfloat        jValue;
                    asTagType   = cU("float"); 
                    jValue      = jEnv->GetFloatField(jObject, jField);
                    aStr        << jValue;
                    asTagResult = aStr.str().c_str();
                    break;
                }
                case cR('D'): { /* float */
                    SAP_stringstream aStr;
                    jdouble       jValue;
                    asTagType   = cU("double"); 
                    jValue      = jEnv->GetDoubleField(jObject, jField);
                    aStr        << jValue;
                    asTagResult = aStr.str().c_str();
                    break;
                }
                default:
                    aFound      = false;
                    break;
            }
        }
        else {
            // If not explicite mentioned: The last action is to retrieve a string
            if (STRCMP(asName.str(), cU("toString")) != 0) {
                jMethod = jEnv->GetMethodID(jClass, cR("toString"), cR("()Ljava/lang/String;"));
                jObject = jEnv->CallObjectMethod(jObject, jMethod);
            }

            // Set the output accordingly
            if (STRCMP(asName.str(), cU("getClass")) == 0) {
                asTagType = cU("getClass");
            }
            else {
                asTagType = cU("toString");
            }


            if (jObject != NULL) {
                aFound    = true;
                jString   = (jstring)(jObject);
                asTagResult.assign(jEnv, jString);
            }
        }        
        
        if (aFound) {
            aTag = aRootTag->addTag(cU("Trace"));
            aTag->addAttribute(cU("Type"),   cU("Variable"));
            aTag->addAttribute(cU("Name"),   asTagName.str());
            aTag->addAttribute(cU("Info"),   asTagType.str());
            aTag->addAttribute(cU("Value"),  asTagResult.str());
            aTag->addAttribute(cU("Class"),  asClass.str());
        }
        jEnv->ExceptionClear(); 
    }
    // ----------------------------------------------------
    // TMonitor::reset
    //! \brief Reset the profiler
    //! \param aJvmti       The Java tool interface
    //! \param aRootTag     The output tag list
    // ----------------------------------------------------
    void reset(
            jvmtiEnv        *aJvmti, 
            TXmlTag         *aRootTag) {

        stop(aJvmti, aRootTag);
        resetMonitorFields(aJvmti, true);
        if (mProperties->doMonitoring()) {
            start(aJvmti, aRootTag);
        }
    }
    // ----------------------------------------------------
    // TMonitor::resetMonitorFields
    //! \param aJvmti       The Java tool interface
    //! \param aAllowStart \c TRUE if restart is required
    // ----------------------------------------------------
    void resetMonitorFields(
            jvmtiEnv    *aJvmti,
            bool         aAllowStart,
		    bool         aInitVm = false) {
        
        mNrCallsFkt    = 0;
        mTriggerMethod = NULL;
        
        resetThreads(aJvmti);
        reset(aJvmti, &mClasses, aAllowStart);
        reset(aJvmti, &mContextClasses, aAllowStart);
        reset(aJvmti, &mMethods, aAllowStart);
        reset(aJvmti, &mContextMethods, aAllowStart);
        clearHeapDump(aJvmti);

        TMonitorLock aLockAccess(mRawMonitorAccess); 
        mMemoryLeaks.reset();
        mDelClasses.reset();        

        TMonitorLock aLockMemory(mRawMonitorMemory);
        mNewAllocation = 0;
        mNewObjects    = 0;

		if (!aInitVm) {
			gTransaction++;
		}
    }
    // ----------------------------------------------------
    // TMonitor::resetClasses
    //! \brief Reset interal structures
    //! \param aJvmti       The Java tool interface
    //! \param aClasses The interal hash table for reset
    //! \param aAllowStart \c TRUE if restart is required
    // ----------------------------------------------------
    void reset(
            jvmtiEnv            *aJvmti,
            THashClasses        *aClasses, 
            bool                 aAllowStart) {

        THashClasses::iterator aPtrHashClasses;
        TMonitorClass   *aClass;

        TMonitorLock aLockAccess(mRawMonitorAccess);
        for (aPtrHashClasses  = aClasses->begin();
             aPtrHashClasses != aClasses->end();
             aPtrHashClasses  = aClasses->next()) {

            aClass = aPtrHashClasses->aValue;
            aClass->reset();

            if (aAllowStart) {
                resetClass(aClass);
            }
        }
    }
    // ----------------------------------------------------
    // TMonitor::reset
    //! \brief Reset interal structures
    //! \param aClass The interal structure for reset
    // ----------------------------------------------------
    void resetClass(
            TMonitorClass   *aClass) {

        bool  aVisible;
        bool  isProfiled = false;
        bool  isExcluded = false;

        isExcluded = !mProperties->doMonitorScope(aClass->getName());

        if (!isExcluded) {
            isExcluded = mProperties->dontMonitorPackage(aClass->getName());
        }

        if (!isExcluded) {
            isProfiled = mProperties->doMonitorPackage(aClass->getName());
            aVisible   = mProperties->doMonitorVisible(aClass->getName());
            aClass->setVisibility(aVisible);
        }
        aClass->exclude(isExcluded);
        aClass->enable(isProfiled, true);
    }
    // ----------------------------------------------------
    // TMonitor::reset
    //! \brief Reset interal structures
    //! \param aMethod The interal structure for reset
    // ----------------------------------------------------
    void resetMethod(
            TMonitorMethod *aMethod) {

        bool  isTimer    = false;
        bool  aActivate  = false;
        SAP_UC *aEntry;

        if (aMethod->getClass()->getExcluded()) {
            aMethod->setContextDebug(NULL);
            aMethod->setContextMonitor(NULL);
            aMethod->setTimer(false);
            aMethod->enable(false);
            return;
        }
        aActivate = aMethod->getClass()->getMethodStatus();

        isTimer   = (mProperties->doExecutionTimer(TIMER_METHOD) ||
                     mProperties->doMonitorTimer(aMethod->getClass()->getName(), aMethod->getName()));
        aMethod->setTimer(isTimer);

        aEntry    = mProperties->getMonitorDebugEntry(aMethod->getClass()->getName(), aMethod->getName());
        aMethod->setContextDebug(aEntry);
        
		aActivate = aActivate || (aEntry != NULL);

        aEntry = mProperties->getMonitorMethodEntry(aMethod->getClass()->getName(), aMethod->getName());
        aMethod->setContextMonitor(aEntry);
        
		aActivate = aActivate || (aEntry != NULL);

        if (mProperties->doTrigger(aMethod->getClass()->getName(), aMethod->getName(), aMethod->getSignature()->str())) {
            if (mTraceEvent != NULL) {
                TXmlTag *aTag = mTraceEvent->addTag(cU("Trace"));
                aTag->addAttribute(cU("Type"),          cU("Message"));
                aTag->addAttribute(cU("Info"),          cU("Trigger activated"));
                aTag->addAttribute(cU("ClassName"),     aMethod->getClass()->getName());
                aTag->addAttribute(cU("MethodName"),    aMethod->getName());
                aTag->addAttribute(cU("Signature"),     aMethod->getSignature());
            }
            mTriggerMethod = aMethod;
            aActivate      = true;
        }
        
        aMethod->enable(aActivate);
        if (aActivate) {
            aMethod->getClass()->enable(true, false);
        }
    }
    // ----------------------------------------------------
    // TMonitor::resetMethods
    //! \brief Reset interal structures
    //! \param aJvmti       The Java tool interface
    //! \param aMethods     The interal hash table for reset
    //! \param aAllowStart \c TRUE if restart is required
    // ----------------------------------------------------
    void reset(
            jvmtiEnv            *aJvmti,
            THashMethods        *aMethods, 
            bool                 aAllowStart) {

        TMonitorMethod *aMethod;
        THashMethods::iterator aPtrHashMethods;

        TMonitorLock aLockMemory(mRawMonitorAccess);
        for (aPtrHashMethods  = aMethods->begin();
             aPtrHashMethods != aMethods->end();
             aPtrHashMethods  = aMethods->next()) {

            aMethod   = aPtrHashMethods->aValue;
            aMethod->reset();

            if (!aAllowStart) {
                continue;
            }
            resetMethod(aMethod);
        }
    } 
    // ----------------------------------------------------
    // TMonitor::getState
    //! \return \c TRUE if profiler is running
    // ----------------------------------------------------
    bool getState() {
        return (mProperties->getStatus() == MONITOR_ACTIVE );
    }
    // ----------------------------------------------------
    // TMonitor::setTraceContention
    //! \param aJvmti       The Java tool interface
    //! \param aEnable      Enable contention trace
    //! \param aOptions     The trace options
    // ----------------------------------------------------
    void setTraceContention(
            jvmtiEnv    *aJvmti,
            bool         aEnable, 
            TValues     *aOptions) {

        jvmtiError aResult;
        mTracer->setTraceContention(aEnable, aOptions);
        if (aEnable) {
            aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTER,   NULL);
            aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTERED, NULL);
            aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_MONITOR_WAIT,              NULL);
            aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_MONITOR_WAITED,            NULL);
            return;
        }
        
        if (mProperties->getStatus() == MONITOR_ACTIVE &&
            mProperties->doExecutionTimer(TIMER_HPC)) {
            return;
        }
        aResult = aJvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTER,  NULL);
        aResult = aJvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTERED,NULL);
        aResult = aJvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_MONITOR_WAIT,             NULL);
        aResult = aJvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_MONITOR_WAITED,           NULL);
    }
    // ----------------------------------------------------
    // TMonitor::start
    //! \brief Starts the profiler
    //! \param aJvmti       The Java tool interface
    //! \param aRootTag     The output tag list
    // ----------------------------------------------------
    void start(
            jvmtiEnv    *aJvmti,
            TXmlTag     *aRootTag,
		    bool         aInitVm = false) {        

        TXmlTag *aTag;
        jvmtiError aResult;
        
        if (mProperties->getStatus() != MONITOR_IDLE) {
            if (aRootTag != NULL) {
                aTag = aRootTag->addTag(cU("Trace")); 
                aTag->addAttribute(cU("Type"),  cU("Message"));
                aTag->addAttribute(cU("Info"),  cU("Monitor running"));
            }
            return;
        }

        mProperties->setStatus(MONITOR_ACTIVE);

        TMonitorThread::resetThreads();
        mTraceEvent   = aRootTag;
        resetMonitorFields(aJvmti, true, aInitVm);
        mTraceEvent   = NULL;
        mNrCallsTrace = 0;

        if (aRootTag != NULL) {
            aTag = aRootTag->addTag(cU("Trace")); 
            aTag->addAttribute(cU("Type"),  cU("Message"));
            aTag->addAttribute(cU("Info"),  cU("Monitor started"));
        }

        if (mProperties->doExecutionTimer(TIMER_HPC)) {
            aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTER,   NULL);
            aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTERED, NULL);
            aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_MONITOR_WAIT,              NULL);
            aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_MONITOR_WAITED,            NULL);
        }

        if (mProperties->getProfilerMode() == PROFILER_MODE_PROFILE) {
            aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_EXIT,       NULL);
            aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY,      NULL);
        }
        aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION_CATCH,       NULL);
        aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_BREAKPOINT,            NULL);
        aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_FIELD_MODIFICATION,    NULL);

        if (mProperties->doMonitorMemoryOn()) {
            aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_OBJECT_ALLOC,    NULL);
			aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_FIELD_MODIFICATION, NULL);
        }
    }
    // ----------------------------------------------------
    // TMonitor::stop
    //! Stops the profiler
    //! \param aJvmti       The Java tool interface
    //! \param aRootTag     The output tag list
    // ----------------------------------------------------
    void stop(
            jvmtiEnv    *aJvmti,
            TXmlTag     *aRootTag) {

        jvmtiError  aResult;
        TXmlTag    *aTag;

        if (mProperties->getStatus() == MONITOR_IDLE) {
            if (aRootTag != NULL) {
                aTag = aRootTag->addTag(cU("Trace")); 
                aTag->addAttribute(cU("Type"),  cU("Message"));
                aTag->addAttribute(cU("Info"),  cU("Monitor idle"));
            }
            return;
        }

        if (aRootTag != NULL) {
            aTag = aRootTag->addTag(cU("Trace")); 
            aTag->addAttribute(cU("Type"),  cU("Message"));
            aTag->addAttribute(cU("Info"),  cU("Monitor stopped"));        
        }
        mProperties->setStatus(MONITOR_IDLE);

        aResult = aJvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_VM_OBJECT_ALLOC,          NULL);
        aResult = aJvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_BREAKPOINT,               NULL);
        aResult = aJvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_METHOD_ENTRY,             NULL);
        aResult = aJvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_METHOD_EXIT,              NULL);
        aResult = aJvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_EXCEPTION_CATCH,          NULL);
        aResult = aJvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_FIELD_MODIFICATION,       NULL);
		aResult = aJvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_FIELD_ACCESS,             NULL);

        if (!mTracer->doTraceContention(-1)) {
            aResult = aJvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTER,  NULL);
            aResult = aJvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_MONITOR_CONTENDED_ENTERED,NULL);
            aResult = aJvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_MONITOR_WAIT,             NULL);
            aResult = aJvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_MONITOR_WAITED,           NULL);
        }
    }
    // ----------------------------------------------------
    // TMonitor::dumpHeap
    //! \brief Generates a heap dump
    //! \param aJvmti       The Java tool interface
    //! \param aRootTag     The output tag list
    //! \param aContext     Hash code of a selected class
    //! \param aOptions     Dump options
    //!         -m<size>    Minimum heap size
    //!         -n<size>    Minimum heap count
    //!         -g          Work with existing result set
    //!         -c          Clear result set
    //!         -s<col>     Sort column
    //!         -f<name>    Filter
    //!         -C<#hash>   Context class hash code
    //!
    //! It is possible to give the hash of a class as context to this 
    //! method. In the case this is not NULL the dump will only trace
    //! objects, which where allocated in the context of this class.
    // ----------------------------------------------------
    void dumpHeap(
            jvmtiEnv    *aJvmti,
            TXmlTag     *aRootTag, 
            jlong        aContext, 
            TValues     *aOptions) {

        TValues::iterator aPtrOptions;
        THashClasses::iterator aPtrClass;
        TMonitorClass    *aClass;
        TString           aColumnSort;
        TString           aColumnFilter;
        TString           aDumpFile;
        jint              aCnt      = 0;
        jlong             aHeapSize = 1;
        jlong             aHeapCnt  = 0;
        jint              aColumnHeapCnt;
        jint              aColumnHeapSize;
        bool              aNewHeapDump = true;
        bool              aClear       = false;
        SAP_UC            aBuffer[32];

        aColumnFilter   = cU(".");
        aColumnSort     = cU("HeapSize");
        aColumnHeapCnt  = TMonitorClass::getSortCol(cU("HeapCount"));
        aColumnHeapSize = TMonitorClass::getSortCol(cU("HeapSize"));

        if (aOptions != NULL) {
            for (aPtrOptions = aOptions->begin();
                 aPtrOptions != aOptions->end();
                 aPtrOptions = aOptions->next()) {

                if (!STRNCMP(*aPtrOptions, cU("-m"), 2)) {
                    aHeapSize = TString::toInteger(*aPtrOptions + 2);
                }
                else if (!STRNCMP(*aPtrOptions, cU("-n"), 2)) {
                    aHeapCnt  = TString::toInteger(*aPtrOptions + 2);
                }
                else if (!STRNCMP(*aPtrOptions, cU("-g"), 2)) {
                    aNewHeapDump = false;
                }
                else if (!STRNCMP(*aPtrOptions, cU("-c"), 2)) {
                    aClear       = true;
                }
                else if (!STRNCMP(*aPtrOptions, cU("-s"), 2)) {
                    aColumnSort = (*aPtrOptions) + 2;
                }
                else if (!STRNCMP(*aPtrOptions, cU("-f"), 2)) {
                    aColumnFilter = (*aPtrOptions) + 2;
                }
                else if (!STRNCMP(*aPtrOptions, cU("-C"), 2)) {
                    aContext = TString::toInteger(*aPtrOptions + 2);
                }
            }
        }

        if (aNewHeapDump) {
            clearHeapDump(aJvmti);
            aJvmti->IterateOverHeap(JVMTI_HEAP_OBJECT_TAGGED, TMonitorHeapCallback, (void*)aContext);
        }

        TMonitorLock aLockAccess(mRawMonitorAccess);
        for (aPtrClass  = mClasses.begin();
             aPtrClass != mClasses.end();
             aPtrClass  = mClasses.next()) {

            aClass = aPtrClass->aValue;
            if (aClass->compare(aColumnHeapCnt,  aHeapCnt)   >= 0 &&
                aClass->compare(aColumnHeapSize, aHeapSize ) >= 0 &&
                aClass->filterName(aColumnFilter.str())) { 

                if (aCnt++ < mProperties->getLimit(LIMIT_IO)) {
                    aClass->dumpHeap(aRootTag);
					// mMemoryLeaks.insert(aPtrClass);
                }
            }
        }
        aLockAccess.exit();

        // exception
        if (aCnt > mProperties->getLimit(LIMIT_IO)) {
            TString aString;
            aString.concat(cU("Exceed Maximum Number of Entries "));
            aString.concat(TString::parseInt(aCnt, aBuffer));
            aRootTag->addAttribute(cU("Result"), aString.str());
        }
        aRootTag->qsort(aColumnSort.str());
    }
private:
    // ----------------------------------------------------
    // TMonitor::clearHeapDump
    //! \brief Reset internal structures
    //!
    //! \param aJvmti       The Java tool interface
    // ----------------------------------------------------
    void clearHeapDump(
            jvmtiEnv        *aJvmti) {

        THashClasses::iterator aPtrClass = NULL;
        // Delete reference lists
        TMonitorLock aLockAccess(mRawMonitorAccess);
        for (aPtrClass  = mClasses.begin();
            aPtrClass != mClasses.end();
            aPtrClass  = mClasses.next()) {
            aPtrClass->aValue->resetHeapCount();
        }
    }
public:
    // ----------------------------------------------------
    // TMonitor::resetThreads
    //! \brief Reset internal structures
    //!
    // ----------------------------------------------------
    void resetThreads(
            jvmtiEnv        *aJvmti) {
        
        jvmtiError       aResult;
        jint             aCount;
        jthread         *jThreads;
        TMonitorThread  *aThread;
        
        aResult = aJvmti->GetAllThreads(&aCount,  &jThreads);
        for (int i = 0; i < aCount; i++) {
            aJvmti->GetThreadLocalStorage(jThreads[i], (void **)&aThread);
            if (aThread != NULL) {
                aThread->reset();
            }
        }
        aJvmti->Deallocate((unsigned char*)jThreads);
    }
    // ----------------------------------------------------
    // TMonitor::dumpStatistic
    //! \brief List internal state
    //! \param aJvmti       The Java tool interface
    //! \param aRootTag     The output tag list
    // ----------------------------------------------------
    void dumpStatistic(
            jvmtiEnv            *aJvmti,
            TXmlTag             *aRootTag) {

        SAP_UC      aBuffer[128];
        TXmlTag    *aTag;
        jlong       aSize;

        aTag = aRootTag->addTag(cU("Monitor"));
        aTag->addAttribute(cU("Name"), cU("NewFktCalls"));
        aTag->addAttribute(cU("Value"), TString::parseInt(mNrCallsFkt, aBuffer), PROPERTY_TYPE_INT);

        aTag = aRootTag->addTag(cU("Monitor"));
        aTag->addAttribute(cU("Name"), cU("NewObjects"));
        aTag->addAttribute(cU("Value"), TString::parseInt(mNewObjects, aBuffer), PROPERTY_TYPE_INT);
        
        aTag = aRootTag->addTag(cU("Monitor"));
        aTag->addAttribute(cU("Name"), cU("NewAllocation"));
        aTag->addAttribute(cU("Value"), TString::parseInt(mNewAllocation, aBuffer), PROPERTY_TYPE_INT);

        //jthread  *jThreads;
        //mJvmti->GetAllThreads(&mNrThreads, &jThreads);
        //mJvmti->Deallocate((unsigned char *)jThreads);
        jint aNrThreads = (jint)TMonitorThread::getNrThreads();

        aTag = aRootTag->addTag(cU("Monitor"));
        aTag->addAttribute(cU("Name"), cU("NrThreads"));
        aTag->addAttribute(cU("Value"), TString::parseInt(aNrThreads, aBuffer), PROPERTY_TYPE_INT);

        aSize = mClasses.getSize();
        ASSERT(aSize >= 0);

        aTag = aRootTag->addTag(cU("Monitor"));
        aTag->addAttribute(cU("Name"), cU("NrClasses"));
        aTag->addAttribute(cU("Value"), TString::parseInt(mClasses.getSize(), aBuffer), PROPERTY_TYPE_INT);

        aTag = aRootTag->addTag(cU("Monitor"));
        aTag->addAttribute(cU("Name"), cU("CpuTime"));
        aTag->addAttribute(cU("Value"), TString::parseInt(getCpuTimeMicro(aJvmti), aBuffer), PROPERTY_TYPE_INT);

        aTag = aRootTag->addTag(cU("Monitor"));
        aTag->addAttribute(cU("Name"), cU("Monitor"));

        switch (mProperties->getStatus()) {
            case MONITOR_ACTIVE: aTag->addAttribute(cU("Value"), cU("running")); break;
            case MONITOR_PAUSE:  aTag->addAttribute(cU("Value"), cU("pause"));   break;
            default:             aTag->addAttribute(cU("Value"), cU("idle"));    break;
        }
    }
    // ----------------------------------------------------
    // TMonitor: dumpClasses
    //! \brief Dump interal structrue
    //!
    //! Dumps all monitored classes. With a given aRef the 
    //! method constructs creates a unique ID, which can be used for the detail view.  
    //! \param aJvmti       The Java tool interface
    //! \param aRootTag     The output tag list
    //! \param aOptions     The trace options
    //! \param aRef         Optional reference.
    // ----------------------------------------------------
    void dumpClasses(
            jvmtiEnv        *aJvmti,
            TXmlTag         *aRootTag, 
            TValues         *aOptions, 
            const SAP_UC    *aRef = NULL) {

        if (mProperties->getProfilerMode() == PROFILER_MODE_JARM ||
            mProperties->getProfilerMode() == PROFILER_MODE_ATS) {
            dumpMemoryUsage(aJvmti, &mContextClasses, aRootTag, cU("Class"), aOptions, aRef);
        }
        else {
            dumpMemoryUsage(aJvmti, &mClasses, aRootTag, cU("Class"), aOptions, aRef);
        }
    }
    // ----------------------------------------------------
    // TMonitor: dumpDeletedClasses
    //! \brief Dump interal structrue
    //! \see TMonitor::dumpClasses
    // ----------------------------------------------------
    void dumpDeletedClasses(
            TXmlTag         *aRootTag, 
            TValues         *aOptions, 
            const SAP_UC    *aRef = NULL) {

        TValues::iterator aPtrOptions;
        TMonitorClass    *aClass;
        TString           aColumnSort;
        TString           aColumnFilter;
        int               aColumnCurrSize = 0;
        jint              aCnt     = 0;
        jlong             aMin     = 1;
        bool              aDumpHistory = false;
        bool              aDelete      = false;
        SAP_UC              aBuffer[128];

        aColumnFilter   = cU(".");
        aColumnSort     = cU("CurrSize");
        aColumnCurrSize = TMonitorClass::getSortCol(aColumnSort.str());
        aMin            = mProperties->getMinMemorySize();

        if (aOptions != NULL) {
            for (aPtrOptions  = aOptions->begin();
                 aPtrOptions != aOptions->end();
                 aPtrOptions  = aOptions->next()) {

                if (!STRNCMP(*aPtrOptions, cU("-h"), 2)) {
                    aDumpHistory = true;
                }
                else if (!STRNCMP(*aPtrOptions, cU("-m"), 2)) {
                    aMin = TString::toInteger(*aPtrOptions + 2);
                }
                else if (!STRNCMP(*aPtrOptions, cU("-s"), 2)) {
                    aColumnSort = (*aPtrOptions) + 2;
                }
                else if (!STRNCMP(*aPtrOptions, cU("-f"), 2)) {
                    aColumnFilter = (*aPtrOptions) + 2;
                }
                else if (!STRNCMP(*aPtrOptions, cU("-d"), 2)) {
                    aDelete = true;
                }                
            }
        }
        TListClasses::iterator aPtr;
        for (aPtr  = mDelClasses.begin();
             aPtr != mDelClasses.end();
             aPtr  = mDelClasses.next()) {
            aClass = aPtr->mElement;
            if (aClass->compare(aColumnCurrSize, aMin) >= 0 &&
                aClass->filterName(aColumnFilter.str())) { 
                if (aCnt++ < mProperties->getLimit(LIMIT_IO)) { 
                    TXmlTag *aTagClass = aRootTag->addTag(cU("Class"),   XMLTAG_TYPE_NODE);
                    aClass->dump(aTagClass, aRef, aDumpHistory);
                }
                if (aDelete && aClass->deleteClass()) {
                    mDelClasses.remove(aPtr);
                    delete aClass;
                }
            }
        }
        // exception
        if (aCnt > mProperties->getLimit(LIMIT_IO)) {
            TString aString;
            aString.concat(cU("Exceed Maximum Number of Entries "));
            aString.concat(TString::parseInt(aCnt, aBuffer));
            aRootTag->addAttribute(cU("Exception"), aString.str());
        }
        aRootTag->qsort(aColumnSort.str());
    }
    // ----------------------------------------------------
    // TMonitor: dumpClassDetails
    //! \brief Dump interal structrue
    //! \see TMonitor::dumpClasses
    // ----------------------------------------------------
    void dumpClassDetails(
            jvmtiEnv        *aJvmti, 
            TXmlTag         *aRootTag, 
            TValues         *aOptions) {

        TValues::iterator aPtrOptions;
        TMonitorClass  *aClass  = NULL;
        jlong           aObject = 0;
        int             aDetail = 0;

        if (aOptions != NULL) {
            aPtrOptions = aOptions->begin();
            while (aPtrOptions != aOptions->end()) {
                if (!STRNCMP(*aPtrOptions, cU("-r"), 2)) {
                    aObject = TString::toInteger((*aPtrOptions) + 2);
                }
                else if (!STRNCMP(*aPtrOptions, cU("-dh"), 3)) {
                    aDetail = 1;
                }
                else if (!STRNCMP(*aPtrOptions, cU("-df"), 3)) {
                    aDetail = 2;
                }
                else if (!STRNCMP(*aPtrOptions, cU("-dm"), 3)) {
                    aDetail = 3;
                }
                aPtrOptions = aOptions->next();
            }
        }

        if (aObject == 0 || (aClass = findClass(aJvmti, aObject)) == NULL) {
            return;
        }

        switch (aDetail) {
            case 1: aClass->dumpHistory(aRootTag);      break;
            case 2: aClass->dumpFields(aRootTag);       break;
            case 3: aClass->dumpMethods(aRootTag);      break;
        }
    }
    // ----------------------------------------------------
    // TMonitor::findMethod
    //! \brief Find a method from hash
    //! \param aJvmti   The Java tool interface
    //! \param aID      The hash value
    // ----------------------------------------------------
    inline TMonitorMethod *findMethod(
            jvmtiEnv        *aJvmti,
            jmethodID        aID) {

        THashMethods::iterator aPtr;
        TMonitorMethod *aMethod = NULL;

        TMonitorLock aLockAccess(mRawMonitorAccess, true, false);
        aPtr = mMethods.find(aID);
        if (aPtr != mMethods.end()) {
            aMethod = aPtr->aValue;
        }
        return aMethod;
    }
    // ----------------------------------------------------
    // TMonitor: dumpMemoryLeaks
    //! \brief Dump interal structrue
    //! \see TMonitor::dumpClasses
    // ----------------------------------------------------
    void dumpMemoryLeaks(
            jvmtiEnv        *aJvmti,
            TXmlTag         *aRootTag, 
            TValues         *aOptions, 
            const SAP_UC    *aRef = NULL) {
        dumpMemoryUsage(aJvmti, &mMemoryLeaks, aRootTag, cU("Leak"), aOptions, aRef);
    }
private:
    // ----------------------------------------------------
    // TMonitor: dumpMemoryUsage
    //! \brief Dump interal structrue
    //! \see TMonitor::dumpClasses
    // ----------------------------------------------------
    void dumpMemoryUsage(
                jvmtiEnv        *aJvmti,
                THashClasses    *aHashTable,
                TXmlTag         *aRootTag,
		        const SAP_UC    *aType,
                TValues         *aOptions, 
                const SAP_UC    *aRef = NULL) {

		TXmlTag          *aTagClass     = NULL;
		TValues::iterator aPtrOptions;
        THashClasses::iterator aPtr;
        TValues           aSortDetail(2);
		TProperty        *aProperty;
        TMonitorClass    *aClass        = NULL;
		TMonitorClass    *aClassOption  = NULL;
		
        jlong             jObject       = 0;
        TString           aColumnSort;
        TString           aColumnFilter;
        SAP_UC            aBuffer[128];
        jint              aCnt          = 0;
        jlong             aMin          = 1;
        int               aColumnCurrSize = 0;
        bool              aDumpHistory  = false;
        bool              aStatus       = false;
        bool              aDumpHash     = false;
        bool              aDumpMethods  = false;
		bool              aDumpHeap     = false;
		bool              aSetType      = true;

        aColumnFilter   = cU(".");
        aColumnSort     = cU("CurrSize");
        aColumnCurrSize = TMonitorClass::getSortCol(aColumnSort.str());
        aMin            = mProperties->getMinMemorySize();

        if (aOptions != NULL) {
            aPtrOptions = aOptions->begin();
            while (aPtrOptions != aOptions->end()) {
                if (!STRNCMP(*aPtrOptions, cU("-h"), 2)) {
                    aDumpHistory = true;
                }
				else if (!STRNCMP(*aPtrOptions, cU("-H"), 2)) {
					aDumpHeap = true;
				}
				else if (!STRNCMP(*aPtrOptions, cU("-x"), 2)) {
                    aDumpHash = true;
                }
                else if (!STRNCMP(*aPtrOptions, cU("-m"), 2)) {
                    aMin = TString::toInteger(*aPtrOptions + 2);
                }
                else if (!STRNCMP(*aPtrOptions, cU("-M"), 2)) {
                    aDumpMethods = true;
                }
                else if (!STRNCMP(*aPtrOptions, cU("-s"), 2)) {
                    aColumnSort = (*aPtrOptions) + 2;
                }
                else if (!STRNCMP(*aPtrOptions, cU("-f"), 2)) {
                    aColumnFilter = (*aPtrOptions) + 2;
                }
                else if (!STRNCMP(*aPtrOptions, cU("-F"), 2)) {
                    aColumnFilter = (*aPtrOptions) + 2;
                    aStatus       = true;
                }
                else if (!STRNCMP(*aPtrOptions, cU("-C"), 2)) {
                    jObject      = TString::toInteger((*aPtrOptions) + 2);
					aClassOption = findClass(aJvmti, jObject);
                }                
                aPtrOptions = aOptions->next();
            }
        }
		
		aTagClass = NULL;
		
		if (aClassOption == NULL) {
			aSetType = false;
		}

		TMonitorLock aLockAccess(mRawMonitorAccess);
        for (aPtr  = aHashTable->begin();
             aPtr != aHashTable->end();
             aPtr  = aHashTable->next()) {


			aTagClass = NULL;
			aClass    = aPtr->aValue;

			if (aClassOption != NULL) {
				if (aClassOption != aClass) {
					continue;
				}
			}

            if ((aClass->getStatus() || aStatus) &&
                 aClass->compare(aColumnCurrSize, aMin) >= 0 &&
                 aClass->filterName(aColumnFilter.str())) { 
               
				if (aCnt++ < mProperties->getLimit(LIMIT_IO)) { 
                    if (aDumpHistory) {
						if (aTagClass == NULL) {
							aTagClass = aRootTag->addTag(cU("Class"), XMLTAG_TYPE_NODE);
							aClass->dump(aTagClass, aRef, aDumpHash);
						}

						TXmlTag *aTagHisty = aTagClass->addTag(cU("List"), XMLTAG_TYPE_NODE);
						if (aSetType) {
							aTagHisty->addAttribute(cU("Type"), cU("History"));
						}
						else {
							aTagHisty->addAttribute(cU("Detail"), cU("History"));
						}
						aTagHisty->addAttribute(cU("ID"),     TString::parseHex(aClass->getID(), aBuffer));
						aClass->dumpHistory(aTagHisty);
					}
                    if (aDumpMethods) {
						if (aTagClass == NULL) {
							aTagClass = aRootTag->addTag(cU("Class"), XMLTAG_TYPE_NODE);
							aClass->dump(aTagClass, aRef, aDumpHash);
						}

                        TXmlTag *aRootMeth = aTagClass->addTag(cU("List"), XMLTAG_TYPE_NODE);
						aRootMeth->addAttribute(cU("Detail"), cU("Methods"));
						aRootMeth->addAttribute(cU("ID"),     TString::parseHex(aClass->getID(), aBuffer));
						aClass->dumpMethods(aRootMeth);
                    }

					if (aDumpHeap) {
						if (aTagClass == 0) {
							aTagClass = aRootTag->addTag(cU("Class"), XMLTAG_TYPE_NODE);
							aClass->dump(aTagClass, aRef, aDumpHash);
						}
						TXmlTag *aRootMeth = aTagClass->addTag(cU("List"), XMLTAG_TYPE_NODE);
						aRootMeth->addAttribute(cU("Detail"), cU("Heap"));
						aRootMeth->addAttribute(cU("ID"),     TString::parseHex(aClass->getID(), aBuffer));
						dumpHeap(aJvmti, aRootMeth, aClass->getID(), aOptions);
						// aClass->dumpHeap(aRootMeth);
					}

					if (aTagClass == NULL) {
						aTagClass = aRootTag->addTag(cU("Class"));
						aClass->dump(aTagClass, aRef, aDumpHash);
					}
                }                
            }
        }    
        aLockAccess.exit();

        // exception
        if (aCnt > mProperties->getLimit(LIMIT_IO)) {
            TString aString;
            aString.concat(cU("Exceed Maximum Number of Entries "));
            aString.concat(TString::parseInt(aCnt, aBuffer));
            aRootTag->addAttribute(cU("Result"), aString.str());
        }

		if (aCnt > 0 && aClassOption == NULL) {
			aRootTag->addAttribute(cU("Type"), aType);
		}
        // sort the columns as specified
        aRootTag->qsort(aColumnSort.str());
    }
public:
    // ----------------------------------------------------
    // TMonitor::dumpMethods
    //! \brief Dumps all methods 
    //! \param aJvmti       The Java tool interface
    //! \param aRootTag     The output tag list
    //! \param aOptions     The trace options
    //! \param aClassID     The class id
    //! \param aMethodID    The method id
    //! \param aHashMethods The method hash table
    // ----------------------------------------------------
    void dumpMethods(
            jvmtiEnv        *aJvmti,
            TXmlTag         *aRootTag, 
            TValues         *aOptions, 
            jlong            aClassID, 
            jmethodID        aMethodID, 
            THashMethods    *aHashMethods,
		    const SAP_UC    *aType) {

        TValues::iterator       aPtrOptions;
        THashMethods::iterator  aPtrHashMethods;
        TString::iterator       aPos;

        TString         aFullName;
        TMonitorMethod *aMethod;
        TString         aColumnFilter;
        jint            aCnt            = 0;
        int             aColumnCpu;
        int             aColumnNrCall;
        int             aColumnElapsed;
        int             aColumnContent;
        jlong           aMinContent     = 0;
        bool            aOutputCont     = false;
        bool            aOutputSign     = false;
        bool            aOutputParam    = false;
        bool            aOutputHash     = false;
        bool            aFound          = false;
        bool            aOutputAll      = false;
        jlong           aMinCpu         = 0;
        jlong           aMinCall        = 0;
        jlong           aMinElapsed     = 0;
        const SAP_UC   *aColumnSort     = cU("CpuTime");
        SAP_UC          aBuffer[128];

        // restrict output to sort creteria
        aColumnCpu     = TMonitorMethod::getSortCol(cU("CpuTime"));
        aColumnNrCall  = TMonitorMethod::getSortCol(cU("NrCalls"));
        aColumnElapsed = TMonitorMethod::getSortCol(cU("Elapsed"));
        aColumnContent = TMonitorMethod::getSortCol(cU("Contention"));

        aColumnFilter  = cU(".");
        // evaluate options
        if (aOptions != NULL) {
            for (aPtrOptions  = aOptions->begin();
                 aPtrOptions != aOptions->end();
                 aPtrOptions  = aOptions->next()) {

                if (!STRNCMP(*aPtrOptions, cU("-m"), 2)) {
                    aMinCpu     = TString::toInteger(*aPtrOptions + 2);
                }
                else if (!STRNCMP(*aPtrOptions, cU("-n"), 2)) {
                    aMinCall    = TString::toInteger(*aPtrOptions + 2);
                }
                else if (!STRNCMP(*aPtrOptions, cU("-e"), 2)) {
                    aMinElapsed = TString::toInteger(*aPtrOptions + 2);
                }
                else if (!STRNCMP(*aPtrOptions, cU("-c"), 2)) {
                    aMinContent = TString::toInteger(*aPtrOptions + 2);
                    aOutputCont = true;
                }
                else if (!STRNCMP(*aPtrOptions, cU("-s"), 2)) {
                    aColumnSort = (*aPtrOptions) + 2;
                }
                else if (!STRNCMP(*aPtrOptions, cU("-f"), 2)) {
                    aColumnFilter = (*aPtrOptions) + 2;
                }                
                else if (!STRNCMP(*aPtrOptions, cU("-F"), 2)) {
                    aOutputAll    = true;
                    aColumnFilter = (*aPtrOptions) + 2;
                }                
                else if (!STRNCMP(*aPtrOptions, cU("-a"), 2)) {
                    aOutputSign = true;
                }                
                else if (!STRNCMP(*aPtrOptions, cU("-x"), 2)) {
                    aOutputHash = true;
                }                
                else if (!STRNCMP(*aPtrOptions, cU("-p"), 2)) {
                    aOutputParam = true;
                }                
                else if (!STRNCMP(*aPtrOptions, cU("-C"), 2)) {
                    aRootTag->addAttribute(cU("Detail"), cU("Class"));
                    aClassID     = TString::toInteger(*aPtrOptions + 2);
                }                
                else if (!STRNCMP(*aPtrOptions, cU("-M"), 2)) {
                    aMethodID    = (jmethodID)TString::toInteger(*aPtrOptions + 2);
                }                
            }
        }
    
        TMonitorLock aLockAccess(mRawMonitorAccess);
        if (aHashMethods == NULL) {
            aHashMethods = &mMethods;
        }

        if (aMethodID != NULL) {
            aPtrHashMethods = aHashMethods->find(aMethodID);
            if (aPtrHashMethods == aHashMethods->end()) {
                return;
            }
            aMethod = aPtrHashMethods->aValue;
            if (aOutputParam) {
                aRootTag->addAttribute(cU("Detail"), cU("Parameter"));
                aMethod->dumpLocalVariables(aRootTag);
            }
            else {
                aMethod->dump(aRootTag, aOutputSign, aOutputCont, aOutputHash);  
            }
            return;
        }

        
        aPos    = aColumnFilter.findLastOf(cU('.'));
        aFound  = (aPos != aColumnFilter.end());

        if (aFound) {
            aFound = ((aPos = aColumnFilter.find(cU(".super"), aPos)) != aColumnFilter.end());
            if (aFound) {
                TString aClassPart;
                aClassPart = aColumnFilter.str();
                aClassPart.cut(0, aPos);

                TMonitorClass *aClass = findClass(aJvmti, aClassPart.str());
                if (aClass == NULL) {
                    return;
                }
                aClass = aClass->getSuper();
                if (aClass == NULL) {
                    return;
                }
                aClass->dumpMethods(aRootTag);
                return;
            }
        }

        for (aPtrHashMethods  = aHashMethods->begin();
             aPtrHashMethods != aHashMethods->end();
             aPtrHashMethods  = aHashMethods->next()) {

            aMethod    = aPtrHashMethods->aValue;
            if (!aOutputAll && !aMethod->getStatus()) {
                continue;
            }

            aFullName  = aMethod->getFullName();
            if (aFullName.findWithWildcard(aColumnFilter.str(), cU('.')) == -1) {
                continue;
            }

            if (aMethod->compare(aColumnCpu,     aMinCpu)     >= 0    &&
                aMethod->compare(aColumnNrCall,  aMinCall)    >= 0    &&
                aMethod->compare(aColumnElapsed, aMinElapsed) >= 0) {
                
                if (aClassID != 0) {
                    if (aMethod->getClass()->getID() != aClassID) {
                        continue;
                    }
                }
                if (aOutputCont && aMethod->compare(aColumnContent, aMinContent) < 0) {
                   continue;
                }
                if (aCnt++ > mProperties->getLimit(LIMIT_IO)) {
                    continue;
                }
                aMethod->dump(aRootTag, aOutputSign, aOutputCont, aOutputHash);
                if (aOutputParam) {
                    aRootTag->addAttribute(cU("Detail"), cU("Parameter"));
                    aMethod->dumpLocalVariables(aRootTag);
                }
            }
        }
        aLockAccess.exit();

        // exception
        if (aCnt > mProperties->getLimit(LIMIT_IO)) {
            TString aString;
            aString.concat(cU("Exceed Maximum Number of Entries "));
            aString.concat(TString::parseInt(aCnt, aBuffer));
            aRootTag->addAttribute(cU("Result"), aString.str());
        }
		
		if (aCnt > 0) {
			aRootTag->addAttribute(cU("Type"), cU("Method"));
		}
        aRootTag->qsort(aColumnSort);
    }
public:
    // ----------------------------------------------------
    // TMonitor::setThreadStatus
    //! \brief Timer and contention
    //! \param aJvmti   The Java tool interface
    //! \param aJni     The Java native interface
    //! \param jThread  The current thread
    //! \param aObject  The calling object
    //! \param aEvent   The event causing a thread state change
    // ----------------------------------------------------
    void setThreadStatus(            
            jvmtiEnv   *aJvmti,
            JNIEnv     *aJni,
            jthread     jThread,
            jobject     aObject,
            jvmtiEvent  aEvent) {

        TMonitorThread *aThreadObj = NULL;
        jlong           aDiff      = 0;
        jvmtiError      aResult;

        aResult = aJvmti->GetThreadLocalStorage(jThread, (void**)&aThreadObj);
        if (aResult   != JVMTI_ERROR_NONE ||
            aThreadObj == NULL ||
           !aThreadObj->doCheck(false)) {
            return;
        }

        aDiff = aThreadObj->changeState(aEvent);
        if (aEvent == JVMTI_EVENT_MONITOR_CONTENDED_ENTERED) {
            if (aDiff > 0 && mTracer->doTraceContention(aDiff)) {
                int             aLevel = 0;
                TMonitorMethod *aTopMethod;
                TXmlTag         aRootTag(cU("Traces"), XMLTAG_TYPE_NODE);
                SAP_UC          aBuffer[32];

                aRootTag.addAttribute(cU("Type"), cU("Contention"));                
                aTopMethod = dumpSingleStack(aJvmti, aJni, &aRootTag, &aLevel, mTracer->getTraceEvent(EVENT_CONTENTION), TString::parseInt(aDiff, aBuffer), true, 0, true, jThread, NULL);
                
                mRawMonitorOutput->enter();
                    mTracer->printTrace(&aRootTag, aLevel, true);
                mRawMonitorOutput->exit();

                if (aTopMethod != NULL) {
                    aTopMethod->setContention(aDiff);
                }
            }
        }     
    }
    // ----------------------------------------------------
    // TMonitor::dumpGC
    //! \brief Evaluates a timestamp for the GC
    // ----------------------------------------------------
    void setGCTime() {
        mGCTime = TSystem::getTimestampHp();
    }
    // ----------------------------------------------------
    // TMonitor::dumpGC
    //! \brief Dump garbage collection events
    //! \param aJvmti The Java tool interface
    //! \param aJni   The Java native interface
    // ----------------------------------------------------
    void dumpGC(
            jvmtiEnv *aJvmti,
            JNIEnv   *aJni,
            bool      aStart = false) {

        jclass           jClass;
        jmethodID        jMethod;
        jobject          jObject;
        jlong            jCommit = 0;
        jlong            jUsed   = 0;
        jlong            jInit   = 0;
        jvmtiError       aResult;
        TMonitorThread  *aThread;

        mGCNr ++;
        if (!mTracer->doTraceGC() || aJvmti == NULL) {
            return;
        }
        aResult = aJvmti->GetThreadLocalStorage(NULL, (void**)&aThread);
        if (aThread == NULL || aResult != JVMTI_ERROR_NONE) {
            aThread = new TMonitorThread(aJvmti, aJni, NULL);
            aResult = aJvmti->SetThreadLocalStorage(NULL, (void *)aThread);
        }
        aThread->setProcessJni(true);

        for (int i = 0; i == 0; i ++) {            
            if ((mMxFact = aJni->FindClass(cR("java/lang/management/ManagementFactory"))) == NULL) {
                break;
            }
            if ((jMethod = aJni->GetStaticMethodID(mMxFact, cR("getMemoryMXBean"), cR("()Ljava/lang/management/MemoryMXBean;"))) == NULL) {
                break;
            }            
            if ((mMxBean = aJni->CallStaticObjectMethod(mMxFact, jMethod)) == NULL) {
                break;
            }
            if ((jClass  = aJni->GetObjectClass(mMxBean)) == NULL) {
                break;
            }           
            if ((mUsage  = aJni->GetMethodID(jClass, cR("getHeapMemoryUsage"), cR("()Ljava/lang/management/MemoryUsage;"))) == NULL) {
                break;
            }
                        
            if ((jObject = aJni->CallObjectMethod(mMxBean, mUsage)) == NULL) {
                break;
            }
            jClass  = aJni->GetObjectClass(jObject);

            if ((jMethod = aJni->GetMethodID(jClass, cR("getCommitted"), cR("()J"))) == NULL) {
                break;
            }
            jCommit = aJni->CallLongMethod(jObject, jMethod);

            if ((jMethod = aJni->GetMethodID(jClass, cR("getUsed"),      cR("()J"))) == NULL) {
                break;
            }
            jUsed   = aJni->CallLongMethod(jObject, jMethod);

            if ((jMethod = aJni->GetMethodID(jClass, cR("getInit"),      cR("()J"))) == NULL) {
                break;
            }
            jInit   = aJni->CallLongMethod(jObject, jMethod);        
            break;
        }

        if (aStart) {
            mGCUsageStart = jUsed;
            aThread->setProcessJni(false);
            return;
        }

        TXmlTag    aRootTag(cU("Trace"));
        TXmlWriter aWriter(XMLWRITER_TYPE_LINE);
            
        if (aJni->ExceptionOccurred()) {
            // aJni->ExceptionDescribe();
            aJni->ExceptionClear();
        }

        SAP_UC aBuffer[128];
        aRootTag.addAttribute(cU("Type"), cU("GCV9"));
        aRootTag.addAttribute(cU("NrGC"),       TString::parseInt(mGCNr,     aBuffer));
        aRootTag.addAttribute(cU("Timestamp"),  TString::parseInt(TSystem::getTimestamp(),   aBuffer));
        aRootTag.addAttribute(cU("Committed"),  TString::parseInt(jCommit,   aBuffer));
        aRootTag.addAttribute(cU("Init"),       TString::parseInt(jInit,     aBuffer));
        aRootTag.addAttribute(cU("Used"),       TString::parseInt(jUsed,     aBuffer));
        aRootTag.addAttribute(cU("Time"),       TString::parseInt(TSystem::getDiffHp(mGCTime), aBuffer));

        syncOutput(&aRootTag, XMLWRITER_TYPE_LINE);
        aThread->setProcessJni(false);
    }
    // ----------------------------------------------------
    // TMonitor::getCpuTimeMicro
    //! \param aJvmti       The Java tool interface
    //! \return CPU time in micro seconds
    // ----------------------------------------------------
    jlong getCpuTimeMicro(
                jvmtiEnv        *aJvmti) {

        jvmtiError       aResult;
        jint             aCount   = 0;
        jlong            aCpuTime = 0;
        jthread         *jThreads = NULL;
        TMonitorThread  *aThread  = NULL;
        
        aResult = aJvmti->GetAllThreads(&aCount,  &jThreads);
        for (int i = 0; i < aCount; i++) {
            aJvmti->GetThreadLocalStorage(jThreads[i], (void **)&aThread);
            if (aThread != NULL) {
                aCpuTime += aThread->getStoredCpuTime();
            }
        }
        /*SAPUNICODEOK_CHARTYPE*/
        aJvmti->Deallocate((unsigned char*)jThreads);
        return aCpuTime;
    }

    // ----------------------------------------------------
    // TMonitor::enterContext
    //! \brief Alternative to class/method view
    //!
    //! This method can be used in ATS and JARM mode 
    //! \param aJvmti       The Java tool interface
    //! \param aJni         The Java native interface
    //! \param aRequest     The name of the request, which is equivalent to class
    //! \param aContext     The name of the context, which is equivalent to method
    // ----------------------------------------------------
    void onContextEnter(
            jvmtiEnv        *aJvmti, 
            JNIEnv          *aJni, 
            const SAP_UC    *aRequest, 
            const SAP_UC    *aContext) {

        THashClasses::iterator aPtrClass;
        TMonitorMethod *aMethod    = NULL;
        TMonitorThread *aThreadObj = NULL;
        TMonitorClass  *aClass;    

        aJvmti->GetThreadLocalStorage(NULL, (void**)&aThreadObj);

        if (mProperties->getStatus() != MONITOR_ACTIVE) {
            return;
        }
        if (aRequest == NULL || STRLEN(aRequest) == 0) {
            return;
        }
        if (aContext == NULL) {
            aContext = cU("<init>");
        }
        if (STRLEN(aContext) == 0) {
            return;
        }

        TString     aStr(aRequest, aContext);
        jmethodID   jMethod = (jmethodID)aStr.getHash();
        jlong       jClass  = 0;

        TMonitorLock aLockAccess(mRawMonitorAccess);
        THashMethods::iterator aPtr = mContextMethods.find(jMethod);
        // Find the method for a given request/component pair
        if (aPtr != mContextMethods.end()) {
            aMethod = aPtr->aValue;
        }
        else {
            // create a class if not already registered
            aStr   = aRequest;
            jClass = aStr.getHash();

            aPtrClass = mContextClasses.find(jClass);
            if (aPtrClass != mContextClasses.end()) {
                aClass = aPtrClass->aValue;
            }
            else {
                aClass = new TMonitorClass(aRequest);
                resetClass(aClass);
                mContextClasses.insert(jClass, aClass);
                aClass->setID(jClass);
            }
            aMethod = new TMonitorMethod(aJvmti, aJni, jMethod, false, aClass, aClass->getName());
            aClass->registerMethod(aMethod);
            resetMethod(aMethod);
            mContextMethods.insert(jMethod, aMethod);
        }

        aLockAccess.exit();
        onMethodEnter(aJvmti, aJni, NULL, jMethod, aMethod, aThreadObj);
    }
    // ----------------------------------------------------
    // TMonitor::exitContext
    //! \brief Alternative to class/method view
    //!
    //! This method can be used in ATS and JARM mode 
    //! \param aJvmti   The Java tool interface
    //! \param aJni     The Java native interface
    //! \param aRequest The name of the request, which is equivalent to class
    //! \param aContext The name of the context, which is equivalent to method
    // ----------------------------------------------------
    jlong onContextExit(
                jvmtiEnv        *aJvmti, 
                JNIEnv          *aJni,
                const SAP_UC    *aRequest, 
                const SAP_UC    *aContext) {

        jvalue aReturn;
        TMonitorThread *aThreadObj = NULL;
        aJvmti->GetThreadLocalStorage(NULL, (void**)&aThreadObj);

        if (mProperties->getStatus() != MONITOR_ACTIVE) {
            return 0;
        }
        if (aContext == NULL) {
            aContext = cU("<init>");
        }
        TString   aStr(aRequest, aContext);
        jmethodID jMethod = (jmethodID)aStr.getHash();
        
        aReturn.i = 0;
        onMethodExit(aJvmti, aJni, NULL, jMethod, aThreadObj);
        return 0;
    }
};

#endif

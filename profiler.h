// -----------------------------------------------------------------
//
// Author: Albert Rossmann
// Date  : 14.04.2003
//! \file  profiler.h
//! \brief Profiler elements
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
#ifndef PROFILER_H
#define PROFILER_H
#include <jni.h>
#include <jni_md.h>

// ----------------------------------------------------
// ----------------------------------------------------
class   TMonitorClass;
class   TMonitorMethod;
class   TMonitorField;
// ----------------------------------------------------
//! \class TMonitorTimer
//! \brief Container for a method and entry time
//!
// ----------------------------------------------------
class TMonitorTimer {
private:
    jlong           mTime;
    jlong           mTimeElapsed;
    jint            mCount;
    jlong           mMemory;
    jlong           mLocation;
    TMonitorMethod *mMethod;
public:
    // ----------------------------------------------------
    // TMonitorTimer::TMontiorTimer
    //! \brief Constructor
    //! \param aMethod The method on the stack
    //! \param aTime   The timer value
    // ----------------------------------------------------
    TMonitorTimer(
        TMonitorMethod *aMethod,
        jlong           aTime       = 0,
        jlong           aLocation   = 0) {        
        set(aMethod, aTime, 0, 0, aLocation);
    }
    // ----------------------------------------------------
    // TMonitorTimer::TMonitorTimer
    //! Constructor
    // ----------------------------------------------------
    TMonitorTimer() {
        mMethod = NULL;
        mTime   = 0;
        mCount  = 0;
        mMemory = 0;
        mLocation = 0;
        mTimeElapsed = 0;
    }
    // ----------------------------------------------------
    // TMonitorTimer::TMonitorTimer
    //! Constructor
    // ----------------------------------------------------
    TMonitorTimer(TMonitorTimer &aTimer) {
        set(aTimer.mMethod, aTimer.mTime, aTimer.mCount, aTimer.mMemory, aTimer.mLocation);
    }
    // ----------------------------------------------------
    // TMonitorTimer::set
    //! \brief Reuse a timer object
    //! \param aMethod  The new method
    //! \param aTime    The new time
    //! \param aCount   The new stack level
    // ----------------------------------------------------
    inline void set(
        TMonitorMethod *aMethod,
        jlong           aTime   = 0,
        jint            aCount  = 0,
        jlong           aMemory = 0,
        jlong           aLocation = 0) {

        mMethod      = aMethod;
        mTime        = aTime;
        mCount       = aCount;
        mMemory      = aMemory;
        mLocation    = aLocation;
        mTimeElapsed = TSystem::getTimestampHp();
    }
    // ----------------------------------------------------
    // TMonitorTimer::getMethod
    //! \return Return the method
    // ----------------------------------------------------
    inline TMonitorMethod *getMethod() {
        return mMethod;
    }
    // ----------------------------------------------------
    // TMonitorTimer::getTime
    //! \return Return the time
    // ----------------------------------------------------
    inline jlong getTime() {
        return mTime;
    }
    // ----------------------------------------------------
    // ----------------------------------------------------
    inline jlong getMemory() {
        return mMemory;
    }
    // ----------------------------------------------------
    // ----------------------------------------------------
    inline jlong getLocation() {
        return mLocation;
    }
    // ----------------------------------------------------
    // TMonitorTimer::getCount
    //! \return Return the stack frame count
    // ----------------------------------------------------
    inline jint getCount() {
        return mCount;
    }
    inline jint incCount() {
        return ++mCount;
    }
    inline jint decCount() {
        if (mCount > 0) 
            mCount --;
        return mCount;
    }
    inline void setCount(jint aCnt) {
        mCount = aCnt;
    }
    // ----------------------------------------------------
    // TMonitorTimer::getTimeStamp
    //! \return Return the elapsed time for the method
    // ----------------------------------------------------
    inline jlong getTimeStamp() {
        return mTimeElapsed;
    }
    // ----------------------------------------------------
    // TMonitorTimer::getElapsed
    //! \brief Evaluates the elapsed time for the method
    //! \return The elapsed time
    // ----------------------------------------------------
    inline jlong getElapsed() {
        return TSystem::getDiffHp(mTimeElapsed);
    }
    // ----------------------------------------------------
    // TMonitorTimer::copy
    //! \brief Reuse a timer object
    //! \param aTimer timer to copy
    // ----------------------------------------------------
    void copy(TMonitorTimer *aTimer) {
        mMethod      = aTimer->mMethod;
        mTime        = aTimer->mTime;
        mTimeElapsed = aTimer->mTimeElapsed;
        mLocation    = aTimer->mLocation;
        mCount       = aTimer->mCount;
        mMemory      = aTimer->mMemory;
    }
};
// ----------------------------------------------------
// ----------------------------------------------------
typedef TList  <TMonitorMethod*>            TListMethods; //!< List of methods
typedef TStack <TMonitorTimer  >            TCallstack;   //!< Callstack
typedef THash  <jmethodID, 
                TMonitorMethod*, 
                TMonitorClass *>            THashMethods; //!< Hash of methods
typedef THash  <jlong, TMonitorClass *>     THashClasses; //!< Hash of classes
typedef THash  <jfieldID, TMonitorField *, 
                TMonitorClass *>            THashFields;  //!< Hash of fields
typedef TList  <TMonitorClass *>            TListClasses; //!< List of classes
// ----------------------------------------------------
//! \class TContext
//! \brief Context parser and analyser
//!
//! TContext can find a method in a call stack. 
//! Wildcards for stack entries are possible 
// ----------------------------------------------------
class TContext {
private:
    SAP_UC    *mEntry;
    const SAP_UC *mName;
    SAP_UC    *mContext;
    SAP_UC    *mAttributes;
    TValues   *mStackContext;
    TValues   *mStackAttribute;
public:
    // ----------------------------------------------------
    // TContext::TContext
    //! \brief Constructor
    //!
    //! Entry is of the format: 
    //! /context1/context2/ClassName.MethodName{attr1,attr2}
    //! - stack part: /conext1/context2/...
    //! - name part: ClassName.MethodName
    //! - attributes: {attr1, attr2}
    //! \param aEntry Context description
    // ----------------------------------------------------
    TContext(const SAP_UC *aEntry) {
        mEntry          = NULL;
        mName           = NULL;
        mContext        = NULL;
        mAttributes     = NULL;
        mStackContext   = NULL;
        mStackAttribute = NULL;

        if (aEntry != NULL) {
            int aSize   = STRLEN(aEntry) + 1;
            mEntry      = new SAP_UC [aSize + 1];
			memsetU(mEntry, 0, aSize + 1);
            STRCPY(mEntry, aEntry, aSize + 1);

            parseEntry();
            parseContext();
            parseAttributes();
        }
    }
    // ----------------------------------------------------
    // TContext::~TContext
    //! Destructor
    // ----------------------------------------------------
    ~TContext() {
        if (mEntry          != NULL) delete [] mEntry;
        if (mStackContext   != NULL) delete mStackContext;
        if (mStackAttribute != NULL) delete mStackAttribute;
    }
    // ----------------------------------------------------
    // TContext::getName
    //! \return Name part of the context
    // ----------------------------------------------------
    const SAP_UC *getName() {
        return mName;
    }
    // ----------------------------------------------------
    // TContext::getStack
    //! \return List of context stack elements
    // ----------------------------------------------------
    inline TValues *getStack() {
        return mStackContext;
    }
    // ----------------------------------------------------
    // TContext::getAttributes
    //! \return List of context attribute elements
    // ----------------------------------------------------
    inline TValues *getAttributes() {
        return mStackAttribute;
    }
private:
    // ----------------------------------------------------
    // TContext::parseContext
    //! Context parser
    // ----------------------------------------------------
    void parseContext()    {
        if (mContext == NULL) {
            return;
        }
		if (mStackContext != NULL) {
			delete mStackContext;
		}
        mStackContext = new TValues(8);
        TString aString(mContext);
        aString.split(mStackContext, cU('/'));
    }
    // ----------------------------------------------------
    // TContext::parseAttributes
    //! Attribute parser level 1
    // ----------------------------------------------------
    void parseAttributes()    {
        if (mAttributes == NULL) {
            return;
        }
        TString aString(mAttributes);
        if (mStackAttribute != NULL) 
            delete mStackAttribute;
        mStackAttribute = new TValues(8);
        aString.split(mStackAttribute, cU(','));
    }
    // ----------------------------------------------------
    // TContext::parseEntry
    //! Attribute parser level 0
    // ----------------------------------------------------
    void parseEntry() {
        int aPosStart;
        int aPosEnd;
        int aPos;
        TString aString(mEntry);
        
        aPos = aString.findLastOf(cU('/'));
        if (aPos != -1) {
            mContext = mEntry;        
            mName    = mEntry + aPos + 1;
            mEntry[aPos] = 0;
        }
        else {
            mName    = mEntry;
            mContext = NULL;
        }
        
        aPosStart = aString.findFirstOf(cU('{'));
        aPosEnd   = aString.findFirstOf(cU('}'));

        if (aPosStart != -1) {
            mAttributes  = mEntry + aPosStart + 1;
            mEntry[aPosStart] = 0;
        }
        else {
            mAttributes  = NULL;
        }

        if (aPosEnd != -1) {
            mEntry[aPosEnd] = 0;
        }
    }
};
// ----------------------------------------------------
// ----------------------------------------------------
typedef TList<jvmtiLocalVariableEntry *> TVariableList;
// ----------------------------------------------------
//! \class TMonitorMethod
//!
// ----------------------------------------------------
class TMonitorMethod: public THashObj {
private:
    TString        mName;               //!< Method name
    TString        mSignature;          //!< Method signature
    TString        mClassName;          //!< Class name
    TString        mFullName;

    jmethodID      mID;                 //!< Java ID 
    bool           mStatus;             //!< Visible for profiler/tracer
    bool           mActiveBreakpoints;
    bool           mExluded;            //!< Excluded from profiling
    bool           mIsTimer;            
    TMonitorClass *mClass;              //!< Class
    jlong          mTimeComp;           //!< CPU time spend on this method    
    jlong          mTimeElapsed;        //!< Elapsed time spend in this method
    jlong          mTimeContention;     //! Contention spend within this method
    jlong          mTimeContentionMax;
    int            mNrContention;
    int            mNrCalls;            //! Number of calls
    bool           mIsDebug;            //! Visible for tracer
    bool           mTriggerStack;       
    bool           mProfPointMemory;
    bool           mProfPointTrack;
    bool           mProfPointParam;
    bool           mLocalVariables;
    jvalue        *mVariableVal;
    jlocation      mLocationStart;
    jlocation      mLocationEnd;
    TContext      *mContextDebug;       //
    TContext      *mContextMonitor;     //! Context as alternative for Java methods
    TProperties   *mProperties;         //! Configuration
    jvmtiEnv      *mJvmti;              

    TVariableList            mVarList;
    jvmtiLocalVariableEntry *mVariables;    //!< List of call parameter
    jvmtiLineNumberEntry    *mEntryTable;   //!< Source info
    jint                     mVariableCnt;  //!< Number of call parameter
    jint                     mNrArguments;  
    bool                     mHasVariables;

    // ------------------------------------------------
    // TMonitorMethod::init
    //! Initialization
    // ------------------------------------------------
    void init(
            TMonitorClass  *aClass,
            jmethodID       aID) {

        mProfPointTrack     = false;
        mProfPointParam     = false;
        mActiveBreakpoints  = false;
        mNrCalls            = 0;
        mStatus             = false;
        mClass              = aClass;
        mID                 = aID;
        mTimeComp           = 0;
        mTimeElapsed        = 0;
        mIsDebug            = false;
        mIsTimer            = false;
        mTriggerStack       = false;
        mExluded            = false;
        mLocalVariables     = false;
        mContextDebug       = NULL;
        mContextMonitor     = NULL;
        mVariables          = NULL;
        mEntryTable         = NULL;
        mHasVariables       = false;
        mVariableCnt        = 0;
        mLocationStart      = -1;
        mLocationEnd        = -1;
        mTimeContentionMax  = 0;
        mTimeContention     = 0;
        mNrContention       = 0;
        mProperties         = TProperties::getInstance();
        mProfPointMemory    = false;
        mVariableVal        = NULL;
    }
public:
    // ------------------------------------------------
    // TMonitorMethod::TMonitorMethod
    //! \brief  Constructor
    //! \param  jMethod         The hash value
    //! \param  aMethodName     The name of the method
    //! \param  aClass          The parent class
    //! \param  aClassName      The parent class name
    // ------------------------------------------------
    TMonitorMethod(
            const SAP_UC   *aMethodName,
            const SAP_UC   *aMethodSign,
            TMonitorClass  *aClass,
            const SAP_UC   *aClassName) {

        jmethodID jMethod;

        mName.concat(aMethodName);
        mName.replace(cU('/'), cU('.'));
    
        mClassName  = aClassName;
        mFullName   = aClassName;
        mFullName.concat(cU("."));
        mFullName.concat(aMethodName);

        mJvmti      = NULL;
        mSignature  = aMethodSign;

        jMethod     = reinterpret_cast<jmethodID>(this);
        init(aClass, jMethod);
    }
    // ------------------------------------------------
    // TMonitorMethod::TMonitorMethod
    //! \brief  Constructor
    //! \param  aJvmti      The Java tool interface
    //! \param  aJni        The Java native interface
    //! \param  jMethod     The hash value
    //! \param  aIsInterface \c TRUE if method is an interface
    //! \param  aClass      The parent class
    //! \param  aClassName  The parent class name
    // ------------------------------------------------
    TMonitorMethod(
                jvmtiEnv       *aJvmti,
                JNIEnv         *aJni,
                jmethodID       jMethod,
                jboolean        aIsInterface,
                TMonitorClass  *aClass,
                const SAP_UC   *aClassName) {

        jint            aCount;
        const char     *aName;
        const char     *aSignature;
        const char     *aGeneric;
        jvmtiError      aResult;

        mJvmti = aJvmti;
        mJvmti->GetMethodName(jMethod, (char**)&aName, (char**)&aSignature, (char**)&aGeneric);

        mName.assignR(aName, STRLEN_A7(aName));
        mSignature.assignR(aSignature, STRLEN_A7(aSignature));

        mName.replace(cU('/'), cU('.'));
        mSignature.replace(cU('/'), cU('.'));
        mClassName = aClassName;
        mFullName  = aClassName;
        mFullName.concat(cU("."));
        mFullName.concat(mName.str());

        mJvmti->Deallocate((unsigned char*)aSignature);
        mJvmti->Deallocate((unsigned char*)aGeneric);
        mJvmti->Deallocate((unsigned char*)aName);
        init(aClass, jMethod);

        if (aIsInterface) {
            return;
        }
        mProfPointMemory = STRNCMP(mName.str(), cU("<init>"),   6) == 0 ||
                           STRNCMP(mName.str(), cU("<clinit>"), 8) == 0;

        aResult = mJvmti->GetLineNumberTable(jMethod, &aCount, &mEntryTable);
        if (aResult != JVMTI_ERROR_NONE) {
            return;
        }

        if (aCount > 1) {
            mLocationStart = mEntryTable[0].start_location;
            mLocationEnd   = mEntryTable[aCount - 1].start_location;
        }        
    }
    // ------------------------------------------------
    // TMonitorMethod::~TMonitorMethod
    //! Destructor
    // ------------------------------------------------
    virtual ~TMonitorMethod() {        
        if (mContextDebug   != NULL) delete mContextDebug;
        if (mContextMonitor != NULL) delete mContextMonitor;

        if (mLocalVariables) {
            getVariables(&mVariables, &mVariableVal, &mVariableCnt);
            for (int i = 0; i < mVariableCnt; i++) {
                delete [] mVariables[i].name;
                delete [] mVariables[i].signature;
            }
            if (mVariables != NULL) {
                delete [] mVariables;
            }
            delete [] mVariableVal;
            return;
        }

        if (mVariables != NULL) {
            for (int i = 0; i < mVariableCnt; i++) {
                mJvmti->Deallocate((unsigned char*)mVariables[i].name);
                mJvmti->Deallocate((unsigned char*)mVariables[i].signature);
            }
            mJvmti->Deallocate((unsigned char*)mVariables);
        }

        if (mEntryTable != NULL) {
            mJvmti->Deallocate((unsigned char*)mEntryTable);
        }
    }
    // ------------------------------------------------
    // ------------------------------------------------
    void addVariable(
            const SAP_UC    *aName,
            const SAP_UC    *aSignature) {

        TString lName(aName);
        TString lSignature(aSignature);

        jvmtiLocalVariableEntry *aVariable  = new jvmtiLocalVariableEntry;
        aVariable->start_location           = 0;
        aVariable->length                   = 0;

        aVariable->name                     = const_cast<char*>(lName.a7_str(true));
        aVariable->signature                = const_cast<char*>(lSignature.a7_str(true));
        
        if (mVariables != NULL) {
            delete [] mVariables;
            mVariables      = NULL;
        }
        if (mVariableVal != NULL) {
            delete [] mVariableVal;
            mVariableVal    = NULL;
        }
        mVarList.push_back(aVariable);
    }
    // ------------------------------------------------
    // ------------------------------------------------
    jint getVariables(
            jvmtiLocalVariableEntry **aTable,
            jvalue                  **aValues,
            jint                     *aCount) {
        
        jint i;
        TVariableList::iterator  aPtr;

        if  (mVariables  != NULL) {
            *aTable     = mVariables;
            *aCount     = mVariableCnt;
            *aValues    = mVariableVal;
            return JNI_OK;
        }

        if  (mVarList.getSize() == 0) {
            *aTable     = NULL;
            *aCount     = 0;
            *aValues    = NULL;
            return JNI_OK;
        }
        mLocalVariables = true;
        mVariableCnt    = (jint)mVarList.getSize();
        mVariables      = new jvmtiLocalVariableEntry[(size_t)mVarList.getSize()];

        for (aPtr  = mVarList.begin(), i = 0;
             aPtr != mVarList.end();
             aPtr  = mVarList.next(),  i ++) {
            mVariables[i].length    = aPtr->mElement->length;
            mVariables[i].name      = aPtr->mElement->name;
            mVariables[i].signature = aPtr->mElement->signature;
            mVariables[i].slot      = i;
        }        
        *aTable         = mVariables;
        *aCount         = mVariableCnt;

         mVariableVal   = new jvalue[mVariableCnt];
        *aValues        = mVariableVal;
        return JNI_OK;
    }
    // ------------------------------------------------
    // TMonitorMethod::getVariableTable
    //! \brief Find call parameter
    //! \param aTable The variable table 
    //! \param aCount The number of entries
    // ------------------------------------------------
    jint getVariableTable(
            jvmtiLocalVariableEntry **aTable,
            jint                     *aCount)
    {
        jint aResult = 0;

        *aCount = 0;
        *aTable = NULL;

        if (mJvmti == NULL) {
            getVariables(aTable, &mVariableVal, aCount);
            return aResult;
        }

        if (mVariables == NULL) {
            aResult = mJvmti->GetLocalVariableTable(getID(), &mVariableCnt, &mVariables);
            if (aResult == JVMTI_ERROR_NONE) {
                aResult = mJvmti->GetArgumentsSize(getID(), &mNrArguments);
            }

            // Generate a variable info, if no debug info available
            if (aResult == JVMTI_ERROR_ABSENT_INFORMATION) {
                TString lVarName(cU("this"));
                TString lVarSign;


                lVarSign.concat(cU("L"));
                lVarSign.concat(mClassName.str());
                lVarSign.concat(cU(";"));

                mLocalVariables                 = true;
                mVariableCnt                    = 1;
                mVariables                      = new jvmtiLocalVariableEntry[1];
                
                mVariables[0].start_location    = 0;
                mVariables[0].length            = 0;
                /*SAPUNICODEOK_CHARTYPE*/
                mVariables[0].name              = const_cast<char*>(lVarName.a7_str(true)); 
                /*SAPUNICODEOK_CHARTYPE*/
                mVariables[0].signature         = const_cast<char*>(lVarSign.a7_str(true));
                mVariables[0].slot              = 0;
            }
        }
        
        *aTable = mVariables;
        *aCount = mVariableCnt;
        return aResult;
    }
    // ------------------------------------------------
    // ------------------------------------------------
    bool isNative() {
        return (mJvmti == NULL);
    }
    // ------------------------------------------------
    // TMonitorMethod::isProfPointMem
    //! \return \c TRUE if method is a Java constructor
    // ------------------------------------------------
    bool isProfPointMem() {
        return mProfPointMemory;
    }
    // ------------------------------------------------
    // TMonitorMethod::getDebug
    //! \return \c TRUE if method is visible for tracing
    // ------------------------------------------------
    inline bool getDebug() {
        return mIsDebug;
    }
    // ------------------------------------------------
    // TMonitorMethod::enter
    //! Register a method call
    // ------------------------------------------------
    inline void enter() {
        mNrCalls ++;
    }
    // ------------------------------------------------
    // TMonitorMethod::reset
    //! Reset statistical data
    // ------------------------------------------------
    virtual void reset() {
        mNrCalls            = 0;
        mTimeComp           = 0;
        mTimeElapsed        = 0;
        mTimeContentionMax  = 0;
        mTimeContention     = 0;
        mNrContention       = 0;
    }
    // ------------------------------------------------
    // TMonitorMethod::exit
    //! Register a method call and calculate statistics
    // ------------------------------------------------
    inline void exit(jlong aDeltaTime, jlong aElapsedTime) {
        mTimeComp    += aDeltaTime;
        mTimeElapsed += aElapsedTime;
    }
    // ------------------------------------------------
    // TMonitorMethod::getCpuTime
    //! \return The accumulated CPU time
    // ------------------------------------------------
    inline jlong getCpuTime() {
        return mTimeComp;
    }
    // ------------------------------------------------
    // TMonitorMethod::getCpuDelta
    //! \return Calculate Elapsed time
    // ------------------------------------------------
    inline jlong getElapsed() {
        return mTimeElapsed;
    }
    // ------------------------------------------------
    // TMonitorMethod::getStatus
    //! \return \c TRUE if method is visible for profiler
    // ------------------------------------------------
    inline bool getStatus() {
        return mStatus && !mExluded;
    }

    // ------------------------------------------------
    // TMonitorMethod::enable
    //! \brief Activate method for profiler
    //! \param aStatus \c TRUE to activate method
    // ------------------------------------------------
    void enable(bool aStatus) {
        jvmtiError  aResult;
        jboolean    aExcept;
        bool        aActivateBreakpoint; 

        mStatus = aStatus;

        if (mJvmti == NULL || 
            mProperties->getProfilerMode() != PROFILER_MODE_TRIGGER) {
            return;
        }

        aActivateBreakpoint = aStatus || 
            (mProperties->doMonitorMemoryOn() && mProfPointMemory);

        if (mActiveBreakpoints == aActivateBreakpoint) {
            return;
        }

        if (!mActiveBreakpoints) {
            if (mLocationEnd < 0 || mLocationEnd <= mLocationStart) {
                return;
            }
            aResult = mJvmti->IsMethodSynthetic(mID, &aExcept);
            if (aExcept) {
                return;
            }
            aResult = mJvmti->IsMethodNative(mID,    &aExcept);
            if (aExcept) {
                return;
            }
            aResult = mJvmti->IsMethodObsolete(mID,  &aExcept);
            if (aExcept) {
                return;
            }
            aResult = mJvmti->SetBreakpoint(mID, mLocationStart);
            if (aResult != JVMTI_ERROR_NONE) {
                ERROR_OUT(getName(), aResult);
                return;
            }
            aResult = mJvmti->SetBreakpoint(mID, mLocationEnd);
            if (aResult != JVMTI_ERROR_NONE) {
                aResult = mJvmti->ClearBreakpoint(mID, mLocationStart);
                aResult = mJvmti->ClearBreakpoint(mID, mLocationEnd);
                ERROR_OUT(getName(), aResult);
                return;
            }
            mActiveBreakpoints = true;
        }
        else {
            aResult = mJvmti->ClearBreakpoint(mID, mLocationStart);
            aResult = mJvmti->ClearBreakpoint(mID, mLocationEnd);
            mActiveBreakpoints = false;
        }
    }
    // ------------------------------------------------
    // TMonitorMethod::getID
    //! \return The unique method hash
    // ------------------------------------------------
    inline jmethodID getID() {
        return mID;
    }
    // ------------------------------------------------
    // TMonitorMethod::getClass
    //! \return The method defining class
    // ------------------------------------------------
    TMonitorClass* getClass() {
        return mClass;
    }
    // ------------------------------------------------
    // TMonitorMethod::getName
    //! \return The name of the method
    // ------------------------------------------------
    virtual const SAP_UC *getName() {
        return mName.str();
    }
    // ------------------------------------------------
    // TMonitorMethod::getFullName
    //! \return The name of the method
    // ------------------------------------------------
    virtual const SAP_UC *getFullName() {
        return mFullName.str();
    }
    // ------------------------------------------------
    // TMonitorMethod::getSignature
    //! \return The signature of the method
    // ------------------------------------------------
    virtual TString *getSignature() {
        return &mSignature;
    }
    // ------------------------------------------------
    // TMonitorMethod::setContextDebug
    //! \brief Set context for tracing
    //! \param aContext The stack context 
    // ------------------------------------------------
    void setContextDebug(const SAP_UC *aContext) {
        mIsDebug = (aContext != NULL);

        if (mContextDebug != NULL) {
            delete mContextDebug;
            mContextDebug = NULL;
        }
        if (aContext != NULL) {
            mContextDebug = new TContext(aContext);
        }        
    }
    // ------------------------------------------------
    // TMonitorMethod::setContextMonitor
    //! \brief Set context for profiling
    //! \param aContext The stack context
    // ------------------------------------------------
    void setContextMonitor(const SAP_UC *aContext) {
        if (mContextMonitor != NULL) {
            delete mContextMonitor;
            mContextMonitor = NULL;
        }
        if (aContext != NULL) {
            mContextMonitor = new TContext(aContext);
        }
    }
    // ------------------------------------------------
    // TMonitorMethod::setContention
    //! \brief Register a contention
    //! \param aTime The contention time
    // ------------------------------------------------
    void setContention(jlong aTime) {
        if (mTimeContentionMax < aTime) {
            mTimeContentionMax = aTime;
        }
        mTimeContention += aTime;
        mNrContention   ++;
    }
    // ------------------------------------------------
    // TMonitorMethod::getNrCalls
    //! \return The number of registered calls 
    // ------------------------------------------------
    int getNrCalls() {
        return mNrCalls;
    }
    // ------------------------------------------------
    // TMonitorMethod::getStartPos
    //! \return The source start position
    // ------------------------------------------------
    jlocation getStartLocation() {
        return mLocationStart;
    }
    // ------------------------------------------------
    // TMonitorMethod::getEndLocation
    //! \return The source end position
    // ------------------------------------------------
    jlocation getEndLocation() {
        return mLocationEnd;
    }
    // ------------------------------------------------
    // TMonitorMethod::getLocalVariables
    //! \brief  Evaluates the local variables of a method
    //! \param  aNrArgs     After call: Contains the number of arguments
    //! \param  aNrVars     After call: Contains the number of variables
    //! \param  aVariables  After call: List of variables
    // ------------------------------------------------
    void getLocalVariables(
            jint    *aNrArgs, 
            jint    *aNrVars, 
            jvmtiLocalVariableEntry **aVariables) {
        
        jint aResult = 0;

        if (mVariables == NULL) {
            aResult = mJvmti->GetLocalVariableTable(getID(), &mVariableCnt, &mVariables);
            aResult = mJvmti->GetArgumentsSize(getID(), &mNrArguments);
        }
        *aNrArgs    = mNrArguments;
        *aNrVars    = mVariableCnt;
        *aVariables = mVariables;
    }
    // ------------------------------------------------
    // TMonitorMethod::dumpLocalVariables
    //! \brief Dump local variables
    //! \param aRootTag The output tag list
    // ------------------------------------------------
    void dumpLocalVariables(
        TXmlTag *aRootTag) {

        TXmlTag   *aTag;
        TString    aName;
        int        i;
        jvmtiError aResult;

        if (mJvmti == NULL) {
            return;
        }

        if (mVariables == NULL) {
            aResult = mJvmti->GetLocalVariableTable(getID(), &mVariableCnt, &mVariables);
            aResult = mJvmti->GetArgumentsSize(getID(), &mNrArguments);
        }

        if (mVariableCnt == 0) {
            aTag = aRootTag->addTag(cU("Argument"));
            aTag->addAttribute(cU("Name"),      cU(""));
            aTag->addAttribute(cU("Signature"), cU(""));
        }

        for (i = 0; i < mVariableCnt; i++) {
            if (mVariables[i].slot >= mNrArguments) {
                continue;
            }
            aTag = aRootTag->addTag(cU("Argument"));
            aName.assignR(mVariables[i].name, STRLEN_A7(mVariables[i].name));
            aTag->addAttribute(cU("Name"), aName.str());

            aName.assignR(mVariables[i].signature, STRLEN_A7(mVariables[i].signature));
            aName.replace(cU('/'), cU('.'));
            aTag->addAttribute(cU("Signature"), aName.str());
        }
    }
    // ------------------------------------------------
    // TMonitorMethod::setTimer
    //! \brief Timer events for method
    //! \param enable Set \c TRUE to enable timer events
    // ------------------------------------------------
    inline void setTimer(bool enable) {
        mIsTimer     = enable;
        mNrCalls     = 0;
        mTimeComp    = 0;
        mTimeElapsed = 0;
    }
    // ------------------------------------------------
    // TMonitorMethod::getTimer
    //! \return \c TRUE if timer is enabled 
    // ------------------------------------------------
    inline bool getTimer() {
        return mIsTimer;
    }
    // ------------------------------------------------
    // TMonitorMethod::getDebugAttributes
    //! \return The context attibutes 
    // ------------------------------------------------
    inline TValues *getDebugAttributes() {
        if (mContextDebug == NULL) {
            return NULL;
        }
        return mContextDebug->getAttributes();
    }
    // ------------------------------------------------
    // TMonitorMethod::getSortCol
    //! \brief  Allows      sorting for different parameter
    //! \param  aColName    of the sort parameter
    //! \return The sort    index used for TMonitorMethod::compare
    // ------------------------------------------------
    static int getSortCol(const SAP_UC *aColName) {
        int aCol = 0; 
        if (aColName != NULL) {
            if      (!STRNCMP(aColName, cU("CpuTime"),    7)) { aCol = 1; }
            else if (!STRNCMP(aColName, cU("Elapsed"),    7)) { aCol = 2; }
            else if (!STRNCMP(aColName, cU("Content"),    7)) { aCol = 3; }
            else if (!STRNCMP(aColName, cU("NrConte"),    7)) { aCol = 4; }
            else if (!STRNCMP(aColName, cU("NrCalls"),    7)) { aCol = 5; }
        }
        return aCol;
    }
    // ------------------------------------------------
    // TMonitorMethod::compare
    //! \brief Callback for sort algorithm
    //! \param aCmpCol  The sort index
    //! \param aCmp     A sort criterium
    //! \return The relation of column entry to aCmp
    // ------------------------------------------------
    jlong compare(int aCmpCol, jlong aCmp) {
        switch (aCmpCol) {
            case 1 : return getCpuTime()    - aCmp;
            case 2 : return getElapsed()    - aCmp;
            case 3 : return getContention() - aCmp;
            case 4 : return mNrContention   - aCmp;
            case 5 : return mNrCalls        - aCmp; 
            default: return 0;
        }
    }
    // ------------------------------------------------
    // TMonitorMethod::getContention
    //! \return \c TRUE if there was at least one contention
    // ------------------------------------------------
    jlong getContention() {
        return mTimeContention;
    }
    jlong getNrContention() {
        return mNrContention;
    }
    // ------------------------------------------------
    // TMonitorMethod::dump
    //! \brief Dump a method
    //! \param aRootTag    The output tag list
    //! \param aSignature  Dump method signature
    //! \param aContention Dump method contention statistic
    //! \param aOutputHash Dump method hash
    // ------------------------------------------------
    void dump(
            TXmlTag *aRootTag, 
            bool     aSignature, 
            bool     aContention,
            bool     aOutputHash = false) {

        SAP_UC aBuffer[128];
        
        TXmlTag *aTag = aRootTag->addTag(cU("Method"));


        aTag->addAttribute(cU("CpuTime"),       TString::parseInt(getCpuTime(),     aBuffer), PROPERTY_TYPE_INT | PROPERTY_TYPE_MICROSEC);
        aTag->addAttribute(cU("Elapsed"),       TString::parseInt(getElapsed(),     aBuffer), PROPERTY_TYPE_INT | PROPERTY_TYPE_MICROSEC);        
        aTag->addAttribute(cU("NrCalls"),       TString::parseInt(mNrCalls,         aBuffer), PROPERTY_TYPE_INT);
        aTag->addAttribute(cU("ClassName"),     mClassName.str());
        aTag->addAttribute(cU("MethodName"),    getName());        
        aTag->addAttribute(cU("Signature"),     mSignature.str());
 
        if (mProperties->doContention()) {
            aTag->addAttribute(cU("CtnEl"),     TString::parseInt(mTimeContention,  aBuffer), PROPERTY_TYPE_INT | PROPERTY_TYPE_MICROSEC);
            aTag->addAttribute(cU("CntNr"),     TString::parseInt(mNrContention,    aBuffer), PROPERTY_TYPE_INT);
        }

        if (aOutputHash) {
            aTag->addAttribute(cU("ID"), TString::parseHex(reinterpret_cast<jlong>(mID), aBuffer));
        }
        else {
            aTag->addAttribute(cU("ID"), TString::parseHex(reinterpret_cast<jlong>(mID), aBuffer), PROPERTY_TYPE_HIDDEN);
        }
    }
    // ------------------------------------------------
    // TMonitorMethod::checkContext
    //! \brief Analyse call stack context
    //! \param aStack  The callstack to analyse
    //! \param isDebug Decide to parse either debug or monitor callstack
    // ------------------------------------------------
    inline bool checkContext(TCallstack *aStack, bool isDebug = true);
};
// ----------------------------------------------------
//! \struct THistoryEntry
// ----------------------------------------------------
typedef struct SHistoryEntry {
    jlong mTimestamp;       //!< Timestamp of GC
    jlong mAllocated;       //!< Allocated memory
    jlong mDeallocated;     //!< Deallocated memory 
    jlong mSize;            //!< Current size
    jint  mNr;              //!< Number of GC
} THistoryEntry;
// ----------------------------------------------------
//! \class THistory
// ----------------------------------------------------
class THistory: public TRing <THistoryEntry> {
public:
    // ----------------------------------------------------
    // THistory::THistory
    //! Constructor
    // ----------------------------------------------------
    THistory(int aSize): TRing <THistoryEntry> (aSize) {
    }
    // ----------------------------------------------------
    // THistory::evalMin
    //! \brief Delete all values which are bigger than the current
    // ----------------------------------------------------
    void evalMin() {
        int aCntElements = 0;
        iterator aPtr1;
        iterator aPtr2;

        if (mNrElements < 2) {
            return;
        }
        
        aPtr2 = top();
        for (aPtr1  = begin();
             aPtr1 != aPtr2;
             aPtr1  = next()) {
            aCntElements ++;
            if (aPtr1->mSize >= aPtr2->mSize) {
                memcpyR(aPtr1, aPtr2, sizeofR(THistoryEntry));
                resize(aCntElements);
                return;
            }
        }
    }
};  
// ----------------------------------------------------
//! \class TMonitorField
// ----------------------------------------------------
class TMonitorField : public THashObj {
protected:
    TMonitorClass *mClass;    //!< The parent class
    TString   mName;          //!< The name 
    TString   mSign;          //!< The signature
    TString   mGeneric;
    int       mOffset;        //!< Offset
    int       mDimension;
    int       mElemSize;
    jfieldID  mFieldID;
    bool      mStatic;        //!< Static identifier
    int       mRefCnt;        //!< Reference counter
public:
    int       mType;          //!< Type identifier
    // -----------------------------------------------------
    // TMonitorField::TMonitorField
    //! Constructor
    // -----------------------------------------------------
    TMonitorField(
            TMonitorClass  *aClass,
            jfieldID        jField, /*SAPUNICODEOK_CHARTYPE*/
            const char     *aName,  /*SAPUNICODEOK_CHARTYPE*/
            const char     *aSign): 
            mName(), 
            mSign(),
            mGeneric() {
        mDimension = 0;
        mRefCnt    = 0;
        mStatic    = FALSE;
        mOffset    = 0;
        mClass     = aClass;
        mFieldID   = jField;
        mName.assignR(aName, STRLEN_A7(aName));
        mSign.assignR(aSign, STRLEN_A7(aSign));

        // calculate dimension:
        /*SAPUNICODEOK_CHARTYPE*/
        while (*aSign == cR('[')) {
            mDimension++;
            aSign++;
        }
        /*SAPUNICODEOK_CHARTYPE*/
        mType = *aSign;

        /*SAPUNICODEOK_LIBFCT*/
        if (!strcmpR(cR("mBytes"), aName)) {
            mType = *aSign;
        }
        switch (mType) {
        case cR('Z'): mElemSize = sizeofR(jboolean);     break;
        case cR('B'): mElemSize = sizeofR(jbyte);        break;
        case cR('C'): mElemSize = sizeofR(jchar);        break;
        case cR('S'): mElemSize = sizeofR(jshort);       break;
        case cR('I'): mElemSize = sizeofR(jint);         break;
        case cR('J'): mElemSize = sizeofR(jlong);        break;
        case cR('F'): mElemSize = sizeofR(jfloat);       break;
        case cR('D'): mElemSize = sizeofR(jdouble);      break;
        case cR('L'): mElemSize = sizeofR(jobject);      break;
        default:  mElemSize = 1; break;
        }
    }
    // -----------------------------------------------------
    // -----------------------------------------------------
    jsize getArraySize(
            jvmtiEnv    *aJvmti,
            JNIEnv      *aJni,
            jarray      jObject) {
        
        jsize aAccumulate = 1;
        if (jObject == NULL && mDimension <= 0) {
            return mElemSize;
        }

        aAccumulate = aJni->GetArrayLength(jObject);

        for (int i = 0; i < mDimension-1; i++) {
            jObject = (jarray)aJni->GetObjectArrayElement((jobjectArray)jObject, 0);

            if (jObject == NULL) {
                break;
            }
            jsize aDimension = aJni->GetArrayLength(jObject);

            if (aDimension == 0) {
                break;
            }
            aAccumulate *= aDimension;
        }
        return aAccumulate * mElemSize;
    }
    // -----------------------------------------------------
    // TMonitorField::Set
    //! \brief Initialize
    //! \param aClass The class for this field
    //! \param aName  The field name
    //! \param aSign  The field signatur
    //! \param aOffset The field offset
    //! \param isStatic \c TRUE if field is static
    // -----------------------------------------------------
    void set(TMonitorClass *aClass, /*SAPUNICODEOK_CHARTYPE*/
             const char    *aName,  /*SAPUNICODEOK_CHARTYPE*/
             const char    *aSign,  
             jfieldID       aFieldID,
             bool           isStatic = true) {

        mClass  = aClass;
        mName.assignR(aName, STRLEN_A7(aName));
        mSign.assignR(aSign, STRLEN_A7(aSign));

        mFieldID = aFieldID;
        mStatic  = isStatic;
        mRefCnt  = 0;
    }
    // -----------------------------------------------------
    //! Increase reference count
    // -----------------------------------------------------
    void incRefCnt() {
        mRefCnt ++;
    }
    // -----------------------------------------------------
    //! \return Reference count
    // -----------------------------------------------------
    int getRefCnt() {
        return mRefCnt;
    }
    // -----------------------------------------------------
    //! Reset reference count
    // -----------------------------------------------------
    void reset() {
        mRefCnt = 0;
    }
    // -----------------------------------------------------
    //! \return Field name
    // -----------------------------------------------------
    const SAP_UC *getName() {
        return mName.str();
    }
    // -----------------------------------------------------
    //! \return Field signature
    // -----------------------------------------------------
    const SAP_UC *getSign() {
        return mSign.str();
    }
    // -----------------------------------------------------
    //! \return Fields class
    // -----------------------------------------------------
    TMonitorClass *getClass() {
        return mClass;
    }
    // -----------------------------------------------------
    //! \return \c TRUE if field is static 
    // -----------------------------------------------------
    bool isStatic() {
        return mStatic;
    }
    // -----------------------------------------------------
    //! \return Field offset
    // -----------------------------------------------------
    int getOffset() {
        return mOffset;
    }
};
// ----------------------------------------------------
//! \class TMonitorConnector
//! \brief Structure for deep analysis
//!
// ----------------------------------------------------
class TMonitorConnector {
public:
    TMonitorField *mField;      //!< Field
    jobject        mObject;     //!< Object
    // ----------------------------------------------------
    //! Constructor
    //! \param aField   The connection field
    //! \param aObject  The connection destination
    // ----------------------------------------------------
    TMonitorConnector(
        TMonitorField *aField,
        jobject        aObject) {
        mField  = aField;
        mObject = aObject;
    }
};

class   TMemoryBit;
// ----------------------------------------------------
//! \class TMonitorClass
// ----------------------------------------------------
class TMonitorClass: public THashObj {
protected:
    TMonitorClass *mSuper;
    TMemoryBit    *mTag;
    TString        mName;               //!< Name of the class
    TListMethods  *mMethods;            //!< List of methods
    jlong          mID;                 //!< Hash value for the class
    bool           mIsProfiled;         //!< Visibility for profiler
    bool           mMemoryAlert;        //!< Memory history statistic
    jlong          mRefCount;           //!< Reference count for heap dump     
    jlong          mHeapCount;          //!< Number of instances
    jlong          mHeapSize;           //!< Size of all instances
    jlong          mSize;               //!< Size of this class
    jlong          mMaxSize;            //!< Max size of allocated memory
    jlong          mNrBits;             //!< Number of allocated objects
    jlong          mInstances;          //!< Number of allocated instances
    jlong          mTimestamp;          //!< Timestamps for GC history
    THistory      *mHistory;            //!< GC history for leak detection
    THistoryEntry *mHistoryEntry;       //!< History entry
    TLogger       *mLogger;             //!< Logger
    jlong          mNrInterfaces;       //!< Interfaces
    jlong          mStaticSize;         //!< Static size
    bool           mDelete;             //!< Delete flag
    bool           mVisible;            //!< Visible for statistic
    bool           mExcluded;           //!< Excluded from statistic 
    bool           mIsProfiledAll;      //!< Include for statistic
    bool           mIsObject;
    TProperties   *mProperties;         //!< Configuration
    jvmtiEnv      *mJvmti;              //!< Java tool interface
    jmethodID      mMethodLength;       //!< Call "length"
    jmethodID      mMethodConstr;       //!< Constructor
    jmethodID      mMethodFinalize;     //!< Finalizer
    THashFields   *mFields;             //!< Hash table for class fields

public:  
    jint           mNrMethods;          //!< Number of methods
    // ------------------------------------------------
    // TMonitorClass::TMonitorClass
    //! \brief Constructor
    //! \param aJvmti The Java tool interface
    //! \param aClass The current class
    // ------------------------------------------------
    TMonitorClass(
            jvmtiEnv        *aJvmti,
            jclass           jClass,
            TMonitorClass   *aSuper) {

        char        *aSignature;
        char        *aGeneric;

        mJvmti      = aJvmti;
        mSuper      = aSuper;
        mProperties = TProperties::getInstance();
        mFields     = NULL;

        mJvmti->GetClassSignature(jClass, &aSignature, &aGeneric);        
        mName.assignR(aSignature, STRLEN_A7(aSignature));
        mName.replace(cU('/'), cU('.'));
        mName.cut(1, mName.pcount()-1);

        /*SAPUNICODEOK_CHARTYPE*/ mJvmti->Deallocate((unsigned char*)aSignature);
        /*SAPUNICODEOK_CHARTYPE*/ mJvmti->Deallocate((unsigned char*)aGeneric);
        //mJvmti->GetObjectHashCode((jobject)jClass, (jint*)&mID);
        //mID = aID      
        init();
    }
    // ------------------------------------------------
    // TMonitorClass::TMonitorClass
    //! \brief Constructor
    //! \param aJvmti The Java tool interface
    //! \param aName  The name of the class
    // ------------------------------------------------
    TMonitorClass(
        const SAP_UC *aName) {

        mJvmti      = NULL;
        mProperties = TProperties::getInstance();        
        mName       = aName;
        mSuper      = NULL;
        mName.replace(cU('/'), cU('.'));
        mFields     = NULL;
        
        //mID = reinterpret_cast<jlong>(this);
        init();
    }
    // ------------------------------------------------
    // TMonitorClass::~TMonitorClass
    //! Destructor
    // ------------------------------------------------
    virtual ~TMonitorClass() {
        if (mHistory != NULL) {
            delete mHistory;
            mHistory = NULL;
        }
        if (mMethods != NULL) {
            delete mMethods;
            mMethods = NULL;
        }

        if (mFields != NULL) {
        	delete mFields;
        	mFields = NULL;
        }
    }

    // ------------------------------------------------
    // TMontiorClass::init
    //! Initializer
    // ------------------------------------------------
    void init() {
        mMemoryAlert      = false;
        mRefCount         = 0;
        mHeapCount        = 0;
        mHeapSize         = 0;
        mSize             = 0;
        mMaxSize          = 0;
        mNrMethods        = 0;
        mNrBits           = 0;
        mInstances        = 0;
        mTimestamp        = 0;
        mStaticSize       = 0;
        mIsProfiled       = false;
        mDelete           = false;
        mVisible          = true;
        mExcluded         = false;
        mIsProfiledAll    = false;
        mHistory          = NULL;
        mNrInterfaces     = 0;
        mTag              = 0;
        mLogger           = TLogger::getInstance();
        mMethods          = new TListMethods;
        mHistory          = new THistory(mProperties->getLimit(LIMIT_HISTORY));
        mMethodLength     = NULL;
        mMethodConstr     = NULL;

        setClass((jclass)this);
        mHistoryEntry = mHistory->push();
        mHistoryEntry->mTimestamp   = TSystem::getTimestampHp();
        mHistoryEntry->mAllocated   = 0;
        mHistoryEntry->mDeallocated = 0;
        mHistoryEntry->mSize        = 0;
        mHistoryEntry->mNr          = 0;
    }
public:
    // ------------------------------------------------
    // TMontiorClass::getSuper
    // ------------------------------------------------
    TMonitorClass *getSuper() {
        return mSuper;
    }
    // ------------------------------------------------
    // ------------------------------------------------
    jmethodID getConstructor() {
        return mMethodConstr;
    }
    // ------------------------------------------------
    // ------------------------------------------------
    void setConstructor(jmethodID aMethodConst) {
        mMethodConstr = aMethodConst;
    }
    // ------------------------------------------------
    // ------------------------------------------------
    void setFinalizer(jmethodID aMethodFinalize) {
        mMethodFinalize = aMethodFinalize;
    }
    // ------------------------------------------------
    // ------------------------------------------------
    jmethodID getFinalizer() {
        return mMethodFinalize;
    }
    // ------------------------------------------------
    // TMonitorClass::incRefCount
    //! \return New reference count
    // ------------------------------------------------
    inline jlong incRefCount() {
        return ++mRefCount;
    }
    //! \return New reference count
    inline jlong decRefCount() {
        return --mRefCount;
    }
    // ------------------------------------------------
    // TMonitorClass::incRefCount
    //! \return New heap count
    // ------------------------------------------------
    inline jlong incHeapCount(jlong aSize) {
        mHeapSize += aSize;
        return ++mHeapCount;
    }
    //! \return New heap count
    inline jlong getHeapCount() {
        return mHeapCount;
    }
    //! \return New heap size
    inline jlong getHeapSize() {
        return mHeapSize;
    }
    //! Reset heap count
    inline void resetHeapCount() {
        // TClassField::iterator aPtr;
        mHeapSize  = 0;
        mHeapCount = 0;
        //if (mFields == NULL) {
        //    return;
        //}
        //for (aPtr  = mFields->begin();
        //     aPtr != mFields->end();
        //     aPtr  = mFields->next()) {
        //    aPtr->reset();
        //}
    }
    // ------------------------------------------------
    // TMonitorClass::setStaus
    //! \brief Make class and methods visible for profiling
    //! \param aEnable \c TRUE to activate class
    //! \param aAll \c TRUE to activate all methods of the class
    // ------------------------------------------------
    void enable(bool aEnable, bool aAll = true) {
        TListMethods::iterator aPtr;
        mIsProfiled    = aEnable;
        if (aAll) {
            mIsProfiledAll = aEnable;
        }

        if (mMethods != NULL && aAll) {
            for (aPtr  = mMethods->begin();
                 aPtr != mMethods->end();
                 aPtr  = mMethods->next()) {
                aPtr->mElement->enable(aEnable);
            }
        }
    }
    // ------------------------------------------------
    // TMonitorClass::getMethodStatus
    //! \return \c TRUE if class is visible and active for profiler
    // ------------------------------------------------
    bool getMethodStatus() {
        return mIsProfiledAll;
    }
    // ------------------------------------------------
    // TMonitorClass::exclude
    //! \return \c TRUE if class is excluded from profiling
    // ------------------------------------------------
    void exclude(bool aExclude) {
        mExcluded = aExclude;
    }
    // ------------------------------------------------
    // TMonitorClass::setVisibility
    //! \brief Set visibility in statistic records
    //! \param enable \c FALSE to hide class for output
    // ------------------------------------------------
    void setVisibility(bool enable) {
        mVisible = enable;
    }
    // ------------------------------------------------
    // TMonitorClass::getVisibility
    //! \return \c TRUE if class is hidden
    // ------------------------------------------------
    bool getVisibility() {
        return mVisible;
    }
    // ------------------------------------------------
    // TMonitorClass::getExcluded
    //! \return \c TRUE if class is excluded from profiling
    // ------------------------------------------------
    bool getExcluded() {
        return mExcluded;
    }
    // ------------------------------------------------
    // TMonitorClass::getStatus
    //! \return \c TRUE if class is active for profiling
    // ------------------------------------------------
    inline bool getStatus() {
        return ((mIsProfiled || mSize > 0) && !mDelete) && !mExcluded;
    }
    // ------------------------------------------------
    // TMonitorClass::registerMethod
    //! \brief Register one method
    //! \param aMethod The method to register 
    // ------------------------------------------------
    void registerMethod(TMonitorMethod *aMethod) {
        if (mMethods != NULL && aMethod != NULL) {
            mMethods->push_back(aMethod);
            aMethod->enable(mIsProfiled);
        }
        else {
            ERROR_OUT(cU("register method"), 0);
        }
    }
    // ------------------------------------------------
    // ------------------------------------------------
    void registerFields(
            jvmtiEnv *aJvmti,
            JNIEnv   *aJni,
            jclass    jClass) {


        jvmtiError  aResult;
        jint        aCntField = 0;
        jfieldID   *aPtrField;
        int i;

        aResult = aJvmti->GetClassFields(jClass, &aCntField, &aPtrField);
        if (aCntField == 0)
            mFields = NULL;
        else if (aCntField < 7)
            mFields = new THashFields(11);
        else if (aCntField < 51)
            mFields = new THashFields(71);
        else if (aCntField < 110)
            mFields = new THashFields(127);
        else
            mFields = new THashFields(367);

        for (i = 0; i < aCntField; i++) {
            char *aFieldName;
            char *aSignature;
            char *aGeneric;
            jfieldID jField = aPtrField[i];

            aResult = aJvmti->GetFieldName(jClass, jField, &aFieldName, &aSignature, &aGeneric);
            TMonitorField *aClassField = new TMonitorField(this, jField, aFieldName, aSignature);
            mFields->insert(jField, aClassField, this);

            /*SAPUNICODEOK_LIBFCT*/
            if (!strncmpR(aSignature, cR("["), 1)) {
                aResult = aJvmti->SetFieldModificationWatch(jClass, jField);
            }
            aJvmti->Deallocate((unsigned char*)aFieldName);
            aJvmti->Deallocate((unsigned char*)aSignature);
            aJvmti->Deallocate((unsigned char*)aGeneric);
        }
        /*SAPUNICODEOK_CHARTYPE*/
        aJvmti->Deallocate((unsigned char*)aPtrField);
    }
    // ------------------------------------------------
    // ------------------------------------------------
    TMonitorField *getField(jfieldID jField) {
    	if (mFields == NULL) {
    		return NULL;
    	}
        THashFields::iterator aPtr = mFields->find(jField);
        if (aPtr != mFields->end()) {
            return aPtr->aValue;
        }
        else {
            return NULL;
        }
    }
    // ------------------------------------------------
    // TMonitorClass::allocate
    //! \brief Register allocation for this class
    //! \param aSize        The number of bytes allocated
    //! \param aTimestamp   Time at last GC
    //! \param aNr          The number of the GC
    // ------------------------------------------------
    void allocate(jlong aSize, jlong aTimestamp, jint aNr) {
        TProperties *aProperties = TProperties::getInstance();

        if (aProperties->doHistory()    && 
            mHistory    != NULL         && 
            mTimestamp  != aTimestamp   && 
            aTimestamp  != 0) {

            if (aProperties->doHistoryAlert()) {
                mHistory->evalMin();
                if (mHistory->getNrElements() > (mHistory->getSize() - 2)) {
                    mMemoryAlert = true;
                }
            }
            mTimestamp    = aTimestamp;
            mHistoryEntry = mHistory->push();

            mHistoryEntry->mAllocated   = 0;
            mHistoryEntry->mDeallocated = 0;
            mHistoryEntry->mSize        = 0;
            mHistoryEntry->mNr          = aNr + 1;
            mHistoryEntry->mTimestamp   = aTimestamp;
        }
        mNrBits  ++;
        mSize    += aSize;
        mMaxSize  = (mMaxSize < mSize) ? mSize : mMaxSize;

        if (mHistory != NULL) {
            mHistoryEntry->mAllocated += aSize;
            mHistoryEntry->mSize       = mSize;
        }
    }
    // ------------------------------------------------
    //! \brief Return the leak detector status
    //! \return \c TRUE if leak detector was active
    // ------------------------------------------------
    inline bool getAlert() {
        return mMemoryAlert;
    }
    // ------------------------------------------------
    //! \brief reset the leak detector
    //! 
    // ------------------------------------------------
    inline void resetAlert() {
        mMemoryAlert = false;
        mHistory->trunc(1);
    }
    // ------------------------------------------------
    // TMonitorClass::getHistorySize
    //! \return The number of history entries
    // ------------------------------------------------
    int getHistorySize() {
        if (mHistory != NULL) {
            return mHistory->getNrElements();
        }
        else {
            return 0;
        }
    }
    // ------------------------------------------------
    // TMonitorClass::deallocate
    //! \brief Register deallocation
    //! \param aSize The number of bytes to deallocate
    // ------------------------------------------------
    virtual void deallocate(jlong aSize, jboolean aStatistic = 1) {
        if (aSize == 0 || mSize == 0) {
            return;
        }
        if (mSize < aSize) {
            if (mNrBits > 1) {
                SAP_UC aBuffer[64];

                ERROR_STR << cU(" TMonitorClass::deallocate: ") << mName.str() << cU(" ")
                    << TString::parseInt(mSize, aBuffer)        << cU(" < ")
                    << TString::parseInt(aSize, aBuffer)        << std::endl;
            }
            aSize = mSize;
        }
        mNrBits--;
        mSize -= aSize;

        if (mHistory != NULL) {
            if (aStatistic == 1) {
                mHistoryEntry->mDeallocated += aSize;
            }
            else {
                mHistoryEntry->mAllocated -= aSize;
            }
            mHistoryEntry->mSize = mSize;
        }
    }
    // ------------------------------------------------
    // TMonitorClass::getName
    //! \return The name of the class
    // ------------------------------------------------
    virtual const SAP_UC *getName() {
        return mName.str();
    }
    // ------------------------------------------------
    // TMonitorClass::reset
    //! Reset the internal statistical data
    // ------------------------------------------------
    virtual void reset() {
        jint  aGCNr = 0;
        jlong aTime = 0;

        mNrBits           = 0;
        mSize             = 0;
        mMaxSize          = 0;
        mInstances        = 0;
        mRefCount         = 0;
        //resetVariables();
        if (mHistory != NULL) {
            if (mHistory->getSize() > 0) {
                mHistoryEntry = mHistory->top();
                aGCNr = mHistoryEntry->mNr;
                aTime = mHistoryEntry->mTimestamp;
            }
            mHistory->trunc(0);
            mHistoryEntry = mHistory->push();
            mHistoryEntry->mAllocated   = 0;
            mHistoryEntry->mDeallocated = 0;
            mHistoryEntry->mSize        = 0;
            mHistoryEntry->mNr          = aGCNr;
            mHistoryEntry->mTimestamp   = aTime;
        }
    }
    // ------------------------------------------------
    // TMonitorClass::getSize
    //! \return The current number of bytes allocated
    // ------------------------------------------------
    jlong getSize() {
        return mSize;
    }
    // ------------------------------------------------
    // TMonitorClass::getMaxSize
    //! \return The maximal size of this class
    // ------------------------------------------------
    jlong getMaxSize() {
        return mMaxSize;
    }
    // ------------------------------------------------
    // TMonitorClass::getID
    //! \return The hash value
    // ------------------------------------------------
    jlong getID() {
        return mID;
    }
    // ------------------------------------------------
    // TMonitorClass::getID
    //! \return The hash value
    // ------------------------------------------------
    void setID(jlong aID) {
        mID = aID;
    }
    // ------------------------------------------------
    // TMonitorClass::getMethods
    //! \return The list of methods 
    // ------------------------------------------------
    TListMethods *getMethods() {
        return mMethods;
    }
    // ------------------------------------------------
    // TMonitorClass::getNrInterfaces
    //! \return The number of interfaces
    // ------------------------------------------------
    jlong getNrInterfaces() {
        return mNrInterfaces;
    }
    // ------------------------------------------------
    // TMonitorClass::getStaticSize
    //! return The static size needed for heap dump level > 0
    // ------------------------------------------------
    jlong getStaticSize() { 
        return mStaticSize;
    }
    // ------------------------------------------------
    // TMonitorClass::getSortCol
    //! \brief Allows to select an attribute for sorting
    //! \param  aColName The name of the attribute
    //! \return The sort index
    // ------------------------------------------------
    static int getSortCol(const SAP_UC *aColName) {
        int aCol = 0; 
        if (aColName != NULL) {
            if      (!STRNCMP(aColName, cU("CurrSize"), 8))      { aCol = 1; }
            else if (!STRNCMP(aColName, cU("NewInstances"), 12)) { aCol = 2; }
            else if (!STRNCMP(aColName, cU("HeapCount"), 9))     { aCol = 3; }
            else if (!STRNCMP(aColName, cU("HeapSize"),  8))     { aCol = 4; }
        }
        return aCol;
    }
    // ------------------------------------------------
    // TMonitorClass::dumpColNames
    //! \brief Dump name of sort attributes
    // @param aRootTag The output tag list
    // ------------------------------------------------
    static void dumpColNames(TXmlTag *aRootTag) {
        TXmlTag *aTag;
        aTag = aRootTag->addTag(cU("Column"));
        aTag->addAttribute(cU("Name"), cU("CurrSize"));
        aTag->addAttribute(cU("Description"), cU("allocated bytes since start of monitor"));

        aTag = aRootTag->addTag(cU("Column"));
        aTag->addAttribute(cU("Name"), cU("NewInstances"));
        aTag->addAttribute(cU("Description"), cU("instances found in monitor memory table"));
    }
    // ------------------------------------------------
    // TMonitorClass::compare
    //! \see TMonitorClass::getSortCol
    //! \param aCmpCol  The sort index
    //! \param aCmp     A sort criterium
    //! \return The relation of column entry to aCmp
    // ------------------------------------------------
    jlong compare(int aCmpCol, jlong aCmp) {
        switch (aCmpCol) {
            case 1 : return mSize      - aCmp;
            case 2 : return mInstances - aCmp; 
            case 3 : return mHeapCount - aCmp;
            case 4 : return mHeapSize  - aCmp;
            default: return 0;
        }
    }
    // ------------------------------------------------
    // TMonitorClass::filterName
    //! \brief Find attibute with wildcard
    //! \param aCmpName The search string
    //! \return The attribute
    // ------------------------------------------------
    bool filterName(const SAP_UC *aCmpName) {
        if (aCmpName == NULL || STRLEN(aCmpName) == 0) {
            return true;
        }
        else {
            return (mName.findWithWildcard(aCmpName, cU('.')) != -1);
        }
    }
    // ------------------------------------------------
    // TMonitorClass::dump
    //! \brief Dump memory usage
    //! \param aRootTag  The output tag list
    //! \param aRef      Optional reference for detail navigation
    //! \param bDumpHash Dump hash value for external detail navigation
    // ------------------------------------------------
    TXmlTag *dump(
            TXmlTag      *aTag, 
            const SAP_UC *aRef      = NULL, 
            bool          bDumpHash = false) {

        SAP_UC   aBuffer[128];
        
        if (!mVisible) {
            return NULL;
        }
        //TXmlTag *aTag = aRootTag->addTag(cU("Class"));
        aTag->addAttribute(cU("CurrSize"), TString::parseInt(getSize(), aBuffer), PROPERTY_TYPE_INT);
        // allow detailed view for class name attribute
        if (aRef != NULL) {
            TString aAttrStr;
            aAttrStr.concat((SAP_UC*)aRef);
            aAttrStr.concat(cU("="));
            aAttrStr.concat(TString::parseInt(getID(), aBuffer));

            aTag->addAttribute(cU("ClassName"), getName(), aAttrStr.str());
        }
        else {
            aTag->addAttribute(cU("ClassName"), getName());
        }

        if (bDumpHash) {
            aTag->addAttribute(cU("Hash"), TString::parseHex(getID(), aBuffer), PROPERTY_TYPE_INT);
        }
        aTag->addAttribute(cU("ID"), TString::parseHex(getID(), aBuffer), PROPERTY_TYPE_HIDDEN);
        return aTag;
    }
    // ------------------------------------------------
    // TMonitorClass::dumpFields
    //! \brief Dump class fields
    //! \param aRootTag The output tag list
    // ------------------------------------------------
    void dumpFields(TXmlTag *aRootTag) {
        SAP_UC   aBuffer[128];
        TXmlTag *aTag = NULL;
        THashFields::iterator aPtr;
        
        if (mFields == NULL) {
        	return;
        }
        for (aPtr  = mFields->begin();
             aPtr != mFields->end();
             aPtr  = mFields->next()) {

            TMonitorField *aField = aPtr->aValue;
            aTag = aRootTag->addTag(cU("Field"));
            aTag->addAttribute(cU("ClassName"), getName());
            aTag->addAttribute(cU("FieldName"), aField->getName());
            aTag->addAttribute(cU("Signature"), aField->getSign());
            aTag->addAttribute(cU("ID"),        TString::parseHex(getID(), aBuffer), PROPERTY_TYPE_HIDDEN);
        }
    }
    // ------------------------------------------------
    // TMonitorClass::dumpHeap
    //! \brief Dump heap statistics
    //! \param aRootTag The output tag list
    // ------------------------------------------------
    void dumpHeap(TXmlTag *aRootTag) {
        SAP_UC aBuffer[128];
        TXmlTag *aTag;        

        if (mHeapCount != 0) {
            aTag = aRootTag->addTag(cU("Stack"));
            aTag->addAttribute(cU("HeapCount"),  TString::parseInt(mHeapCount, aBuffer), PROPERTY_TYPE_INT);
            aTag->addAttribute(cU("HeapSize"),   TString::parseInt(mHeapSize,  aBuffer), PROPERTY_TYPE_INT);
            aTag->addAttribute(cU("ClassName"),  getName());
            aTag->addAttribute(cU("ID"),         TString::parseHex(getID(), aBuffer), PROPERTY_TYPE_HIDDEN);
        }
    }
    // ------------------------------------------------
    // TMonitorClass::dumpHistory
    //! \brief Dump garbage collection history
    //! \param aRootTag The output tag list
    // ------------------------------------------------
    void dumpHistory(TXmlTag *aRootTag) {
        jlong aSize;
        THistory::iterator aPtr;
        TXmlTag           *aTag;
        THistoryEntry     *aEntry;

        SAP_UC aBuffer[128];
        if (mHistory == NULL || mHistory->getNrElements() < 1) {
            return;
        }

        for (aPtr  = mHistory->begin();
             aPtr != mHistory->end();
             aPtr  = mHistory->next()) {
            aEntry = aPtr;
            
            aSize = aEntry->mSize;
            if (aEntry->mTimestamp == 0) {
                aSize = mSize;
            }
            aTag = aRootTag->addTag(cU("History"));            
            aTag->addAttribute(cU("NrGC"),        TString::parseInt(aEntry->mNr,          aBuffer), PROPERTY_TYPE_INT);
            aTag->addAttribute(cU("Total"),       TString::parseInt(aSize,                aBuffer), PROPERTY_TYPE_INT);
            aTag->addAttribute(cU("Allocated"),   TString::parseInt(aEntry->mAllocated,   aBuffer), PROPERTY_TYPE_INT);
            aTag->addAttribute(cU("Deallocated"), TString::parseInt(aEntry->mDeallocated, aBuffer), PROPERTY_TYPE_INT);
            aTag->addAttribute(cU("TimeStamp"),   TString::parseInt(aEntry->mTimestamp,   aBuffer), PROPERTY_TYPE_INT);
        }
    }
    // ------------------------------------------------
    // TMonitorClass::dumpAlert
    //! \brief Dump the alert for possible memory leaks
    //! \param aRootTag The output tag list
    // ------------------------------------------------
    void dumpAlert(TXmlTag *aRootTag) {
        dumpHistory(aRootTag);
    }
    // ------------------------------------------------
    // TMonitorClass::dumpMethods
    //! \brief Dump all methods
    //! \param aRootTag The output tag list
    // ------------------------------------------------
    void dumpMethods(TXmlTag *aRootTag) {

        TListMethods::iterator aPtr;
        TMonitorMethod *aMethod;

        if (mMethods == NULL) {
            return;
        }
        for (aPtr  = mMethods->begin();
             aPtr != mMethods->end();
             aPtr  = aPtr->mNext) {
            aMethod = aPtr->mElement;
            aMethod->dump(aRootTag, false, false);
        }
    }
    // ------------------------------------------------
    // TMonitorClass::setDeleteFlag
    //! \brief Mark class to delete.
    //!
    //! The profiler removes a class from profiling, if it
    //! has freed all associates memory.
    //! \param aDelete \c TRUE to mark for delete
    // ------------------------------------------------
    void setDeleteFlag(bool aDelete) {
        mDelete = aDelete;
    }
    // ------------------------------------------------
    // TMonitorClass::deleteClass
    //! \see TMonitorClass::setDeleteFlag
    //! \return \c TRUE if the class can be removed from profiler
    // ------------------------------------------------
    bool deleteClass() {
        return (mDelete && mNrBits == 0 && mRefCount == 0);
    }
};
// ------------------------------------------------
// TMonitorMethod::checkContext
//! \brief Compares the current stack with the trace context
//! \param aStack  the callstack to analyse
//! \param isDebug decide to parse either debug or monitor callstack
//! \return \c TRUE if the method is in the given context
// ------------------------------------------------
inline bool TMonitorMethod::checkContext(TCallstack *aStack, bool isDebug) {
    TContext *aContext = mContextMonitor;
    bool aFound = false;
    if (isDebug) {
        aContext = mContextDebug;
    }
    if (aContext == NULL || 
        aContext->getStack() == NULL ||
        aContext->getStack()->getDepth() < 1) {
        return true;
    }
    if (aStack == NULL ||
        aStack->getDepth() < aContext->getStack()->getDepth()) {
        return false;
    }
    TCallstack::iterator aPtrStack;
    TValues::iterator    aPtrContext;
    // aStep is the aPtrContext stepper. Possible values are 0 or 1
    int aStep  = 0;

    aPtrContext = aContext->getStack()->end();
    aPtrContext -= 1;
    aPtrStack   = aStack->end();

    do {
        if (aPtrContext != aContext->getStack()->begin()) {
            aPtrContext -= aStep;
        }
        if (aPtrStack != aStack->begin()) {
            aPtrStack  --;
        }

        if (!STRCMP((*aPtrContext), cU("."))) {
            // whith the wildcard "." the 
            // aPtrStack and the aPtrContext are processed for next value
            aFound = true;
            aStep  = 1;
        }
        else if (!STRCMP((*aPtrContext), cU("..."))) {
            // whith the wildcard "..." the aPtrContext is kept on this stage
            // until the next strcmp is successful
            aFound = true;
            aStep  = 0;
            if (aPtrContext != aContext->getStack()->begin()) {
                aPtrContext--;
            }
        }
        else {
            // either wildcard or strcmp must be successfull
            aFound = false;

            TString  aString(
                (*aPtrStack).getMethod()->getClass()->getName(),
                (*aPtrStack).getMethod()->getName());  
            if (aString.findWithWildcard((*aPtrContext), cU('.')) != -1) {
                aFound = true;
                aStep  = 1;
            }
            else if (aStep == 1) {
                return false;
            }
        }
    }
    while (aPtrStack != aStack->begin());

    aFound = (aFound && aPtrContext == aContext->getStack()->begin());
    return aFound; 
};

// ---------------------------------------------------------
//! \class TMonitorThread
//! Administration for thread based profiling
// ---------------------------------------------------------
class   TMonitorThread;
typedef TList<TMonitorThread *> TThreadList;

class TMonitorThread: public THashObj {
    
    static TCallstack  *mGCallstack; 
    static jint         mGlobalHash;

    TString        mThreadName;         //!< Thread name
    TString        mGroupName;          //!< Thread group
    TString        mParentName;         //!< Thread parent
    TCallstack    *mCallstack;          //!< Callstack representation
    TCallstack    *mDebugOutput;        //!< Trace stack 
    TCallstack    *mAllocation;         //!< Allocation stack
    TCallstack    *mVirtualCallstack;
    jlong          mClock;
    jlong          mWaitTime;
    jlong          mRunTime;
    jlong          mCpuTime;
    jlong          mCpuCalculated;
    jlong          mCollisionTime;
    jlong          mIdleTime;
    jint           mStatus;
    jint           mContendedEnter;
    jlong          mHash;
    bool           mCallstackReference; //!< Common stack for ATS mode
    bool           mProcessJni;
    bool           mAttached;
    TProperties   *mProperties;         //!< Configuration
    jvmtiEnv      *mJvmti;
    TThreadList::iterator mThreadElem;
    static TThreadList mThreads;

public:

    // -----------------------------------------------------
    // TMonitorThread::TMonitorThread
    //! \brief Constructor
    //! \param aJvmti The Java tool interface
    //! \param jThread The current thread
    //! \param aCallstack Reference callstack for ATS mode
    // -----------------------------------------------------
    TMonitorThread(
            jvmtiEnv        *aJvmti,
            JNIEnv          *aJni,
            jthread          jThread,
            const SAP_UC    *aThreadName = NULL,
            TCallstack      *aCallstack  = NULL) {

        jvmtiThreadInfo  jThreadInfo;
        jvmtiError       aResult;

        mGroupName.assignR (cR("_group_"),  7);
        mParentName.assignR(cR("_parent_"), 8);
        
        mProperties     = TProperties::getInstance();
        mCallstack      = NULL;
        mClock          = 0;
        mWaitTime       = 0;
        mCpuTime        = 0;
        mCpuCalculated  = 0;
        mCollisionTime  = 0;
        mIdleTime       = 0;
        mContendedEnter = 0;
        mStatus         = 0;
        mDebugOutput    = new TCallstack(1024);
        mAllocation     = new TCallstack(64);
        mVirtualCallstack = NULL;
        mRunTime        = TSystem::getTimestamp();
        mProcessJni     = false;
        mAttached       = false;
        mJvmti          = aJvmti;
        mCallstackReference = false;
        mThreadElem     = NULL;

        if (aThreadName != NULL) {
            mThreadName = aThreadName;
        }
        else {
            aResult = mJvmti->GetThreadInfo(jThread, &jThreadInfo);
            if (aResult == JVMTI_ERROR_NONE) {
                mThreadName.assignR(jThreadInfo.name, STRLEN_A7(jThreadInfo.name));
                aResult = mJvmti->Deallocate((unsigned char*)jThreadInfo.name);
            }
        }

        if (aCallstack != NULL) {
            mCallstackReference = true;
            mCallstack = aCallstack;
        }
        mHash       = mGlobalHash++;
        mThreadElem = mThreads.push_back(this);
    }
    // -----------------------------------------------------
    // TMonitorThread::~TMonitorThread
    //! Destructor
    // -----------------------------------------------------
    virtual ~TMonitorThread() {
        mThreads.remove(mThreadElem);
        mThreadElem = NULL;

        if (mDebugOutput != NULL) {
            delete mDebugOutput;
        }
        
        if (mAllocation  != NULL) {
            delete mAllocation;
        }
        
        if (mCallstack != NULL && !mCallstackReference) {
            delete mCallstack;
        }

        if (mVirtualCallstack != NULL) {
            delete mVirtualCallstack;
        }

        mDebugOutput = NULL;
        mCallstack   = NULL;
        mAllocation  = NULL;
    }
    // -----------------------------------------------------
    // TMonitorThread::deallocate
    //! Implements a THashObject interface
    // -----------------------------------------------------
    virtual void deallocate(jlong) {
    }
    // ------------------------------------------------------------
    // TMonitorThread::attach
    //! Attach a thread to JVM
    // ------------------------------------------------------------
    void attach(
            JNIEnv **aJniEnv) {

        jint     aResult;
        JNIEnv  *aJni   = *aJniEnv;
        JavaVM  *aJvm   = NULL;

        if (aJniEnv == NULL) {
            return;
        }
        aJvm    = mProperties->getJavaVm();
        aResult = aJvm->GetEnv((void **)&aJni, (jint)JNI_VERSION_1_2);

        if (aResult == JNI_EDETACHED) {
            aResult = aJvm->AttachCurrentThread((void**)&aJni, NULL);
        }
        *aJniEnv  = aJni;
        mAttached = (aResult == JVMTI_ERROR_NONE);
    }
 
    // -----------------------------------------------------
    // TMonitorThread::getNrThreads
    //! \return the actual number of monitor threads
    // -----------------------------------------------------
    static jlong getNrThreads() {
        return mThreads.getSize();
    }
    // -----------------------------------------------------
    // TMonitorThread::resetThreads
    //! \brief Reset thread statistic
    // -----------------------------------------------------
    static void resetThreads() {
        TThreadList::iterator aPtr;
        for (aPtr = mThreads.begin(); aPtr != mThreads.end(); aPtr = mThreads.next()) {
            aPtr->mElement->reset();
        }
    }
    // -----------------------------------------------------
    // TMonitorThread::getName
    //! \return The name of the thread
    // -----------------------------------------------------
    inline virtual const SAP_UC *getName(jthread jThread = NULL) {
        jvmtiError      aResult;
        jvmtiThreadInfo jThreadInfo;

        if (mJvmti != NULL && jThread != NULL) {
            aResult = mJvmti->GetThreadInfo(jThread, &jThreadInfo);
            if (aResult == JVMTI_ERROR_NONE) {
                mThreadName.assignR(jThreadInfo.name, STRLEN_A7(jThreadInfo.name));
                /*SAPUNICODEOK_CHARTYPE*/
                mJvmti->Deallocate((unsigned char *)jThreadInfo.name);
            }
        }
        return mThreadName.str();
    }
    // -----------------------------------------------------
    // TMonitorThread::getHash
    //! \return The unique hash value
    // -----------------------------------------------------
    jlong getID() {
        return mHash;
    }
    // -----------------------------------------------------
    // TMonitorThread::setName
    //! \param aName The new name of the thread
    // -----------------------------------------------------
    inline void setName(const SAP_UC *aName) {
        mThreadName = aName;
    }
    // -----------------------------------------------------
    // TMonitorThread::setTimer
    //! \brief Store current clock for elapsed time calculation
    //! \param aClock
    // -----------------------------------------------------
    inline void setTimer(jlong aClock) {
        mClock += aClock;
    }
    // -----------------------------------------------------
    // TMonitorThread::getCallstack
    //! \brief  Create callstack on first call
    //! \return The callstack
    // -----------------------------------------------------
    inline TCallstack *getCallstack() {
        if (mCallstack == NULL) {
            mCallstack = new TCallstack(mProperties->getStackSize());
        }
        return mCallstack;
    }
    // -----------------------------------------------------
    // TMonitorThread::getVirtualStack
    //! \brief Return the virtual stack. This construct is usefull
    //! \brief to keep track on context, without propagating
    //! \brief the complete stack
    // -----------------------------------------------------
    inline TCallstack *getVirtualStack() {
        if (mVirtualCallstack == NULL) {
            mVirtualCallstack = new TCallstack(mProperties->getStackSize());
        }
        return mVirtualCallstack;
    }
    // -----------------------------------------------------
    // TMonitorThread::getDebugstack
    //! \return The debug callstack
    // -----------------------------------------------------
    inline TCallstack *getDebugstack() {
        return mDebugOutput;
    }
    // -----------------------------------------------------
    // TMonitorThread::hasCallstack
    //! \return \c TRUE if callstack not empty
    // -----------------------------------------------------
    inline bool hasCallstack() {
        return (mCallstack != NULL) && (!mCallstack->empty());
    }
    // -----------------------------------------------------
    // TMonitorThread::reset
    //! Reset callstacks
    // -----------------------------------------------------
    inline void reset() {
        if (mDebugOutput != NULL) {
            mDebugOutput->reset();
        }
        if (mCallstack != NULL) {
            mCallstack->reset();
        }
    }
    // -----------------------------------------------------
    //! \return \c TRUE if profiler is JNI mode
    // -----------------------------------------------------
    inline bool getProcessJni() {
        return mProcessJni;
    }
    // -----------------------------------------------------
    //! \brief Set the profiler thread in JNI mode
    //!
    //! It is not desired to register calls, which are generated
    //! by the profiler itself.
    //! \param aProcessJni Enables/Disables JNI mode
    // -----------------------------------------------------
    void setProcessJni(bool aProcessJni) {
        mProcessJni = aProcessJni;
    }
    // -----------------------------------------------------
    //! \brief Convenience function to check callstack and mode
    //! \param aCreateCallstack \c TRUE to create a callstack if necessary
    // -----------------------------------------------------
    bool doCheck(bool aCreateCallstack) {
        TCallstack *aCallstack = NULL;
        if (aCreateCallstack) {
            aCallstack = getCallstack();
        }
        else {
            aCallstack = mCallstack;
        }

        if (aCallstack == NULL) {
            return false;
        }       
        if (aCallstack->empty()) {
            return false;
        }
        if (mProcessJni) {
            return false;
        }
        return true;
    }
    // -----------------------------------------------------
    //! \return The stored CPU time in microseconds
    // -----------------------------------------------------
    inline jlong getStoredCpuTime() {
        return mClock;
    }
    // -----------------------------------------------------
    //! \return The current CPU time in microseconds
    // -----------------------------------------------------
    inline jlong getCurrentCpuTime() {
        jlong       aNanos = 0;
        jlong       aMicos = 0;
        jvmtiError  aResult;

        if (mProperties->doExecutionTimer(TIMER_CLOCK)) {
            aMicos = TSystem::GetCurrentThreadCpuTime();
        }
        else {
            aResult = mJvmti->GetCurrentThreadCpuTime(&aNanos);
            if (aResult == JVMTI_ERROR_NONE) {
                aMicos = aNanos / 1000;
            }
            else {
                // Convert time from microseconds into nano
                aMicos = TSystem::GetCurrentThreadCpuTime();
            }
        }
        mCpuTime = aMicos;
        return mCpuTime;
    }
    // -----------------------------------------------------
    // TMonitorThread::changeState
    //! \brief Register state changes for the thread
    //! \param aEvent New state
    // -----------------------------------------------------
    jlong changeState(jvmtiEvent aEvent) {
        jlong aDiff = 0;

        switch (aEvent) {
            case JVMTI_EVENT_MONITOR_WAIT:
                mWaitTime = TSystem::getTimestamp();
                break;
            case JVMTI_EVENT_MONITOR_CONTENDED_ENTER:
                mContendedEnter ++;
                mWaitTime = TSystem::getTimestamp();
                break;
            case JVMTI_EVENT_MONITOR_WAITED:
                if (mWaitTime != 0) {
                    aDiff           = TSystem::getDiff(mWaitTime);
                    mWaitTime       = 0;
                    mIdleTime      += aDiff;
                }
                break;
            case JVMTI_EVENT_MONITOR_CONTENDED_ENTERED:
                if (mWaitTime != 0) {
                    aDiff           = TSystem::getDiff(mWaitTime);
                    mWaitTime       = 0;
                    mCollisionTime += aDiff;
                    mIdleTime      += aDiff;
                }
                break;
        default:
            break;
        }
        return aDiff;
    }
    // -----------------------------------------------------
    // TMonitorThread::dump
    //! \param aRootTag         The output tag list
    // -----------------------------------------------------
    void dump(TXmlTag *aRootTag, jlong aDepth) {
        SAP_UC aBuffer[128];

        aRootTag->addAttribute(cU("ThreadName"), getName()); 
        aRootTag->addAttribute(cU("Depth"),      TString::parseInt(aDepth,            aBuffer), PROPERTY_TYPE_INT);
        aRootTag->addAttribute(cU("Clock"),      TString::parseInt(mClock,            aBuffer), PROPERTY_TYPE_INT);
        aRootTag->addAttribute(cU("ID"),         TString::parseHex(getID(),           aBuffer), PROPERTY_TYPE_INT);
    }
};

// ----------------------------------------------------
//! \class TMemoryBit
//! \brief Manage a chunk of allocated memory
// ----------------------------------------------------
class TMemoryBit: public THashObj {
public:
    TMonitorClass *mCtx;               //!< Reference to context class
    jlong          mSize;              //!< Size
    unsigned short mTID;               //!< Transaction ID, which is unique for each profiler session
    int            mIsClass;           //!< Reference to class or object
    // ----------------------------------------------------
    // TMemoryBit::TMemoryBit
    //! Constructor
    // ----------------------------------------------------
    TMemoryBit(
            TMonitorClass *aCtx,
            jlong          aSize,
            unsigned short aTID,
            int            aIsClass = true) {

        mCtx     = aCtx;
        mSize    = aSize;
        mTID     = aTID;
        mIsClass = aIsClass;
    }
};

#endif

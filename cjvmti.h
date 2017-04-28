// -----------------------------------------------------------------
//
// Author: Albert Rossmann
// File  : cjvmti.h
// Date  : 27.08.2008
// Abstract:
//    Profiler interface for C,C++
//
// 
// Copyright (C) 2015  Albert Zedlitz

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// -----------------------------------------------------------------
#ifndef CJVMTI_H
#define CJVMTI_H

#include "cti.h"
#include "ptypes.h"
#include "standard.h"
#include "extended.h"
#include "console.h"
#include "tracer.h"
#include "profiler.h"

#if defined(PROFILE_STD_CPP)
    #include <mutex>
    #include <condition_variable>
#endif
// -----------------------------------------------------------------
// -----------------------------------------------------------------
typedef THash<jlong, TMonitorThread *> THashThreads;
typedef THash<jlong, TMemoryBit *>     THashString;

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jmethodID JNICALL CtiRegisterMethod( /*SAPUNICODEOK_CHARTYPE*/
        const char      *jPackageName,          /*SAPUNICODEOK_CHARTYPE*/
        const char      *jClassName,            /*SAPUNICODEOK_CHARTYPE*/
        const char      *jMethodName,           /*SAPUNICODEOK_CHARTYPE*/
        const char      *jMethodSign);

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jfieldID JNICALL CtiRegisterField(
        jmethodID        jMethod,               /*SAPUNICODEOK_CHARTYPE*/
        const char      *jFieldName,            /*SAPUNICODEOK_CHARTYPE*/
        const char      *jFieldSign);       

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CtiOnExitMethod(
        jmethodID        jMethodID);

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" jint JNICALL CtiOnEnterMethod(
        jmethodID        jMethodID);

// -----------------------------------------------------------------
//! \brief CtiMonitor replaces JVMTI monitor in case cti is active
// -----------------------------------------------------------------
class CtiMonitor {
private:
#   if defined(PROFILE_SAP_CPP)
        THR_EVT_TYPE    mEvent;     //!< The event handle for RawMonitorWait
        THR_RECMTX_TYPE mMonitor;   //!< The mutex handle for native mutex support
#   elif defined (PROFILE_STD_CPP)
        std::recursive_mutex     *mMonitor;
        std::condition_variable  *mEvent;
        std::mutex               *mEventMtx;
#   endif

public:
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    CtiMonitor( /*SAPUNICODEOK_CHARTYPE*/ const char *aName);
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    ~CtiMonitor();
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    jvmtiError enter(bool aExclusive = true);
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    jvmtiError exit();
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    jvmtiError wait(jlong aTime);
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    jvmtiError notify();
};

// -----------------------------------------------------------------
// -----------------------------------------------------------------
class TObject: public THashObj {
public:
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    TObject(jclass jClass);
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    virtual void deallocate(jlong aSize);
};

typedef THash  <jlong, TObject *> THashObjects; //!< Hash of objects

// -----------------------------------------------------------------
// -----------------------------------------------------------------
class TJvmtiEnv {
public:
    static TJvmtiEnv            *mInstance;
    TCtiInterface               *mCtiEnv;
    JavaVM                      *mCtiJavaVM;
    jvmtiEnv                    *mCtiJvmti;
    JNIEnv                      *mCtiJni;
    jvmtiEventCallbacks         *mEventCallbacks;
    THashThreads                 mThreads;
    THashString                  mClassTags;
    jvmtiEventMode               mEventSettings[100];
    THashObjects                 mObjects;
    CtiMonitor                   mLockAccess;
private:
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    TJvmtiEnv();
public:
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    void setEventMode(jvmtiEventMode aMode, jvmtiEvent aEvent);
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    jvmtiEventMode getEventMode(jvmtiEvent aEvent);
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    static TJvmtiEnv *getInstance();
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    jint onAgentLoad(JavaVM *aJavaVm, jvmtiEnv *aJvmti);
};


#endif /*CJVMTI_H*/


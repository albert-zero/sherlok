// -----------------------------------------------------------------
//
// Author: Albert Rossmann
// File  : cti.h
// Date  : 27.08.2008
// Abstract:
//    Profiler interface for C,C++

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
#ifndef CTI_H
#define CTI_H
#include <jni.h>

#if defined (__cplusplus)
    #define USE_CONT_LOCTIME
    #define SAPUC_H_WITH_statU 
    #define CTI_VERSION_1 0x30000001

    typedef void (JNICALL *TCtiCallback)(/*SAPUNICODEOK_CHARTYPE*/const char *aTrace);        
    
    extern "C" {
        // -----------------------------------------------------------------
        // The tool interface
        // -----------------------------------------------------------------
        class TCtiInterface {
        public:
            jint mVersion;

            TCtiInterface & operator=(const TCtiInterface &aCti);

            jmethodID(JNICALL *registerMethod)(/*SAPUNICODEOK_CHARTYPE*/
                const char      *jPackageName,  /*SAPUNICODEOK_CHARTYPE*/
                const char      *jClassName,    /*SAPUNICODEOK_CHARTYPE*/
                const char      *jMethodName,   /*SAPUNICODEOK_CHARTYPE*/
                const char      *jMethodSign);

            jfieldID(JNICALL *registerField)(
                jmethodID        jMethod,       /*SAPUNICODEOK_CHARTYPE*/
                const char      *jFieldName,    /*SAPUNICODEOK_CHARTYPE*/
                const char      *jFieldSign);

            jint(JNICALL *onEnterMethod)       (jmethodID jMethodID);
            jint(JNICALL *onExitMethod)        (jmethodID jMethodID);
            jint(JNICALL *onException)         (jmethodID jMethodID, jlong jTLocation);
            jint(JNICALL *onExceptionCatch)    (jmethodID jMethodID, jlong jTLocation);

            jint(JNICALL *onObjectAlloc)(
                jlong            aSize,
                jmethodID        jMethod,
                jlong            jLocation,     /*SAPUNICODEOK_CHARTYPE*/
                unsigned char  **aMem);

            jint(JNICALL *onObjectCalloc)(
                jlong            aCount,
                jlong            aSize,
                jmethodID        jMethod,
                jlong            jLocation,     /*SAPUNICODEOK_CHARTYPE*/
                unsigned char  **aMem);

            jint(JNICALL *registerObject)(
                void            *aMem,
                jlong            aSize,
                jmethodID        jMethod,
                jlong            jLocation);

            jint(JNICALL *unregisterObject)(
                void            *aMem);

            jint(JNICALL *onObjectRealloc)(
                jlong            aSize,
                jmethodID        jMethod,
                jlong            jLocation, /*SAPUNICODEOK_CHARTYPE*/
                unsigned char  **aMem);

            jint(JNICALL *onObjectFree)(    /*SAPUNICODEOK_CHARTYPE*/
                unsigned char   *aMem);

            jint(JNICALL *doCommand)(       /*SAPUNICODEOK_CHARTYPE*/
                const char    *aCommand);

            jint(JNICALL *onVMDeath)();

            jint(JNICALL *registerCallback)(TCtiCallback aCallback);
        };

        // -----------------------------------------------------------------
        // The profiler interface
        // -----------------------------------------------------------------
        class TCtiProfiler {
        protected:
            bool                  mIsMain;
            jmethodID             mMethod;
            static TCtiInterface *mCti;

        public:
            TCtiProfiler(int argc, /*SAPUNICODEOK_CHARTYPE*/ char *argv[]);
            TCtiProfiler(
                jmethodID   *jMethod,           /*SAPUNICODEOK_CHARTYPE*/
                const char  *aPackage,          /*SAPUNICODEOK_CHARTYPE*/
                const char  *aClass,            /*SAPUNICODEOK_CHARTYPE*/
                const char  *aMethod,           /*SAPUNICODEOK_CHARTYPE*/
                const char  *aSignature);

            ~TCtiProfiler();

            static TCtiInterface *getCti();
            void * ctiAlloc(size_t aSize, jmethodID jMethod, int aLine);
            void * ctiCalloc(size_t aSize, jmethodID jMethod, int aLine);
            void * ctiRealloc(void *aPtr, size_t aSize, jmethodID jMethod, int aLine);
            void   ctiRegisterObject(void *aPtr, size_t aSize, int aLine);
            void   ctiUnregisterObject(void *aPtr);
            void   ctiDelete(void *aPtr);

        };
    }
    #define CCQ_SHERLOK_FCT_BEGIN(aPackage, aClass, aMethod, aSignature) { int __aRes; void *_aMem; static jmethodID _aMethodID = 0; TCtiProfiler _aProfiler(&_aMethodID, aPackage, aClass, aMethod, aSignature); {
    #define CCQ_SHERLOK_FCT_END                                          }}
        
    extern "C" JNIEXPORT jint (JNICALL *AgentOnLoad)( /*SAPUNICODEOK_CHARTYPE*/
            const char      *aOptions, 
            TCtiInterface  **CtiEnv,
            jint             aVersion);

    #define CCQ_SHERLOK_MAIN_BEGIN(argc, argv) { TCtiProfiler _aProfiler(argc, argv); {
    #define CCQ_SHERLOK_MAIN_END               }};                  

    // namespace stl { extern "C" double hypot (double x, double y) { return 0; } }

    #ifdef PROFILE_NEW                
        #undef  mallocR
        #define mallocR(aSize)        _aProfiler.ctiAlloc(aSize, __LINE__)
                
        #undef  mallocU
        #define mallocU(aSize)        (SAP_UC *)_aProfiler.ctiAlloc((aSize) * SAP_UC_LN, __LINE__)
                
        #undef  reallocR
        #define reallocR(aPtr, aSize) _aProfiler.ctiRealloc(aPtr, aSize, __LINE__)
                
        #undef  reallocU
        #define reallocU(aPtr, aSize) (SAP_UC *)_aProfiler.ctiRealloc(aPtr, (aSize) * SAP_UC_LN, __LINE__)
            
        #undef  callocR
        #define callocR(aCnt, aSize)  _aProfiler.ctiCalloc(aCnt, aSize, __LINE__)

        #undef  callocU
        #define callocU(aCnt, aSize)  (SAP_UC *)_aProfiler.ctiCalloc(aCnt, (aSize) * SAP_UC_LN, __LINE__)

        #undef  free
        #define free(aPtr)            _aProfiler.ctiDelete(aPtr)
        
        #define ipl_Malloc(_type, _size, _id) ( _aMem = ipl_Malloc(_type, _size, _id), _aProfiler.ctiRegisterObject(_aMem, _size, __LINE__), _aMem )
        #define ipl_Free(_type, _ptr, _id)    ( ipl_Free(_type, _ptr, _id), _aProfiler.ctiUnregisterObject(_ptr) )
    #endif
#else    
    #define CCQ_SHERLOK_FCT_BEGIN(aPackage, aClass, aMethod, aSignature) {{
    #define CCQ_SHERLOK_FCT_END                                          }}
            
    #define CCQ_SHERLOK_MAIN_BEGIN(argc, argv) {{
    #define CCQ_SHERLOK_MAIN_END               }}
#endif
#endif

// -----------------------------------------------------------------
//
// Author: Albert Rossmann
// Date  : 14.04.2003
// Abstract:
//! \file standard.h
//! \brief Data Administration.
//!
//! The classes in standard implement a vendor and operating system independend
//! runtime for the following data container.
//!     - TStack  = Fixed sized stack
//!     - TRing   = Ring buffer
//!     - TList   = Dynamic list
//!     - THash   = Dynamic size hash table
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
#ifndef STANDARD_H
#define STANDARD_H
#include <iomanip>

// ----------------------------------------------------------------
//! \class TSystem
//! \brief The system class implements different timer
// ----------------------------------------------------------------
class TSystem {
private:
    static SAP_UC        mBuffer[128];  //!< Buffer to return the time string
    static bool          mHasHpcTimer;  //!< On NT its possible to run the High-Performance-Counter
    static jlong         mOffset;
    static long double   mScale;        //!< All output has the same scale
#ifdef _WINDOWS
    static WSADATA       mWsaData;
    static WORD          mVersionRequested;

    static SYSTEMTIME    mSystemTime;   //!< Store system time 
    static LARGE_INTEGER mHpcStartTime; //!< Store start time of application
    static LARGE_INTEGER mHpcTime;      //!< Store current time
    static LARGE_INTEGER mHpcFrequence; //!< Store frequence for scaling
#endif
public:
    // ----------------------------------------------------
    // TSystem::getTimestamp
    //! Get the current timestamp
    //
    //! The timestamp is evaluated
    //! - On UNIX as clock
    //! - On NT with High-Precision-Counter or clock
    // 
    //! In case USE_RDTSC is active, this function evaluates
    //! the time in assembler, which is 10 times faster, than
    //! a call to QueryPerformanceCounter
    //! \return OS dependend timestamp scaled to mirco seconds
    // ----------------------------------------------------
    static jlong getTimestamp();
    static jlong getTimestampHp();
    // ----------------------------------------------------
    // TSystem::setHpcTimer
    //! \brief Evaluate the scaling factor.
    //!
    //! The high precision counter scales to micro seconds.
    //! This routine calculates the member variable mScale. 
    // ----------------------------------------------------
    static bool setHpcTimer();
    // ----------------------------------------------------
    // TSystem::calculateOffset
    //! \brief Evaluate the offset 1/1/1601 to 1/1/1970.
    //!
    //! The offset is used to transfer from NT-Filetime
    //! to UNIX timestamp format
    // ----------------------------------------------------
    static jlong calculateOffset(bool aForce);
    // ----------------------------------------------------
    // TSystem::hasHpc
    //! \brief  The system timer can return timestamps with
    //!         different precisions.
    //! \return TRUE if High-Precisioin-Counter is active
    // ----------------------------------------------------
    static bool hasHpc();
    // ----------------------------------------------------
    // TSystem::getSystemTime
    //! \brief  Human readable format for time
    //! \return Static buffer with time string
    // ----------------------------------------------------
    static SAP_UC *getSystemTime();
    // ----------------------------------------------------
    // TSystem::getDiff
    //! \brief Evaluate the difference between a timestamp and
    //!        the current time.
    //! \param  aTime A given timestamp in the past 
    //! \return The difference from current time to the given timestamp 
    //!         or NULL if the parameter aTime is NULL
    // ----------------------------------------------------
    static jlong getDiff(jlong aTime);
    static jlong getDiffHp(jlong aHpTime);
    // ----------------------------------------------------
    // TSystem::GetCurrentThreadCpuTime
    //! \brief Function used for Common Trace Interface 
    //!
    // ----------------------------------------------------
    static jlong GetCurrentThreadCpuTime();

    // ----------------------------------------------------
    // TSystem::Execute
    // ----------------------------------------------------
    static long Execute(const SAP_UC *aCmdLine);
    bool acceptClient(SOCKET aHostSocket, SOCKET *aClientSocket);
    void startup();
    bool openSocket(SOCKET *aHostSocket, unsigned short aPort, const SAP_UC *aHostName);
};

// ----------------------------------------------------------------
//! \class TStack
//! \brief Container to maintain elements in a FILO list.
//
//! TStack is called frequently, so we need the fastest 
//! possible implementation: The stack has a fixed size. 
//! - If the type \c _Ty is a pointer, the client has to allocate new elements and use TStack<class _Ty>::push(_Ty)
//! - If the type \c _Ty is a reference, the client can access a new element using TStack::push()
// ----------------------------------------------------------------
template <class _Ty> class TStack {
protected:
    _Ty    *mVector;        //!< The working memory area 
    int     mCursorWrite;   //!< The actual input position
    int     mSize;          //!< The maximum size
    SAP_UC *mMemory;        //!< Optional memory
    jlong   mHighMem;      
    int     mCursorRead;    //!< The actual read position
    int     mSequenceRead;  //!< A temporary read position
    int     mVirtualDepth;  //!< Allows to count virtual entries
public:
    typedef _Ty *iterator;  //!< Iterator for the container
public:
    // ----------------------------------------------------------------
    // TStack::TStack
    //! \brief Constructor
    //! \param aSize The fixed size of the stack
    // ----------------------------------------------------------------
    TStack(int aSize) {
        mSize         = aSize;
        mCursorWrite  = 0;
        mSequenceRead = 0;
        mVector       = new _Ty [mSize + 1];
        memsetR(mVector, 0, sizeofR(_Ty));
        mMemory       = NULL;
        mVirtualDepth = 0;
        mHighMem      = 0;
    }
    // ----------------------------------------------------------------
    // TStack::~TStack
    //! Destructor
    // ----------------------------------------------------------------
    ~TStack() {
        delete [] mVector;
        if (mMemory != NULL) {
            delete mMemory;
            mMemory  = NULL;
        }
    }
    // ----------------------------------------------------------------
    // TStack::setMemory
    //! \brief Management for client memory
    //!
    //! The memory pointer will be lost by the caller. TStack will take
    //! care of it and will delete it. This is useful, if the client 
    //! allocates a whole chunck of memory for elements to enter. 
    //! \param aMemory Pointer to be deleted in destructor of TStack
    // ----------------------------------------------------------------
    void setMemory(SAP_UC *aMemory) {
        if (mMemory != NULL) {
            delete mMemory;
            mMemory  = NULL;
        }
        mCursorWrite = 0;
        mMemory      = aMemory;
    }
    // ----------------------------------------------------------------
    // TStack::push
    //! \brief Adds a new element to the stack.
    //!
    //! \param aPtr The new element
    // ----------------------------------------------------------------
    inline void push(_Ty aPtr) {
        if (mCursorWrite < mSize) {
            mVector[mCursorWrite] = aPtr;
            mCursorRead = mCursorWrite;
            mCursorWrite ++;
        }
        else {
            ERROR_OUT(cU("TStack::push <stack overflow>"), mSize);
        }
    }
    // ----------------------------------------------------------------
    // TStack::push
    //! \brief Adds a new element to the stack.
    //! 
    //! The stack creates the element and returns an iterator. The TStack
    //! will keep the responsibility for iterator and the user must not delete it.
    //!
    //! \return Iterator to the new element
    // ----------------------------------------------------------------
    inline iterator push() {
        iterator aPtr;

        if (mCursorWrite < mSize) {
            aPtr = &mVector[mCursorWrite];
            mCursorRead = mCursorWrite;
            mCursorWrite ++;
        }
        else {
            ERROR_OUT(cU("TStack::push <stack overflow>"), mSize);
            aPtr = end();
        }
        return aPtr;
    }
    // ----------------------------------------------------------------
    // TStack::pop
    //! \brief Removes an element from stack
    //!
    //! The stack returns the top element and decrements the actual stack size
    //! \return Iterator to the top element
    // ----------------------------------------------------------------
    inline iterator pop() {
        if (mCursorWrite > 0) {
            mCursorWrite --;
            mCursorRead = mCursorWrite;
        }
        if (mSequenceRead > mCursorWrite) {
            mSequenceRead = mCursorWrite;
        }
        return mVector + mCursorWrite;
    }
    // ----------------------------------------------------------------
    // TStack::empty
    //! \brief Check if stack has elements
    //! \return \c true if stack is empty
    // ----------------------------------------------------------------
    inline bool empty() {
        return mCursorWrite == 0;
    }
    // ----------------------------------------------------------------
    // TStack::reset
    //! \brief Sets the internal iterator to the start of the stack and frees client memory
    //!
    //! \see TStack::begin
    // ----------------------------------------------------------------
    void reset(int aSize = 0) {
        if (aSize > 0) {
            if (mCursorWrite > aSize) {
                mCursorWrite = aSize;
                mCursorRead  = aSize;
            }
            if (mSequenceRead > aSize) {
                mSequenceRead = aSize;
            }
            return;
        }

        if (mMemory != NULL) {
            delete [] mMemory;
            mMemory  = NULL;
        }
        mCursorWrite  = 0;
        mSequenceRead = 0;
        mHighMem      = 0;
    }
    // ----------------------------------------------------------------
    // TStack::top
    //! \brief Access the top element without removing it.
    //!
    //! \return Iterator to the top of the stack
    // ----------------------------------------------------------------
    inline iterator top() {
        if (mCursorWrite > 0) {
            return mVector + mCursorWrite - 1;
        }
        else {
            return end();
        }
    }
    // ----------------------------------------------------------------
    // TStack::getDepth
    //! \brief  Number of stack elements
    //! \return Number of elements
    // ----------------------------------------------------------------
    int getDepth() {
        return mCursorWrite;
    }
    // ----------------------------------------------------------------
    // TStack::getSize
    //! \brief  Number of stack elements
    //! \return Number of elements
    // ----------------------------------------------------------------
    inline int getSize() {
        return mCursorWrite;
    }
    // ----------------------------------------------------------------
    // TStack::begin
    //! \brief Set the internal iterator to the start of the stack.
    //!
    //! Now the client can run subsequent calls the TStack::next 
    //! to get the elements of the stack until TStack::end is returned
    //! \return Iterator to the first element.
    // ----------------------------------------------------------------
    iterator begin() {
        mCursorRead   = 0;
        return mVector;
    }
    // ----------------------------------------------------------------
    // TStack::end
    //! \brief End of the stack used for iteration
    //!
    //! The returned iterator is the first element after the top element.        
    //! This is not a valid element.
    //! \see TStack::begin
    //! \return Iterator to the end of stack. 
    // ----------------------------------------------------------------
    iterator end() {
        return mVector + mCursorWrite;
    }
    // ----------------------------------------------------------------
    // TStack::next
    //! \brief The returned iterator is the next element after the current
    //!
    //! \see TStack::begin
    //! \return Iterator to the next element
    // ----------------------------------------------------------------
    iterator next() {
        if (mCursorRead < mCursorWrite) {
            mCursorRead++;
        }
        return mVector + mCursorRead;
    }
    // ----------------------------------------------------------------
    // TStack::beginSequence
    //! \brief  Sequence maintenance.
    //!
    //! A sequnce is a read position, which could be used as marker for 
    //! subsequent read operations.
    //! The sequence read position is modified with each access with TStack::nextSequence. 
    //! This way the sequence reader remembers to top most element, which was returned
    //! before new elements where pushed to the stack. TStack will adjust the read position
    //! to the current read, if elements are removed.
    //! \return Iterator to sequence start
    // ----------------------------------------------------------------
    iterator beginSequence() {
        return mVector + mSequenceRead;
    }
    // ----------------------------------------------------------------
    // TStack::nextSequence
    //! \brief  Next element in sequence
    //!
    //! \see TStack::beginSequence
    //! \return Iterator to the next sequence element 
    // ----------------------------------------------------------------
    iterator nextSequence() {
        if (mSequenceRead < mCursorWrite) {
            mSequenceRead ++;
        }
        return mVector + mSequenceRead;
    }
    // ----------------------------------------------------------------
    // TStack::getSequence
    //! \brief  The element at the sequence read position
    //!
    //! \see TStack::beginSequence
    //! \return Iterator to the sequence
    // ----------------------------------------------------------------
    int getSequence() {
        if (mSequenceRead < mCursorWrite) {
            return mSequenceRead;
        }
        else {
            return mCursorWrite;
        }
    }
    // ----------------------------------------------------------------
    // TStack::resetSequence
    //! \brief Resets the sequence.
    //!
    //! After reset the sequence read iteration starts from begin of the stack.
    //! \see TStack::beginSequence
    // ----------------------------------------------------------------
    void resetSequence(int aCnt = 0) {
        if (aCnt > 0 && aCnt < mCursorWrite) {
            mSequenceRead = aCnt;
        }
        else {
            mSequenceRead = 0;
        }
    }
    // ----------------------------------------------------------------
    // ----------------------------------------------------------------
    void adjustSequence() {
        if (mSequenceRead > mCursorWrite) {
            mSequenceRead = mCursorWrite;
        }
    }
    // ----------------------------------------------------------------
    // TStack::incVirtualDepth()
    //! \brief Increment virtual stack. 
    //!
    //! The virtual depth is a counter for elements, which are pushed
    //! virtually to stack. Only the push and pop events are counted, and
    //! if virtual count is zero the real stack is reached again.
    // ----------------------------------------------------------------
    inline void incVirtualDepth() {
        mVirtualDepth ++;
    }
    // ----------------------------------------------------------------
    // TStack::decVirtualDepth()
    //! \brief Decrement virtual stack 
    //!
    //! \see TStack::incVirtualDepth
    // ----------------------------------------------------------------
    inline int decVirtualDepth() {
        int aDepth = mVirtualDepth;
        if (mVirtualDepth > 0) {
            mVirtualDepth --;
        }
        return aDepth;
    }
    // ----------------------------------------------------------------
    // TStack::getVirtualDepth()
    //! \brief Get virtual stack depth
    //!
    //! \see TStack::incVirtualDepth
    // ----------------------------------------------------------------
    inline int getVirtualDepth() {
        return mVirtualDepth;
    }
    // ----------------------------------------------------------------
    // TStack::operator[]
    //! \brief  Access the element at the given position
    //!
    //! \param  aInx     The index of the element
    //! \return Iterator at the specified position
    // ----------------------------------------------------------------
    iterator operator[](jlong aInx) {
        if (aInx < mCursorWrite && aInx >= 0) {
            return mVector + aInx;
        }
        else {
            return end();
        }
    }
    // ----------------------------------------------------------------
    // ----------------------------------------------------------------
    void incHighMemMark(jlong aSize) {
        mHighMem += aSize;
    }
    // ----------------------------------------------------------------
    // ----------------------------------------------------------------
    jlong getHighMemMark() {
        return mHighMem;
    }
};
// ----------------------------------------------------------------
//! \class TRing
//! \brief Container to maintain elements in a ring.
//!
//! The TRing class implements a ring buffer of an initial size. 
//! TRing maintains elements for each entry. The client can access
//! a new ring element using TRing::push to modify the element content.
//! Elements are removed from ring buffer using 
//! - TRing::pop    remove single element from top
//! - TRing::resize remove elements from top
//! - TRing::trunc  remove elements from end
//!
//! If the ring is full, it starts overwriting elements from the begin.
// ----------------------------------------------------------------
template <class _Ty> class TRing {
protected:
    _Ty  **mVector;         //!< The element container 
    int    mCursorRead;     //!< The actual read position
    int    mCursorWrite;    //!< The actual write position
    int    mNrElements;     //!< Number of element in the ring
    int    mSize;           //!< Maximal number of elements
public:
    typedef _Ty *iterator;  //!< Iterator
public:
    // ----------------------------------------------------------------
    // TRing::TRing
    //! Constructor
    // \param aSize The maximal number of elements in the ring
    // ----------------------------------------------------------------
    TRing(int aSize) {
        int i;

        mSize        = aSize+1;
        mCursorRead  = 0;
        mCursorWrite = 0;
        mNrElements  = 0;
        mVector      = new iterator [mSize + 1];

        for (i = 0; i < mSize; i++) {
            mVector[i] = new _Ty;
        }
    }
    // ----------------------------------------------------------------
    // TRing::~TRing
    //! Destructor
    // ----------------------------------------------------------------
    ~TRing() {
        int  i;        
        for (i = 0; i < mSize; i++) {
            delete mVector[i];
        }
        delete [] mVector;
    }
    // ----------------------------------------------------------------
    // TRing::push
    //! \brief Add element to ring
    //!
    //! TRing inserts a new element and returns the iterator on it
    //! \return increment the write interator and return the top element
    // ----------------------------------------------------------------
    iterator push() {
        iterator aPtr = mVector[mCursorWrite];
        mCursorWrite  = (mCursorWrite + 1) % mSize;
        if (mNrElements < (mSize-1)) {
            mNrElements ++;
        }
        return aPtr;
    }
    // ----------------------------------------------------------------
    // TRing::pop
    //! \brief Remove top element from ring
    //! \return Iterator to the element at the top position
    // ----------------------------------------------------------------
    iterator pop() {
        int aTopInx = (mSize + mCursorWrite - 1) % mSize;

        if (mNrElements > 0) {
            mNrElements --;
            mCursorWrite = aTopInx;
        }
        mCursorRead = 0;
        return mVector[aTopInx];
    }
    // ----------------------------------------------------------------
    // TRing::trunc
    //! \brief Removes bottom elements from ring
    //!
    //! The write position is stable.
    //! \param aTrunc The number of elements to keep
    // ----------------------------------------------------------------
    iterator trunc(int aTrunc) {
        if (aTrunc <  mNrElements && aTrunc >= 0) {
            mNrElements  = aTrunc;
            mCursorRead  = 0;
        }
        return mVector[0];
    }
    // ----------------------------------------------------------------
    // TRing::resize
    //! \brief Removes top elements from ring
    //!
    //! The write position is adapted.
    //! \param aTrunc The number of elements to keep
    // ----------------------------------------------------------------
    void resize(int aTrunc) {
        if (aTrunc <  mNrElements && 
            aTrunc >= 0) {
            mCursorWrite = (mSize + mCursorWrite - mNrElements + aTrunc) % mSize;
            mNrElements  = aTrunc;
            mCursorRead  = 0;
        }
    }
    // ----------------------------------------------------------------
    // TRing::top
    //! \brief Access to top element
    //! \return The top element
    // ----------------------------------------------------------------
    iterator top() {
        if (mNrElements == 0) {
            return end();
        }
        mCursorRead  = 0; 
        int aLastInx = (mSize  + mCursorWrite - 1) % mSize; 
        return mVector[aLastInx];
    }
    // ----------------------------------------------------------------
    // TRing::begin
    //! \brief Start iteration over ring buffer
    //! \return Iterator to the first element
    // ----------------------------------------------------------------
    iterator begin() {
        int aCurrentPos = 0;
        mCursorRead     = 0;
        aCurrentPos     = (mSize  + mCursorWrite - mNrElements) % mSize;
        return mVector[aCurrentPos];
    }
    // ----------------------------------------------------------------
    // TRing::end
    //! \brief End iteration over ring buffer
    //! \see TRing::begin
    //! \return Iterator to the element after the last entry
    // ----------------------------------------------------------------
    iterator end() {
        return mVector[mCursorWrite];
    }
    // ----------------------------------------------------------------
    // TRing::next
    //! \brief Continue iteration over ring buffer
    //! \see TRing::begin
    //! \return Iterator to the next element
    // ----------------------------------------------------------------
    iterator next() {
        int aCurrentPos = 0;
        if (mCursorRead < mNrElements) {
            mCursorRead ++;
        }
        aCurrentPos = (mSize  + mCursorWrite - mNrElements + mCursorRead) % mSize;
        return mVector[aCurrentPos];
    }
    // ----------------------------------------------------------------
    // TRing::getNrElements
    //! \brief Number of elements
    //! \return Number of elments
    // ----------------------------------------------------------------
    int getNrElements() {
        return mNrElements;
    }
    // ----------------------------------------------------------------
    // TRing::getSize
    //! \brief Maximum number of elements
    //! \return Maximum number of elements
    // ----------------------------------------------------------------
    int getSize() {
        return mSize;
    }
};
// ----------------------------------------------------------------
//! \typedef TValues
//! Stack of Strings
// ----------------------------------------------------------------
typedef TStack <SAP_UC *> TValues;
// ----------------------------------------------------------------
//! \class TString
//! \brief String manipulation
//!
//! The string class implements converter for different formats and
//! an integer parser. It is also used for property list maintenance 
//! and for command line editing.
// ----------------------------------------------------------------
class TString {
protected:
    SAP_UC     *mString;        //!< Internal string
    int         mInsertPos;     //!< Insert position for editor
    int         mBytes;         //!< Number of allocated bytes
    bool        mReference;     //!< Indicates that internal sting is a reference
    SAP_A7     *mA7String;      //!< ASCII string representation

    // ----------------------------------------------------------------
    // TString::splitValues
    //! \brief Transforms a list of values into an array
    //!
    //! The client has to provide a TValues stack of suffient size and
    //! this method will fill elements into it. 
    //! \param aValues The stack to fill 
    //! \param aSource The source string as a list of values
    //! \param aChar   The separator char in the list of values
    // ----------------------------------------------------------------
    void splitValue(TValues *aValues, const SAP_UC *aSource, SAP_UC aChar) {
        SAP_UC *aClone;
        if (aSource == NULL || *aSource == cU('\0')) {
            return;
        }
        // memory allocation for TValues
        int aLen    = STRLEN(aSource) + 1;
        aClone      = new SAP_UC[aLen + 1];
        STRCPY(aClone, aSource,  aLen + 1);

        aValues->setMemory(aClone);
        aClone = splitPush(aValues, aClone, aChar);

        // split source
        while (*aClone != 0) {
            aClone ++;
            if (*aClone == aChar) {
                *aClone = 0;
                 aClone ++;

                if (*aClone == aChar) {
                    aClone = splitPush(aValues, aClone, cU(' '));
                }
                else {
                    aClone = splitPush(aValues, aClone, aChar);
                }
            }
        }
    }    
    // ----------------------------------------------------------------
    // TString::splitPush
    //! \brief Parser for split operations
    //!
    //! \param aValues The stack to fill 
    //! \param aSource The source string as a list of values
    //! \param aChar   The separator char in the list of values
    //! \return        The pointer to seperator char in source string
    // ----------------------------------------------------------------
    SAP_UC *splitPush(TValues *aValues, SAP_UC *aSource, SAP_UC aChar) {
        while (*aSource == aChar && *aSource != 0) {
            aSource++;
        }
        if (*aSource != 0) {
            aValues->push((SAP_UC*)aSource);
        }
        return aSource;
    }
public:
    typedef int iterator;   //!< Iterator
    // ----------------------------------------------------------------
    // TString::TString
    //! Constructor
    // ----------------------------------------------------------------
    TString() 
            : mBytes(0), mInsertPos(0), mA7String(NULL), mReference(0), mString(NULL) {
    }
    // ----------------------------------------------------------------
    // TString::TString
    //! \brief Constructor
    //! 
    //! Allows to create an internal buffer segment of a given size. This
    //! optimizes the reuse a TString object.
    //! \param aString An initial character sequence.
    //! \param aBytes  The inital length of the interal buffer
    // ----------------------------------------------------------------
    TString(const SAP_UC *aString, int aBytes = 0)
            : mBytes(0), mInsertPos(0), mA7String(NULL), mReference(0), mString(NULL) {
        
        if (aString == NULL) {
            return;
        }

        if (aBytes == 0) {
            aBytes = STRLEN(aString);
            mBytes = max(128, STRLEN(aString));
        }
        else {
            mBytes = max(128, aBytes);
        }

        mString = new SAP_UC[mBytes + 1];
        memsetR(mString, 0, (mBytes + 1) * sizeofR(SAP_UC) );
        STRNCPY(mString, aString, mBytes, mBytes + 1);
        
        mInsertPos = STRLEN(aString);
    }
    // ----------------------------------------------------------------
    // TString::TString
    //! \brief Constructor
    //!
    //! Allows to create a TString object with an outer reference.
    //! \param aString    The inital character sequence.
    //! \param aReference If \c TRUE TString handles the string argument as reference
    // ----------------------------------------------------------------
    TString(SAP_UC *aString, bool aReference)
            : mBytes(0), mInsertPos(0), mA7String(NULL), mReference(0), mString(NULL) {
        
        if (aReference) {
            mString    = aString;
            mReference = aReference;
        }
        else {
            mBytes    = max(128, STRLEN(aString));
            mString   = new SAP_UC[mBytes + 1];
            memsetR(mString, 0,   (mBytes + 1) * sizeofR(SAP_UC));
            STRNCPY(mString, aString, mBytes, mBytes + 1);
        }
        mInsertPos = STRLEN(aString);
    }

    // ----------------------------------------------------------------
    // TString::TString
    //! \brief Constructor
    //!
    //! Creates a character sequence from two strings separated by a dot.
    //! \param aString1 The first part of an inital character sequence.
    //! \param aString2 The second part of an inital character sequence.
    // ----------------------------------------------------------------
    TString(const SAP_UC *aString1, const SAP_UC *aString2, jlong aExt = 0)
            : mBytes(0), mInsertPos(0), mA7String(NULL), mReference(0), mString(NULL) {

        SAP_UC aBuffer[16];
        mBytes     = max(128, (STRLEN(aString1) + STRLEN(aString2) + 20));
        mString    = new SAP_UC [mBytes + 1];
        memsetR(mString, 0,     (mBytes + 1) * sizeofR(SAP_UC));

        STRCPY(mString, aString1, mBytes);
        STRCAT(mString, cU("."),  mBytes);
        STRCAT(mString, aString2, mBytes);

        if (aExt > 0) {
            STRCAT(mString, cU("_"), mBytes);
            STRCAT(mString, TString::parseInt(aExt, aBuffer), mBytes);
        }
        mInsertPos = STRLEN(mString);
    }
    // ----------------------------------------------------------------
    // TString::TString
    //! Copy Constructor
    // ----------------------------------------------------------------
    TString(TString &aStr) 
            : mBytes(0), mInsertPos(0), mA7String(NULL), mReference(0), mString(NULL) {
        
        if (aStr.mBytes == 0) {
            return;
        }
        mBytes  = aStr.mBytes;
        mString = new SAP_UC[mBytes + 1];
	    memsetR(mString, 0,  (mBytes + 1) * sizeofR(SAP_UC));
        STRCPY(mString, aStr.mString, mBytes + 1);
    
        mInsertPos = STRLEN(mString);
    }

    // ----------------------------------------------------------------
    // TString::TString
    //! \brief Constructor for jstring
    //!
    //! The jstring supplied by the JNI is converted to a TString object
    //! \param jEnv    The JNI environment
    //! \param jString The inital JNI string
    // ----------------------------------------------------------------
    TString(JNIEnv *jEnv, jstring jString)
            : mBytes(0), mInsertPos(0), mA7String(NULL), mReference(0), mString(NULL) {
        assign(jEnv, jString);
    }

    // ----------------------------------------------------------------
    // TString::~TString
    //! Destructor
    // ----------------------------------------------------------------
    ~TString() {
        if (mA7String != NULL) {
            delete [] mA7String;
        }

        if (mString  != NULL) {
            if (!mReference) {
                delete [] mString;
            }
        }
    }

    // ----------------------------------------------------------------
    // TString::operator=
    //! \brief Copy operator
    //! 
    //! The copy operation will check, if current buffer size is big emough 
    //! for the operation. Otherwise it will allocate new space.
    // \param aBuffer Source string to copy
    // ----------------------------------------------------------------
    void operator=(const SAP_UC *aBuffer) {
        mInsertPos = 0;
        
        if (aBuffer == NULL || aBuffer[0] == cU('\0')) {
            if (mString != NULL) {
                mInsertPos = 0;
                mString[0] = cU('\0');
            }
            return;
        }
        
        int aCpyBytes  = max(128, STRLEN(aBuffer)+1);
        
        if (mBytes < aCpyBytes) {
            if (mReference) {
                ERROR_OUT(cU("TString::operator="), aCpyBytes);
                return;
            }
            else {
                if (mString != NULL) {
                    delete [] mString;
                }
                mBytes  = max(32, aCpyBytes + 1);
                mString = new SAP_UC[mBytes + 1];
		        memsetR(mString, 0,  (mBytes + 1) * sizeofR(SAP_UC));
            }
        }

        STRNCPY(mString, aBuffer, aCpyBytes, mBytes + 1);
        mInsertPos = STRLEN(mString);
    }
    // ----------------------------------------------------------------
    // TString::moveCursor
    //! \brief Edit operation
    //!
    //! The Edit operations allows the client to keep track over the current
    //! curser, insert or delete position.
    //! \param aPos Position in string for positioning the curser.
    // ----------------------------------------------------------------
    iterator moveCursor(int aPos) {
        if (mString == NULL) {
            return 0;
        }
        iterator aAbsPos = abs(aPos);
        if (aPos < 0) {
            mInsertPos -= aAbsPos;
            if (mInsertPos < 0) {
                mInsertPos = 0;
            }
        }
        else {
            mInsertPos += aAbsPos;
		    if (mInsertPos > (int)STRLEN(mString)) {
                mInsertPos = (int)STRLEN(mString);
            }
        }
        return mInsertPos;
    }
    // ----------------------------------------------------------------
    // TString::backspace
    //! \brief Edit operation
    //! \see TString::moveCursor
    //! Removes the character before the edit cursor.
    // ----------------------------------------------------------------
    void backspace() {
        int i;
        int aBytes;
        if (mString == NULL) {
            return;
        }
        if (mInsertPos == 0) {
            return;
        }
        aBytes = (int)STRLEN(mString);
        for (i = mInsertPos - 1; i < aBytes; i++) {
            *(mString + i) = *(mString + i + 1);
        }
    }

    // ----------------------------------------------------------------
    // TString::operator[]
    //! \brief Direct access to TString character sequence
    //! \param  aPos The requested index
    //! \return The character at the given position
    // ----------------------------------------------------------------
    SAP_UC operator[] (int aPos) {
        if (mString == NULL) {
            return cU('\0');
        }
        if (aPos > mBytes) {
            aPos = mBytes;
        }
        return mString[aPos];
    }
    // ----------------------------------------------------------------
    // TString::compareSignatur
    //! \brief Compares java signatur with current string representation
    //!
    //! TString stores signatur and java path information using a dot as
    //! separator. The comparission takes account to this and ignores slashes
    //! in a given signatur.
    //! \param  aSigPtr The signatur of a class or method to compare
    //! \return \c TRUE if signatur matches the string.
    // ----------------------------------------------------------------
    bool compareSignatur(const SAP_A7 *aSigPtr) {
        SAP_UC *aPtr = mString;
        SAP_UC  aChar;

        if (mString == NULL) {
            return false;
        }
        aSigPtr ++;
        while (*aPtr != 0 && *aSigPtr != 0) {
            aChar = *aPtr;

            if (aChar == cU('.')) {
                aChar = cU('/');
            }
            /*SAPUNICODEOK_CAST*/
            if ((SAP_A7)aChar != *aSigPtr) {
                return false;
            }
            aSigPtr ++;
            aPtr    ++;
        }
        return (*aSigPtr == cR(';'));
    }
    // ----------------------------------------------------------------
    // TString::replace
    //! \brief Replaces characters
    //! 
    //! Replaces all occurences of a given character in a string
    //! \param aOld The character to replace
    //! \param aNew The new character
    // ----------------------------------------------------------------
    void replace(SAP_UC aOld, SAP_UC aNew) {
        SAP_UC *aStr = mString;
        if (mString == NULL) {
            return;
        }
        while (*aStr != cU('\0')) {
            if (*aStr == aOld) {
                *aStr  = aNew;
            }
            aStr++;
        }
    }
    // ----------------------------------------------------------------
    // ----------------------------------------------------------------
    void encodeXML() {
        if (mString == NULL) {
            return;
        }
        int     aCnt    = 0;
        SAP_UC *aStrOld = mString;
        
        while (*aStrOld != cU('\0')) {
            if (*aStrOld == cU('<') ||
                *aStrOld == cU('>')) {
                aCnt ++;
            }
            aStrOld ++;
        }

        if (aCnt == 0) {
            return;
        }
        mBytes += (aCnt * 4) + 1;
        aStrOld = mString;
        SAP_UC *aString = new SAP_UC [mBytes];
        SAP_UC *aStrNew = aString;

        while (*aStrOld != cU('\0')) {
            if (*aStrOld == cU('<')) {
                *aStrNew++ = cU('&');
                *aStrNew++ = cU('l');
                *aStrNew++ = cU('t');
                *aStrNew++ = cU(';');
            }
            else if (*aStrOld == cU('>')) {
                *aStrNew++ = cU('&');
                *aStrNew++ = cU('g');
                *aStrNew++ = cU('t');
                *aStrNew++ = cU(';');
            }
            else {
                *aStrNew++ = *aStrOld;
            }
            aStrOld++;
        }
        *aStrNew = cU('\0');
        delete [] mString;
        mString  = aString;        
    }
    // ----------------------------------------------------------------
    // TString::replace
    //! \brief Replaces characters
    //!
    //! Replaces all occurences of a given character in a string
    //! by a character sequence
    //! \param aOld The character to replace
    //! \param aNew The character sequence
    // ----------------------------------------------------------------
    void replace(SAP_UC aOld, SAP_UC *aNew) {
        SAP_UC *aStr    = mString;
        SAP_UC *aNewStr;
        SAP_UC *aNewPtr;
        int     i;
        int     aLen    = 0;
        int     aRepl   = 0;

        if (mString == NULL) {
            return;
        }
        if (STRLEN(aNew) == 1) {
            replace(aOld, aNew[0]);
            return;
        }
        if (mReference) {
            return;
        }
        // Find out if there is something to replace
        while (*aStr != cU('\0')) {
            if (*aStr == aOld) {
                aRepl += (STRLEN(aNew)-1);
            }
            aLen++;
            aStr++;
        }        
        // Nothing to replace: return
        if (aRepl == 0) {
            return;
        }
        // Create new string for replacement results
        aNewStr    = new SAP_UC[aLen + aRepl + 1];
        aStr       = mString;
        aNewPtr    = aNewStr;
        mInsertPos = 0;

        while (*aStr != cU('\0')) {
            if (*aStr == aOld) {
                for (i = 0; i < STRLEN(aNew); i++) {
                    *aNewPtr++ = aNew[i];
                }
            }
            else {
                *aNewPtr++ = *aStr;
            }
            aStr++;
        }
        *aNewPtr = cU('\0');
        delete [] mString;
        mString    = aNewStr;
        mBytes     = aLen + aRepl;
        mInsertPos = mBytes;
    }
    // ----------------------------------------------------------------
    // TString::split
    //! \brief Split the internal string to a given stack.
    //! \see TString::splitValue
    //! \param aValues The stack with the elements
    //! \param aChar   The separator char in the TString 
    // ----------------------------------------------------------------
    void split(TValues *aValues, SAP_UC aChar) {
        if (mString != NULL) {
            splitValue(aValues, mString, aChar);
        }    
    }    
    // ----------------------------------------------------------------
    // TString::trim
    //! Erase all blanks from a string
    // ----------------------------------------------------------------
    void trim() {
        int i = 0;
        int j = 0;
        if (mString == NULL) {
            return;
        }
        do { if (mString[i] != cU(' ') && mString[i] != cU('\t')) {
            mString[j++]  = mString[i]; }
        } while (mString[i++] != cU('\0'));
        mInsertPos = (int)STRLEN(mString);
    }        
    // ----------------------------------------------------------------
    // TString::trimLeft
    //! Erase blanks from the begin
    // ----------------------------------------------------------------
    void trimLeft() {
        int i    = 0;
        int aPos = 0;
        if (mString == NULL) {
            return;
        }
        while (mString[aPos] == cU(' ')) {
            aPos++;
        }

        if (aPos > 0) {
            while (mString[aPos] != cU('\0')) {
                mString[i++] = mString[aPos++];
            }
            mString[i] = cU('\0');
        }
        mInsertPos = (int)STRLEN(mString);
    }        
    // ----------------------------------------------------------------
    // TString::findFirstOf
    //! \brief Find the first occurence of a char in a string
    //!
    //! \param  aChar  The char to search for
    //! \param  aStart The start index for the search
    //! \return The index of first occurence of aChar in TString or 
    //! TString::end() if the char was not found
    // ----------------------------------------------------------------
    iterator findFirstOf(SAP_UC aChar, int aStart = 0) {
        int  i;
        if (mString == NULL) {
            return (iterator)-1;
        }
        for (i = aStart; i < mBytes; i++) {
            if (mString[i] == aChar) {
                return i;
            }
        }
        return (iterator)-1;
    }
    // ----------------------------------------------------------------
    // TString::findLastOf
    //! \brief Find the last occurence of a char in a string
    //!
    //! \param aChar  The char to search for
    //! \return The index of last occurence of aChar in TString or 
    //! TString::end() if the char was not found 
    // ----------------------------------------------------------------
    iterator findLastOf(SAP_UC aChar) {
        int  i;
        if (mString == NULL) {
            return (iterator)-1;
        }
        for (i = mBytes - 1; i >= 0; i--) {
            if (mString[i] == aChar) {
                return i;
            }
        }
        return (iterator)-1;
    }
    // ----------------------------------------------------------------
    // TString::pcount
    //! \brief  Counts the number of characters in TString
    //! \return Number of characters
    // ----------------------------------------------------------------
    int pcount() {
        if (mString == NULL) {
            return 0;
        }
        else {
            return (int)STRLEN(mString);
        }
    }
    // ----------------------------------------------------------------
    // TString::end
    //! \brief  Defines an iterator end
    //! \return Iterator at the end of a string
    // ----------------------------------------------------------------
    inline iterator end() {
        return (iterator)-1;
    }
    // ----------------------------------------------------------------
    // TString::findWithWildcard
    //! \brief Searches a substring allowing wildcards.
    //!
    //! \param  aSubString The substring to find
    //! \param  aWildCard  The wildcard
    //! \param  aStart     The start position
    //! \return The index of the start of aSubString in TString or 
    //!         TString::end() if the substring was not found.
    // ----------------------------------------------------------------
    iterator findWithWildcard(const SAP_UC *aSubString, SAP_UC aWildCard, int aStart = 0) {
        if (aSubString == NULL) {
            return -1;
        }
        if (mString == NULL) {
            return -1;
        }
        if ((aSubString[0] != aWildCard) &&
            (aSubString[0] != mString[aStart])) {
            return -1;
        }

        int  aFinal = (int)STRLEN(aSubString) - 1;
        int  aLen   = (int)STRLEN(mString)    - 1;
        bool aEndsWithWildcard   = (aSubString[aFinal] == aWildCard);
        bool aStartsWithWildcard = (aSubString[0] == aWildCard);

        if (aLen < aFinal) {
            return end();
        }

        if (aEndsWithWildcard) {
            if (aFinal > 0) {
                aFinal --;
            }
            else {
                return 0;
            }
        }
        else {
            // aStart = aLen - aFinal;
        }

        if (aStartsWithWildcard) {
            aSubString++;
            aStart ++;
            aFinal --;
        }
        iterator aPos = find(aSubString, aStart, aFinal);

        if (aPos == end()) {
            return aPos;
        }
        if (aEndsWithWildcard) {
            return aPos;
        }
        if (aPos + aFinal != aLen) {
            return end();
        }
        return aPos;
    }
    // ----------------------------------------------------------------
    // TString::find
    //! \brief Searches a substring in TString
    //! \param  aSubString The sub string to find
    //! \param  aStart     The start index for the search
    //! \param  aFinal     The end index
    //! \return The index of the start of aSubString in TString or 
    //!         TString::end() if the substring was not found.
    // ----------------------------------------------------------------
    iterator find(const SAP_UC *aSubString, int aStart = 0, int aFinal = -1) {
        int aState = 0;
        int aPos   = aStart;

        // evaluate final state
        if (aFinal == -1) {
            aFinal = (int)STRLEN(aSubString) - 1;
        }

        if (mString == NULL) {
            return -1;
        }

        while (aPos <= mBytes) {
            if (mString[aPos] == 0) {
                break;
            }
            
            if (mString[aPos] == aSubString[aState]) {
                aState ++;
            }
            else {
                aState = 0;
            }

            if (aState == aFinal + 1) {
                return aPos - aFinal;
            }
            aPos ++;
        }
        return -1;
    }
    // ----------------------------------------------------------------
    // TString::parseInt
    //! \brief Creates a string representation on decimal base.
    //!
    //! The parser inserts decimal points, so the output is of the format 100.000
    //! \param  aInt    The integer to parse
    //! \param  aBuffer The work memory
    //! \param  aRight  Index to which a aBuffer is fill up with spaces. 
    //!                 This parameter is used for alignment.
    //! \param  aSigned /c TRUE for signed integer 
    //! \return The pointer to aBuffer
    // ----------------------------------------------------------------
    static SAP_UC* parseInt(
            jlong   aInt, 
            SAP_UC *aBuffer, 
            int     aRight = 0, 
            bool    aSigned = false);
    // ----------------------------------------------------------------
    // TString::parseHex
    //! \brief Creates a string representation on hexadecimal base.
    //! \param  aInt the integer to parse
    //! \param  aBuffer the work memory
    //! \return The pointer to aBuffer
    // ----------------------------------------------------------------
    static SAP_UC* parseHex(jlong aInt, SAP_UC *aBuffer);
    // ----------------------------------------------------------------
    // TString::parseBool
    //! \brief Creates a string representation for boolean values.
    //!
    //! At return the buffer contains either \c "true" or \c "false"
    //! \param  bValue  The bool value to parse
    //! \param  aBuffer The work memory
    //! \return The pointer to aBuffer
    // ----------------------------------------------------------------
    static SAP_UC* parseBool(bool bValue, SAP_UC *aBuffer);
    // ----------------------------------------------------------------
    // TString::toInteger
    //! \brief Transforms the string representation of an integer to a jlong value
    //!
    //! The parser accepts decimal points
    //! \return The integer value
    // ----------------------------------------------------------------
    jlong toInteger() {
        if (mString == NULL) {
            ERROR_OUT("toInteger", 0);
            return 0;
        }
        return TString::toInteger(mString);
    }
    
    // ----------------------------------------------------------------
    // ----------------------------------------------------------------
    SAP_A7 *toBase64() {
        int i;
        int a8BitSrc[3];
        int a6BitDst[4]; 

        int aCntSource   = 0;
        int aCntDest     = 0;

        int aFillBytes   = mInsertPos % 3;
        int aRequiredLen = 4 * ((mInsertPos + aFillBytes) / 3) + 3;
        
        if (mA7String != NULL) {
            delete [] mA7String;
        }
        mA7String = new SAP_A7[aRequiredLen+2];

        while (aCntSource < mInsertPos) {
            /*SAPUNICODEOK_SIZEOF*/memsetR(a8BitSrc, 0, sizeofR(a8BitSrc));
            /*SAPUNICODEOK_SIZEOF*/memsetR(a6BitDst, 0, sizeofR(a6BitDst));

            for (i = 0; i < min(3, mInsertPos - aCntSource); i++) {
                a8BitSrc[i] = mString[aCntSource++];
            }
            /*SAPUNICODEOK_CHARCONST*/a6BitDst[0] = ((a8BitSrc[0] >> 2)) & 0x3F;
            /*SAPUNICODEOK_CHARCONST*/a6BitDst[1] = ((a8BitSrc[0] << 4   & 0x30) | (a8BitSrc[1] >> 4 & 0x0F)) & 0x3F;
            /*SAPUNICODEOK_CHARCONST*/a6BitDst[2] = ((a8BitSrc[1] << 2   & 0x3C) | (a8BitSrc[2] >> 6 & 0x03)) & 0x3F;
            /*SAPUNICODEOK_CHARCONST*/a6BitDst[3] =   a8BitSrc[2] & 0x3F;
            
            for (i = 0; i < 4; i++) {
                if (a6BitDst[i] < 26) 
                    /*SAPUNICODEOK_CHARCONST*/mA7String[aCntDest++] = (int)'A' + a6BitDst[i];
                else if (a6BitDst[i] < 52)
                    /*SAPUNICODEOK_CHARCONST*/mA7String[aCntDest++] = (int)'a' + a6BitDst[i] - 26;
                else if (a6BitDst[i] < 62)
                    /*SAPUNICODEOK_CHARCONST*/mA7String[aCntDest++] = (int)'0' + a6BitDst[i] - 52;
                else if (a6BitDst[i] == 62)
                    mA7String[aCntDest++] = cR('+');
                else if (a6BitDst[i] == 63)
                    mA7String[aCntDest++] = cR('/');
            }
        }
        for (i = 0; i < aFillBytes; i++) {
            mA7String[aCntDest++] = cR('=');
        }
        return mA7String;
    }

    // ----------------------------------------------------------------
    // TString::toInteger
    //! \see TString::toInteger
    // ----------------------------------------------------------------
    static jlong toInteger(const SAP_UC *aBuffer) {
        jlong aNumber = 0;
        int   aBase   = 10;
        int   aDigit;

        if (aBuffer == NULL) {
            return 0;
        }
        // remove leading blanks
        while (*aBuffer == cU(' ')) {
            aBuffer ++;
        }

        if ((*(aBuffer+1) == cU('x')  ||
             *(aBuffer+1) == cU('X')) &&
             *(aBuffer+0) == cU('0')) {
                aBase    = 16;
                aBuffer +=  2;
        }

        while (*aBuffer != cU('\0')) {
            aDigit = *aBuffer;
            if (aDigit >= cU('a') && 
                aDigit <= cU('f')) {
                    aNumber *= aBase;
                    aNumber += (aDigit - cU('a') + 10);
            }
            else if 
               (aDigit >= cU('A') &&
                aDigit <= cU('F')) {
                    aNumber *= aBase;
                    aNumber += (aDigit - cU('A') + 10);
            }
            else if 
               (aDigit >= cU('0') &&
                aDigit <= cU('9')) { 
                    aNumber *= aBase;
                    aNumber += (aDigit - cU('0'));
            }
            else if 
               (aDigit != cU('.') &&
                aDigit != cU(',')) {
                    break;
            }
            aBuffer ++;
        }
        return aNumber;
    }
    // ----------------------------------------------------------------
    // TString::reverse
    //! \brief  Changes the sequence of a string
    //! \param  aBuffer The string to revert
    //! \return The pointer to aBuffer
    // ----------------------------------------------------------------
    static SAP_UC *reverse(SAP_UC *aBuffer);
    // ----------------------------------------------------------------
    // TString::insert
    //! \brief Inserts a string at index TString::mInsertPos
    //! \param aInsert The string to insert
    // ----------------------------------------------------------------
    void insert(const SAP_UC *aInsert) {
        int i;
        int aLenInsert = (int)STRLEN(aInsert);
        int aBytes     = 0;

        if (aLenInsert == 0) {
            return;
        }
        if (mString != NULL) {
            aBytes = STRLEN(mString);
        }
        // check if there is enough space for insertion
        if (mBytes < aBytes + aLenInsert + mInsertPos) {
            if (mReference) {
                ERROR_OUT(cU("insert"), mBytes);
                return;
            }
            mBytes = aBytes + aLenInsert;
            SAP_UC *aNewString = new SAP_UC [mBytes + 1];
            memsetR( aNewString, 0, (mBytes + 1) * sizeofR(SAP_UC));

            if (mString != NULL) {
                STRCPY(aNewString, mString, mBytes + 1);
                delete [] mString;
            }
            mString = aNewString;
        }

        // shift the content of the string to the right
        for (i = aBytes; i >= mInsertPos; i--) {
            *(mString + aLenInsert + i) = *(mString + i);
        }
        // insert something
        for (i = 0; i < aLenInsert; i ++) {
            *(mString + i + mInsertPos) = *(aInsert + i);
        }
        mInsertPos += aLenInsert;
    }
    // ----------------------------------------------------------------
    // TString::copy
    //! \brief Copy TString to aBuffer
    //!
    //! \param aBuffer The buffer to write
    //! \param aLen    The number of bytes to copy
    // ----------------------------------------------------------------
    void copy(jchar *aBuffer, jsize aLen) {
        jsize i;
        aBuffer[0] = 0;

        if (mString == NULL) {
            return;
        }
        for (i = 0; i < aLen && i < (jsize)mBytes; i++) {
            aBuffer[i] = mString[i];
            if (mString[i] == 0) {
                break;
            }
        }
    }
    // ----------------------------------------------------------------
    // TString::cut
    //! \brief Removes a sequence from TString
    //! \param aStart Cutting interval start
    //! \param aEnd   Cutting interval end
    // ----------------------------------------------------------------
    void cut(int aStart, int aEnd = -1) {
        int i;

        mInsertPos = 0;
        if (mString == NULL) {
            return;
        }
        if (aEnd == -1) {
            aEnd = (int)STRLEN(mString);
        }

        if (aEnd > mBytes || aEnd < 0) {
            aEnd = mBytes;
        }
        if (aEnd < aStart) {
            mString[0] = 0;
            return;
        }
        if (aStart != 0) {
            for (i = 0; i < aEnd - aStart; i++) {
                mString[i] = mString[aStart + i];
            }
        }
        mString[aEnd - aStart] = 0;
        mInsertPos = (int)STRLEN(mString);
    }
    // ----------------------------------------------------------------
    // TString::concat
    //! \brief Appends a string to TString
    //! \param aStr The string to append
    // ----------------------------------------------------------------
    void concat(const SAP_UC *aStr) {
        int aStrLen = 0;
        int aNeeded = 0;
        int aCpyLen = 0;
        SAP_UC *aTmpStr;

        if (aStr == NULL) {
            return;
        }
        
        if (mString == NULL) {
            mBytes  = max(128, (int)STRLEN(aStr) + 1);
            mString = new SAP_UC[mBytes + 1];
            memsetR(mString, 0, (mBytes + 1) * sizeofR(SAP_UC));
            STRCPY(mString, aStr, mBytes);
            mInsertPos = (int)STRLEN(aStr);
            return;
        }
        aNeeded = max(128, STRLEN(aStr) + STRLEN(mString) + 1);
        
        if (aNeeded >= mBytes) {
            if (mReference) {
                ERROR_OUT(cU("concat"), mBytes);
                return;
            }
            aTmpStr     = mString;
            mBytes      = aNeeded + 1;
            mString     = new SAP_UC[mBytes + 1];
            memsetR(mString, 0, (mBytes + 1) * sizeofR(SAP_UC));
            STRCPY(mString, aTmpStr, STRLEN(aTmpStr)+1);
            delete[] aTmpStr;
        }
        STRCAT(mString, aStr, mBytes + 1);
        mInsertPos = STRLEN(mString);
    }
    // ----------------------------------------------------------------
    // TString::concat
    //! \brief Appends a character to TString
    //! \param aChar The character to append
    // ----------------------------------------------------------------
    void concat(const SAP_UC aChar) {
        SAP_UC xChar[2];
        xChar[0] = aChar;
        xChar[1] = cU('\0');
        concat(xChar);
    }
    // ----------------------------------------------------------------
    // TString::concatPathExt
    //! \brief Extends a file path. 
    //!
    //! This method ensures, that the new path segment is separated by FILESEPARATOR.
    //! \param aPathExt The new path segment.
    // ----------------------------------------------------------------
    void concatPathExt(const SAP_UC *aPathExt) {
        int aLen = 0;
        if (mString != NULL) {
            aLen = (int)STRLEN(mString);
            if (mString[aLen - 1] != FILESEPARATOR) {
                concat(FILESEPARATOR);
            }
        }
        concat(aPathExt);
    }
    // ----------------------------------------------------------------
    // TString::checkPath
    //! \brief Converts different path representations.
    //!
    //! File separator differs on Unix and Windows OS. The values in
    //! the configuration are converted to the current platform
    // ----------------------------------------------------------------
    void checkPath() {
        if (mString == NULL) {
            return;
        }
        size_t aLen    = STRLEN(mString);
        size_t i;
        for (i = 0; i < aLen; i++) {
            if (mString[i] == cU('\\') || mString[i] == cU('/')) {
                mString[i] = FILESEPARATOR;
            }
        }
    }
    // ----------------------------------------------------------------
    // TString::str
    //! \brief  Access to a C-string representation
    //! \return Pointer to the TString::mString
    // ----------------------------------------------------------------
    const SAP_UC *str() {
        if (mString == NULL) {
            return cU("");
        }
        else {
            return mString;
        }
    }
    // ----------------------------------------------------------------
    // TString::a7_str
    //! \brief  Access to a ASCII C-string representation
    //!
    //! \return Pointer to the TString::mA7String
    // ----------------------------------------------------------------
    const SAP_A7 *a7_str(bool aCopy = false) {        
        SAP_A7 *aTmpStr  = NULL;
        int     aTmpSize = mBytes;

        if (mString == NULL) {
            return cR("");
        }

        if (mA7String != NULL) {
            delete [] mA7String;
        }

        aTmpStr = new SAP_A7[aTmpSize + 2];

        if (!aCopy) {
            mA7String = aTmpStr;
        }
        
        for (int i = 0; i < mBytes; i++) {
            /*SAPUNICODEOK_CAST*/
            aTmpStr[i] = (SAP_A7)mString[i];
            if (mString[i] == cR('\0')) {
                break;
            }
        }
        aTmpStr[mBytes] = cR('\0');
        return aTmpStr;
    }
    // ----------------------------------------------------------------
    // TString::assign
    //! \brief Assigns a character sequence to TString
    //!
    //! If TString is UNICODE, the assignment makes a conversion. 
    //! \param aBuffer The character sequence to copy
    //! \param aLen    The number of bytes to copy
    // ----------------------------------------------------------------
    void assign(jchar *aBuffer, int aLen) {
        jsize i;
        if (mBytes < aLen) {
            if (mReference) {
                ERROR_OUT(cU("assign"), mBytes);
                return;
            }
            if (mString != NULL) {
                delete [] mString;
            }
            mBytes  = aLen;
            mString = new SAP_UC [mBytes + 1];
            memsetR(mString, 0, (mBytes + 1) * sizeofR(SAP_UC));
        }

        for (i = 0; i < aLen && i < (jsize)mBytes; i++) {
            mString[i] = (SAP_UC)aBuffer[i];
            if (mString[i] == 0) {
                break;
            }
        }
        mString[aLen] = cU('\0');
        mInsertPos    = STRLEN(mString);
    }
    // ----------------------------------------------------------------
    // TString::assign
    //! \brief Assigns a Java string object to TString
    //! \param  jEnv    The JNI environment
    //! \param  jString The Java string object
    // ----------------------------------------------------------------
    void assign(JNIEnv *jEnv, jstring jString) {
        jchar  *jBuffer;
        jint    jLen;

        if (jString == NULL) {
            return;
        }

        jLen = jEnv->GetStringLength(jString);
        if (jLen == 0) {
            return;
        }
        mBytes  = jLen + 1;
        jBuffer = new jchar [mBytes];
        mString = new SAP_UC[mBytes + 1];
        memsetR(mString, 0,  (mBytes + 1) * sizeofR(SAP_UC));

        jEnv->GetStringRegion(jString, 0, jLen, jBuffer); 
        assign(jBuffer, jLen);
        delete [] jBuffer;

        mInsertPos = STRLEN(mString);
    }

    // ----------------------------------------------------------------
    // ----------------------------------------------------------------
    void assignBytes(const jbyte *aBuffer, size_t aLen) {
        SAP_ostringstream aStrStream;
        mInsertPos = 0;

        if (aBuffer == NULL || aLen == 0) {
            if (mString != NULL) {
                mString[0] = cU('\0');
            }
            return;
        }

        for (int i = 0; i < aLen; i++) {
            aStrStream << cU("\\x");
            aStrStream << hex << setfill(cU('0')) << setw(2) << (aBuffer[i] & 0xff);
        }
        aStrStream << ends;
        aStrStream.seekp(0, ios::end);
        aLen = aStrStream.tellp();

        if (mBytes < aLen || mString == NULL) {
            if (mString != NULL) {
                delete[] mString;
            }
            mBytes  = (int)max(64, (aLen + 1));
            mString = new SAP_UC[mBytes + 1];
            memsetR(mString, 0,  (mBytes + 1) * sizeofR(SAP_UC));
        }

        STRNCPY(mString, aStrStream.str().c_str(), aLen, mBytes + 1);

        mInsertPos = STRLEN(mString);
    }
    // ----------------------------------------------------------------
    // TString::assignR
    //! \brief Assigns a character sequence to TString
    //! \see TString::assign(jchar *, int)
    // ----------------------------------------------------------------
    void assignR(/*SAPUNICODEOK_CHARTYPE*/
            const char *aBuffer, 
            size_t      aLen) {
        
        if (aBuffer == NULL || aLen == 0) {
            if (mString != NULL) {
                mString[0] = 0;
            }
            return;
        }

        if (mReference && (size_t)mBytes < aLen) {
            ERROR_OUT(cU("assign"), mBytes);
            return;
        }

        if ((size_t)mBytes < aLen) {
            if (mString != NULL) {
                delete [] mString;
            }
            mBytes  = (int)max((int)32, (int)(aLen + 1));
            mString = new SAP_UC [mBytes + 1];
            memsetR(mString, 0,   (mBytes + 1) * sizeofR(SAP_UC));;
        }

        for (size_t i = 0; i < aLen; i++) {
            mString[i] = (SAP_UC)aBuffer[i];
        }

        if (mString != NULL) {
            mString[aLen] = 0;
        }

        mInsertPos = STRLEN(mString);
    }
    // ----------------------------------------------------------------
    // TString::reset
    //! \brief  Editor function
    //!
    //! Resets TString::mInsertPos for editor
    // ----------------------------------------------------------------
    void reset() {
        if (mString == NULL) {
            mInsertPos = 0;
        }
        else {
            mInsertPos = (int)STRLEN(mString);
        }
    }

    // ----------------------------------------------------------------
    // TString::strInsert
    //! \brief  Editor function
    //! \return The string at the insert position
    // ----------------------------------------------------------------
    const SAP_UC *strInsert() {
        if (mString == NULL) {
            return cU("");
        }
        else {
            return mString + mInsertPos;
        }
    }
    // ----------------------------------------------------------------
    // TString::getInsertPos
    //! \brief  Editor function
    //! \return The insert position index
    // ----------------------------------------------------------------
    iterator getInsertPos() {
        if (mString == NULL) {
            return 0;
        }
        else {
            return mInsertPos;
        }
    }
    // ----------------------------------------------------------------
    // TString::getHash
    //! \brief  Hash for string maintenance.
    //! \return The hansh code of a TString.
    // ----------------------------------------------------------------
    unsigned long getHash() {
        unsigned long aHash = 0;
        if (mString == NULL) {
            return 0;
        }
        SAP_UC *aPtr = mString;
        while (*(++aPtr) != 0) {
            aHash *= 31;
            aHash += *aPtr;
        }
        return labs(aHash);
    }
};

// ----------------------------------------------------------------
//! \class TEditBuffer
//! \brief Command line editor
//!
//! TEditBuffer is a ring buffer with methods for navigation within
//! the input history.
// ----------------------------------------------------------------
class TEditBuffer: public TRing <TString> {
public:
    // ----------------------------------------------------------------
    // TEditBuffer::TEditBuffer
    //! \brief Constructor
    //! \param aSize The size of the history buffer
    // ----------------------------------------------------------------
    TEditBuffer(int aSize): TRing <class TString> (aSize) {
    }
    // ----------------------------------------------------------------
    // TEditBuffer::up
    //! \brief  Previous item in history
    //! \return The previous command (roll overflow)
    // ----------------------------------------------------------------
    iterator up() {
        int aCurrentPos = 0;

        if (mNrElements > 0) {
            mCursorRead = (mNrElements + mCursorRead - 1) % mNrElements;
            aCurrentPos = (mSize + mCursorWrite - mNrElements + mCursorRead) % mSize;
        }
        else {
            aCurrentPos = mCursorWrite;
            mCursorRead = 0;
        }
        iterator aPtr = mVector[aCurrentPos];
        return aPtr;
    }
    // ----------------------------------------------------------------
    // TEditBuffer
    //! \brief  Next item in history
    //! \return The next command (roll overflow)
    // ----------------------------------------------------------------
    iterator down() {
        int aCurrentPos  = 0;

        if (mNrElements > 0) {
            mCursorRead = (mCursorRead + 1) % mNrElements;
            aCurrentPos = (mSize + mCursorWrite - mNrElements + mCursorRead) % mSize;
        }
        else {
            aCurrentPos = mCursorWrite;
            mCursorRead = 0;
        }
        iterator aPtr = mVector[aCurrentPos];
        return aPtr;
    }
};
// ----------------------------------------------------------------
//! \class TList
//! \brief dynamic list
//!
// ----------------------------------------------------------------
template <class _Ty> class TList {
private:
    // ------------------------------------------------------------
    // ! List element
    // ------------------------------------------------------------
    typedef struct SElement {
        //! Contrucor
        SElement() { 
            mVisited = 0; 
            mIndex   = 0;
            mType    = 0;
            mElement = NULL;
            mPrev    = NULL;
            mNext    = NULL;
        }
        SElement *mPrev;    //!< Link to previous element
        SElement *mNext;    //!< Link to next element
        _Ty       mElement; //!< Pointer to element
        int       mVisited; //!< Indicator for list walk and sort
        int       mIndex;   //!< Node weight
        int       mType;    //!< Type information
    } TElement;
public:
    //! Iterator
    typedef TElement *iterator;
private:
    TElement  *mCurrent;    //!< Current element in walk sequence
    TElement  *mRoot;       //!< Root element
    TElement  *mEnd;        //!< End element behind last element
    jlong      mSize;       //!< Number of elements
    jlong      mMaxSize;    //!< Maximal allowed number of elements
public:
    // ----------------------------------------------------------------
    // TList::TList
    //! Constructor
    // ----------------------------------------------------------------
    TList() {
        mEnd         = new TElement();
        mRoot        = new TElement();

        mRoot->mPrev = NULL;
        mRoot->mNext = mEnd;

        mEnd->mPrev  = mRoot;
        mEnd->mNext  = NULL;
        mSize        = 0;
        mMaxSize     = 100000;
        mCurrent     = mRoot;
    }
    // ----------------------------------------------------------------
    // TList::~TList
    //! Destructor
    // ----------------------------------------------------------------
    ~TList() {
        TElement *aNode = mRoot;
        TElement *aTemp;

        while (aNode != mEnd) {
            aTemp = aNode;
            aNode = aNode->mNext;
            delete aTemp;
        }
        delete mEnd;
        mRoot = NULL;
        mEnd  = NULL;
    }
    // ----------------------------------------------------------------
    // TList::push_back
    //! Append a list element
    //! \param aElement The entry to insert at the end of the list
    // ----------------------------------------------------------------
    iterator push_back(_Ty aElement) {
        if (mRoot == NULL) {
            return NULL;
        }
        mSize ++;
        TElement *aCurrent = mEnd;
        aCurrent->mElement = aElement;

        mEnd               = new TElement();
        mEnd->mPrev        = aCurrent;
        mEnd->mNext        = NULL;
        aCurrent->mNext    = mEnd;
        return aCurrent;
    }
    // ----------------------------------------------------------------
    // TList::insertSorted
    //! Insert in lexicographical order
    //! \param aElement the entry to insert in a sorted list
    // ----------------------------------------------------------------
    void insertSorted(_Ty aElement) {
        TElement *aPtr;
        TElement *aNew;

        for (aPtr  = mRoot->mNext;
             aPtr != mEnd;
             aPtr  = aPtr->mNext) {

            if (aPtr->mElement->compare(aElement) < 0) {
                if (mSize < mMaxSize) {   
                    aNew           = new TElement;
                    aNew->mPrev    = aPtr;
                    aNew->mNext    = aPtr->mNext;
                    aNew->mElement = aElement;
                    aPtr->mNext    = aNew;
                    mSize ++;
                }
                return;
            }
        }
        
        if (mSize < mMaxSize) {
            push_back(aElement);
        }
        else {
            mEnd->mPrev->mElement = aElement;
        }
    }
    // ----------------------------------------------------------------
    // TList::find
    //! Search a node element
    //! \param  aElement The element to search for
    //! \return Iterator with the given element or 
    //!         TList::end if aElement is not in TList
    // ----------------------------------------------------------------
    iterator find(_Ty aElement) {
        TElement *aPtr;

        if (mRoot == NULL) {
            return NULL;
        }
        for (aPtr  = mRoot->mNext;
             aPtr != mEnd;
             aPtr  = aPtr->mNext) {

            if (aPtr->mElement == aElement) {
                return aPtr;
            }
        }
        return end();
    }
    // ----------------------------------------------------------------
    // TList::seekPos
    //! \brief  Positioning of the read cursor
    //! \param  aSeekPos Relative position within a list, could be negative
    //! \return The iterator to the new read position
    // ----------------------------------------------------------------
    iterator seekPos(int aSeekPos) {
        int i;

        if (aSeekPos == 0) {
            return mCurrent;
        }
        else if (aSeekPos > 0) {
            for (i = 0; i < aSeekPos && mCurrent != mEnd; i++) {
                mCurrent = mCurrent->mNext;
            }
        }
        else {
            aSeekPos = -aSeekPos;
            if (mCurrent == mRoot->mNext) {
                mCurrent = mEnd;
            }
            for (i = 0; i < aSeekPos && mCurrent != mRoot->mNext; i++) {
                mCurrent = mCurrent->mPrev;
            }
        }
        return mCurrent;
    }
    // ----------------------------------------------------------------
    // TList::setPos
    //! \brief  Positioning of the read cursor
    //! \param  aSetPos Absolute position within a list
    //! \return Iterator to the new read cursor
    // ----------------------------------------------------------------
    iterator setPos(int aSetPos) {
        if (aSetPos == 0) {
            mCurrent = mRoot->mNext;
        }
        else if (aSetPos > 0) {
            mCurrent = mRoot->mNext;
            mCurrent = seekPos(aSetPos);    
        }
        else {
            mCurrent = mEnd;
            mCurrent = seekPos(aSetPos);  
        }
        return mCurrent;
    }
    // ----------------------------------------------------------------
    // TList::remove
    //! \brief  Removes given list node
    //! \param  aPtr  The iterator to be deleted
    //! \return Iterator after the deleted node or 
    //!         TList::end if TList is empty
    // ----------------------------------------------------------------
    iterator remove(iterator aPtr) {
        iterator aTmp;
        iterator aDel;

        if (aPtr == mEnd || aPtr == NULL || aPtr == mRoot) {
            return mEnd;
        }
        
        for (aTmp  = mRoot;
             aTmp != mEnd;
             aTmp  = aTmp->mNext) {

            if (aTmp == aPtr) {
                aDel     = aTmp;
                mCurrent = aTmp->mNext;
                aDel->mNext->mPrev = aDel->mPrev;
                aDel->mPrev->mNext = aDel->mNext;
                delete aDel;
                mSize --;
                return mCurrent;
            }
        }
        return mEnd;
    }
    // ----------------------------------------------------------------
    // TList::begin
    //! Start iterator
    //! \return Iterator at the first element or 
    //!         TList::end if TList is empty
    // ----------------------------------------------------------------
    inline iterator begin() {
        mCurrent = mRoot->mNext;
        return mCurrent;
    }
    // ----------------------------------------------------------------
    // TList::current
    //! Current iterator
    //! \return Iterator at the current read position 
    // ----------------------------------------------------------------
    iterator current() {
        return mCurrent;
    }
    // ----------------------------------------------------------------
    // TList::next
    //! Next iterator
    //! \return Iterator after the current read position
    // ----------------------------------------------------------------
    inline iterator next() {
        if (mCurrent != mEnd) {
            mCurrent  = mCurrent->mNext;
        }
        return mCurrent;
    }
    // ----------------------------------------------------------------
    // TList::end
    //! End iterator
    //! \return Iterator to the end of TList
    // ----------------------------------------------------------------
    inline iterator end() {
        return mEnd;
    }
    // ----------------------------------------------------------------
    // TList::last
    //! End iterator
    //! \return Iterator to the last element
    // ----------------------------------------------------------------
    inline iterator last() {
        return mEnd->mPrev;
    }
    // ----------------------------------------------------------------
    // TList::getSize
    //! \return The number of elements in TList
    // ----------------------------------------------------------------
    jlong getSize() {
        return mSize;
    }
    // ----------------------------------------------------------------
    // TList::empty
    //! \return \c TRUE if the list is empty
    // ----------------------------------------------------------------
    bool empty() {
        return (mSize == 0);
    }
    // ----------------------------------------------------------------
    // TList::clear
    //! \brief removes all elements from TList
    // ----------------------------------------------------------------
    void clear() {
        mSize = 0;
        iterator aTmp;
        iterator aPtr = mRoot->mNext;

        while (aPtr != mEnd) {
            aTmp = aPtr;
            aPtr = aPtr->mNext;
            delete aTmp;
        }
        mRoot->mNext = mEnd;
        mEnd->mPrev  = mRoot;
    }
    // -------------------------------------------------------------
    // TList::reset
    //! \see TList::clear
    // -------------------------------------------------------------
    void reset() {
        clear();
    }
    // -------------------------------------------------------------
    // TList::qsort
    //! \brief  Sort list by elements using QSort
    //!
    //! Advantages of the qsort:
    //! - qsort is of linar order (10*n), while bubble sort is (n*n)
    //! - evaluate compare attribute A only once per cycle
    //! - equal values are only visited once
    //! \param aSortAttr The sort attribute allows to select a table 
    //!                  column for elements of type TProperty
    // -------------------------------------------------------------
    void qsort(int aSortAttr) {
        static int aInxVisit = 1;
        iterator   aPtrTemp;
        iterator   aPtrFirst;
        iterator   aPtrNext;
        iterator   aPtrCurr;
        iterator   aPtrComp;
        _Ty        aElementA;
        _Ty        aElementB;
        int        aCompare = 0;
        
        if (empty()) {
            return;
        }
        // quick sort for attribute
        aPtrFirst = begin();
        aPtrComp  = aPtrFirst;
        aInxVisit = (aInxVisit % 8) + 1;
        // Iterate starting at begin of the list
        while (aPtrComp != end()) {
            // find the starting point
            aPtrFirst = begin();
            while (aPtrFirst != end() && 
                   aPtrFirst->mVisited == aInxVisit) {
                aPtrFirst = aPtrFirst->mNext;
            }
            aPtrCurr = aPtrFirst;
            aPtrComp = aPtrCurr;
            if (aPtrCurr != end()) {
                aPtrCurr->mVisited = aInxVisit;
                aPtrCurr  = aPtrCurr->mNext;
            }            
            aElementA = aPtrComp->mElement;
            // compare + sort any entry, which was not yet visited
            while (aPtrCurr != end() && 
                   aPtrCurr->mVisited != aInxVisit) {
                aPtrNext  = aPtrCurr->mNext;
                aElementB = aPtrCurr->mElement;
                aCompare  = aElementA->compare(aElementB, aSortAttr);
                if (aCompare < 0) {
                    // move left to aPtrComp
                    aPtrTemp               = aPtrComp->mPrev;
                    aPtrCurr->mPrev->mNext = aPtrCurr->mNext;                    
                    aPtrCurr->mNext->mPrev = aPtrCurr->mPrev;
                    aPtrComp->mPrev        = aPtrCurr;
                    aPtrCurr->mNext        = aPtrComp;
                    aPtrCurr->mPrev        = aPtrTemp;
                    aPtrTemp->mNext        = aPtrCurr;
                }
                else if (aCompare == 0) {
                    // move right to aPtrComp
                    aPtrCurr->mVisited = aInxVisit;
                    if (aPtrCurr != aPtrComp->mNext) {
                        aPtrTemp               = aPtrComp->mNext;
                        aPtrCurr->mPrev->mNext = aPtrCurr->mNext;                    
                        aPtrCurr->mNext->mPrev = aPtrCurr->mPrev;
                        aPtrComp->mNext        = aPtrCurr;
                        aPtrCurr->mPrev        = aPtrComp;
                        aPtrCurr->mNext        = aPtrTemp;
                        aPtrTemp->mPrev        = aPtrCurr;
                    }
                }
                aPtrCurr = aPtrNext;
            }
        }
    }
};

// -----------------------------------------------------------------
static const unsigned gHashValue = 131071;
// -----------------------------------------------------------------
//! \class THashObj
//! \brief Entries for THash has to be derived from this class
// -----------------------------------------------------------------
class THashObj {
    jlong   mTag;
    jclass  mClass;

public:
    THashObj() {
        mTag   = 0;
        mClass = NULL;
    }
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    virtual ~THashObj() {
    }
    // -----------------------------------------------------------------
    //! Memory maintenance
    //! \param aSize Number of bytes to deallocate
    // -----------------------------------------------------------------
    virtual void deallocate(jlong aSize) {
    }
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    void setTag(jlong aTag) {
        mTag = aTag;
    }
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    jlong getTag() {
        return mTag;
    }
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    void setClass(jclass jClass) {
        mClass = jClass;
    }
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    jclass getClass() {
        return mClass;
    }
};
// -----------------------------------------------------------------
#define HASH_FIND        1      //!< Hash operation: find
#define HASH_FIND_INSERT 2      //!< Hash operation: insert
#define HASH_FIND_REMOVE 4      //!< Hash operation: remove
// -----------------------------------------------------------------
//! \class THash
//! \brief Hash table
//!
//! The hash table is a vector of THashEntry as object reference.
//! At collision a fixed offset is used to find the next free entry.
//! A size of zero indicates a free entry in the hash table.
// -----------------------------------------------------------------
template <class _TKey, class _TValue, class _TObject = jint> class THash {
protected:
#define OFFSET    7 //!< Hash collisin offset
#define SEED   8191 //!< Seed for ramdom access
    // -----------------------------------------------------------------
    //! Hash table entry
    // -----------------------------------------------------------------
    typedef struct SHashEntry {
    public:
        _TKey     aKey;         //!< Hash key
        _TValue   aValue;       //!< Hash value
        _TValue   aRef;         //!< Reference: Link to other elements
        size_t    aSize;        //!< Virtual size: Elements can be inserted with a size > 0
        _TObject  aArena;       //!< Grouping information
        unsigned short aType;   //!< Type 
        unsigned short aCnt;    //!< Number of elements

        // -----------------------------------------------------------------
        //! \param aEntry The entry to copy
        // -----------------------------------------------------------------
        void copy(SHashEntry *aEntry) {
            aKey   = aEntry->aKey;
            aValue = aEntry->aValue;
            aRef   = aEntry->aRef;
            aSize  = aEntry->aSize;
            aArena = aEntry->aArena;
            aType  = aEntry->aType;
        }
    } THashEntry;

public:
    //! Hash iterator
    typedef THashEntry *iterator;
protected:
    iterator      mHashTable;       //!< Iterator to hash table
    size_t        mMaxSize;         //!< Maximal size for variable sized table
    size_t        mSize;            //!< Cummulated virtual size
    size_t        mBasicSize;       //!< Minimal size for variable sized table
    jlong         mEntries;         //!< Number of entries
    jlong         mNrCollisions;    //!< Statistic data 
    jlong         mNrCalls;         //!< Statistic data 
    bool          mResizeable;      //!< THash size can be static or dynamic
    bool          mSetResize;       //!< Intermediate value for reset operation
    bool          mDoErrorOut;      //!< Allow error output
    jlong         mStateCollision;  //!< Error in data maintenance
    jlong         mCursorRead;      //!< Iterator read cursor
    // -------------------------------------------------------------
    // THash::findAction
    //! \brief Find/Insert/Remove an entry
    //!
    //! The remove operation includes a compact for the search chain
    //! \param aKey    The key of the object to find/insert
    //! \param aAction One of 
    //! - HASH_FIND
    //! - HASH_FIND_INSERT
    //! - HASH_FIND_REMOVE
    // -------------------------------------------------------------
    virtual iterator findAction(_TKey aKey, int aAction = HASH_FIND) {
        iterator   aEntry;
        iterator   aNextEntry;
        iterator   aEntryInsert;
        jlong i;
        jlong aChain     = 0;
        jlong aIndex     = 0;
        jlong aIndexNext = 0;

        mNrCalls++;

        if (aKey == (_TKey)0) {
            return end();
        }        
        aIndex       = (unsigned long)(aKey) % (unsigned long)mMaxSize;
        aEntryInsert = end();
        aEntry       = end();

        do {
            aEntry = &mHashTable[aIndex + 1];

            if (aEntryInsert  == end()            &&
                aEntry->aSize <= 0                &&
                aAction       == HASH_FIND_INSERT) {
                aEntryInsert = aEntry;
            } 
            
            // no entry for hash key: break
            if (aEntry->aKey == 0) {
                aEntry->aSize = 0;
                if (aAction == HASH_FIND ||
                    aAction == HASH_FIND_REMOVE) {
                    aEntry = end();
                    break;
                }
                
                if (aAction == HASH_FIND_INSERT) {
                    aEntry = aEntryInsert;
                    break;
                }
                break;
            }

            // found entry for hash key: return element
            if (aEntry->aKey == aKey && aEntry->aSize > 0) {
                if (aAction == HASH_FIND ||
                    aAction == HASH_FIND_INSERT) {
                    break;
                }
                // store deleted element in position 0
                mHashTable[0].copy(aEntry);
                // recalculate hash
                aEntry->aSize = 0;
                // Remove empty chain elements:
                // We have to keep deleted keys unless there are valid elements in chain
                // in any other case, we would loose following entries. 
                // For optimizaiton, condition (key == 0) would terminate the search
                // After deleting an entry, the following algorithm checks, if there
                // predecessors in the chain, which could be removed as well (size==0).
                aIndexNext = (aIndex + OFFSET) % mMaxSize;
                aNextEntry = &mHashTable[aIndexNext + 1];
                if (aNextEntry->aKey == 0) {
                    aEntry->aKey = 0;
                    for (i = 0; i < aChain; i++) {
                        aIndexNext = (mMaxSize + aIndexNext - OFFSET) % mMaxSize;
                        aNextEntry = &mHashTable[aIndexNext + 1];
                        if (aNextEntry->aSize != 0) {
                            break;
                        }
                        aNextEntry->aKey = 0;
                    }
                }
                aEntry = &mHashTable[0];
                break;
            }            
            aIndex = (aIndex + OFFSET) % mMaxSize;
            mNrCollisions++;
            aChain       ++;
        } while ((size_t)aChain < mMaxSize);

        if ((size_t)aChain >= mMaxSize) {
            ERROR_OUT(cU("hash collision"), 1 + aAction);
            rehash(mMaxSize + mBasicSize);
            if (mStateCollision == 0) {
                return findAction(aKey, aAction);
            }
            mStateCollision++;
        }
        return aEntry;
    }
public:
    // -------------------------------------------------------------
    // THash::THash
    //! \brief Constructor.
    //! \param aMaxSize    The initial size of the hash table
    //! \param aResizeable \c TRUE if the hash table has variable size
    // -------------------------------------------------------------
    THash(size_t aMaxSize    = gHashValue, 
          bool   aResizeable = true) {

        mMaxSize        = aMaxSize;
        mBasicSize      = aMaxSize;
        mEntries        = 0;
        mSize           = 0;
        mNrCollisions   = 0;
        mNrCalls        = 0;
        mCursorRead     = 0;
        mStateCollision = 0;
        mNrCollisions   = 0;
        mResizeable     = aResizeable;
        mSetResize      = aResizeable;
        mDoErrorOut     = true;

        mHashTable = new THashEntry[mMaxSize + 2];
        memsetR(mHashTable, 0, (size_t)(sizeofR(THashEntry) * (mMaxSize + 2)));
    }
    // -------------------------------------------------------------
    // THash::~THash
    //! Destructor
    // -------------------------------------------------------------
    virtual ~THash() {
        delete [] mHashTable;
    }
    // -------------------------------------------------------------
    // THash::move
    //! \brief Changing hash key
    //! \param aOldKey   The old key value
    //! \param aNewKey   The new key value
    //! \param aNewArena The new arena
    // -------------------------------------------------------------
    virtual iterator move(
            _TKey    aOldKey, 
            _TKey    aNewKey,
            _TObject aNewArena) {

        iterator  aEntryOld;
        iterator  aEntryNew;

        if (aOldKey != aNewKey) {
            aEntryOld = remove(aOldKey);
            if (aEntryOld == end()) {
                return end();
            }            
            mResizeable = false;
            aEntryNew   = insert(aNewKey, aEntryOld->aValue, aNewArena, aEntryOld->aRef, aEntryOld->aSize);
            mResizeable = mSetResize;
        }
        else {
            aEntryNew = find(aNewKey);
        }
        
        return aEntryNew;
    }
    // -------------------------------------------------------------
    // THash::insert
    //! \brief  Insert a new value
    //! \param  aEntry The entry to insert
    //! \return The iterator at the new entry
    // -------------------------------------------------------------
    virtual iterator insert(iterator aEntry) {
        iterator aNewEntry = insert(
                    aEntry->aKey, 
                    aEntry->aValue, 
                    aEntry->aArena, 
                    aEntry->aRef, 
                    aEntry->aSize);
        return aNewEntry;
    }
    // -------------------------------------------------------------
    // THash::insert
    //! \brief Insert a new value
    //! \param aKey   The key of the object to insert
    //! \param aValue The object to insert
    //! \param aArena Grouping criterium for elements
    //! \param aRef   Reference to other elements
    //! \param aSize  Virtual size of an element
    //! \return The iterator for the new entry
    // -------------------------------------------------------------
    virtual iterator insert(
            _TKey     aKey, 
            _TValue   aValue, 
            _TObject  aArena = 0,
            _TValue   aRef   = NULL,
            size_t    aSize  = 1) {
        
        iterator aEntry;

        if (aKey == (_TKey)0 || aSize == 0) {
            return end();
        }
        if (mResizeable && mEntries > (jlong)((3 * mMaxSize) / 4)) {
            rehash(mMaxSize + mBasicSize);
        }
        aEntry = findAction(aKey, HASH_FIND_INSERT);        
        if (aEntry == end() || mStateCollision > 0) {
            //ASSERT(aEntry != end());
            return end();
        }
        else if (aEntry->aSize > 0) {
            //ASSERT(aEntry->aKey != aKey);
            return end();
        }
        mEntries ++;
        aEntry->aKey   = aKey;
        aEntry->aValue = aValue;
        aEntry->aRef   = aRef;
        aEntry->aSize  = aSize;
        aEntry->aArena = aArena;

        mSize += aSize;
        return aEntry;
    }
    // -------------------------------------------------------------
    // THash::find
    //! \brief Insert an element and return the iterator.
    //! \see THash::insert
    // -------------------------------------------------------------
    virtual iterator findInsert(
            _TKey     aKey, 
            _TValue   aValue, 
            _TObject  aArena = 0,
            _TValue   aRef   = NULL,
            size_t    aSize  = 1) {
        
        iterator aEntry   = NULL;

        if ((jlong)aKey == 0 || aSize == 0) {
            return end();
        }
        if (mResizeable && mEntries > (jlong)((3 * mMaxSize) / 4)) {
            rehash(mMaxSize + mBasicSize);
        }
        aEntry = findAction(aKey, HASH_FIND_INSERT);
        if (aEntry == end() || mStateCollision > 0) {
            // Element could not be inserted
            ASSERT(aEntry != end());
            return end();
        }
        if (aEntry->aSize <= 0) {
            mEntries ++;
            aEntry->aKey   = aKey;
            aEntry->aValue = aValue;
            aEntry->aRef   = aRef;
            aEntry->aSize  = aSize;
            aEntry->aArena = aArena;

            mSize += aSize;
        }
        return aEntry;
    }
    // -------------------------------------------------------------
    // THash::find
    //! \brief Find an element using the hash key
    //! \param aKey The key of the element
    //! \return The iterator for the entry
    // -------------------------------------------------------------
    virtual iterator find(_TKey aKey) {
        return findAction(aKey, HASH_FIND);
    }
    // -------------------------------------------------------------
    // THash::remove
    //! \brief Remove an element with the specified hash key
    //! \param aKey The key of the element
    //! \return The iterator for the deleted entry
    // -------------------------------------------------------------
    virtual iterator remove(_TKey aKey) {
        iterator aEntry;
        aEntry = findAction(aKey, HASH_FIND_REMOVE);
        
        if (aEntry != end()) {
            if (mSize < aEntry->aSize || mEntries == 0)
                ERROR_OUT(cU("Hash corrupted"), mSize);
            mEntries --;
            mSize    -= aEntry->aSize;
        }
        return aEntry;
    }
    // -------------------------------------------------------------
    // THash::rehash
    //! \brief Resize the hash table
    //! \param aNewSize The new size
    // -------------------------------------------------------------
    virtual void rehash(size_t aNewSize) {
        if (!mResizeable) {
            return;
        }
        jlong i;
        iterator      aEntry;
        THashEntry   *aOldHashTable;
        THashEntry   *aNewHashTable;
        jlong         aOldHashSize = mMaxSize;

#ifdef DEBUG
        ERROR_OUT(cU("THash::rehash"), aNewSize);
#endif
        aOldHashTable = mHashTable;
        aNewHashTable = new THashEntry[aNewSize + 2];
        if (aNewHashTable != NULL) {
            memsetR(aNewHashTable, 0, (size_t)(sizeofR(THashEntry) * (aNewSize + 2)));
        }
        else {
            ERROR_OUT(cU("THash::rehash: unable to allocate memory"), (int)aNewSize);
            mStateCollision ++;
            return;
        }
        mMaxSize        = aNewSize;
        mHashTable      = aNewHashTable;
        mEntries        = 0;
        mSize           = 0;
        mStateCollision = 0;
        mNrCollisions   = 0;
        mCursorRead     = 0;

        if (aNewSize > 0) {
            for (i = 1; i <= aOldHashSize; i++) {
                aEntry = &aOldHashTable[i];
                if (aEntry->aSize > 0) {
                    insert(aEntry);
                }
            }
        }
        delete [] aOldHashTable;
    }
    // -------------------------------------------------------------
    // THash::reset
    //! \brief Reset removes all entries.
    //!
    // -------------------------------------------------------------
    virtual void reset() {
        THashEntry  *aOldHashTable = mHashTable;
        iterator     aEntry;
        size_t       i;

        for (i = 1; i <= mMaxSize; i++) {
            aEntry = &mHashTable[i];
            if (aEntry->aSize != 0) {                
                if (aEntry->aValue != NULL) {
                    aEntry->aValue->deallocate(aEntry->aSize);
                }
                mSize -= aEntry->aSize;
                mEntries--;
                aEntry->aSize  = 0;
            }
        }
        mEntries        = 0;
        mSize           = 0;
        mResizeable     = mSetResize;
        mStateCollision = 0;

        if (mMaxSize > mBasicSize) {
            mMaxSize = mBasicSize;
            aOldHashTable = mHashTable;
            mHashTable    = new THashEntry[mMaxSize + 2];
            delete [] aOldHashTable;
        }
        check(true);
        memsetR(mHashTable, 0, (size_t)(sizeofR(THashEntry) * (mMaxSize + 2)));
    }
    // -------------------------------------------------------------
    // THash:::deleteArena
    //! \brief Deletes all entries within the same arena.
    //! \param aArena The arena to delete.
    // -------------------------------------------------------------
    virtual void deleteArena(_TObject aArena) {
        iterator  aEntry;
        size_t    i;

        for (i = 1; i <= mMaxSize; i++) {
            aEntry = &mHashTable[i];
            if (aEntry->aArena == aArena && aEntry->aSize != 0) {
                
                if (aEntry->aValue != NULL) {
                    aEntry->aValue->deallocate(aEntry->aSize);
                }
                mSize -= aEntry->aSize;
                mEntries--;
                aEntry->aSize  = 0;
            }
        }
        checkSize();
    }
    // -------------------------------------------------------------
    // THash::getSize
    //! \brief  Number of entries.
    //! \return The number of elements within the hash table.
    // -------------------------------------------------------------
    jlong getSize() {
        if (mEntries < 0 && mStateCollision == 0) {
            mStateCollision ++;
            ERROR_OUT(cU("hash corrupt"), mSize);
            ASSERT(false);
        }
        return mEntries;
    }
    // -------------------------------------------------------------
    // THash::getVolumn
    //! \brief  Virtual size.
    //! \return The cummulated virtual size of all elements
    // -------------------------------------------------------------
    jlong getVolume() {
        return mSize;
    }
    // -------------------------------------------------------------
    // THash::begin
    //! \brief  Start iterator.
    //! \return The start of the hash table
    // -------------------------------------------------------------
    virtual iterator begin() {
        mCursorRead = 0;
        return next();
    }
    // -------------------------------------------------------------
    // THash::next
    //! \brief  Next iterator.
    //! \return The next element of the hash table.
    // -------------------------------------------------------------
    virtual iterator next() {
        iterator aEntry;
        while ((size_t)mCursorRead < mMaxSize + 1) {
            mCursorRead ++;
            aEntry = mHashTable + mCursorRead;
            if (aEntry->aSize != 0) {
                return aEntry;
            }
        }
        return end();
    }
    // -------------------------------------------------------------
    // THash::end 
    //! \brief  End iterator.
    //! \return The end of the hash table
    // -------------------------------------------------------------
    virtual iterator end() {
        return mHashTable + mMaxSize + 1;
    }
    // -------------------------------------------------------------
    // THash::checkSize 
    //! \brief Implements the shrink part of a resize hysteresis.
    //!
    // -------------------------------------------------------------
    void checkSize() {
        size_t aNewSize = mMaxSize;
        bool   aResize  = false;

        while ((jlong)aNewSize >  (jlong)(32 * mEntries)   && 
               (jlong)aNewSize >= (jlong)( 2 * mBasicSize) && 
               mResizeable) {
            aNewSize = aNewSize / 2;
            aResize  = true;
        }

        if (aResize) {
            rehash(aNewSize);
        }
    }
    // -------------------------------------------------------------
    // THash::check
    //! \brief Check the consistency of the hash table
    //!
    //! The hash table can be used again after calling THash::reset. The
    //! hash entries remain until then.
    //! \param aReset Reset the output option. The output is disabled after first error.
    //! \return \c TRUE if hash table is clean.
    // -------------------------------------------------------------
    bool check(bool aReset = false) {
        if (aReset) {
            mDoErrorOut = true;
        }        

        if (!mDoErrorOut) {
            return false;
        }

        if (!mResizeable && mEntries > (jlong)(9 * mMaxSize) / 10) {
            mDoErrorOut = false;
            ERROR_OUT(cU("THash: running out of memory"), (int)mEntries);
            return false;
        }

        if (mStateCollision > 0) {
            mResizeable = false;
            mDoErrorOut = false;
            ERROR_OUT(cU("THash: collision"), (int)mMaxSize);
            return false;
        }
        return true;
    }
};

// -------------------------------------------------------------
//! \class TFastHash
//! \brief Fast hash implementation for self optimizing hash tables.
//!
//! The TFastHash uses two hash tables, one for primary access, the
//! second for hash collision resolution.
// -------------------------------------------------------------
template <class _TKey, class _TValue, class _TObject = jint> 
            class TFastHash: public THash<_TKey, _TValue, _TObject> {
protected:
    typedef  THash<_TKey, _TValue, _TObject> TChainHash;    //!< Classic hash
public:
    typedef struct TChainHash::SHashEntry *iterator;        //!< Iterator
protected:
    typedef struct TChainHash::SHashEntry THashEntry;       //!< Hash entry
    iterator mPrimaryHashTable;                             //!< Primary hash table
    jlong    mCursorRead1;                                  //!< Read cursor
    
    // -------------------------------------------------------------
    // THash::findAction
    //! \see THash::findAction
    // -------------------------------------------------------------
    virtual inline iterator findAction(_TKey aKey, int aAction = HASH_FIND) {
        THashEntry aSwap;
        iterator aEntry;
        iterator aEntryInsert;
        unsigned long aIndex;
        static   int  aCnt = 0;

        if (aKey == (_TKey)0) {
            return this->end();
        }        
        aIndex = (unsigned long)(aKey) % (unsigned long)gHashValue;
        aEntry = &mPrimaryHashTable[aIndex + 1];

        // no entry for hash key: break
        if (aEntry->aKey == 0) {
            if (aAction == HASH_FIND_INSERT)
                return aEntry;
            else
                return this->end();
        }

        // found entry for hash key: return element
        if (aEntry->aKey == aKey && aEntry->aSize != 0) {
            if (aAction == HASH_FIND ||
                aAction == HASH_FIND_INSERT) {
                return aEntry;
            }
            else {
                mPrimaryHashTable[0].copy(aEntry);
                aEntry->aSize = 0;
                return &mPrimaryHashTable[0];
            }
        }

        aEntryInsert = TChainHash::findAction(aKey, aAction);
        // Now decide to swap entries
        if (aEntryInsert != this->end() && aAction == HASH_FIND) {
            if (aEntry->aSize == 0 || rand() < 20) {
                aSwap.copy(aEntry);
                aEntry->copy(aEntryInsert);
                aEntryInsert->aSize = 0;
                TChainHash::insert(&aSwap);
                return aEntry;
            }
        }
        return aEntryInsert;
    }
public:
    // -------------------------------------------------------------
    // TFastHash::TFastHash
    //! Constructor.
    // -------------------------------------------------------------
    TFastHash(
        jint aMaxSize    = gHashValue, 
        bool aResizeable = true): 
            TChainHash(aMaxSize, aResizeable) {
                
        mPrimaryHashTable = new THashEntry[gHashValue + 2];
        memsetR(mPrimaryHashTable, 0, sizeofR(THashEntry) * (gHashValue + 2));
    }
    // -------------------------------------------------------------
    // TFastHash::~TFastHash
    //! Destructor
    // -------------------------------------------------------------
    virtual ~TFastHash() {
    }
    // -------------------------------------------------------------
    // TFastHash::reset
    //! \see THash::reset
    // -------------------------------------------------------------
    virtual void reset() {
        memsetR(mPrimaryHashTable, 0, sizeofR(THashEntry) * (gHashValue + 2));
        TChainHash::reset();
    }
    // -------------------------------------------------------------
    // TFastHash::next
    //! \see THash::next
    // -------------------------------------------------------------
    virtual iterator next() {        
        iterator aEntry;
        if (this->mCursorRead == 0)
            mCursorRead1 = 0;

        aEntry = TChainHash::next();
        if (aEntry != this->end())
            return aEntry;

        while (mCursorRead1 < gHashValue + 1) {
            mCursorRead1 ++;
            aEntry = mPrimaryHashTable + mCursorRead1;
            if (aEntry->aSize != 0) {
                return aEntry;
            }
        }
        return this->end();
    }
};

#endif

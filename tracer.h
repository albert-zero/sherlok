// -----------------------------------------------------------------
//
// Author: Albert Rossmann
// File  : tracer.h
// Date  : 25.11.2003
//! \file tracer.h
//! \brief Trace functions
//!
// -----------------------------------------------------------------
#ifndef TRACER_H
#define TRACER_H

#define EVENT_TIME   2          //!< Trace event timer
#define EVENT_MEMORY 4          //!< Trace event memory
#define EVENT_COUNT  8          //!< Trace event count
#define EVENT_CONTENTION 16     //!< Trace contention
#define EVENT_CLASSLOAD  32     //!< Trace class load
#define EVENT_CLASSUNLOAD 64     //!< Trace class unload
#define EVENT_TRIGGER    128     //!< Trace trigger
#define EVENT_VARIABLES  256     //!< Trace variables

// ----------------------------------------------------
//! \class TTracer
//! \brief Implements trace functions
//!
// ----------------------------------------------------
class TTracer {
private:
    int  mLevel;                //!< Current trace depth for tree trace
    bool mTriggerStarted;       //!< Trace on trigger
    bool mTraceTrigger;         //!< Trace on trigger
    bool mTraceStarted;         //!< Tracer is active
    bool mTraceGC;              //!< Trace garbage collection
    bool mTraceCounter;         //!< Count method calls
    bool mTraceCounterRunning;
    bool mTraceMethod;          //!< Trace methods
    bool mTraceEvent;           //!< Trace on timer event
    bool mTraceThread;          //!< Trace threads
    bool mTraceException;       //!< Trace exceptions
    bool mTraceParameter;       //!< Trace call parameter
    bool mTraceStack;           //!< Trace call stack
    bool mTraceFull;            
    bool mTraceContention;      //!< Trace contention
    bool mTraceClass;           
    bool mForce;
    bool mConsOut;
    int  mEventType;
    bool mInitialized;
    TString  mInfoText;

    SAP_ostringstream mStream;
    jlong           mMaxTimeElapsed;
    jlong           mMaxMemoryUsed;
    jlong           mMaxCount;
    jlong           mDeltaMemory;
    SAP_ofstream    mFile;    
    jlong           mInfo;
    TString         mThreadName;
    const  SAP_UC  *mTriggerPoint;
    TXmlWriter      mWriter;
    TConsole       *mConsole;
    TProperties    *mProperties;
    static TTracer *mInstance;
    // ----------------------------------------------------
    // TTracer::TTracer
    //! Copy constructor
    // ----------------------------------------------------
    TTracer(const TTracer&) {
    }
    // ----------------------------------------------------
    // TTracer::operator=
    //! Copy constructor
    // ----------------------------------------------------
    TTracer *operator=(const TTracer&) {
        return this;
    }
    // ----------------------------------------------------
    // TTrace::TTracer
    //! Constructor
    // ----------------------------------------------------
    TTracer () {
        mConsole        = TConsole::getInstance();
        mProperties     = TProperties::getInstance();
        mTraceTrigger   = false;
        mTriggerStarted = false;
        mTriggerPoint   = NULL;
        mTraceStarted   = false;
        mTraceCounter   = false;
        mTraceMethod    = false;
        mTraceClass     = false;
        mTraceEvent     = false;
        mTraceThread    = false;
        mTraceException = false;
        mTraceParameter = false;
        mTraceStack     = false;
        mTraceContention= false;
        mConsOut        = false;
        mEventType      = 0;
        mMaxCount       = 0;
        mTraceGC        = false;
        mInitialized    = false;
        mTraceCounterRunning = false;
    }
public:
    // -----------------------------------------------------------------
    // TTracer::initialize
    //! Initializer
    // -----------------------------------------------------------------
    void initialize() {   

        if (!mInitialized) {
            mInitialized = true;
        }
    }
    // ----------------------------------------------------
    // TTrace::dump
    //! \brief Dump tracer state
    //! \param aRootTag The output tag list
    // ----------------------------------------------------
    void dump(TXmlTag *aRootTag) {
        TString aValue(cU(""));
        TXmlTag *aTag = aRootTag->addTag(cU("Monitor"));

        if (mTraceStarted)    aValue.concat(cU("started "));
	else                  aValue.concat(cU("stopped "));
	
        if (mTraceMethod)     aValue.concat(cU("/methods"));
        if (mTraceParameter)  aValue.concat(cU("/parameter"));
        if (mTraceStack)      aValue.concat(cU("/stack"));
        if (mTraceClass)      aValue.concat(cU("/class"));
        if (mTraceException)  aValue.concat(cU("/exception"));
        if (mTraceContention) aValue.concat(cU("/contention"));
        if (mTraceCounter)    aValue.concat(cU("/counter"));
        if (mTraceTrigger)    aValue.concat(cU("/trigger"));
        if (mTraceThread)     aValue.concat(cU("/thread"));
        if (mTraceGC)         aValue.concat(cU("/gc"));

        aTag->addAttribute(cU("Name") , cU("Trace"));
        aTag->addAttribute(cU("Value"), aValue.str());
   } 
private:
    // ----------------------------------------------------
    // TTrace::print
    //! Prints a formatted line to trace file and console
    // ----------------------------------------------------
    void print() {
        if (mFile.rdbuf()->is_open()) {
            mStream << ends;
            mFile   << mStream.str() << endl;
        }
        else if (mConsOut) {
            mStream << ends;
            TWriter::getInstance()->printLn(mStream.str().c_str());
        }
        else {
            mStream << ends;
            mConsole->printLn(mStream.str().c_str());
        }
        mStream.str(cU(""));
    }
public:
    // ----------------------------------------------------
    // TTrrace::setOptions
    //! \brief Sets the tracer options
    //! \param aEnable Enable/Disable trace
    //! \param aOptions Tracer option list
    //!         - -a       TraceFull
    //!         - -p       Trace output to log file
    //!         - -c       Trace method count
    //!         - -tree    Trace output as tree
    //!         - -ascii   Trace output as comma separated list
    //!         - -xml     Trace output as XML
    //!         - -e&gt;time&lt; Elapsed time    
    //!         - -m&gt;byte&lt; Delta memory    
    //!         - -n&gt;name&lt; Thread name
    //!         - -f&gt;name&lt; File name
    // ----------------------------------------------------
    void setOptions(
            bool     aEnable, 
            TValues *aOptions = NULL) {

        TString aFileName;
        TValues::iterator aPtr;

        if (!aEnable) {
            closeFile();
            return;
        }
        mConsOut        = false;
        mConsOut        = false;
        mTraceFull      = false;
        mThreadName     = cU("");
        mMaxTimeElapsed = 0;
        mDeltaMemory    = 0;
        mTraceCounter   = false;
        mTriggerPoint   = NULL;

        if (aOptions == NULL) {
            return;
        }
        for (aPtr  = aOptions->begin();
             aPtr != aOptions->end();
             aPtr  = aOptions->next()) {
            if (!STRNCMP((*aPtr), cU("-a"), 2)) {
                mTraceFull = true;
            }
            else if (!STRNCMP((*aPtr), cU("-e"), 2)) {
                mMaxTimeElapsed = TString::toInteger((*aPtr) + 2) * CLOCKS_PER_SEC / 1000;
            }
            else if (!STRNCMP((*aPtr), cU("-m"), 2)) {
                mDeltaMemory    = TString::toInteger((*aPtr) + 2);
            }
            else if (!STRNCMP((*aPtr), cU("-n"), 2)) {
                mThreadName = ((*aPtr) + 2);
                mThreadName.trim();
            }
            else if (!STRNCMP((*aPtr), cU("-f"), 2)) {
                aFileName = ((*aPtr) + 2);
                aFileName.trim();
                openFile(aFileName.str());
            }
            else if (!STRNCMP((*aPtr), cU("-p"), 2)) {
                mConsOut = true;
            }
            else if (!STRNCMP((*aPtr), cU("-c"), 2)) {
                mTraceCounter = true;
                mMaxCount = TString::toInteger((*aPtr) + 2);
            }
            else if (!STRNCMP((*aPtr), cU("-ascii"), 2)) {
                mWriter.setType(XMLWRITER_TYPE_LINE);
            }
            else if (!STRNCMP((*aPtr), cU("-xml"),   2)) {
                mWriter.setType(XMLWRITER_TYPE_XML);
            }
            else if (!STRNCMP((*aPtr), cU("-tree"),  2)) {
                mWriter.setType(XMLWRITER_TYPE_TREE);
            }
        }
        mTraceEvent = (mMaxTimeElapsed != 0 || mDeltaMemory != 0 || mMaxCount != 0);
    }
    // ----------------------------------------------------
    // TTrace::getInstance
    //! Singleton constructor
    // ----------------------------------------------------
    static TTracer *getInstance() {
        if (mInstance == NULL) {
            mInstance = new TTracer();
        }
        return mInstance;
    }
    // ----------------------------------------------------
    // TTrace::openFile
    //! Create trace file
    // ----------------------------------------------------
    void openFile(const SAP_UC *aFileName = NULL) {
        closeFile();

        TString aFile;        
        if (aFileName != NULL) {
            aFile.concat(mProperties->getPath());
            aFile.concat(FILESEPARATOR);
            aFile.concat(aFileName);
            mFile.rdbuf()->open(aFile.a7_str(), ios::out);
        }
    }
    // ----------------------------------------------------
    // TTrace::closeFile
    //! Close trace file
    // ----------------------------------------------------
    void closeFile() {
        if (!mFile.rdbuf()->is_open()) {
            return;
        }
        TXmlTag aRootTag(cU("Trace"));
        mWriter.printTrace(&aRootTag, 0);
        mFile.close();
    }
    // ----------------------------------------------------
    // TTrace::printTrace
    //! \brief Trace tag list
    //! \param aRootTag The tag list to trace
    //! \param aLevel   The start level for tree output
    //! \param aFinish  \c TRUE to close all open XML tags
    // ----------------------------------------------------
    void printTrace(
        TXmlTag *aRootTag, 
        int      aLevel  = 0, 
        bool     aFinish = false) {
            
        if (aLevel < 0) {
            return;
        }
        mWriter.printTrace(aRootTag, (SAP_UC*)cU("Trace"), aLevel, aFinish);
    }
    // ----------------------------------------------------
    //! \brief Trace tag list
    //! \param aRootTag The tag list to trace
    // ----------------------------------------------------
    void print(TXmlTag *aRootTag, int aOutputType = -1) {
        mWriter.print(aRootTag, aOutputType);
    }
    // ------------------------------------------------------------
    // TTracer::setTrigger
    //! \param aTriggerPoint The thread to trigger
    // ------------------------------------------------------------
    void startTrigger(const SAP_UC *aTriggerPoint = NULL) {
        if (mTraceTrigger && !mTriggerStarted) {
            mTriggerStarted = true;
            mTriggerPoint   = aTriggerPoint;
        }
    }
    // ------------------------------------------------------------
    // TTracer::stopTrigger
    //! \brief Stop the trace trigger
    //!
    // ------------------------------------------------------------
    void stopTrigger() {
        mTriggerStarted = false;
        mTriggerPoint   = NULL;        
    }
    // ------------------------------------------------------------
    // TTracer::getThreadName
    //! \return The name of the trace thread
    // ------------------------------------------------------------
    const SAP_UC *getThreadName() {
        return mThreadName.str();
    }
    // ----------------------------------------------------
    // TTrace::start
    //! Start the tracer
    // ----------------------------------------------------
    void start() {
        mTraceStarted = true;
    }
    // ----------------------------------------------------
    // TTrace::stop
    //! Stop the tracer
    // ----------------------------------------------------
    void stop() {
        mTraceStarted = false;
    }
    // ----------------------------------------------------
    // TTrace::getStatus
    //! \return \c TRUE if tracer is active
    // ----------------------------------------------------
    inline bool getStatus() {
        return mTraceStarted;
    }
    // ----------------------------------------------------
    // TTracer::setVerbose
    //! \brief Trace additional info
    //! \param aEnable \c TRUE to enable addional trace
    // ----------------------------------------------------
    void setVerbose(bool aEnable) {
        mProperties->setComprLine(!aEnable);
    }
    // ----------------------------------------------------
    // TTracer::doVerbose
    //! \return \c TRUE if additional trace info required
    // ----------------------------------------------------
    bool doVerbose() {
        return !mProperties->getComprLine();
    }
    // ----------------------------------------------------
    // TTrace::doTraceClass
    //! \return \c TRUE if trace of class load events is required
    // ----------------------------------------------------
    bool doTraceClass() {
        return (mTraceStarted && mTraceClass);
    }
    // ----------------------------------------------------
    // TTrace::setTraceClass
    //! \param aEnable \c TRUE to trace class load events
    //! \param aOptions Trace options
    // ----------------------------------------------------
    void setTraceClass(bool aEnable, TValues *aOptions = NULL) {
        mTraceClass = aEnable;
        setOptions(aEnable, aOptions);
    }
    // ----------------------------------------------------
    // TTrace::setTraceGC
    //! \param aEnable Enable/Disable GC trigger condition
    //! \param aOptions Trace options
    // ----------------------------------------------------
    void setTraceGC(bool aEnable, TValues *aOptions = NULL) {
        if (mTraceGC || aEnable) {
            setOptions(aEnable, aOptions);
        }
        mTraceGC = aEnable;
    }
    // ----------------------------------------------------
    // TTrace::doTraceGC
    //! \return GC trigger condition
    // ----------------------------------------------------
    bool doTraceGC() {
        return (mTraceStarted && mTraceGC);
    }
    // ----------------------------------------------------
    // TTrace::setTraceMethod
    //! \param aEnable Enable/Disable method trigger
    //! \param aOptions Trace options
    // ----------------------------------------------------
    void setTraceMethod(bool aEnable, TValues *aOptions) {
        mTraceMethod = aEnable;
        if (mTraceMethod) {
            setOptions(aEnable, aOptions);
        }
    }
    // ----------------------------------------------------
    // TTrace::setTraceContention
    //! \param aEnable Enable/Disable method trigger
    //! \param aOptions Trace options
    // ----------------------------------------------------
    void setTraceContention(bool aEnable, TValues *aOptions) {
        if (mTraceContention || aEnable) {
            setOptions(aEnable, aOptions);
        }
        mProperties->setContention(aEnable);
        mTraceContention = aEnable;
    }
    // ----------------------------------------------------
    // TTrace::doTraceContention
    //! \param  aElapsed  Current elapsed time
    //! \return \c TRUE if tracer performs trigger condition
    // ----------------------------------------------------
    bool doTraceContention(jlong aElapsed) {
        bool doContention = (mTraceContention && mTraceStarted);
        if (!doContention) {
            return false;
        }
        if (aElapsed == -1) {
            return doContention;
        }
        doContention = (mMaxTimeElapsed < aElapsed) && 
                       (mMaxTimeElapsed > 0);
        if (!doContention) {
            return false;
        }
        mEventType |= EVENT_CONTENTION;
        mInfo       = aElapsed;
        return true;
    }
    // ----------------------------------------------------
    // TTracer::getInfo
    //! \return Either elapsed time or memory usage
    // ----------------------------------------------------
    const SAP_UC *getInfo() {
        SAP_UC aBuffer[32];

        if ((mEventType & EVENT_CLASSLOAD) == 0) {
            mInfoText = TString::parseInt(mInfo, aBuffer);
        }
        return mInfoText.str();
    }
    // ----------------------------------------------------
    // TTrace::forceTrace
    //! \return \c TRUE if trace is forced for all methods
    // ----------------------------------------------------
    bool forceTrace() {
        return mTraceFull;
    }
    // ----------------------------------------------------
    // TTrace::doTraceMethod
    //! \return The method trigger condition
    // ----------------------------------------------------
    bool doTraceMethod() {
        return (mTraceStarted && (mTraceMethod || mTraceParameter));
    }
    // ----------------------------------------------------
    // TTrrace::setTraceTrigger
    //! \param aEnable Enable/Disable trigger
    //! \param aOptions Tracer options
    // ----------------------------------------------------
    void setTraceTrigger(bool aEnable, TValues *aOptions = NULL) {
        if (mTraceTrigger || aEnable) {
               setOptions(aEnable, aOptions);
        }
        mTraceTrigger   = aEnable;
        mMaxMemoryUsed  = 0;
        mTraceEvent     = false;
        if (aEnable) {
            //--mTraceTrigger = mTraceFull;
            mTraceEvent   = (mMaxTimeElapsed != 0 || mDeltaMemory != 0 || mMaxCount != 0);
        }
    }
    // ----------------------------------------------------
    // TTrrace::setTraceStack
    //! \param aEnable Enable/Disable stack trace
    //! \param aOptions Tracer options
    // ----------------------------------------------------
    void setTraceStack(bool aEnable, TValues *aOptions = NULL) {
        mTraceStack     = aEnable;
        setOptions(aEnable, aOptions);
    }
    // ----------------------------------------------------
    // TTrace::doTraceStack
    //! \return \c TRUE if stack trace required
    // ----------------------------------------------------
    bool doTraceStack() {
        return mTraceStarted && mTraceStack;
    }
    // ----------------------------------------------------
    // TTrace::doTraceStackFull
    //! \return \c TRUE if full stack trace required
    // ----------------------------------------------------
    bool doTraceStackFull() {
        return doTraceStack() && mTraceFull;
    }
    // ----------------------------------------------------
    // TTrace::setTraceParameter
    //! \param aEnable Enable/Disable Parameter tracing
    // ----------------------------------------------------
    void setTraceParameter(bool aEnable, TValues *) {
        mTraceParameter = aEnable;
    }
    // ----------------------------------------------------
    // TTrace::doTraceParameter
    //! \return \c TRUE if parameter tracing is activated
    // ----------------------------------------------------
    bool doTraceParameter() {
        return mTraceStarted && mTraceParameter;
    }
    // ----------------------------------------------------
    // TTrace::setTraceException
    //! \param aEnable Enable/Disable exception trace
    //! \param aOptions Tracer options
    // ----------------------------------------------------
    void setTraceException(bool aEnable, TValues *aOptions) {
        TValues::iterator aPtr;

        if (mTraceThread || aEnable) {
            setOptions(aEnable, aOptions);
            mThreadName = cU("");
        }
        mTraceException = aEnable;
        if (aOptions == NULL || !aEnable) {
            return;
        }
        for (aPtr  = aOptions->begin();
             aPtr != aOptions->end();
             aPtr  = aOptions->next()) {
            
            if (!STRNCMP((*aPtr), cU("-n"), 2)) {
                mProperties->parseExceptions((*aPtr) + 2);
            }
        }
    }
    // ----------------------------------------------------
    // TTrace::doTraceException
    //! \return Exception trigger condition
    // ----------------------------------------------------
    bool doTraceException() {
        return mTraceStarted && mTraceException;
    }
    // ----------------------------------------------------
    // TTrace::setTraceThread
    //! \param aEnable Enable/Disable thread trace
    //! \param aOptions Tracer options
    // ----------------------------------------------------
    void setTraceThread(bool aEnable, TValues *aOptions = NULL) {
        if (mTraceThread || aEnable) {
            setOptions(aEnable, aOptions);
        }
        mTraceThread = aEnable;
    }
    // ----------------------------------------------------
    // ----------------------------------------------------
    inline bool doTraceEvent(
                    const    SAP_UC *aTriggerPoint, 
                    bool     aCheckTraceStack, 
                    jlong    aElapsed, 
                    jlong    aMemory, 
                    jlong   *aTraceCount, 
                    jlong   *aTraceType, 
                    jlong   *aTraceInfo) {

        if (!mTraceStarted) {
            return false;
        }

        *aTraceType  = 0;
        *aTraceInfo  = 0;

        if (mTriggerStarted) {
            *aTraceType = EVENT_TRIGGER;
            return (mTriggerPoint == aTriggerPoint);
        }

        if (aCheckTraceStack && mTraceStack) {
            *aTraceType = EVENT_TRIGGER;
            return true;
        }

        if (mTraceThread) {
            *aTraceType = EVENT_TRIGGER;
            if (mTriggerPoint == NULL && mThreadName.pcount() > 1) {
                TString aStr(aTriggerPoint);
                if (aStr.findWithWildcard(mThreadName.str(), cU('.')) != -1) {
                    mTriggerPoint = aTriggerPoint;
                }
            }            
            return (mTriggerPoint == aTriggerPoint);
        }

        if ((mMaxTimeElapsed > 0) &&
            (mMaxTimeElapsed < aElapsed)) {
            *aTraceType |= EVENT_TIME;
            *aTraceInfo  = aElapsed;
        }        
        else if (
            (mDeltaMemory > 0) && 
            (mDeltaMemory < aMemory)) {
            *aTraceType |= EVENT_MEMORY;
            *aTraceInfo = aMemory;
        }
        else if (
            (mMaxCount > 0) && 
            (mMaxCount < *aTraceCount)) {
            *aTraceType |= EVENT_COUNT;
            *aTraceInfo  = *aTraceCount;
            *aTraceCount   = 0;
            mTraceStarted  = false;
        }
        // Return trigger condition
        return *aTraceType != 0;
    }
    // ----------------------------------------------------
    //! \return String representation for trigger condition
    // ----------------------------------------------------
    const SAP_UC *getTraceEvent(jlong aTraceType) {
        if ((aTraceType & EVENT_COUNT) != 0) {
            return cU("Count");
        }
        if ((aTraceType & EVENT_TIME) != 0) {
            return cU("Time");
        }
        if ((aTraceType & EVENT_MEMORY) != 0) {
            return cU("Memory");
        }
        if ((aTraceType & EVENT_CONTENTION) != 0) {
            return cU("Contention");
        }
        if ((aTraceType & EVENT_CLASSLOAD) != 0) {
            return cU("ClassLoad");
        }
        if ((aTraceType & EVENT_CLASSUNLOAD) != 0) {
            return cU("ClassUnload");
        }
        if ((aTraceType & EVENT_TRIGGER) != 0) {
            return cU("Trigger");
        }
        if ((aTraceType & EVENT_VARIABLES) != 0) {
            return cU("Variables");
        }
        return cU("Call");
    }
    // ----------------------------------------------------
    // TTrace::doTraceCount
    //! \return count trigger condition
    // ----------------------------------------------------
    inline bool doTraceCount() {
        return   mTraceCounter && mTraceCounterRunning;
    }
    // ----------------------------------------------------
    // ----------------------------------------------------
    inline void startTraceCount() {
        if (mTraceCounter) {
            mTraceCounterRunning = true;
        }
    }
    // ----------------------------------------------------
    // ----------------------------------------------------
    inline void stopTraceCount() {
        if (mTraceCounter) {
            mTraceCounterRunning = false;
        }
    }
    // ----------------------------------------------------
    // TTrace::doTraceThread
    //! \return thread trigger condition
    // ----------------------------------------------------
    inline bool doTraceThread() {
        return mTraceThread;
    }
};

#endif

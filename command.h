// -----------------------------------------------------------------
//
// Author: Albert Rossmann
// Date  : 14.04.2003
//! \file  command.h
//! \brief Command interface
//!
// -----------------------------------------------------------------
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
//! \class TCommand
//! \brief Command line interpreter
//!
// -----------------------------------------------------------------
class TCommand {
private:
    struct TPrepareCmd {
        int     mCmd;
        jlong   mTime;
    };
public:
    TStack<TPrepareCmd> mCmdStack;
private:
    static TCommand *mInstance;         //!< Static instance
    TProperties     *mProperties;       //!< Global configuration
    TTracer         *mTracer;           //!< Tracer

    bool             mRepeat;           //!< Repeat last command
    bool             mInitialized;
    jlong            mRepeatTime;       
    int              mCmdLen;           
    const SAP_UC    *mCmdLine;          //!< Raw command line
    SAP_UC           mCmdRepeat[128];

    int              mCmd;
    int              mStackCmd;
    TMonitor        *mMonitor;          //!< Profiler
    TReader         *mReader;           
    TConsole        *mConsole;          //!< Console IO processor
    TString         *mCommand;          //!< Processed command line
    TValues         *mOptionList;       //!< Options
    TXmlWriter      *mWriter;

    // -----------------------------------------------------------------
    // TCommand::doHelp
    //! \brief Help for console output
    //!
    // Online command line help
    // Parameters are passed by mOptionList
    // -----------------------------------------------------------------
    void doHelp() {
        mOptionList->begin();
        TValues::iterator aPtrAttr = mOptionList->next();
        TXmlTag *aTag;
        TXmlTag *aRootTag = new TXmlTag(cU("List"), XMLTAG_TYPE_NODE);
        aRootTag->addAttribute(cU("Type"), cU("Help"));

        if (aPtrAttr == mOptionList->end()) {
            aRootTag->addAttribute(cU("Info"), cU("Commands"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Command"),     cU("man|help [<command>]"));
            aTag->addAttribute(cU("Description"), cU("list commands"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Command"),     cU("start <function>"));
            aTag->addAttribute(cU("Description"), cU("start monitor/trace/log"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Command"),      cU("stop  <function>"));
            aTag->addAttribute(cU("Description"), cU("stop monitor/trace/log"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Command"),      cU("lsc [-m|-s|-h|-a|-f|-F|-v|-p]"));
            aTag->addAttribute(cU("Description"), cU("list classes"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Command"),      cU("lml [-m|-s|-h]"));
            aTag->addAttribute(cU("Description"), cU("list growing classes/memory leaks"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Command"),      cU("lsm [-m|-n|-e|-s|-a|-C|-M]"));
            aTag->addAttribute(cU("Description"), cU("list methods"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Command"),      cU("lss"));
            aTag->addAttribute(cU("Description"), cU("list monitor statistics"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Command"),      cU("lsp [-s<file>"));
            aTag->addAttribute(cU("Description"), cU("list property keys and values, use -s to store the values in skp format"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Command"),      cU("lhd [-m|-n|-g|-s|-C]"));
            aTag->addAttribute(cU("Description"), cU("list heap dump"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Command"),      cU("reset [-s]"));
            aTag->addAttribute(cU("Description"), cU("reload the configuration and clears all values"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Command"),      cU("repeat [<seconds>]"));
            aTag->addAttribute(cU("Description"), cU("repeat the last command"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Command"),      cU("gc"));
            aTag->addAttribute(cU("Description"), cU("start garbage collection"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Command"),      cU("dt [-c|-j|-m|-s|-f|-x]"));
            aTag->addAttribute(cU("Description"), cU("dump threads"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Command"),      cU("info"));
            aTag->addAttribute(cU("Description"), cU("writes a timestamp and the info string to a log file"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Command"),      cU("trace <options>"));
            aTag->addAttribute(cU("Description"), cU("trace dynamic runtime behaviour"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Command"),      cU("lcf"));
            aTag->addAttribute(cU("Description"), cU("list configuration files"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Command"),      cU("dex"));
            aTag->addAttribute(cU("Description"), cU("dump exceptions"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Command"),      cU("set"));
            aTag->addAttribute(cU("Description"), cU("set name=value"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Command"),      cU("exit"));
            aTag->addAttribute(cU("Description"), cU("leave the telnet session"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Command"),      cU("chpwd"));
            aTag->addAttribute(cU("Description"), cU("change password for the current user"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Command"),      cU("version"));
            aTag->addAttribute(cU("Description"), cU("display the current version"));
        }
        else if (!STRNCMP(*aPtrAttr, cU("start"), 5)) {
            aRootTag->addAttribute(cU("Command"), cU("start"));
            aRootTag->addAttribute(cU("Description"), cU("starts a monitor funtion"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("monitor"));
            aTag->addAttribute(cU("Description"), cU("aEnable memory, timer and history"));
            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("trace"));
            aTag->addAttribute(cU("Description"), cU("start tracing (toggle trace with \"s\"))"));
            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("log -f<log file name> -append"));
            aTag->addAttribute(cU("Description"), cU("opens a log file"));
        } 
        else if (!STRNCMP(*aPtrAttr, cU("stop"), 5)) {
            aRootTag->addAttribute(cU("Command"), cU("stop"));
            aRootTag->addAttribute(cU("Description"), cU("stops a monitor funtion"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("monitor"));
            aTag->addAttribute(cU("Description"), cU("disable memory, timer and history"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("trace"));
            aTag->addAttribute(cU("Description"), cU("stop tracing (toggle trace with \"s\"))"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("log"));
            aTag->addAttribute(cU("Description"), cU("close monitor output file"));
        }
        else if (!STRNCMP(*aPtrAttr, cU("repeat"), 6)) {
            aRootTag->addAttribute(cU("Command"), cU("repeat"));
            aRootTag->addAttribute(cU("Description"), cU("[<seconds>]: repeat the last command"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("<seconds>"));
            aTag->addAttribute(cU("Description"), cU("repeater time intervall in seconds (default is 1 sec)"));
        }
        else if (!STRNCMP(*aPtrAttr, cU("lsc"), 3)) {
            aRootTag->addAttribute(cU("Command"), cU("lsc"));
            aRootTag->addAttribute(cU("Description"), cU("[-m<number>][-s<column name>][-h][-f<filter>]: list monitored classes"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-m<number>"));
            aTag->addAttribute(cU("Description"), cU("select classes with allocated bytes > <number>"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-s<column name>"));
            aTag->addAttribute(cU("Description"), cU("sort by column name"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-f<filter>"));
            aTag->addAttribute(cU("Description"), cU("filter class names in scope"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-F<filter>"));
            aTag->addAttribute(cU("Description"), cU("filter all class names"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-h"));
            aTag->addAttribute(cU("Description"), cU("output with GC history"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-v"));
            aTag->addAttribute(cU("Description"), cU("output with field symbols"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-p"));
            aTag->addAttribute(cU("Description"), cU("output with methods"));
        }
        else if (!STRNCMP(*aPtrAttr, cU("lml"), 3)) {
            aRootTag->addAttribute(cU("Command"), cU("lml"));
            aRootTag->addAttribute(cU("Description"), cU("[-m<number>][-s<column name>][-h][-f<filter>]: list growing classes"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-h"));
            aTag->addAttribute(cU("Description"), cU("output with GC history"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-m<number>"));
            aTag->addAttribute(cU("Description"), cU("select classes with allocated bytes > <number>"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-s<column name>"));
            aTag->addAttribute(cU("Description"), cU("sort by column name"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-f<filter>"));
            aTag->addAttribute(cU("Description"), cU("filter class names"));
        }
        else if (!STRNCMP(*aPtrAttr, cU("lsm"), 3)) {
            aRootTag->addAttribute(cU("Command"), cU("lsm"));
            aRootTag->addAttribute(cU("Description"), cU("[-m<number>][-n<number>][-e<number>][-c<number>][-s<column name>][-f<filter>][-p]: list monitored methods"));
            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-m<number>"));
            aTag->addAttribute(cU("Description"), cU("select methods with CpuTime > <number>"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-n<number>"));
            aTag->addAttribute(cU("Description"), cU("select methods with NrCalls > <number>"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-e<number>"));
            aTag->addAttribute(cU("Description"), cU("select methods with Elapsed > <number>"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-c<number>"));
            aTag->addAttribute(cU("Description"), cU("select methods with contentions > <number> and add contention columns to output"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-s<column name>"));
            aTag->addAttribute(cU("Description"), cU("sort by column name"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-f<filter>"));
            aTag->addAttribute(cU("Description"), cU("filter method names"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-a"));
            aTag->addAttribute(cU("Description"), cU("list signature and id"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-p"));
            aTag->addAttribute(cU("Description"), cU("list parameter"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-C<class id>"));
            aTag->addAttribute(cU("Description"), cU("list method of class"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-M<method id>"));
            aTag->addAttribute(cU("Description"), cU("list method with given id"));
        }
        else if (!STRNCMP(*aPtrAttr, cU("gc"), 2)) {
            aRootTag->addAttribute(cU("Command"), cU("gc"));
            aRootTag->addAttribute(cU("Description"), cU("starts garbage collection"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("gc"));
            aTag->addAttribute(cU("Description"), cU("Format: (GC | timestamp | used objects | used object space | total object space)"));
        }
        else if (!STRNCMP(*aPtrAttr, cU("reset"), 5)) {
            aRootTag->addAttribute(cU("Command"), cU("reset"));
            aRootTag->addAttribute(cU("Description"), cU("reload the configuration and clears all values"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-s"));
            aTag->addAttribute(cU("Description"), cU("configuration file overwrites local settings"));
        }
        else if (!STRNCMP(*aPtrAttr, cU("lhd"), 3)) {
            aRootTag->addAttribute(cU("Command"), cU("lhd"));
            aRootTag->addAttribute(cU("Description"), cU("list heap dump"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-m<number>"));
            aTag->addAttribute(cU("Description"), cU("selects classes with heap size > <number>"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-n<number>"));
            aTag->addAttribute(cU("Description"), cU("selects classes with heap count > <number>"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-s<column name>"));
            aTag->addAttribute(cU("Description"), cU("sort by column name"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-f<filter>"));
            aTag->addAttribute(cU("Description"), cU("filter class names"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-g"));
            aTag->addAttribute(cU("Description"), cU("run filter on result set of last call"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-C<class id>"));
            aTag->addAttribute(cU("Description"), cU("run heap for specified class"));
        }
        else if (!STRNCMP(*aPtrAttr, cU("dex"), 3)) {
            aRootTag->addAttribute(cU("Command"), cU("dex"));
            aRootTag->addAttribute(cU("Description"), cU("dump exception statistics: collected by trace add exceptions)"));
        }
        else if (!STRNCMP(*aPtrAttr, cU("dt"), 2)) {
            aRootTag->addAttribute(cU("Command"), cU("dt [-c][-s<column>][-j|-m<depth>|-k]"));
            aRootTag->addAttribute(cU("Description"), cU("dump threads"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-c"));
            aTag->addAttribute(cU("Description"), cU("dump callstack (if not empty)"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-x"));
            aTag->addAttribute(cU("Description"), cU("dump callstack for specified thread-id"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-s<column>"));
            aTag->addAttribute(cU("Description"), cU("sort output"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-j"));
            aTag->addAttribute(cU("Description"), cU("java heap dump"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-m<depth>"));
            aTag->addAttribute(cU("Description"), cU("restrict output to min depth"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-k"));
            aTag->addAttribute(cU("Description"), cU("kill -3 CAUTION: THIS MIGHT TERMINATE APPLICATION"));
        }
        else if (!STRNCMP(*aPtrAttr, cU("trace"), 5)) {
            aRootTag->addAttribute(cU("Command"), cU("trace"));
            aRootTag->addAttribute(cU("Description"), cU("[-verbose] [add|remove <trace-option>]"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("-verbose"));
            aTag->addAttribute(cU("Description"), cU("add additional information to the console output"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("gc"));
            aTag->addAttribute(cU("Description"), cU("trace garbage collection"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("parameter"));
            aTag->addAttribute(cU("Description"), cU("trace call parameters for TraceMethods"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("exception"));
            aTag->addAttribute(cU("Description"), cU("trace exceptions: stop on OutOfMemoryError"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("contention -e<elapsed-time> -a -ascii|-tree|-xml"));
            aTag->addAttribute(cU("Description"), cU("trace thread contentions"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("stack"));
            aTag->addAttribute(cU("Description"), cU("trace callstack for TraceTrigger method"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("method"));
            aTag->addAttribute(cU("Description"), cU("trace enter and exit events for TraceMethods"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("class"));
            aTag->addAttribute(cU("Description"), cU("trace class load and unload events"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("thread -n<thread-name>"));
            aTag->addAttribute(cU("Description"), cU("trace method enter events for <thread-name>"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("trigger <options>"));
            aTag->addAttribute(cU("Description"), cU("trace triggered by TraceTrigger"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("trigger -ascii|-xml|-tree"));
            aTag->addAttribute(cU("Description"), cU("set output to ascii, xml or tree view"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("trigger -e<elapsed-time>"));
            aTag->addAttribute(cU("Description"), cU("trace all methods, which ecceed given elapsed-time"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("trigger -a"));
            aTag->addAttribute(cU("Description"), cU("trace all method enter events"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("trigger -c"));
            aTag->addAttribute(cU("Description"), cU("count up method enter events"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("trigger -p"));
            aTag->addAttribute(cU("Description"), cU("redirect output to sherlok.log"));

            aTag = aRootTag->addTag(cU("Item"));
            aTag->addAttribute(cU("Attribute"), cU("trigger -f<file-name>"));
            aTag->addAttribute(cU("Description"), cU("redirect output to <file-name>"));
        }
        else {
            aRootTag->addAttribute(cU("Command"), *aPtrAttr);
            aRootTag->addAttribute(cU("Description"), cU("no help available"));
        }
        mMonitor->syncOutput(aRootTag);
        delete aRootTag;
    }
    // -----------------------------------------------------------------
    // TCommand::TCommand
    //! Constructor
    // -----------------------------------------------------------------
    TCommand(): mCmdStack(16) {
        mTracer         = TTracer::getInstance();
        mProperties     = TProperties::getInstance();
        mMonitor        = TMonitor::getInstance();
        mConsole        = TConsole::getInstance();
        mReader         = new TReader();
        mRepeatTime     = 1;
        mCmd            = COMMAND_CONTINUE;
        mStackCmd       = COMMAND_CONTINUE;
        mCommand        = new TString;
        mOptionList     = new TValues(10);
        mWriter         = new TXmlWriter(mProperties->getConsoleWriterType());
        mRepeat         = false;
        mInitialized    = false;
    }
public:
    // -----------------------------------------------------------------
    // TCommand::~TCommand
    //! Destructor
    // -----------------------------------------------------------------
    ~TCommand() {
        delete mReader;
        delete mCommand;
        delete mOptionList;
        delete mWriter;
    }
    // -----------------------------------------------------------------
    // TCommand::initialize
    //! Initializer
    // -----------------------------------------------------------------
    void initialize() {  
        if (!mInitialized) {
            mInitialized = true;
        }
    }
    // -----------------------------------------------------------------
    // TCommand::setStackCmd
    //! \brief Allows setting commands from different threads
    //! \param aStackCmd The command to execute in repeater thread
    // -----------------------------------------------------------------
    void pushStackCmd(int aStackCmd) {
        TPrepareCmd *aReq = mCmdStack.push();
        aReq->mCmd   =  aStackCmd;
    }
    // -----------------------------------------------------------------
    // TCommand::executeStackCmd
    //! \brief Allows setting commands from different threads
    //! \param aJni The environment to run the command.
    // -----------------------------------------------------------------
    void executeStackCmd(
            jvmtiEnv    *aJvmti,
            JNIEnv      *aJni) {
        TPrepareCmd *aReq;
        
        while (!mCmdStack.empty()) {
            aReq = mCmdStack.pop();
            execute(aJvmti, aJni, &(aReq->mCmd));
        }
    }
    // -----------------------------------------------------------------
    // TCommand::parse
    //! \brief Process a command from console input
    //! \param aCmdLine The command line to parse
    //! \return \c TRUE if command is recongnized
    // -----------------------------------------------------------------
    bool parse(const SAP_UC *aCmdLine) {
        TString aCommand;
        TValues::iterator aPtr;
        TXmlTag    aRoot(cU("Properties"), XMLTAG_TYPE_NODE);;

        // stop trace and repeater
        mCmd     = COMMAND_CONTINUE;
        mCmdLine = aCmdLine; 

        if (aCmdLine    == NULL || 
            aCmdLine[0] == 0) {
            return true;
        }
        aCommand = aCmdLine;
        aCommand.trimLeft();
        aCommand.split(mOptionList, cU(' '));
        aPtr = mOptionList->begin();

        if (!STRNCMP((*aPtr), cU("man"),  3) ||
            !STRNCMP((*aPtr), cU("help"), 4)) {
            mCmd = COMMAND_HELP;
        } else if (!STRNCMP((*aPtr), cU("dt"),     2)) {
            mCmd = COMMAND_DT;
        } else if (!STRNCMP((*aPtr), cU("dex"),    2)) {
            mCmd = COMMAND_DEX;
        } else if (!STRNCMP((*aPtr), cU("lsc"),    3)) {
            mCmd = COMMAND_LSC;
        } else if (!STRNCMP((*aPtr), cU("lhd"),    3)) {
            mCmd = COMMAND_LHD;
        } else if (!STRNCMP((*aPtr), cU("lss"),    3)) {
            mCmd = COMMAND_LSS;
        } else if (!STRNCMP((*aPtr), cU("lml"),    3)) {
            mCmd = COMMAND_LML;
        } else if (!STRNCMP((*aPtr), cU("lsp"),    3)) {
            mCmd = COMMAND_LSP;
        } else if (!STRNCMP((*aPtr), cU("repeat"), 6)) {
            mCmd = COMMAND_REPEAT;
        } else if (!STRNCMP((*aPtr), cU("lsm"),    3)) {
            mCmd = COMMAND_LSM;
        } else if (!STRNCMP((*aPtr), cU("start"),  5)) {
            mCmd = COMMAND_START;
        } else if (!STRNCMP((*aPtr), cU("stop"),   4)) {
            mCmd = COMMAND_STOP;
        } else if (!STRNCMP((*aPtr), cU("reset"),  5)) {
            mCmd = COMMAND_RESET;
        } else if (!STRNCMP((*aPtr), cU("info"),   4)) {
            mCmd = COMMAND_INFO;
        } else if (!STRNCMP((*aPtr), cU("gc"),     2)) {
            mCmd = COMMAND_GC;
        } else if (!STRNCMP((*aPtr), cU("echo"),   4)) {
            mCmd = COMMAND_ECHO;
        } else if (!STRNCMP((*aPtr), cU("lcf"),    3)) {
            mCmd = COMMAND_LCF;
        } else if (!STRNCMP((*aPtr), cU("set"),    3)) {
            mCmd = COMMAND_SET;
        } else if (!STRNCMP((*aPtr), cU("exit"),   4)) {
            mCmd = COMMAND_EXIT;
        } else if (!STRNCMP((*aPtr), cU("trace"),  5)) {
            mCmd = COMMAND_TRIGGER;
        } else if (!STRNCMP((*aPtr), cU("chpwd"),  5)) {
            mCmd = COMMAND_PASSWD_CHANGE;
        } else if (!STRNCMP((*aPtr), cU("version"),3)) {
            mConsole->getVersion();
            mCmd = COMMAND_CONTINUE;
        } else if (!STRNCMP((*aPtr), cU("s"),      2)) {
            if (mTracer->getStatus()) {
                mCmd = COMMAND_STOP;
                aCommand = cU("stop trace");
            }
            else {
                mCmd = COMMAND_START;
                aCommand = cU("start trace");
            }
            aCommand.split(mOptionList, cU(' '));
        } else {
            mCmd = COMMAND_UNKNOWN;
        }        
        return (mCmd != COMMAND_UNKNOWN &&
                mCmd != COMMAND_CONTINUE);
    }
    // -----------------------------------------------------------------
    // TCommand::getInstance
    //! Singleton constructor
    // -----------------------------------------------------------------
    static TCommand *getInstance() {
        if (mInstance == NULL) {
            mInstance = new TCommand();
        }
        return mInstance;
    }
    // -----------------------------------------------------------------
    // TCommand::getSleepTime
    //! \return The time interval for repeater thread
    // -----------------------------------------------------------------
    int getSleepTime() {
        return (int)mRepeatTime * 1000;
    }
    // -----------------------------------------------------------------
    // TCommand::getCmd
    //! \return The last command ID
    // -----------------------------------------------------------------
    int getCmd() {
        return mCmd;
    }
    // -----------------------------------------------------------------
    // TCommand::read
    //! \brief Reads and parses a command line
    //! \return \c TRUE if a valid command was read from console
    // -----------------------------------------------------------------
    bool read() {
        bool  aSuccess = false;
        SAP_UC aBuffer[32];

        mCmdLine = mReader->getLine();
        mRepeat  = false;
        if (mCmdLine == NULL) {
            return aSuccess;
        }
        aSuccess = parse(mCmdLine);

        if (aSuccess && 
            mCmd != COMMAND_REPEAT &&
            mCmd != COMMAND_CONTINUE) {
            const SAP_UC *aUserCmd = mReader->getCurrent();

            TXmlTag     aRootTag(cU("Trace"));
            aRootTag.addAttribute(cU("Type"),    cU("UserCommand"));
            aRootTag.addAttribute(cU("Command"), aUserCmd);
            aRootTag.addAttribute(cU("Time"),    TString::parseInt(TSystem::getTimestamp(), aBuffer));
            mMonitor->syncOutput(&aRootTag);
            mReader->accept();
        }
        return aSuccess;
    }
    // -----------------------------------------------------------------
    // TCommand::repeat
    //! \brief Repeater
    //! \return \c TRUE if there is command to repeat
    // -----------------------------------------------------------------
    bool repeat() {
        return (mRepeat &&
                mCmd != COMMAND_ERR &&
                mCmd != COMMAND_CONTINUE);
    }
    // -----------------------------------------------------------------
    // TCommand::execute
    //! \brief Execute a command
    //! \param aJni The environment to execute the command
    // -----------------------------------------------------------------
    void execute(
            jvmtiEnv    *aJvmti,
            JNIEnv      *aJni, 
            int         *aCmd) {

        SAP_UC aBuffer[128];
        TValues::iterator aPtrCmd;
        TValues::iterator aPtrAttr;

        aPtrCmd  = mOptionList->begin();
        aPtrAttr = mOptionList->next();
        
        if (aCmd == NULL) {
            aCmd = &mCmd;
        }

        switch (*aCmd) {
            case COMMAND_ECHO:
                *aCmd = COMMAND_CONTINUE;
                if (aPtrAttr != mOptionList->end() &&
                   !STRNCMP((*aPtrAttr), cU("off"), 3)) {
                    mConsole->setEcho(false);
                }
                else {
                    mConsole->setEcho(true);
                }
                break;
            case COMMAND_CONTINUE:
                break;
            case COMMAND_HELP:
                *aCmd = COMMAND_CONTINUE;
                doHelp();
                break;
            case COMMAND_LML: {
                TXmlTag aRootTag(cU("List"), XMLTAG_TYPE_NODE);
                aRootTag.addAttribute(cU("Info"), cU("Growing Classes"));
				mMonitor->dumpMemoryLeaks(aJvmti, &aRootTag, mOptionList);
                mMonitor->syncOutput(&aRootTag);
                break;
            }
            case COMMAND_LSC: {
                TXmlTag aRootTag(cU("List"), XMLTAG_TYPE_NODE);
                aRootTag.addAttribute(cU("Info"), cU("Monitored Classes"));
                mMonitor->dumpClasses(aJvmti, &aRootTag, mOptionList);
                mMonitor->syncOutput(&aRootTag);
                break;
            }
            case COMMAND_LHD: {
                TXmlTag aRootTag(cU("List"), XMLTAG_TYPE_NODE);
                aRootTag.addAttribute(cU("Type"), cU("Heap"));
                aRootTag.addAttribute(cU("Info"), cU("Heap Dump"));
                mMonitor->dumpHeap(aJvmti, &aRootTag, 0, mOptionList);
                mMonitor->syncOutput(&aRootTag);
                break;
            }                
            case COMMAND_SET: {
                TXmlTag aRootTag(cU("Message"));
                aRootTag.addAttribute(cU("Type"), cU("Command"));
                aRootTag.addAttribute(cU("Info"), cU("Set"));
                TProperty aProperty;
                TString   aCmdOutput;

                aProperty  = *aPtrAttr;
                aCmdOutput = *aPtrAttr;

                if (mProperties->parseProperty(&aProperty)) {
                    aRootTag.addAttribute(cU("Result"), aCmdOutput.str());
                    if (aProperty.equalsKey(cU("Tracer"))) {
                        setTraceOptions(aJvmti);
                    }
                }
                else {
                    aRootTag.addAttribute(cU("Result"), cU("Unknown"));
                }
                mMonitor->syncOutput(&aRootTag);
                break;
            }
            case COMMAND_LCF: {
                TXmlTag aRootTag(cU("List"), XMLTAG_TYPE_NODE);
                aRootTag.addAttribute(cU("Type"), cU("File"));
                aRootTag.addAttribute(cU("Info"), cU("List Configuration Files"));
                mProperties->dumpFileList(&aRootTag);
                mMonitor->syncOutput(&aRootTag);
                break;
            }
            case COMMAND_LSS: {
                TXmlTag  aRootTag(cU("List"), XMLTAG_TYPE_NODE);
                aRootTag.addAttribute(cU("Type"), cU("Statistic"));
                TXmlTag *aTag;
                TLogger *aLogger = TLogger::getInstance();                                
                aRootTag.addAttribute(cU("Info"), cU("List Monitor Statistic"));

                mMonitor->dumpStatistic(aJvmti, &aRootTag);
                mTracer->dump(&aRootTag);

                aTag = aRootTag.addTag(cU("Monitor"));
                aTag->addAttribute(cU("Name") , cU("Logging"));
                if (aLogger->getStatus()) {
                    aTag->addAttribute(cU("Value"), mProperties->getLogFile()->str());
                }
                else {
                    aTag->addAttribute(cU("Value"), TString::parseBool(aLogger->getStatus(), aBuffer));
                }
                mMonitor->syncOutput(&aRootTag);
                break;
            }
            case COMMAND_LSP: {
                TXmlTag  aRootTag(cU("List"), XMLTAG_TYPE_NODE);
                aRootTag.addAttribute(cU("Type"), cU("Config"));

                if (aPtrAttr != mOptionList->end()      &&
                   !STRNCMP((*aPtrAttr), cU("-s"), 2)   && 
                    STRLEN(*aPtrAttr) > 3) {

                    TXmlWriter aWriter(XMLWRITER_TYPE_PROPERTY, true);
                    TXmlTag    aRootProp(cU("Property"), XMLTAG_TYPE_NODE);
                    TString    aPath;
                    TString    aFile((*aPtrAttr)+2);

                    if (aFile.findLastOf(cU('.')) == aFile.end()) {
                        aFile.concat(cU(".skp"));
                    }

                    TProperty  aProp(cU("ConfigFile"), aFile.str());
                    mProperties->parseProperty(&aProp);
                    
                    aPath = mProperties->getPath();
                    aPath.concatPathExt(mProperties->getPropertyFile());

                    mProperties->dumpProperties(&aRootProp);
                    aWriter.print(&aRootProp);
                    aWriter.dump(aPath.str());

                    aRootTag.addAttribute(cU("Info"),   cU("Save Configuration"));
                    aRootTag.addAttribute(cU("Result"), aPath.str());
                }
                else {
                    aRootTag.addAttribute(cU("Info"),   cU("List Configuration"));
                    mProperties->dumpProperties(&aRootTag);
                }
                mMonitor->syncOutput(&aRootTag);
                break;
            }
            case COMMAND_EXIT: {
                TConsole  *aConsole  = TConsole::getInstance();
                if (mProperties->getConsoleWriterType() == XMLWRITER_TYPE_XML) {
                    mMonitor->syncOutput(cU("</sherlok>\n"));
                }
                aConsole->exitConnection();
                break;
            }
            case COMMAND_LSM: {
                TXmlTag aRootTag(cU("List"), XMLTAG_TYPE_NODE);
                aRootTag.addAttribute(cU("Info"), cU("List Monitored Methods"));
                mMonitor->dumpMethods(aJvmti, &aRootTag, mOptionList, 0, NULL, NULL, cU("Method"));
                mMonitor->syncOutput(&aRootTag);
                break;
            }
            case COMMAND_INFO: {
                TXmlTag aRootTag(cU("Message"));
                aRootTag.addAttribute(cU("Type"),   cU("Command"));
                aRootTag.addAttribute(cU("Info"),   mCmdLine);
                aRootTag.addAttribute(cU("Result"), TString::parseInt(TSystem::getTimestamp(), aBuffer));
                mMonitor->syncOutput(&aRootTag);
                break;
            }
            case COMMAND_RESET: {
                TXmlTag aTraceTag(cU("Messages"), XMLTAG_TYPE_NODE);
                aTraceTag.addAttribute(cU("Type"),   cU("Command"));
                bool aParse = true;
                if (aPtrAttr != mOptionList->end()) {
                    aParse = STRNCMP(*aPtrAttr, cU("-s"), 2) != 0;
                }

                if (!mProperties->reset(aParse)) {
                    TXmlTag     aRootTag(cU("List"), XMLTAG_TYPE_NODE);
                    TXmlWriter  aWriter(XMLWRITER_TYPE_PROPERTY, true); 

                    aRootTag.addAttribute(cU("Type"), cU("Properties"));
                    mProperties->dumpProperties(&aRootTag);
                    
                    aWriter.print(&aRootTag);
                    aWriter.dump(mProperties->getPropertyFilePath());
                    mProperties->parseFile();
                    mProperties->loadScpFiles(false);
                }
                mMonitor->reset(aJvmti, &aTraceTag);
                mMonitor->syncOutput(&aTraceTag);
                break;
            }
            case COMMAND_START: {
                TXmlTag  aTraceTag(cU("Messages"), XMLTAG_TYPE_NODE);
                aTraceTag.addAttribute(cU("Type"), cU("Command"));

                TXmlTag *aTag;
                bool     aSuccess = (aPtrAttr != mOptionList->end());

                *aCmd = COMMAND_CONTINUE;
                if (aSuccess) {
                    if (!STRNCMP(*aPtrAttr, cU("monitor"), 7)) {
                        mProperties->reset(false);
                        mMonitor->start(aJvmti, &aTraceTag);
                    }
                    else if (!STRNCMP(*aPtrAttr, cU("trace"),   5)) {
                        aTag = aTraceTag.addTag(cU("Message"));
                        aTag->addAttribute(cU("Type"), cU("Command"));
                        aTag->addAttribute(cU("Info"), cU("Trace started"));
                        startThreadTrace(aJvmti, aJni);
                        mTracer->start();

                        if (mTracer->doTraceException()) {
                            aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION, NULL);
                        }
                    }
                    else if (!STRNCMP(*aPtrAttr, cU("log"),     3)) {
                        bool aAppend = false;
                        aTag = aTraceTag.addTag(cU("Message"));
                        aTag->addAttribute(cU("Type"), cU("Command"));
                        aTag->addAttribute(cU("Info"), cU("Logging started"));
                        
                        for (;;) {
                            aPtrAttr = mOptionList->next();
                            if (aPtrAttr == mOptionList->end()) {
                                break;
                            }
                            if (!STRNCMP(*aPtrAttr, cU("-f"), 2)) {
                                mProperties->setLogFile(*aPtrAttr + 2);
                            }
                            if (!STRNCMP(*aPtrAttr, cU("-a"), 2)) {
                                aAppend = true;
                            }
                        }
                        TLogger *aLogger = TLogger::getInstance();
                        aLogger->start(aAppend);
                    }
                    else if (!STRNCMP(*aPtrAttr, cU("jarm"),   4) ||
                             !STRNCMP(*aPtrAttr, cU("ats"),    3)) {
                        if (mProperties->getProfilerMode() != PROFILER_MODE_JARM) {
                            mMonitor->stop(aJvmti, &aTraceTag);
                            mProperties->setProfilerMode(PROFILER_MODE_JARM);
                        }
                        mMonitor->start(aJvmti, &aTraceTag);
                    }
                    else {
                        aSuccess = false;
                    }
                }

                if (!aSuccess) {
                    aTag = aTraceTag.addTag(cU("Message"));
                    aTag->addAttribute(cU("Type"), cU("Command"));
                    aTag->addAttribute(cU("Info"), cU("Command failed"));
                    *aCmd = COMMAND_ERR;
                }
                mMonitor->syncOutput(&aTraceTag);
                break;
            }
            case COMMAND_STOP: {
                TXmlTag  aTraceTag(cU("Messages"), XMLTAG_TYPE_NODE);
                aTraceTag.addAttribute(cU("Type"), cU("Command"));

                TXmlTag *aTag;
                bool     aSuccess = (aPtrAttr != mOptionList->end());

                *aCmd = COMMAND_CONTINUE;

                if (aSuccess) {
                    if (!STRNCMP(*aPtrAttr, cU("monitor"), 7)) {
                        mMonitor->stop(aJvmti, &aTraceTag);
                    }
                    else if (!STRNCMP(*aPtrAttr, cU("trace"),   5)) {
                        aTag = aTraceTag.addTag(cU("Message"));
                        aTag->addAttribute(cU("Type"), cU("Command"));
                        aTag->addAttribute(cU("Info"), cU("Trace stopped"));
                        mTracer->stop();
                        aJvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_EXCEPTION, NULL);
                    }
                    else if (!STRNCMP(*aPtrAttr, cU("log"),     3)) {
                        aTag = aTraceTag.addTag(cU("Message"));
                        aTag->addAttribute(cU("Type"), cU("Command"));
                        aTag->addAttribute(cU("Info"), cU("Logging stopped"));
                        TLogger *aLogger = TLogger::getInstance();
                        aLogger->stop();
                    }
                    else if (!STRNCMP(*aPtrAttr, cU("jarm"),   4)) {
                        mMonitor->stop(aJvmti, &aTraceTag);
                        mProperties->setProfilerMode(PROFILER_MODE_PROFILE);
                    }
                    else {
                        aSuccess = false;
                    }
                }

                if (!aSuccess) {
                    aTag = aTraceTag.addTag(cU("Message"));
                    aTag->addAttribute(cU("Type"), cU("Command"));
                    aTag->addAttribute(cU("Info"), cU("Command failed"));
                    *aCmd = COMMAND_ERR;
                }
                mMonitor->syncOutput(&aTraceTag);
                break;
            }
            case COMMAND_GC: {
                mMonitor->dumpGC(aJvmti, aJni);
                break;
            }
            case COMMAND_DEX: {
                TXmlTag aRootTag(cU("List"), XMLTAG_TYPE_NODE);
                aRootTag.addAttribute(cU("Type"), cU("Exceptions"));
                aRootTag.addAttribute(cU("Info"), cU("List Exceptions"));
                mMonitor->dumpExceptions(&aRootTag);
                mMonitor->syncOutput(&aRootTag);
                break;
            }
            case COMMAND_DT: {
                TXmlTag aRootTag(cU("List"), XMLTAG_TYPE_NODE);
                aRootTag.addAttribute(cU("Type"), cU("Thread"));
                aRootTag.addAttribute(cU("Info"), cU("List Threads"));
                mMonitor->dumpFullStack(aJvmti, aJni, &aRootTag, mOptionList);
                mMonitor->syncOutput(&aRootTag);
                break;
            }
            case COMMAND_REPEAT: {
                if (mRepeat) {
                    mRepeat = false;
                    break;
                }
                mRepeatTime = 1;
                const SAP_UC *aCommand = mReader->getPrevious();
                if (aPtrAttr != mOptionList->end()) {
                    mRepeatTime = TString::toInteger(*aPtrAttr);
                }
                STRCPY(mCmdRepeat, mReader->getPrevious(), 128);
                parse(aCommand);
                
                mRepeat = true;
                TXmlTag aRootTag(cU("Message"));
                aRootTag.addAttribute(cU("Type"),   cU("Command"));
                aRootTag.addAttribute(cU("Result"), aCommand);
                mMonitor->syncOutput(&aRootTag);
                break; 
            }
            case COMMAND_TRIGGER: {
                jvmtiError   aResult;
                TXmlTag      aTraceTag(cU("Messages"), XMLTAG_TYPE_NODE);                
                TXmlTag     *aTag;

                aTraceTag.addAttribute(cU("Type"), cU("Command"));

                TString aAction;
                bool    aEnable   = true;
                bool    aSuccess = false;
                mTracer->setVerbose(false);

                *aCmd = COMMAND_CONTINUE;
                if (aPtrAttr != mOptionList->end()) {
                    if (!STRNCMP((*aPtrAttr), cU("add"), 3)) {
                        aSuccess = true;
                        aEnable   = true;
                    }
                    else if (!STRNCMP((*aPtrAttr), cU("remove"), 3)) {
                        aSuccess = true;
                        aEnable   = false;
                    }
                    aPtrAttr = mOptionList->next();
                }

                if (aSuccess && aPtrAttr != mOptionList->end()) {
                    if (!STRNCMP((*aPtrAttr), cU("method"), 3)) {
                        aTag = aTraceTag.addTag(cU("Message"));
                        aTag->addAttribute(cU("Type"), cU("Command"));
                        aTag->addAttribute(cU("Info"), aEnable? cU("Trace Methods added") : cU("Trace Methods removed"));
                        mTracer->setTraceMethod(aEnable, mOptionList);
                    }
                    else if (!STRNCMP((*aPtrAttr), cU("trigger"), 3)) {
                        aTag = aTraceTag.addTag(cU("Message"));
                        aTag->addAttribute(cU("Type"), cU("Command"));
                        aTag->addAttribute(cU("Info"), aEnable? cU("Trace Trigger added") : cU("Trace Trigger removed"));
                        mTracer->setTraceTrigger(aEnable, mOptionList);
                        if (aEnable) {
                            mMonitor->start(aJvmti, &aTraceTag);
                        }
                    }
                    else if (!STRNCMP((*aPtrAttr), cU("gc"), 2)) {
                        aTag = aTraceTag.addTag(cU("Message"));
                        aTag->addAttribute(cU("Type"), cU("Command"));
                        aTag->addAttribute(cU("Info"), aEnable? cU("Trace GC added") : cU("Trace GC removed"));
                        mTracer->setTraceGC(aEnable, mOptionList);
                    }
                    else if (!STRNCMP((*aPtrAttr), cU("stack"), 2)) {
                        aTag = aTraceTag.addTag(cU("Message"));
                        aTag->addAttribute(cU("Type"), cU("Command"));
                        aTag->addAttribute(cU("Info"), aEnable? cU("Trace Stack added") : cU("Trace Stack removed"));
                        mTracer->setTraceStack(aEnable, mOptionList);
                    }
                    else if (!STRNCMP((*aPtrAttr), cU("thread"), 2)) {
                        aTag = aTraceTag.addTag(cU("Message"));
                        aTag->addAttribute(cU("Type"), cU("Command"));
                        aTag->addAttribute(cU("Info"), aEnable? cU("Trace Thread added") : cU("Trace Thread removed"));
                        mTracer->setTraceThread(aEnable, mOptionList);
                    }
                    else if (!STRNCMP((*aPtrAttr), cU("contention"), 2)) {
                        aTag = aTraceTag.addTag(cU("Message"));
                        aTag->addAttribute(cU("Type"), cU("Command"));
                        aTag->addAttribute(cU("Info"), aEnable? cU("Trace Contention added") : cU("Trace Contention removed"));
                        mMonitor->setTraceContention(aJvmti, aEnable, mOptionList);
                    }
                    else if (!STRNCMP((*aPtrAttr), cU("class"), 5)) {
                        aTag = aTraceTag.addTag(cU("Message"));
                        aTag->addAttribute(cU("Type"), cU("Command"));
                        aTag->addAttribute(cU("Info"), aEnable? cU("Trace Classes added") : cU("Trace Classes removed"));
                        mTracer->setTraceClass(aEnable, mOptionList);
                    }
                    else if (!STRNCMP((*aPtrAttr), cU("exception"),  2)) {
                        aTag = aTraceTag.addTag(cU("Message"));
                        aTag->addAttribute(cU("Type"), cU("Command"));
                        aTag->addAttribute(cU("Info"), aEnable? cU("Trace Exception added") : cU("Trace Exception removed"));
                        mTracer->setTraceException(aEnable, mOptionList);
                    }
                    else if (!STRNCMP((*aPtrAttr), cU("parameter"), 3)) {
                        aTag = aTraceTag.addTag(cU("Message"));
                        aTag->addAttribute(cU("Type"), cU("Command"));
                        aTag->addAttribute(cU("Info"), aEnable? cU("Trace Parameter added") : cU("Trace Parameter removed"));
                        mTracer->setTraceParameter(aEnable, mOptionList);
                    }
                    else if (!STRNCMP((*aPtrAttr), cU("verbose"), 2)) {
                        aTag = aTraceTag.addTag(cU("Message"));
                        aTag->addAttribute(cU("Type"), cU("Command"));
                        aTag->addAttribute(cU("Info"), aEnable? cU("Trace Verbose added") : cU("Trace Verbose removed"));
                        mTracer->setVerbose(aEnable);
                    }
                    else {
                        aSuccess = false;
                    }
                }

                if (!aSuccess) {
                    aTag = aTraceTag.addTag(cU("Message"));
                    aTag->addAttribute(cU("Type"), cU("Command"));
                    aTag->addAttribute(cU("Info"), cU("Command failed"));
                    *aCmd = COMMAND_ERR;
                }
                else { 
                    if (mTracer->doTraceException()) {
                        aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION, NULL);
                    }
                    else {
                        aResult = aJvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_EXCEPTION, NULL);
                    }
                }
                mMonitor->syncOutput(&aTraceTag);
                break;
            }
            case COMMAND_PASSWD_CHANGE: {
                TSecurity *aSecurity = TSecurity::getInstance();
                aSecurity->changePasswd();
                *aCmd = COMMAND_CONTINUE;
                break;
            }
            default: {
                TXmlTag aRootTag(cU("Message"));
                aRootTag.addAttribute(cU("Type"),  cU("Command"));
                aRootTag.addAttribute(cU("Info"),  *aPtrCmd);
                aRootTag.addAttribute(cU("Result"), cU("not implemented"));
                mMonitor->syncOutput(&aRootTag);
                *aCmd = COMMAND_ERR;
                break;
            }
        }
     }

     // -----------------------------------------------------------------
     // -----------------------------------------------------------------
     void setTraceOptions(jvmtiEnv *aJvmti) {
        TLogger *aLogger = TLogger::getInstance();
        TTracer *aTracer = TTracer::getInstance();
        TValues *aValues = mProperties->getTraceOptions();
        TValues  aTrcOptions(16);
        TValues::iterator aPtr;        
        jvmtiError   aResult;

        aTracer->setTraceMethod(false, NULL);
        aTracer->setTraceException(false, NULL);
        aTracer->setTraceParameter(false, NULL);
        aTracer->setTraceTrigger(false, NULL);
        aTracer->setTraceClass(false, NULL);
        aTracer->setTraceGC(false, NULL);
        aTracer->setTraceContention(false, NULL);
        aTracer->setTraceStack(false, NULL);
        mProperties->setDumpOnExit(false);

        for (aPtr  = aValues->begin(); 
             aPtr != aValues->end();
             aPtr  = aValues->next()) {

            TString aOpt(*aPtr);
            aOpt.split(&aTrcOptions, cU('-'));

            if (!STRNCMP(*aPtr, cU("exception"), 3)) {
                aTracer->setTraceException(true, &aTrcOptions);
            }
            else if (!STRNCMP(*aPtr, cU("method"),    3)) {
                aTracer->setTraceMethod(true, &aTrcOptions);
            }
            else if (!STRNCMP(*aPtr, cU("parameter"), 3)) {
                aTracer->setTraceParameter(true, &aTrcOptions);
            }
            else if (!STRNCMP(*aPtr, cU("trigger"), 3)) {
                aTracer->setTraceTrigger(true, &aTrcOptions);
            }
            else if (!STRNCMP(*aPtr, cU("class"),  3)) {
                aTracer->setTraceClass(true, &aTrcOptions);
            }
            else if (!STRNCMP(*aPtr, cU("gc"),  2)) {
                aTracer->setTraceGC(true, &aTrcOptions);
            }
            else if (!STRNCMP(*aPtr, cU("contention"),  4)) {
                mMonitor->setTraceContention(aJvmti, true, &aTrcOptions);
            }
            else if (!STRNCMP(*aPtr, cU("stack"),  4)) {
                aTracer->setTraceStack(true, &aTrcOptions);
            }
            else if (!STRNCMP(*aPtr, cU("append"), 4)) {
                aLogger->start(true);
            }
            else if (!STRNCMP(*aPtr, cU("dumpOnExit"), 4)) {
                mProperties->setDumpOnExit(true);
            }
        }

        if (mTracer->doTraceException()) {
            aResult = aJvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION, NULL);
        }
        else {
            aResult = aJvmti->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_EXCEPTION, NULL);
        }
    }

    // -----------------------------------------------------------------
    // Start tracing on a given thread
    //! \brief Start the trace on threads given by name or hash
    //!
    //! The tracer has all information about the thread. If the thread 
    //! name starts with "0x" this method assumes, that the hash code
    //! is used for search, else the Java thread name.
    //! \param aJni The Java native interface
    // -----------------------------------------------------------------
    void startThreadTrace(
            jvmtiEnv        *aJvmti,
            JNIEnv          *aJni) {

        jvmtiError       aResult;
        jint             aCnt;
        jthread         *jThreads = NULL;
        jvmtiThreadInfo  jInfo;
        TMonitorThread  *aThread  = NULL;
        TString          aName;
        TString          aInfo;
        bool             aUseHash = false;
        bool             aFound   = false;
        jlong            aID      = 0;

        if (!mTracer->doTraceThread()) {
            return;
        }

        aName = mTracer->getThreadName();
        if (aName.pcount() == 0) {
            return;
        }
        if (aName[0] == cU('0')) {
            aID      = aName.toInteger();
            aUseHash = true;
        }
        aResult  = aJvmti->GetAllThreads(&aCnt, &jThreads);

        for (jint i = 0; i < aCnt; i++) {
            aResult = aJvmti->GetThreadInfo(jThreads[i], &jInfo);
            aInfo.assignR(jInfo.name, STRLEN_A7(jInfo.name));
            aResult = aJvmti->Deallocate((unsigned char*)jInfo.name);
            aResult = aJvmti->GetThreadLocalStorage(jThreads[i], (void **)&aThread);

            if (aUseHash) {
                aFound = (aThread->getID() == aID);
            }
            else {
                aFound = STRCMP(aThread->getName(), aName.str()) == 0;
            }

            if (aFound) {
                mTracer->startTrigger(aThread->getName());
                break;
            }
        }
	    /*SAPUNICODEOK_CHARTYPE*/
        aResult = aJvmti->Deallocate((unsigned char*)jThreads);
    }
};

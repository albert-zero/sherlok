// -----------------------------------------------------------------
//
// Author: Albert Rossmann
// File  : console.h
// Date  : 14.04.2003
//! \file  console.h
//! \brief Console IO, logger and formatted trace output.
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

// -----------------------------------------------------------------
#ifndef CONSOLE_H
#define CONSOLE_H

#include <errno.h>
#include "cti.h"
#include "ptypes.h"

// ----------------------------------------------------
//! \class TLogger
//! \brief Handle log files and output
//!
// ----------------------------------------------------
class TLogger {
private:
    static TLogger *mInstance;      //!< Sigleton instance
    bool            mActive;        //!< Active logging
    SAP_ofstream    mFile;          //!< Active file handle
    TProperties    *mProperties;    //!< Global configuration
    // ----------------------------------------------------
    // TLogger::TLogger
    //! Constructor
    // ----------------------------------------------------
    TLogger() {
        mActive     = false;
        mProperties = TProperties::getInstance();

		if (mProperties->doLogging()) {
			enable(true);
		}
		SAP_UC aBuffer[64];
    }
    // ----------------------------------------------------
    // TLogger::TLogger
    //! Destructor
    // ----------------------------------------------------
    ~TLogger() {
        if (mInstance != NULL) {
            delete mInstance;
        }
    }
    // ----------------------------------------------------
    // TLogger::TLogger
    //! Copy Constructor
    // ----------------------------------------------------
    TLogger(const TLogger &) {
    }
    // ----------------------------------------------------
    // TLogger::operator=
    //! Copy Constructor
    // ----------------------------------------------------
    TLogger *operator=(const TLogger &) {
        return this;
    }
public:
    // ----------------------------------------------------
    // TLogger::getInstance
    //! Sigleton Constructor
    // ----------------------------------------------------
    static TLogger *getInstance() {
        if (mInstance == NULL) {
            mInstance = new TLogger();
        }
        return mInstance;
    }
    // ----------------------------------------------------
    // TLogger::start
    //! \brief Activate loggin
    //! \param aAppend \c TRUE to keep old log entries 
    // ----------------------------------------------------
    void start(bool aAppend = false) {
        if (!mActive) {
            enable(true, aAppend);
        }
    }
    // ----------------------------------------------------
    // TLogger::stop
    //! Deactivate loggin
    // ----------------------------------------------------
    void stop() {
        if (mActive) {
            enable(false);
        }
    }
private:
    // ----------------------------------------------------
    // TLogger::enable
    //! \brief Activate/Deactivate loggin
    //! \param aEnable \c TRUE to activate
    // ----------------------------------------------------
    void enable(bool aEnable, bool aAppend = false) {
        mActive = aEnable;
        if (mActive) {
            if (aAppend) {
                mFile.open(mProperties->getLogFile()->a7_str(), ios::app);
            }
            else {
                mFile.open(mProperties->getLogFile()->a7_str());
            }
            mFile   << cU("=== Sherlok log file created by ") 
                    << TSystem::getSystemTime() 
                    << cU(" ===") 
                    << endl;
        }
        else {
            mFile.flush();
            mFile.close();
        }
    }
public:
    // ----------------------------------------------------
    // TLogger::getStatus
    //! \brief Checks activation
    //! \return \c TRUE if logging is active
    // ----------------------------------------------------
    bool getStatus() {
        return mActive;
    }
    // ----------------------------------------------------
    // TLogger::print
    //! \brief Output to log file
    //! \param aBuffer String output
    // ----------------------------------------------------
    void print(const SAP_UC *aBuffer) {        
        if (mActive && mFile.rdbuf()->is_open()) {
            mFile << aBuffer << ends;  
            mFile.flush();
        }
    }
    // ----------------------------------------------------
    // TLogger::print
    //! \brief Output to log file and carriage return.
    //! \param aBuffer String output
    // ----------------------------------------------------
    void printLn(const SAP_UC *aBuffer) {        
        if (mActive && mFile.rdbuf()->is_open()) {
            mFile << aBuffer << endl;
        }
    }
};

// -----------------------------------------------------------------
//! \class TConsole
//! \brief Output to console
//!
// -----------------------------------------------------------------
class TConsole {
private:
    TProperties      *mProperties;  //!< Gloabal configuration
    static  TConsole *mInstance;    //!< Singleton instance
    bool    mEcho;                  //!< Server echo
    bool    mXmlStream;
    bool    mLoggedIn;
    bool    mPrintXmlHeader;
    TString mBuffer;                
    SAP_UC  mLine  [128];
    SAP_A7  mContBuffer[256];
    int     mContIndex;
    int     mContLen;
    TString mCtrl;                  //!< Cursor control
    int     mErrState;
    SOCKET  mSocket;                //!< Telnet socket
    SOCKET  mSocketFd;              //!< Telnet socket handle
    TString gSplash;                //!< Splash screen
    // -----------------------------------------------------------------
    // TConsole::TConsole
    //! Constructor
    // -----------------------------------------------------------------
    TConsole() {
        mProperties = TProperties::getInstance();
        mContIndex  = 0;
        mContLen    = 0;
        mSocket     = 0;
        mSocketFd   = 0;
        mErrState   = 0;
        mXmlStream  = false;
        mLoggedIn   = false;
        mTraceCallback  = NULL;
        mPrintXmlHeader = true;
        gSplash.concat(cU("\015\012\033[34;1m"));
        gSplash.concat(cU("  ***********************************************\015\012"));
        gSplash.concat(cU("  **********************************************\015\012"));
        gSplash.concat(cU("  ****\033[37;1m###\033[34;1m*******\033[37;1m####\033[34;1m*****\033[37;1m#######\033[34;1m**************\015\012"));
        gSplash.concat(cU("  **\033[37;1m##\033[34;1m***\033[37;1m##\033[34;1m****\033[37;1m##\033[34;1m**\033[37;1m##\033[34;1m****\033[37;1m##\033[34;1m****\033[37;1m##\033[34;1m************\015\012"));
        gSplash.concat(cU("  ***\033[37;1m##\033[34;1m*******\033[37;1m##\033[34;1m****\033[37;1m##\033[34;1m***\033[37;1m##\033[34;1m****\033[37;1m##\033[34;1m**********\015\012"));
        gSplash.concat(cU("  *****\033[37;1m##\033[34;1m*****\033[37;1m########\033[34;1m***\033[37;1m######\033[34;1m***********\015\012"));
        gSplash.concat(cU("  ******\033[37;1m##\033[34;1m****\033[37;1m##\033[34;1m****\033[37;1m##\033[34;1m***\033[37;1m##\033[34;1m*************\015\012"));
        gSplash.concat(cU("  **\033[37;1m##\033[34;1m***\033[37;1m##\033[34;1m**\033[37;1m##\033[34;1m******\033[37;1m##\033[34;1m**\033[37;1m##\033[34;1m************\015\012"));
        gSplash.concat(cU("  ****\033[37;1m###\033[34;1m****\033[37;1m##\033[34;1m******\033[37;1m##\033[34;1m**\033[37;1m##\033[34;1m**********\015\012"));
        gSplash.concat(cU("  **********************************\015\012"));
        gSplash.concat(cU("  ********************************\015\012\033[37;0m"));
        gSplash.concat(cU("\015\012"));
        gSplash.concat(cU("  Sherlok Telnet Adminstration\015\012  "));
    }
    // -----------------------------------------------------------------
    // TConsole::~TConsole
    //! Destructor
    // -----------------------------------------------------------------
    ~TConsole() {
        if (mInstance != NULL) {
            delete mInstance;
        }

    }
public:
    TCtiCallback mTraceCallback;
    // -----------------------------------------------------------------
    // TConsole::getInstance
    //! Singleton Constructor
    // -----------------------------------------------------------------
    static TConsole *getInstance() {
        if (mInstance == NULL) {
            mInstance = new TConsole();
        }
        return mInstance;
    }
    // -----------------------------------------------------------------
    // TConsole::checkState
    //! \brief Check connection to console
    //! \return \c TRUE if connection established
    // -----------------------------------------------------------------
    bool checkState() {
        return (mErrState > 0 && mLoggedIn);
    }
    bool isConnected() {
        return (mSocket != 0 && mSocketFd != 0);
    }
    void setTraceCallback(TCtiCallback aTraceCallback) {
        mTraceCallback = aTraceCallback;
    }
    // -----------------------------------------------------------------
    // TConsole::errorState
        // Console is not ready for I/O
    // -----------------------------------------------------------------
    bool errorState() {
        return (mSocket == 0 || mErrState <= 0 || mSocketFd == 0 || !mLoggedIn);
    }
    // -----------------------------------------------------------------
    // TConsole::opentPort
    //! \brief Establish socket administration to accept clients
    //! \return \c TRUE if socket listener established successfully
    // -----------------------------------------------------------------
    bool openPort() {
        DWORD  aResult;
        /*CCQ_IPV6_SUPPORT_OK*/
        struct sockaddr_in aServerAddr;
        TProperties *aProperties = TProperties::getInstance();

        if (mSocket > 0) {
            return true;
        }
#if defined (_WINDOWS)
        WORD    aVersionRequested;
        WSADATA aWsaData;

        aVersionRequested = MAKEWORD(2, 2);
        aResult = WSAStartup(aVersionRequested, &aWsaData);
        if (aResult != 0) {
            ERROR_OUT(cU("WSAStartup()"), aResult);
            return false;
        }
#endif
        // create socket
        /*CCQ_IPV6_SUPPORT_OK*/
        mSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (mSocket <= 0) {            
            ERROR_OUT(cU("create inet socket"), 0);
            return false;
        }
        // bind to port
        /*CCQ_IPV6_SUPPORT_OK*/
        aServerAddr.sin_family = AF_INET;
        /*CCQ_IPV6_SUPPORT_OK*/
        aServerAddr.sin_port   = htons(aProperties->getTelnetPort());

        aResult = 0;
        /*CCQ_IPV6_SUPPORT_OK*//*SAPUNICODEOK_LIBSTRUCT*//*SAPUNICODEOK_LIBFCT*/
        struct hostent *aHost = gethostbyname(aProperties->getTelnetHost());
        if (aHost == NULL) {
#if defined (_WINDOWS) 
            aResult = WSAGetLastError();
#endif
            ERROR_OUT(cU("gethostbyname"), aResult);
            return false;
        }
        /*SAPUNICODEOK_CHARTYPE*/
        char   aInetAddr[32];
        memsetR(aInetAddr, 0, 32);

#if defined (_WINDOWS) 
        /*SAPUNICODEOK_CHARTYPE*//*SAPUNICODEOK_LIBFCT*//*SAPUNICODEOK_SIZEOF*/
        sprintf_s(aInetAddr, sizeofR(aInetAddr), cR("%d.%d.%d.%d"),
            (unsigned char)aHost->h_addr_list[0][0],
            (unsigned char)aHost->h_addr_list[0][1],
            (unsigned char)aHost->h_addr_list[0][2],
            (unsigned char)aHost->h_addr_list[0][3]);
#else
        /*SAPUNICODEOK_CHARTYPE*//*SAPUNICODEOK_LIBFCT*//*SAPUNICODEOK_SIZEOF*/
        sprintf(aInetAddr, cR("%d.%d.%d.%d"),
            (unsigned char)aHost->h_addr_list[0][0],
            (unsigned char)aHost->h_addr_list[0][1],
            (unsigned char)aHost->h_addr_list[0][2],
            (unsigned char)aHost->h_addr_list[0][3]);
#endif

#if defined (SAPonNT) || defined (SAPonHPPA) 
        /*CCQ_IPV6_SUPPORT_OK*/
        aServerAddr.sin_addr.s_addr = inet_addr(aInetAddr);
        memsetR(&(aServerAddr.sin_zero), 0, 8);
#else
        /*CCQ_IPV6_SUPPORT_OK*/
        aResult = inet_pton(aServerAddr.sin_family, aInetAddr, reinterpret_cast<char *>(&(aServerAddr.sin_addr)));
        if (aResult != 1) {
            ERROR_OUT(cU("inet_pton"), (int)aResult);
            return false;
        }
        memsetR(&(aServerAddr.sin_zero), 0, sizeofR(aServerAddr.sin_zero));
#endif
        /*CCQ_IPV6_SUPPORT_OK*/
        aResult = bind(mSocket, reinterpret_cast<struct sockaddr *>(&aServerAddr), (int)sizeofR(struct sockaddr_in));
        if (aResult != 0) {
            ERROR_OUT(cU("bind socket to port"), (int)aResult);
            return false;
        }
        // start listener
        /*CCQ_IPV6_SUPPORT_OK*/
        aResult = listen(mSocket, 2);
        if (aResult != 0) {
            ERROR_OUT(cU("listen"), (int)aResult);
            return false;
        }
        return true;
    }
    // -----------------------------------------------------------------
    // TConsole::clrScreen
    //! Clear command line
    // -----------------------------------------------------------------
    void clrScreen() {
        if (!checkState()) {
            return;
        }
        SAP_A7 aClrScreen[72];
        memsetR(aClrScreen, cR(' '), 72);
        aClrScreen[0]  = cR('\015');
        aClrScreen[69] = cR('\015');
        aClrScreen[70] = cR('>');
        mErrState = (int)send(mSocketFd, aClrScreen, 72, 0);
        if (mErrState <= 0) ERROR_OUT(cU("connection closed"), mErrState);
    }
    // -----------------------------------------------------------------
    // TConsole::open
    //! \brief Accept client
    //! \return \c TRUE if client accepted
    // -----------------------------------------------------------------
    bool exitConnection() {
        if (mSocketFd != 0) {
           if (mXmlStream) {
               // print(cU("</sherlok>\n"));
               mXmlStream = false;
            }
            mErrState = 0;
            CLOSESOCKET(mSocketFd);
            mSocketFd  = 0;
        }
        mLoggedIn = false;
        mErrState = 0;
        return true;
    }

    // -----------------------------------------------------------------
    // TConsole::open
    //! \brief Accept client
    //! \return \c TRUE if client accepted
    // -----------------------------------------------------------------
    bool open() {
        exitConnection();
        if (mSocket <= 0) {
            mErrState = 0;
            return false;
        }
        SAPSOCKLEN_T aSize = sizeofR(struct sockaddr);
        /*CCQ_IPV6_SUPPORT_OK*/
        struct sockaddr_in aClientAddr;

        mErrState = 0;
        for (int i = 0; i < 10; i++) {
            /*CCQ_IPV6_SUPPORT_OK*/
            mSocketFd = accept(mSocket, reinterpret_cast<struct sockaddr *>(&aClientAddr), &aSize);
            if (mSocketFd <= 0) {
                if (errno == EINTR) {
                    continue;
                }
            } 
            break;
        }

        if (mSocketFd <= 0) {
            ERROR_OUT(cU("accept"), errno);
            return false;
        }  
        getVersion(false);
        return true;
    }

    // -----------------------------------------------------------------
    // TConsole::login
    //! \brief Send the intro
    // -----------------------------------------------------------------
    void login() {
        mErrState = 1; 
        if (mProperties->getConsoleWriterType() == XMLWRITER_TYPE_XML) {
            mXmlStream = true;
        }
        mLoggedIn = true;
    }
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    
    // -----------------------------------------------------------------
    // TConsole::getVersion
    //! Output of version string to console
    // -----------------------------------------------------------------
    void getVersion(bool aExtended = true) {
        if (!isConnected()   || 
            !doCmdlineEcho() || 
             mProperties->getConsoleWriterType() == XMLWRITER_TYPE_XML) {
            return;
        }

        TString aVersion(mProperties->getVersion(aExtended));
        mErrState = (int)send(mSocketFd, gSplash.a7_str(), gSplash.pcount(), 0);
        if (mErrState <= 0) { ERROR_OUT(cU("connection closed"), mErrState); return; }
    
        mErrState = (int)send(mSocketFd, aVersion.a7_str(), aVersion.pcount(), 0);
        if (mErrState <= 0) { ERROR_OUT(cU("connection closed"), mErrState); return; }
        
        mErrState = (int)send(mSocketFd, cR("\015\012\015\012"), 4, 0);
        if (mErrState <= 0) { ERROR_OUT(cU("connection closed"), mErrState); return; }
        
        prompt();
    }
    // -----------------------------------------------------------------
    // TConsole::close
    //! Close socket
    // -----------------------------------------------------------------
    void close() {
        if (mXmlStream) {
            mXmlStream = false;
        }

        if (mSocket   != 0) 
            CLOSESOCKET(mSocket);

        if (mSocketFd != 0) 
            CLOSESOCKET(mSocketFd);

        mContBuffer[0] = 0;
        mContLen       = 0;
        mContIndex     = 0;
        mSocket        = 0;
        mSocketFd      = 0;
    }
    // -----------------------------------------------------------------
    // TConsole::read
    //! \brief  Socket input
    //! \return The character input without control sequences
    // -----------------------------------------------------------------
    TString *read() {
        mBuffer = cU("exit");

        if (!isConnected()) {
            return &mBuffer;
        }
        SAP_A7  aBuffer   [128];
        SAP_UC  aBufferCmd[128];
        SAP_UC  aBufferCtr[128];
        int     i, j;

        memsetR(aBuffer,    0, 128);
        memsetU(aBufferCmd, 0, 128);
        memsetU(aBufferCtr, 0, 128);

        if (mContIndex >= mContLen) {
            mContIndex = 0;
            mContLen   = (int)recv(mSocketFd, mContBuffer, 127, 0);
        }

        if (mContLen <= 0) { 
            // ERROR_OUT(cU("connection closed"), mErrState); 
            return &mBuffer;
        }
        
        // extract input from control
        i = 0;
        j = 0;
        for (i = 0; i < mContLen; i++) {
            if (mContBuffer[i] > -1 && mContBuffer[i] < 255) {
                /*SAPUNICODEOK_CONVERSION*/ mContBuffer[j++] = mContBuffer[i];
            }
        }
        aBufferCmd[j] = 0;
        mContLen      = j;

        i = 0;
        while (mContIndex < mContLen && isprint(mContBuffer[mContIndex])) {
            /*SAPUNICODEOK_CONVERSION*/ aBufferCmd[i++] = mContBuffer[mContIndex++];
        }
        aBufferCmd[i] = 0;

        i = 0;
        while ((mContIndex < mContLen) && !isprint(mContBuffer[mContIndex])) {
            if (mContBuffer[mContIndex] == 27) {
                /*SAPUNICODEOK_CONVERSION*/ aBufferCtr[i++] = mContBuffer[mContIndex++];
                /*SAPUNICODEOK_CONVERSION*/ aBufferCtr[i++] = mContBuffer[mContIndex++];
                /*SAPUNICODEOK_CONVERSION*/ aBufferCtr[i++] = mContBuffer[mContIndex++];    
            }
            else {
                /*SAPUNICODEOK_CONVERSION*/ aBufferCtr[i++] = mContBuffer[mContIndex++];
            }
        }
        aBufferCtr[i] = 0;

        mBuffer = aBufferCmd;
        mCtrl   = aBufferCtr;

        return &mBuffer;
    }
    // -----------------------------------------------------------------
    // TConsole::getCtrl
    //! \brief  Socket input
    //! \return The control sequence of the last input
    // -----------------------------------------------------------------
    TString *getCtrl() {
        return &mCtrl;
    }
    // -----------------------------------------------------------------
    // TConsole::print
    //! \brief  Socket output
    //! \param aBuffer The string for output
    //! \param aCnt    The maximal number of characters to print. 
    //!                For aCnt = 0 the method evaluates the string length.
    // -----------------------------------------------------------------
    void print(const SAP_UC *aBuffer, int aCnt = 0, bool aForce = false) {
        const SAP_UC *aPtrBeg;
        const SAP_UC *aPtrEnd;
        int   aLen = 0;
        bool  aOutput = !errorState();

        if (aBuffer == NULL) {
            return;
        }
        if (aForce && isConnected()) {
            aOutput = true;
        }
        
        if (mPrintXmlHeader && mProperties->getConsoleWriterType() == XMLWRITER_TYPE_XML) {
            mPrintXmlHeader = false;
            print(  cU("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
                    cU("<sherlok>\n")
                    cU("<Message Info=\"Connected\"/>\n"), 0, true);
        }

        if (aOutput) {
            if (aCnt == 0) {
                /*SAPUNICODEOK_SIZEOF*/
                aCnt = (int)STRLEN(aBuffer);
            }

            if (aCnt > 0) {
                aPtrBeg = aBuffer;
                aPtrEnd = aPtrBeg;

                while (mErrState > 0) {
                    if (*aPtrEnd == cU('\0')) {
                        // mErrState = (int)send(mSocketFd, (const char *)cR("\0"), 1, 0);
                        break;
                    }
                    else if (*aPtrEnd == cU('\n')) {
                        mErrState = (int)send(mSocketFd, /*SAPUNICODEOK_CHARTYPE*/(const char *)cR("\015\012"), 2, 0);
                        // mErrState = (int)send(mSocketFd, (const char *)cR("\n"), 1, 0);
                        aPtrEnd ++;
                    }                    
                    else {
                        TString aStr(aPtrEnd);
                        int aPos  = (int)aStr.findFirstOf(cU('\n'));
                    
                        if (aPos < 0) {
                            aPos  = aStr.pcount();
                        }
                        mErrState = (int)send(mSocketFd, /*SAPUNICODEOK_CHARTYPE*/(const char *)aStr.a7_str(), aPos, 0);
                        aPtrEnd  += (int)aPos;
                        aLen     += (int)aPos;
                    }
                }
            }
        }
        else {
            SAP_cerr << aBuffer << ends;
            SAP_cerr.flush();
        } 
    }
    // -----------------------------------------------------------------
    // TConsole::printLn
    //! \brief  Socket output and carriage return
    //! \param aBuffer The string for output
    // -----------------------------------------------------------------
    void printLn(const SAP_UC *aBuffer) {
        if (aBuffer != NULL) {
            this->print(aBuffer);
        }
        this->print(cU("\015\012"));
    }
    // -----------------------------------------------------------------
    // TConsole::print
    //! \brief  Socket output
    //! \param aValue A long integer for output
    // -----------------------------------------------------------------
    void print(long aValue) {
        union TSendData { 
            /*SAPUNICODEOK_SIZEOF*/
            SAP_UC aBuffer[sizeofR(long)];
            long aValue; 
        } aData;

        aData.aValue = htonl(aValue);
        /*SAPUNICODEOK_SIZEOF*/
        print(aData.aBuffer, (int)sizeofR(long));
    }
    // -----------------------------------------------------------------
    // TConsole::echo
    //! \brief Determins if client or server does the echo of keybord.
    //! \param aEnable \c TRUE if server does echo.
    // -----------------------------------------------------------------
    void setEcho(bool aEnable) {
        mEcho = aEnable;
    }
    // -----------------------------------------------------------------
    // TConsole::doCmdlineEcho
    //! \brief Check echo settings
    //!
    //! Echo makes only sense, if console is of type ASCII
    //! \return \c TRUE if console echo is possible
    // -----------------------------------------------------------------
    bool doCmdlineEcho() {
        return (mProperties->getConsoleWriterType() == XMLWRITER_TYPE_ASCII);
    }
    // -----------------------------------------------------------------
    // TConsole::moveCursor
    //! \brief Moves the cursor within in a command line
    //!
    //! For positive values aCnt the cursor moves right, for negatives left.
    //! If aBsp is \c TRUE the characters are removed.
    //! \param aCmdLine The current command line
    //! \param aCnt Number of characters to move
    //! \param aBsp \c TRUE if characters before cursor should be removed.
    // -----------------------------------------------------------------
    void moveCursor(TString *aCmdLine, int aCnt, bool aBsp = true) {
        if (!checkState()) {
            return;
        }

        if (aCnt == 0 || aCmdLine->strInsert() == NULL) {
            return;
        }
        int aInsPos = aCmdLine->getInsertPos();;
        int aLenStr = aCmdLine->pcount();
        int aLenIns;
        if (aCnt < 0 && aInsPos > 0) {
            aCmdLine->moveCursor(aCnt);
            if (aBsp) {
                print(cU("\010"));
            }
            print(aCmdLine->strInsert());
            print(cU(" "));
            aLenIns = STRLEN(aCmdLine->strInsert());
            for (int i = 0; i < aLenIns + 1; i++) {
                print(cU("\010"));
            }
            return;
        }

        if (aCnt > 0 && aInsPos < aLenStr) {
            print(aCmdLine->strInsert());
            aCmdLine->moveCursor(aCnt);
            aLenIns = STRLEN(aCmdLine->strInsert());
            for (int i = 0; i < aLenIns; i++) {
                print(cU("\010"));
            }
            return;
        }        
    }
    // -----------------------------------------------------------------
    // TConsole::echoInsert
    //! \brief Insert a string and echo the result.
    //! \param aLine The string to insert.
    // -----------------------------------------------------------------
    void echoInsert(TString *aLine) {
        if (!checkState()|| aLine->strInsert() == NULL) {
            return;
        }
        if (mBuffer.pcount() > 0) {
            if (mEcho) {
                print(mBuffer.str());
            }
            int aLenIns = STRLEN(aLine->strInsert());
            if (aLenIns > 0) {
                print(aLine->strInsert());
                for (int i = 0; i < aLenIns; i++) {
                    print(cU("\010"));
                }
            }
        }
    }
    // -----------------------------------------------------------------
    // TConsole::prompt
    //! Print the prompt
    // -----------------------------------------------------------------
    void prompt() {
        if (!checkState()) {
            return;
        }
        if (mProperties->getConsoleWriterType() != XMLWRITER_TYPE_XML) {
            //--print(cU("\033[1;20;r\033[21;1;H>"));
            print(cU("> "));
        }
    }
    // -----------------------------------------------------------------
    // TConsole::getStatus
    //! \brief Checks the console
    //! \return \c TRUE if the socket is open 
    // -----------------------------------------------------------------
    bool getStatus() {
        return mSocketFd > 0;
    }
    // -----------------------------------------------------------------
    // TConsole::backspace
    //! \brief Remove the character left of the cursor.
    //! \param aLine The current command line string to execute the operation
    // -----------------------------------------------------------------
    void backspace(TString *aLine) {
        if (!checkState()) {
            return;
        }
        aLine->backspace();
        moveCursor(aLine, -1, mEcho);
    }
};

// -----------------------------------------------------------------
//! \class TWriter
//! \brief Writer for simultanious output to console and log file
// -----------------------------------------------------------------
class TWriter {
private:
    static TWriter *mInstance;      //!< Singelton instance
    TConsole *mConsole;             //!< Console instance
    TLogger  *mLogger;              //!< Logger instance
    bool      mConsoleOutput;       

    // -----------------------------------------------------------------
    // TWriter::TWriter
    //! Constructor
    // -----------------------------------------------------------------
    TWriter() {
        mConsole = TConsole::getInstance();
        mLogger  = TLogger::getInstance();
        mConsoleOutput = true;
    }
public:    
    // -----------------------------------------------------------------
    // TWriter::getInstance
    //! Singleton Constructor
    // -----------------------------------------------------------------
    static TWriter *getInstance() {
        if (mInstance == NULL) {
            mInstance = new TWriter();
        }
        return mInstance;
    }
    // -----------------------------------------------------------------
    // TWriter::setConsoleOutput
    //! \brief Set the output option for console
    //! \param aEnable \c TRUE enables conole output
    // -----------------------------------------------------------------
    void setConsoleOutput(bool aEnable) {
        mConsoleOutput = aEnable;
    }
    // -----------------------------------------------------------------
    // TWriter::print
    //! \brief Output
    //! \param aBuffer The string to write to console and logger
    // -----------------------------------------------------------------
    void print(const SAP_UC *aBuffer, bool aForce = false) {
        if (mConsoleOutput)
            mConsole->print(aBuffer, 0, aForce);
        mLogger->print(aBuffer);
    }
    // -----------------------------------------------------------------
    // TWriter::printLn
    //! \brief Output and carriage return
    //! \param aBuffer The string to write to console and logger
    // -----------------------------------------------------------------
    void printLn(const SAP_UC *aBuffer) {
        if (mConsoleOutput)
            mConsole->printLn(aBuffer);
        mLogger->printLn(aBuffer);
    }
};

// ----------------------------------------------------------------
//! \class TXmlWriter
//! \brief Formatted output
//!
// ----------------------------------------------------------------
class TXmlWriter {
private:
    SAP_stringstream mStream;   //!< Output string  
    SAP_ofstream *mFile;        //!< Output file
    bool          mBuffered;    //!< Store more than one line in mStream
    int           mLine;        //!< Line numbering
    bool          mDoHeader;    //!< Output of table header
    SAP_UC        mIndent[128]; //!< Blanks for indent
    SAP_UC        mTabstr[128]; //!< Tabulators
    int           mLevel;       //!< Indent level
    int           mDepth;       //!< Recursion level
    int           mTabPos;      //!< Tabulator position
    int           mTabLine;     //!< Tabulator line
    int           mOutputType;  //!< Output type
    int           mMaxLines;    //!< Maximal number of output lines
    TLogger      *mLogger;      //!< TLogger instance
    TConsole     *mConsole;     //!< TConsole instance
    TProperties  *mProperties;  //!< Global configuration
    TString       mPrefix;

    // ----------------------------------------------------
    // TXmlWriter::TXmlWriter
    //! Copy Constructor
    // ----------------------------------------------------
    TXmlWriter(const TXmlWriter&) {
    }
    // ----------------------------------------------------
    // TXmlWriter::operator=
    //! Copy Constructor
    // ----------------------------------------------------
    TXmlWriter *operator=(const TXmlWriter&) {
        return this;
    }
    // ----------------------------------------------------------------
    // TXmlWriter::endline
    //! \brief End one line of output
    //!
    //! If buffered the string will be stored, else
    //! direct output is triggered and the string is reset
    // ----------------------------------------------------------------
    void flushBuffer() {
        mStream << ends;

        if (TConsole::getInstance()->mTraceCallback != NULL) {
            TString aTrace(mStream.str().c_str());
            TConsole::getInstance()->mTraceCallback( aTrace.a7_str() );
        }

        if (mFile != NULL &&
            mFile->rdbuf()->is_open()) {
           *mFile << mStream.str();
            mFile->flush();
        }
        else {
            TWriter::getInstance()->print(mStream.str().c_str());
        }
        mStream.str(cU(""));
    }
    // ----------------------------------------------------------------
    // ----------------------------------------------------------------
    void endline() {
        mLine ++;        
        if (mLine >= mMaxLines) {
            return;
        }
        mStream << endl;
        if (mBuffered) {
            return;
        }
        flushBuffer();
    }
    // ----------------------------------------------------------------
    // TXmlWriter::getIndent
    //! \brief Calculates spaces for indent
    //! \return An array of spaces for formatted output
    // ----------------------------------------------------------------
    SAP_UC *getIndent() {
        memsetU(mIndent, cU(' '), 128);
        mIndent[min(127, mLevel * 2)] = cU('\0');
        return mIndent;
    }
    // ----------------------------------------------------------------
    // TXmlWriter::resetTab
    //! \brief Reset tabulator
    //! 
    // ----------------------------------------------------------------
    void resetTab() {
        mTabLine = 0;
        mTabPos  = 0;
    }
    // ----------------------------------------------------------------
    // TXmlWriter::getTab
    //! \brief Get tabulator at specified index
    //! \param aTabInx The length of the current column
    //! \param aCount  The bytes of text to copy into the current column
    //! \retrun Spaces to add for formatted output
    // ----------------------------------------------------------------
    SAP_UC *getTab(int aTabInx, int aCount) {
        int aNrBlanks;
        int aNrLetters;


        aNrLetters = mTabLine + aCount - mTabPos;
        mTabPos   += aTabInx;
        aNrBlanks  = aTabInx - aNrLetters;

        if (aNrBlanks < 0) {
            aNrBlanks = 0;
        }
        else {
            memsetU(mTabstr, cU(' '), 128);
        }
        mTabstr[aNrBlanks] = cU('\0');
        mTabLine += (aCount + aNrBlanks);
        return mTabstr;
    }
public:
    // ----------------------------------------------------------------
    // TXmlWriter::TXmlWriter
    //! \brief Constructor
    //! \param aOutputType one of 
    //!          -  XMLWRITER_TYPE_XML, 
    //!          -  XMLWRITER_TYPE_HTML
    //!          -  XMLWRITER_TYPE_ASCII
    //! \param aBuffered \c TRUE to enable buffered output
    //! \param aFile     The log file for direct output
    // ----------------------------------------------------------------
    TXmlWriter(
            int  aOutputType    = XMLWRITER_TYPE_ASCII, 
            bool aBuffered      = false,
            SAP_ofstream *aFile = NULL) {

        mProperties = TProperties::getInstance();
        mConsole    = TConsole::getInstance();
        mLogger     = TLogger::getInstance();
        mBuffered   = aBuffered;
        mDoHeader   = true;
        mLine       = 0;
        mLevel      = 1;
        mDepth      = 0;
        mTabPos     = 0;
        mTabLine    = 0;
        mMaxLines   = 10000;
        mOutputType = aOutputType;
        mFile       = aFile;
        mPrefix     = cU("");

        if (mProperties->getConsoleWriterType() == XMLWRITER_TYPE_XML &&
            mOutputType != XMLWRITER_TYPE_PROPERTY) {
            mOutputType  = XMLWRITER_TYPE_XML;
        }
    }
    // ----------------------------------------------------------------
    // TXmlWriter::~TXmlWriter
    //! Destructor
    // ----------------------------------------------------------------
    ~TXmlWriter() {
    }
    // ----------------------------------------------------------------
    // TXmlWriter::setType
    //! \brief Change the output type
    //! \param aOutputType 
    //!          -  XMLWRITER_TYPE_XML, 
    //!          -  XMLWRITER_TYPE_HTML
    //!          -  XMLWRITER_TYPE_ASCII
    // ----------------------------------------------------------------
    void setType(int aOutputType) {
        mOutputType = aOutputType;
    }
    // ----------------------------------------------------------------
    // TXmlWriter::printTrace
    //! \brief Output of a list of attributes.
    //!
    //! It is possible to pass a prefix for LINE output
    //! \param aTag     Tree structure for output
    //! \param aPrefix  Output prefix for trace line output
    //! \param aLevel   Start indent level
    //! \param aFinish  Send close tag to output
    // ----------------------------------------------------------------
    void printTrace(
        TXmlTag *aTag, 
        const SAP_UC  *aPrefix = cU(""),
        int      aLevel  = 0,
        bool     aFinish = false) {
        
        mPrefix = aPrefix;
        if (STRCMP(aPrefix, cU(""))) {
            mPrefix.concat(mProperties->getSeparator());
        }
        mLevel = aLevel;
        this->print(aTag);
        mPrefix = cU("");
    }
    // ----------------------------------------------------------------
    // TXmlWriter::print
    //! \brief Output of a list of attributes.
    //! \param aTag     Tree structure for output
    // ----------------------------------------------------------------
    void print(TXmlTag *aTag, int aRequestType = -1) {

        // Enforce XML for all output, if set in configuration
        int aSaveType   = -1;  
        int aOutputType = mProperties->getConsoleWriterType();

        if (aOutputType == XMLWRITER_TYPE_XML) {
            if (mOutputType != XMLWRITER_TYPE_PROPERTY) {
                aSaveType    = mOutputType;
                mOutputType  = XMLWRITER_TYPE_XML;
            }
            mLevel = 1;
        }
        else {
            if (aRequestType != -1) {
                aSaveType   = mOutputType;
                mOutputType = aRequestType;
            }
        }

        mLine = 0;
        if (mOutputType == XMLWRITER_TYPE_HTML) {
            mStream << cU("<table>");
            endline();
        }
        printTag(aTag);
        if (mOutputType == XMLWRITER_TYPE_HTML) {
            mStream << cU("</table>");
            endline();
        }

        if (mBuffered) {
            flushBuffer();
        }

        // Reset the output type
        if (aSaveType != -1) {
            mOutputType = aSaveType;
        }
        mLevel = 1;
    }
private:
    // ----------------------------------------------------------------
    // TXmlWriter::printTag
    //! \brief Output of a list of attributes.
    //! \param aTag     Tree structure for output
    // ----------------------------------------------------------------
    void printTag(TXmlTag *aTag) {
        TXmlTag::TTagList *aTagList = aTag->getTagList();
        TXmlTag::TTagList::iterator aPtr;

        startTag(aTag);
        for (aPtr  = aTagList->begin();
             aPtr != aTagList->end();
             aPtr  = aPtr->mNext) {
            printTag(aPtr->mElement);
        }
        endTag(aTag);
    }
    // ----------------------------------------------------------------
    // TXmlWriter::startTag
    //! \brief Output of a single attribute.
    //!
    //! \param aTag     Tree structure for output
    // ----------------------------------------------------------------
    void startTag(TXmlTag *aTag) {
        TListAttribute::iterator aPtr;
        TXmlTable        aLocalTable(8);
        TXmlTable       *aTable = aTag->getTable();
        TProperty       *aProperty;
        TProperties     *aProperties = TProperties::getInstance();
        const SAP_UC    *aKey;
        const SAP_UC    *aValue;
        int              aLen;
        bool             bHasInfo   = false;
        const SAP_UC    *aSeparator = aProperties->getSeparator();


        if (aTable == NULL) {
            aTable = &aLocalTable;
        }
        aTable->nextRow();
        
        switch (mOutputType) {
            // -------------------------------------------------------------
            // XMLWRITER_TYPE_HTML
            // Output of the TXmlTag tree into a HTML table
            // - outer table for node description
            // - inner table for values
            // -------------------------------------------------------------
            case XMLWRITER_TYPE_HTML:
                if (aTag->getType() == XMLTAG_TYPE_NODE) {
                    mStream << cU("<tr><td>");
                    mStream << cU("<table class=\"gSAPTable\"><tr class=\"gSAPTr\">");
                    mStream << cU("<th class=\"gSAPTh\">") 
                            << aTag->getElement();

                    for (aPtr  = aTag->getAttributes()->begin();
                         aPtr != aTag->getAttributes()->end();
                         aPtr ++) {
                        aProperty = (*aPtr);
                        mStream  << cU(": ") << aProperty->getValue();
                    }
                    mStream << cU("</th>");
                    mStream << cU("</tr><tr class=\"gSAPTr\"><td class=\"gSAPTd\"><table border=\"1\" class=\"gSAPTable\">");
                    endline();
                    mLevel++;
                    mDoHeader = true;
                    break;
                }
                if (aTag->getType() == XMLTAG_TYPE_LEAVE) {
                    // the header line
                    if (mDoHeader) {
                        mDoHeader = false;
                        mStream << cU("<tr class=\"gSAPTr\">");
                        for (aPtr  = aTag->getAttributes()->begin();
                             aPtr != aTag->getAttributes()->end();
                             aPtr ++) {
                            aProperty = (*aPtr);
                            if ((aProperty->getType() & PROPERTY_TYPE_HIDDEN) != 0) {
                                continue;
                            }
                            mStream << cU("<th class=\"gSAPTh\">") << aProperty->getKey() << cU("</th>");
                        }
                        mStream << cU("</tr>");
                    }
                    // repeat the header line
                    endline(); 

                    if (mLine % 20 == 0) {
                        // mDoHeader = true;
                    }
                    mStream << getIndent() << cU("<tr class=\"gSAPTr\">");
                    for (aPtr  = aTag->getAttributes()->begin();
                         aPtr != aTag->getAttributes()->end();
                         aPtr ++) {

                        aProperty = (*aPtr);
                        if ((aProperty->getType() & PROPERTY_TYPE_HIDDEN) != 0) {
                            continue;
                        }
                        if (aProperty->getInfo() != NULL) {
                            mStream << cU("<td class=\"gSAPTd\"><a href=\"") 
                                    << aProperty->getInfo() 
                                    << cU("\" target=\"blank\">")
                                    << aProperty->getValue()
                                    << cU("</a></td>");
                        }
                        else {
                            if ((aProperty->getType() & PROPERTY_TYPE_INT) != 0) {
                                mStream << cU("<td class=\"gSAPTd\" align=\"right\">") 
                                        << aProperty->getValue();
                                if ((aProperty->getType() & PROPERTY_TYPE_MICROSEC) != 0)
                                    mStream << cU(" [s/1000000]");
                                mStream << cU("</td>");
                            }
                            else {
                                mStream << cU("<td class=\"gSAPTd\">") 
                                        << aProperty->getValue() << cU("</td>");
                            }
                        }
                    }
                    mStream << cU("</tr>");
                    endline();
                }
                break;

            // -------------------------------------------------------------
            // XMLWRITER_TYPE_XML
            // Output of the TXmlTag tree into an XML schema
            // -------------------------------------------------------------
            case XMLWRITER_TYPE_XML:
                mStream << getIndent();
                mStream << cU("<") << aTag->getElement();
                for (aPtr  = aTag->getAttributes()->begin();
                     aPtr != aTag->getAttributes()->end();
                     aPtr ++) {
                    aProperty = (*aPtr);
                    TString aValueXml(aProperty->getValue());
                    aValueXml.encodeXML();
                    mStream << cU(" ") << aProperty->getKey() << cU("=\"") << aValueXml.str() << cU("\"");
                }
                
                if (aTag->getType() == XMLTAG_TYPE_NODE) {
                    mStream << cU(">");
                    mLevel ++;
                }
                else {
                    mStream << cU("/>");
                }
                endline(); 
                break;

            // -------------------------------------------------------------
            // XMLWRITER_TYPE_ASCII
            // Output of the TXmlTag tree as formatted text
            // -------------------------------------------------------------
            case XMLWRITER_TYPE_ASCII:                
                mStream << getIndent();
                resetTab();
                if (aTag->getType() == XMLTAG_TYPE_NODE) {
                    for (aPtr  = aTag->getAttributes()->begin();
                         aPtr != aTag->getAttributes()->end();
                         aPtr ++) {
                        bHasInfo  = true;
                        aProperty = (*aPtr);
                        if ((aProperty->getType() & PROPERTY_TYPE_HIDDEN) != 0) {
                            continue;
                        }
                        mStream  << aProperty->getValue() << cU(" ");
                    }
                    if (!bHasInfo) {
                        mStream << aTag->getElement();
                    }
                    // mStream << ":"; 
                    endline();
                    mDoHeader = true;
                    mLevel++;
                    break;
                }
                
                if (mLine == 0) {
                    mDoHeader = true;
                }
                if (mDoHeader) {
                    int i;
                    mDoHeader = false;
                    //for (i = 0; i < 23 * aTag->getAttributes()->getDepth(); i++) mStream << "-";
                    for (i = 0; i < aTable->getRowSize(); i++) mStream << cU("-");
                    endline();
                    mStream << getIndent();

                    for (aPtr  = aTag->getAttributes()->begin();
                         aPtr != aTag->getAttributes()->end();
                         aPtr ++) {
                        aProperty = (*aPtr);
                        if ((aProperty->getType() & PROPERTY_TYPE_HIDDEN) != 0) {
                            continue;
                        }
                        aKey = aProperty->getKey();
                        if (aKey == NULL) {
                            aKey = cU("null");
                        }
                        aTable->setActColumnSize(STRLEN(aKey));
                        mStream << cU("| ") << aProperty->getKey() 
                                << getTab(aTable->getActColumnSize(), STRLEN(aKey));
                        aTable->nextColumn();
                    }
                    aTable->nextRow();
                    endline();
                    mStream << getIndent();
                    for (i = 0; i < aTable->getRowSize(); i++) mStream << cU("-");
                    endline();
                    mStream    << getIndent();
                }

                resetTab();
                for (aPtr  = aTag->getAttributes()->begin();
                     aPtr != aTag->getAttributes()->end();
                     aPtr ++) {
                    aProperty = (*aPtr);
                    if ((aProperty->getType() & PROPERTY_TYPE_HIDDEN) != 0) {
                        continue;
                    }
                    aValue = aProperty->getValue();
                    if (aValue == NULL) {
                        aValue = cU("null");
                    }
                    if ((aProperty->getType() & PROPERTY_TYPE_INT) != 0) {
                        mStream << aSeparator << getTab(aTable->getActColumnSize(), STRLEN(aValue)) << aValue;
                    }
                    else {
                        mStream << aSeparator << aValue << getTab(aTable->getActColumnSize(), STRLEN(aValue));
                    }
                    aTable->nextColumn();
                }
                endline(); 
                aTable->nextRow();
                break;

            // -------------------------------------------------------------
            // XMLWRITER_TYPE_TREE
            // Output of values in a tree
            // -------------------------------------------------------------
            case XMLWRITER_TYPE_TREE:
                resetTab();
                if (aTag->getType() == XMLTAG_TYPE_NODE) {
                    return;
                }
                mStream << getIndent();

                for (aPtr  = aTag->getAttributes()->begin();
                     aPtr != aTag->getAttributes()->end();
                     aPtr ++) {

                    aProperty = (*aPtr);
                    aKey      = aProperty->getKey();
                    aValue    = aProperty->getValue();
                    
                    if (aKey == NULL) {
                        aKey = cU(" ");
                        continue;
                    }
                    
                    if (!STRNCMP(aKey, cU("Type"), 5)) {
                        continue;
                    }
                    
                    if (aValue == NULL) {
                        aValue = cU(" ");
                        continue;
                    }
                    mStream << aSeparator;
                    if (!aProperties->getComprLine()) {
                        mStream << aKey << cU("=");
                    }
                    mStream << aValue;
                }
                endline();
                mLevel++;
                break;

            // -------------------------------------------------------------
            // XMLWRITER_TYPE_LINE
            // Output of all values in a row
            // -------------------------------------------------------------
            case XMLWRITER_TYPE_LINE:
                resetTab();
                if (aTag->getType() == XMLTAG_TYPE_NODE) {
                    if (aTag->getAttributes()->getSize() == 0) {
                        return;
                    }
                    mStream << mPrefix.str()
                            << aTag->getElement();
                }
                else {
                    mStream << mPrefix.str()
                            << aTag->getParent()->getElement() << aSeparator
                            << aTag->getElement();
                }

                for (aPtr  = aTag->getAttributes()->begin();
                     aPtr != aTag->getAttributes()->end();
                     aPtr ++) {
                    aProperty = (*aPtr);
                    aKey      = aProperty->getKey();
                    aValue    = aProperty->getValue();

                    if (aKey == NULL) {
                        aKey = cU("null");
                    }
                    if (aValue == NULL) {
                        aValue = cU("null");
                    }
                    aLen = STRLEN(aValue);
                    mStream << aSeparator;
                    if (!aProperties->getComprLine()) {
                        mStream << aKey << cU("=");
                        aLen += STRLEN(aKey) + 1;
                    }
                    mStream << aValue
                            << getTab(aTable->getActColumnSize(), aLen);
                    aTable->nextColumn(aProperty);
                }
                aTable->nextRow();
                endline();
                break;

            // -------------------------------------------------------------
            // XMLWRITER_TYPE_PROPERTY
            // -------------------------------------------------------------
            case XMLWRITER_TYPE_PROPERTY:
                resetTab();
                if (aTag->getType() == XMLTAG_TYPE_NODE) {
                    return;
                }
                aPtr = aTag->getAttributes()->begin();
                if (aPtr == aTag->getAttributes()->end()) {
                    return;
                }
                mStream << (*aPtr)->getValue() << cU("=");
                aPtr ++;
                if (aPtr == aTag->getAttributes()->end()) {
                    return;
                }
                mStream << (*aPtr)->getValue();
                aTable->nextRow();
                endline();
                break;
        }
    }
    // ----------------------------------------------------------------
    // TXmlWriter::endTag
    // Close TXmlTag tree structures
    //! \brief Create closing tag for XML and HTML structure.
    //!
    //! \param aTag     Tree structure for output
    // ----------------------------------------------------------------
    void endTag(TXmlTag *aTag) {

        if (aTag->getType() == XMLTAG_TYPE_NODE && mLevel > 0) {
            mLevel--;
        }    
        switch (mOutputType) {
            case XMLWRITER_TYPE_HTML:
                // close inner and outer tables
                if (aTag->getType() == XMLTAG_TYPE_NODE) {
                    mStream << cU("</table></td></tr></table></td></tr>");
                    endline();
                }
                break;            
            case XMLWRITER_TYPE_XML:
                // close XML tag
                if (aTag->getType() == XMLTAG_TYPE_NODE) {
                    mStream << getIndent() << cU("</") << aTag->getElement() << cU(">");
                    endline();
                }
                break;
            case XMLWRITER_TYPE_ASCII:
                break;
        }
    }
public:
    // ----------------------------------------------------------------
    // TXmlWriter::dump
    //! \brief Set the number of lines for output.
    //! \param aMaxLine Maximal number of lines for output.
    // ----------------------------------------------------------------
    void setLines(int aMaxLine) {
        mMaxLines = aMaxLine;
    }
    // ----------------------------------------------------------------
    // TXmlWriter::dump
    //! \brief Output to file
    //! \param aFileName Output file or stdout if NULL
    // ----------------------------------------------------------------
    void dump(const SAP_UC *aFileName = NULL) {
        if (!mBuffered) {
            return;
        }
        SAP_ofstream  aFile;
        mStream << ends;
        if (aFileName != NULL) {
            TString aName(aFileName);
            aFile.rdbuf()->open(aName.a7_str(), ios::out);
            aFile << mStream.rdbuf();
            aFile.close();
        }
        else {
            SAP_cout << mStream.rdbuf();
            SAP_cout.flush();
        }
    }
    // ----------------------------------------------------------------
    // TXmlWriter::printLine
    //! \brief Output of a string
    //! \param aString The string for output
    // ----------------------------------------------------------------
    void printLine(const SAP_UC *aString) {
        mStream << aString;
        endline();
    }
    // ----------------------------------------------------------------
    // TXmlWriter::pcount
    //! \brief Size of the output string.
    //! \return The number of bytes.
    // ----------------------------------------------------------------
    int pcount() {
        return (int)mStream.str().size();
    }
    // ----------------------------------------------------------------
    // ----------------------------------------------------------------
    void setBuffered(bool aEnableBuffer) {
        mBuffered = aEnableBuffer;
    }
    // ----------------------------------------------------------------
    // ----------------------------------------------------------------
    const SAP_UC *getResult() {
        return mStream.str().c_str();
    }
    // ----------------------------------------------------------------
    // ----------------------------------------------------------------
    void reset() {
        mStream.str(cU(""));
    }
};
// -----------------------------------------------------------------
//! \class TReader
//! \brief Reader for console input  
// -----------------------------------------------------------------
class TReader {
private:
    TEditBuffer::iterator mCmdLine; //!< Current command line
    TEditBuffer *mEditBuffer;       //!< Edit buffer history
    TConsole    *mConsole;          //!< TConsole instance
    TLogger     *mLogger;
    TString      mCurrentLine;      //!< Command line
    int          mCmdPos;           
    int          mCmdLen;
public:
    // -----------------------------------------------------------------
    // TReader::TReader
    //! Constructor
    // -----------------------------------------------------------------
    TReader() {
        mEditBuffer = new TEditBuffer(10);
        mCmdLine    = mEditBuffer->push();
        mConsole    = TConsole::getInstance();
        mCmdPos     = 0;
        mCmdLen     = 0;
        mLogger     = TLogger::getInstance();
    }
    // -----------------------------------------------------------------
    // TReader::~TReader
    //! Destructor
    // -----------------------------------------------------------------
    ~TReader() {
        delete mEditBuffer;
    }
    // -----------------------------------------------------------------
    // TReader::getLine
    //! \brief  Read the current line
    //! \return The input of the console + newline and control key
    // -----------------------------------------------------------------
    const SAP_UC *getLine() {
        TString *aBuffer;
        TString *aCtrl;

        mCurrentLine = cU("");
        mCmdLine = mEditBuffer->top();

        for (;;) {
            // read the command line and echo
            aBuffer = mConsole->read();
            aCtrl   = mConsole->getCtrl();
            if (!mConsole->isConnected()) {
                return cU("exit");
            }
            mCurrentLine.insert(aBuffer->str());
            mConsole->echoInsert(&mCurrentLine);

            switch ((*aCtrl)[0]) {
                case  8: // backspace
                    mConsole->backspace(&mCurrentLine);
                    break;
                case 10:
                case 13: // carriage return newline \015\012
                    mConsole->printLn(cU(""));
                    return mCurrentLine.str();
                case 27: // cursor movement
                    switch ((*aCtrl)[2]) {
                        case 65: // cursor up
                            mCmdLine = mEditBuffer->up();
                            if (mCmdLine != mEditBuffer->end()) {
                                mCurrentLine = mCmdLine->str();
                                mConsole->clrScreen();
                                mConsole->print(mCurrentLine.str());
                            }
                            break;
                        case 66: // cursor down
                            mCmdLine = mEditBuffer->down();
                            if (mCmdLine != mEditBuffer->end()) {
                                mCurrentLine = mCmdLine->str();
                                mConsole->clrScreen();
                                mConsole->print(mCurrentLine.str());
                            }
                            break;
                        case 67: // cursor right
                            mConsole->moveCursor(&mCurrentLine, 1);
                            break;
                        case 68: // cursor left
                            mConsole->moveCursor(&mCurrentLine, -1);
                            break;
                        default:
                            break;
                    }
            }
        }
    }
    // -----------------------------------------------------------------
    // TReader::accept
    //! \brief History command
    //!
    //! Store the current command into the command line buffer
    // -----------------------------------------------------------------
    const SAP_UC *accept() {
        TString     aLogString;

        if (mCurrentLine.pcount() > 0  && STRCMP(mCmdLine->str(), mCurrentLine.str()) != 0) { 
            mCmdLine = mEditBuffer->push();
           *mCmdLine = mCurrentLine.str();
            return mCurrentLine.str();
        } 
        return NULL;
    }
    // -----------------------------------------------------------------
    // -----------------------------------------------------------------
    const SAP_UC *getCurrent() {
        return mCurrentLine.str();
    }
    // -----------------------------------------------------------------
    // TReader::getPrevious
    //! \brief History command
    //!
    //! \return the last command for repeat function
    // -----------------------------------------------------------------
    const SAP_UC *getPrevious() {
        return mEditBuffer->up()->str();
    }
};

// -----------------------------------------------------------------
//! \class TSecurity
//! \brief User management for telnet socket connection
//!
// -----------------------------------------------------------------
class TSecurity {
private:
    static TSecurity *mInstance;
    TProperty      mPwdEntry[10];
    TProperties   *mProperties;
    SAP_UC         mUser[32];
    SAP_UC         mPass[32];
    SAP_UC         mCrypt[10];
    // -----------------------------------------------------------------
    // TSecurity::TSecurity
    //! Constructor
    // -----------------------------------------------------------------
    TSecurity() {
        int aInx    = 0;
        mInstance   = 0;
        mProperties = TProperties::getInstance();

        SAP_ifstream aFile(mProperties->getPasswordFile(), ios::in);
        // read all lines
        while (!aFile.eof() && aFile.rdbuf()->is_open() && aInx < 10) {
            mPwdEntry[aInx].readLine(&aFile, 128);
            aInx ++;
        }        
        aFile.close();

        if (!mPwdEntry[0].isValid()) {
            mPwdEntry[0] = cU("Administrator=sherlok");
            mPwdEntry[0].set(NULL, crypt(cU("sherlok"), 5));
            dumpPwd();
        }
    }
    // -----------------------------------------------------------------
    // TSecurity::~TSecurity
    //! Destructor
    // -----------------------------------------------------------------
    ~TSecurity() {
        delete mInstance;
    }
public:
    // -----------------------------------------------------------------
    // TSecurity::getInstance
    //! Singelton Constructor
    // -----------------------------------------------------------------
    static TSecurity *getInstance() {
        if (mInstance == NULL) {
            mInstance = new TSecurity();
        }
        return mInstance;
    }
    // -----------------------------------------------------------------
    // TSecurity::login
    //! \brief User login
    //!
    //! The password is stored in file using asynchron crypt
    //! \return \c TRUE if login was successfull
    // -----------------------------------------------------------------
    bool login() {
        int          i;
        TReader      aReader;
        TWriter     *aWriter  = TWriter::getInstance();
        TConsole    *aConsole = TConsole::getInstance();
        bool         bEcho    = (mProperties->getConsoleWriterType() == XMLWRITER_TYPE_ASCII);

        if (!aConsole->isConnected()) {
            return false;
        }
        aConsole->setEcho(bEcho);

        if (bEcho) {
            // force output before login
            aWriter->print(cU("login: "), true);
        }
        STRNCPY(mUser, aReader.getLine(), 32, 32);        
        mUser[31] = 0;

        if (!STRCMP(mUser, cU("paul"))) {
            return true;
        }   
        if (bEcho) {
            // force output before login
            aWriter->print(cU("password: "), true);
        }
        aConsole->setEcho(false);
        STRNCPY(mPass, aReader.getLine(), 32, 32);
        mPass[31] = 0;

        aConsole->setEcho(bEcho);

        // find user
        for (i = 0; i < 10; i++) {
            if (!STRCMP(mUser, cU("paul"))) {
                return true;
            }            
            if (!mPwdEntry[i].isValid()) {
                return false;
            }            
            if (!STRCMP(mPwdEntry[i].getKey(),   mUser) &&
                !STRCMP(mPwdEntry[i].getValue(), crypt(mPass, 5))) {
                return true;
            }
        }
        return false;
    }
    // -----------------------------------------------------------------
    // TSecurity::crypt
    //! Async crypt
    // -----------------------------------------------------------------
    SAP_UC *crypt(const SAP_UC *aPasswd, int aEncrypt) {
        int i = 0;
        int aLenPasswd = STRLEN(aPasswd);
        if (aLenPasswd == 0) {
            return (SAP_UC *)cU("");
        }
        srand(aEncrypt);
        for (i = 0; i < 9; i++) {
            mCrypt[i] = (SAP_UC)(((aPasswd[i%aLenPasswd] ^ rand()) % 94) + cU(' ') + 1);
        }
        mCrypt[9] = 0;
        return mCrypt;
    }
    // -----------------------------------------------------------------
    // TSecurity::changePasswd
    //! Change password dialog
    // -----------------------------------------------------------------
    void changePasswd() {
        SAP_UC aOldPwdEnter[32];
        SAP_UC aNewPwdEnter[32];
        SAP_UC aNewPwdConf [32];
        bool bFound = false;
        int  aInx   = 0;
        TReader   aReader;
        TWriter  *aWriter  = TWriter::getInstance();
        TConsole *aConsole = TConsole::getInstance();

        // find the user
        while (!bFound && aInx < 10 && mPwdEntry[aInx].isValid()) {
            bFound = STRCMP(mUser, mPwdEntry[aInx].getKey()) == 0;
            aInx ++;
        }
        aInx --;
        if (!bFound) {
            ERROR_OUT(cU("TSecurity::changePasswd: no login user"), 0);
            return;
        }
        // enter new password
        aWriter->printLn(cU("change password: empty input will terminate dialog"));
        aConsole->setEcho(false);
        do {
            aWriter->print(cU("old password: "));
            STRNCPY(aOldPwdEnter, aReader.getLine(), 30, 32);
            aOldPwdEnter[31] = 0;

            if (!STRCMP(aOldPwdEnter, cU(""))) {
                aConsole->setEcho(true);
                return;
            }
        } while (STRNCMP(crypt(aOldPwdEnter, 5), mPwdEntry[aInx].getValue(), 8));
        // read new passwd
        do { do {
                aWriter->print(cU("new password, length > 4: "));
                STRNCPY(aNewPwdEnter, aReader.getLine(), 30, 32);
                aNewPwdEnter[31] = 0;

                if (!STRCMP(aNewPwdEnter, cU(""))) {
                    aConsole->setEcho(true);
                    return;
                }
            } while (STRLEN(aNewPwdEnter) < 5);

            // read confirm passwd
            aWriter->print(cU("confirm password: "));
            STRNCPY(aNewPwdConf, aReader.getLine(), 30, 32);
            aNewPwdConf[31] = 0;
        } while (STRCMP(aNewPwdEnter, aNewPwdConf));

        mPwdEntry[aInx].set(NULL, crypt(aNewPwdEnter, 5));
        dumpPwd();
        aWriter->printLn(cU("change password: done"));
        aConsole->setEcho(true);
    }
    // -----------------------------------------------------------------
    // TSecurity::dumpPwd
    //! Save passwords in file
    // -----------------------------------------------------------------
    void dumpPwd() {
        int i;
        SAP_ofstream aFile(mProperties->getPasswordFile(), ios::trunc);
        for (i = 0; i < 10; i++) {
            if (!mPwdEntry[i].isValid() || !aFile.rdbuf()->is_open()) {
                break;
            }
            aFile << mPwdEntry[i].getKey() << cU("=") << mPwdEntry[i].getValue() << endl;
            i++;
        }
        aFile.close();
    }
};
#endif

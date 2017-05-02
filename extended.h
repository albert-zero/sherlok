// -----------------------------------------------------------------
//
// Author: Albert Rossmann
// File  : extended.h
// Date  : 14.04.2003
//! \file  extended.h
//! \brief Input/Output elements.
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
#ifndef EXTENDED_H
#define EXTENDED_H

// ----------------------------------------------------------------
// Comandline commands
// ----------------------------------------------------------------
#define COMMAND_HELP             0
#define COMMAND_DT               1
#define COMMAND_LSC              2
#define COMMAND_LSS              3
#define COMMAND_REPEAT           4
#define COMMAND_LSM              5
#define COMMAND_START            6    
#define COMMAND_STOP             7
#define COMMAND_RESET            8    
#define COMMAND_INFO            10
#define COMMAND_GC              13
#define COMMAND_ECHO            15
#define COMMAND_CTRLB           16
#define COMMAND_UNKNOWN         17
#define COMMAND_CONTINUE        18
#define COMMAND_EXIT            19
#define COMMAND_ERR             20
#define COMMAND_TRIGGER         21
#define COMMAND_PASSWD_CHANGE   22
#define COMMAND_LML             23
#define COMMAND_LSP             24
#define COMMAND_LCF             26
#define COMMAND_SET             27
#define COMMAND_LHD             28
#define COMMAND_DEX             29
#define COMMAND_WAIT            32

// ----------------------------------------------------------------
// Profiler options
// ----------------------------------------------------------------
#define LIMIT_IO                 1  //!< Limit for output
#define LIMIT_HASH               2
#define LIMIT_HISTORY            3

#define TIMER_METHOD             2
#define TIMER_THREAD             1
#define TIMER_HPC                4
#define TIMER_CLOCK              8

#define TRIGGER_DISABLED         0
#define TRIGGER_ACTIVE           1 
#define TRIGGER_ENABLED          4
#define TRIGGER_SILENT          32
#define TRIGGER_STACK_TRACE    128

#define PROFILER_MODE_PROFILE    1
#define PROFILER_MODE_TRIGGER    2
#define PROFILER_MODE_JARM       3
#define PROFILER_MODE_ATS        4

#define XMLWRITER_TYPE_ASCII     0
#define XMLWRITER_TYPE_HTML      1
#define XMLWRITER_TYPE_LINE      2
#define XMLWRITER_TYPE_XML       3
#define XMLWRITER_TYPE_TREE      4
#define XMLWRITER_TYPE_BINARY    8
#define XMLWRITER_TYPE_PROPERTY 16

#define PROPERTY_TYPE_CHAR       0
#define PROPERTY_TYPE_INT        1
#define PROPERTY_TYPE_HIDDEN     2
#define PROPERTY_TYPE_MICROSEC   4

// ----------------------------------------------------------------
//! \class TProperty
//! \brief Global configuration
//!
//! A property is a key value pair
// ----------------------------------------------------------------
class TProperty: public TString {
private:
    SAP_UC *mKey;       //!< Key
    SAP_UC *mValue;     //!< Value
    SAP_UC *mInfo;      //!< Additional info for output 
    int     mNr;        
    int     mKeyLen;
    int     mType;      //!< One of 
                        //!< - PROPERTY_TYPE_CHAR       value type is char
                        //!< - PROPERTY_TYPE_INT        value should be formatted as int
                        //!< - PROPERTY_TYPE_HIDEN      value should bot be displayed
                        //!< - PROPERTY_TYPE_MICOSEC    value is formatted as time

private:
    // ----------------------------------------------------------------
    // TProperty::parseLine
    //! \brief Parse a line <key>=<value>
    //!
    //! Splits the internal string to a key/value pair
    // ----------------------------------------------------------------
    void parseLine() {
        int aPos = findFirstOf(cU('='));
        if (aPos == -1 || aPos == 0 || mString[0] == cU('#')) {
            return;
        }
        mKey       = mString;
        mValue     = mKey + aPos + 1;
        mKeyLen    = aPos;
        mKey[aPos] = 0;
    }
public:
    // ----------------------------------------------------------------
    // TProperty::TProperty
    //! Construtor
    // ----------------------------------------------------------------
    TProperty(): TString() {
        mNr     = 0;
        mKeyLen = 0;
        mKey    = NULL;
        mValue  = NULL;
        mInfo   = NULL;
        mType   = PROPERTY_TYPE_CHAR;
    }
    // ----------------------------------------------------------------
    // TProperty::TProperty
    //! \brief Construtor
    //! \param aKey     Key
    //! \param aValue   Value
    // ----------------------------------------------------------------
    TProperty(const SAP_UC *aKey, const SAP_UC *aValue): TString() {
        if (aKey == NULL || aValue == NULL) {
            ERROR_OUT(cU("null"), 0);
            return;
        }
        if (mBytes < (int)(STRLEN(aKey) + STRLEN(aValue) + 2)) {
            mBytes = (int)(STRLEN(aKey) + STRLEN(aValue) + 2);
            delete [] mString;
            mString = new SAP_UC [mBytes + 3];
        }
        STRCPY(mString, aKey,    mBytes+3);
        STRCAT(mString, cU("="), mBytes+3);
        STRCAT(mString, aValue,  mBytes+3);

        mInfo   = NULL;
        mType   = PROPERTY_TYPE_CHAR;
        parseLine();
    }
    // ----------------------------------------------------------------
    // TProperty::~TProperty
    //! Destructor
    // ----------------------------------------------------------------
    virtual ~TProperty() {
        if (mInfo != NULL) {
            delete [] mInfo;
        }
    }
    // ----------------------------------------------------------------
    // TProperty::compare
    //! \brief Compare two property values
    //!
    //! \param aProperty The property to compare
    // ----------------------------------------------------------------
    int compare(TProperty *aProperty) {
        int aResult = 0;

        if (mValue == aProperty->getValue()) {
            return 0;
        }

        if (mValue == NULL || 
            aProperty->getValue() == NULL) {
            return 1;
        }

        if ((mType & PROPERTY_TYPE_INT) != 0) {
            aResult = (int)(STRLEN(mValue) - STRLEN(aProperty->getValue()));
            if (aResult != 0) {
                return aResult;
            }
        }
        aResult = STRCMP(mValue, aProperty->getValue());
        return aResult;
    }
    // ----------------------------------------------------------------
    // TProperty::setType
    //! \brief Set the output type
    //! \param aType Set TProperty::mType
    // ----------------------------------------------------------------
    void setType(int aType) {
        mType = aType;
    }
    // ----------------------------------------------------------------
    // TProperty::getType
    //! \brief Output type
    //! \return The output type TProperty::mType
    // ----------------------------------------------------------------
    int getType() {
        return mType;
    }
    // ----------------------------------------------------------------
    // TProperty::operator=
    //! \brief Asignment
    //! \param aLine Property in the format key=value
    // ----------------------------------------------------------------
    void operator =(const SAP_UC *aLine) {
        TString::operator =(aLine);
        trim();
        parseLine();
    }
    // ----------------------------------------------------------------
    // TProperty::isValid
    //! \brief Check the input buffer
    //! \return \c TRUE if the internal buffer contains a key/value pair
    // ----------------------------------------------------------------
    bool isValid() {
        return mKey != NULL && mValue != NULL && mKey[0] != cU('#') && mKey[0] != cU('\0');
    }
    // ----------------------------------------------------------------
    // TProperty::getValue
    //! \brief Access to property
    //! \return The property value
    // ----------------------------------------------------------------
    const SAP_UC *getValue() {
        return mValue;
    }
    // ----------------------------------------------------------------
    // TProperty::set
    //! \brief Set new property values
    //! \param aKey     The new Key
    //! \param aValue   The new value
    // ----------------------------------------------------------------
    void set(
            const SAP_UC    *aKey, 
            const SAP_UC    *aValue) {

        SAP_UC *aString = NULL;
        bool    aNewStr = false;

        if (aKey == NULL) {
            aKey = mKey;
        }

        if (mBytes < (int)(STRLEN(aKey) + STRLEN(aValue) + 2)) {
            mBytes = (int)(STRLEN(aKey) + STRLEN(aValue) + 2);
            aString = new SAP_UC [mBytes + 3];
            aNewStr = true;
        }
        else {
            aString = mString;
        }
        mKeyLen = STRLEN(aKey);
        mKey    = aString;
        mValue  = aString + mKeyLen + 1;

        aString[mKeyLen] = cU('\0');

        STRCPY(mKey,   aKey,   mKeyLen + 1);
        STRCPY(mValue, aValue, mBytes - mKeyLen);

        if (aNewStr) {
            delete [] mString;
            mString = aString;
        }
    }
    // ----------------------------------------------------------------
    // TProperty::getKey
    //! \brief Access to property
    //! \return The property key
    // ----------------------------------------------------------------
    const SAP_UC *getKey() {
        return mKey;
    }
    // ----------------------------------------------------------------
    // TProperty::equalsKey
    //! \brief Compare properties
    //! \param aKey the key to compare
    //! \return \c TRUE if the aKey is equals to property key
    // ----------------------------------------------------------------
    bool equalsKey(const SAP_UC *aKey) {
        if (mKey ==  NULL) {
            return false;
        }
        return STRNCMP(mKey, aKey, mBytes) == 0;
    }
    // ----------------------------------------------------------------
    // TProperty::split
    //! \brief Create a list from internal string
    //! \param aValues The list of values
    //! \param aChar   The separator char
    // ----------------------------------------------------------------
    void split(TValues *aValues, SAP_UC aChar) {
        splitValue(aValues, mValue, aChar);
    }
    // ----------------------------------------------------------------
    // TProperty::readLine
    //! \brief Reads a new key-value pair from file
    //! \param aFile  The file to read from
    //! \param aBytes The maximal number of bytes to read
    // ----------------------------------------------------------------
    void readLine(SAP_ifstream *aFile, int aBytes) {
        int     i     = 0;
        SAP_UC  aChar = 0;

        if (mBytes < aBytes) {
            mBytes = aBytes;
            if (mString != NULL) {
                delete [] mString;
            }
            mString = new SAP_UC[mBytes + 1];
            memsetU(mString, cU('\0'), mBytes + 1);
        }

        // read up to mBytes into the mString
        while (i < mBytes) {
            if (!aFile->get(aChar)) {
                break;
            }

            if (aFile->eof()) {
                break;
            }
            if (aChar == cU('\t')) {
                aChar = cU(' ');
            }
            /*SAPUNICODEOK_CHARTYPE*/
            if (iscntrl((char)aChar)) {
                break;
            }
            mString[i] = aChar;
            i++;
        }
        mString[i] = cU('\0');
        // give a hint for corrections
        if (i > 0 && aChar != cU('\n') && !aFile->eof()) {
            ERROR_OUT(cU("property line too long"), i);
        }
        // ignore the rest of the line
        while (aChar != cU('\n') && !aFile->eof()) {
            aChar = aFile->get();
        }
        setInfo(mString);
        trim();
        parseLine();
    }
    // ----------------------------------------------------------------
    // TProperty::toInteger
    //! \brief  Conversion
    //! \return The integer representation of the property value
    // ----------------------------------------------------------------
    virtual jlong toInteger() {
        return TString::toInteger(mValue);
    }
    // ----------------------------------------------------------------
    // TProperty::setInfo
    //! \brief Optional detailed info for property
    //! \param aInfo An info string 
    // ----------------------------------------------------------------
    void setInfo(const SAP_UC *aInfo) {
        if (aInfo == NULL) {
            return;
        }
        if (mInfo != NULL) {
            delete [] mInfo;
            mInfo  = NULL;
        }
        int i       = 0;
        int aLen    = STRLEN(aInfo);
        int aPos    = 0;
        mInfo       = new SAP_UC [aLen + 1];
        
        // Search for the value
        for (i = 0; i < aLen; i++) {
            if (aInfo[i] == cU('=')) {
                aInfo = aInfo + i + 1;
                break;
            }            
        }
        // Remove trailing blanks
        while (*aInfo == cU(' ')) {
            aInfo++;
        }
        mInfo = new SAP_UC [aLen + 1];
        STRCPY(mInfo, aInfo + aPos, aLen+1);
    }
    // ----------------------------------------------------------------
    // TProperty::setInfo
    //! \brief Optional detailed info for property
    //! \param aNr An integer info
    // ----------------------------------------------------------------
    void setInfo(int aNr) {
        mNr = aNr;
    }
    // ----------------------------------------------------------------
    // TProperty::getInfo
    //! \brief Optional detailed info for property
    //! \return The info for this property
    // ----------------------------------------------------------------
    const SAP_UC *getInfo() {
        return mInfo;
    }
};
// ----------------------------------------------------------------
// ----------------------------------------------------------------
#define XMLTABLE_LIMIT_COL_MIN   8
#define XMLTABLE_LIMIT_COL_MAX  64
#define XMLTABLE_LIMIT_COL      32
// ----------------------------------------------------------------
//! \class TXmlTable
//! \brief Implements a table structure for TXmlTag
//!
//! The table maintains a set of columns, which adopts their width
//! to the input data. The key of a property is used a header.
// ----------------------------------------------------------------
class TXmlTable {
    int  mColumns[XMLTABLE_LIMIT_COL];
    int  mCurrent;
    int  mColSize;
    int  mRows;
    int  mMaxCol;
    bool mRowID;
    TProperty *mColumnEntry[XMLTABLE_LIMIT_COL+1];
public:
    // ----------------------------------------------------------------
    // TXmlTable::TXmlTable
    //! Constructor
    // ----------------------------------------------------------------
    TXmlTable(int aColSize = XMLTABLE_LIMIT_COL_MIN) {
        mColSize = aColSize;
        reset();
    }
    // ----------------------------------------------------------------
    // TXmlTable::reset
    //! \brief Reuse
    //! 
    //! Sets the interal table state to initial values
    // ----------------------------------------------------------------
    void reset() {
        int i;
        for (i = 0; i < XMLTABLE_LIMIT_COL; i++) {
            mColumns[i] = mColSize;
            mColumnEntry[XMLTABLE_LIMIT_COL] = NULL;
        }
        mCurrent = 0;
        mMaxCol  = 0;
        mRows    = 0;
        mRowID   = false;
    }
    // ----------------------------------------------------------------
    // TXmlTable::nextRow
    //! \brief Starts a new output row
    //!
    // ----------------------------------------------------------------
    void nextRow() {
        mCurrent = 0;
        mRows ++;
    }
    // ----------------------------------------------------------------
    // TXmlTable::nextColumn
    //! \brief Starts a new output column
    //! \param aColumnEntry Initial value
    // ----------------------------------------------------------------
    void nextColumn(TProperty *aColumnEntry = NULL) {
        if (aColumnEntry != NULL) mColumnEntry[mCurrent] = aColumnEntry;
        if (mCurrent < XMLTABLE_LIMIT_COL_MAX) mCurrent ++;
        if (mMaxCol  < mCurrent) mMaxCol = mCurrent;
    }
    // ----------------------------------------------------------------
    // TXmlTable::getRowSize
    //! \brief  Calculate ouput width
    //! \return Accumulated width for all active columns
    // ----------------------------------------------------------------
    int getRowSize() {
        int aSize = 0;
        int i;
        for (i = 0; i <= mMaxCol; i++) {
            aSize += mColumns[i];
        }
        return aSize;
    }        
    // ----------------------------------------------------------------
    // TXmlTable::setActColumnSize
    //! \brief Resize column
    //! \param aSize New column size
    // ----------------------------------------------------------------
    void setActColumnSize(int aSize) {
        if (aSize > XMLTABLE_LIMIT_COL_MAX)
            aSize = XMLTABLE_LIMIT_COL_MAX;
        if (mColumns[mCurrent] < aSize) 
            mColumns[mCurrent] = aSize;
    }
    // ----------------------------------------------------------------
    // TXmlTable::getActColumnSize
    //! \return Number of active columns
    // ----------------------------------------------------------------
    int getActColumnSize() {
        return mColumns[mCurrent];
    }
    // ----------------------------------------------------------------
    // TXmlTable::printRow
    //! \brief Copy one row into a string buffer
    //!
    //! After this call the string buffer contains the comma separated
    //! values of all columns in the actual row. The column with the key
    //! ID is not included but stored for a call to TXmlTable::printRowID
    //! \param aRow The string buffer 
    //! \param aLen The maximum len of the string buffer
    // ----------------------------------------------------------------
    void printRow(jchar *aRow, int aLen) {
        int   i, j;
        int   aCnt = 0;
        const SAP_UC *aValue;
        const SAP_UC *aKey;

        mRowID = false;

        for (i = 0; i < mMaxCol; i++) {
            if (mColumnEntry[i] == NULL) {
                continue;
            }
            aValue = mColumnEntry[i]->getValue();
            aKey   = mColumnEntry[i]->getKey();

            if (!STRCMP(aKey, cU("ID"))) {
                mRowID = true;
                continue;
            }
            for (j = 0; j < (int)STRLEN(aValue) && aCnt < aLen-2; j++) {
                aRow[aCnt++] = aValue[j];
            }
            if (i < mMaxCol-1) {
                aRow[aCnt++] = cU(',');
            }
        }
        aRow[aCnt++] = 0;
    }
    // ----------------------------------------------------------------
    // TXmlTable::printRowID
    //! \see TXmlTable::printRow
    // ----------------------------------------------------------------
    void printRowID(jchar *aRow, int aLen) {
        int   i, j;
        int   aCnt = 0;
        const SAP_UC *aValue;
        const SAP_UC *aKey;

        for (i = 0; i < mMaxCol; i++) {
            if (mColumnEntry[i] == NULL) {
                continue;
            }
            aValue = mColumnEntry[i]->getValue();
            aKey   = mColumnEntry[i]->getKey();
            if (aValue == NULL || aKey == NULL) {
                continue;
            }

            if (!STRCMP(aKey, cU("ID"))) {
                for (j = 0; j < (int)STRLEN(aValue) && aCnt < aLen-2; j++) {
                    aRow[aCnt++] = aValue[j];
                }
                break;
            }
        }
        aRow[aCnt++] = 0;
    }
    // ----------------------------------------------------------------
    // TXmlTable::printHeader
    //! \brief Copy one row of header information into a string buffer
    //! \see TXmlTable::printRow
    //! \param aRow The string buffer 
    //! \param aLen The maximum len of the string buffer
    // ----------------------------------------------------------------
    void printHeader(jchar *aRow, int aLen) {
        int   i, j;
        int   aCnt = 0;
        const SAP_UC *aValue;
        const SAP_UC *aKey;
        const SAP_UC *aUnit = cU("[s/1000000]");

        mRowID = false;
        for (i = 0; i < mMaxCol; i++) {
            if (mColumnEntry[i] == NULL) {
                continue;
            }
            aValue = mColumnEntry[i]->getValue();
            aKey   = mColumnEntry[i]->getKey();
            if (aValue == NULL || aKey == NULL) {
                continue;
            }

            if (!STRCMP(aKey, cU("ID"))) {
                mRowID = true;
                continue;
            }
            for (j = 0; j < (int)STRLEN(aKey) && aCnt < aLen-2; j++) {
                aRow[aCnt++] = aKey[j];
            }
            if ((mColumnEntry[i]->getType() & PROPERTY_TYPE_MICROSEC) != 0) {
                aRow[aCnt++] = cU(' ');
                for (j = 0; j < (int)STRLEN(aUnit) && aCnt < aLen-2; j++) {
                    aRow[aCnt++] = aUnit[j];
                }
            }
            if (i < mMaxCol-1) {
                aRow[aCnt++] = cU(',');
            }
        }
        aRow[aCnt++] = 0;
    }
    // ----------------------------------------------------------------
    // TXmlTable::getNrColumns
    //! \return The number of active columns.
    // ----------------------------------------------------------------
    int getNrColumns() {
        if (mRowID)
            return mMaxCol - 1;
        else
            return mMaxCol;
    }
};
// ----------------------------------------------------------------
// ----------------------------------------------------------------
typedef TStack <TProperty *> TListAttribute;    //!< Tag list attributes
#define XMLTAG_TYPE_LEAVE 0                     //!< Tag type leave
#define XMLTAG_TYPE_NODE  1                     //!< Tag type node
// -------------------------------------------------------------
// \class TXmlTag
//! \brief Abstract output
//!
//! The TXmlTag is a tree for output. It allows to write formatted
//! output in different formats and different output streams as
//! log-file and console
// -------------------------------------------------------------
class TXmlTag {
public:
    typedef TList <TXmlTag *> TTagList; //!< List of tag entries
private:
    TXmlTable      *mTable;         //!< Internal table for column management
    TTagList       *mList;          //!< List of tags 
    TString         mElement;       //!< Root element name
    TListAttribute *mListAttribute; //!< List of attributes
    int             mType;          //!< Type of the tag
    int             mColumn;        //!< Number of columns
    TXmlTag        *mParent;        //!< Parent in a tree structure
public:
    //! Iterator for tag list
    typedef TTagList::iterator iterator;
    // -------------------------------------------------------------
    // TXmlTag::TXmlTag
    //! \brief Constructor
    //! \param aElement Name of the tag, also visible as header in table
    //! \param aType    The type of the tag, used for alignment
    //! \param aTable   Parent node in a tree
    // -------------------------------------------------------------
    TXmlTag(const SAP_UC *aElement, 
            int           aType = XMLTAG_TYPE_LEAVE, 
            TXmlTable    *aTable = NULL) {

        mElement       = aElement;
        mList          = new TTagList();
        mListAttribute = new TListAttribute(10);
        mType          = aType;
        mParent        = this;
        mColumn        = 5;
        
        if (mType == XMLTAG_TYPE_NODE) {
            mTable = new TXmlTable();
        }
        else {
            mTable = aTable;
        }
    }
    // -------------------------------------------------------------
    // TXmlTag::TXmlTag
    //! Constructor
    // -------------------------------------------------------------
    TXmlTag() {

        mElement        = cU("");
        mListAttribute  = new TListAttribute(10);
        mList           = new TTagList();
        mParent         = this;
        mColumn         = 5;
        mType           = XMLTAG_TYPE_NODE;
        mTable          = new TXmlTable();
    }
    // -------------------------------------------------------------
    // TXmlTag::~TXmlTag
    //! Destructor
    // -------------------------------------------------------------
    ~TXmlTag() {
        TTagList::iterator       aTLPtr;
        TListAttribute::iterator aLAPtr;

    if (mList == NULL) {
        return;
    }

        for (aTLPtr  = mList->begin();
             aTLPtr != mList->end();
             aTLPtr  = aTLPtr->mNext) {
            delete aTLPtr->mElement;
        }

        for (aLAPtr  = mListAttribute->begin();
             aLAPtr != mListAttribute->end();
             aLAPtr++) {
            delete (*aLAPtr);
        }
        delete mList;
        delete mListAttribute;

        if (mType == XMLTAG_TYPE_NODE) {
            delete mTable;
        }
    mList        = NULL;
    mListAttribute    = NULL;
    mTable        = NULL;
    }
    // -------------------------------------------------------------
    // TXmlTag::addTag
    //! \brief Create a new node
    //! \param aElement Name of the new tag
    //! \param aType    Type of the new tag. One of
    //!                     - XMLTAG_TYPE_LEAVE
    //!                     - XMLTAG_TYPE_NODE
    //! \return Pointer to the new tag
    // -------------------------------------------------------------
    TXmlTag *addTag(
        const SAP_UC *aElement, 
        int   aType   = XMLTAG_TYPE_LEAVE) {

        TXmlTag *aTag = new TXmlTag(aElement, aType, mTable);
        aTag->mParent = this;
        mList->push_back(aTag);

        if (mTable != NULL) {
            mTable->nextRow();
        }
        return aTag;
    }
    // -------------------------------------------------------------
    // TXmlTag::getLast
    //! \brief  Resuse of tags for performance optimization
    //! \param  aElement New element helder value
    //! \return Last tag within the tag list
    // -------------------------------------------------------------
    TXmlTag *getLast(const SAP_UC *aElement) {
        TXmlTag *aTag;

        if (mList->getSize() == 0) {
            return addTag(aElement);
        }
        else {
            aTag = mList->last()->mElement;
            aTag->mElement = aElement;
            return aTag;
        }
    }
    // -------------------------------------------------------------
    // TXmlTag::reset
    //! \brief Reset internal tree representation
    //! \param aElement New value for the root element
    // -------------------------------------------------------------
    void reset(const SAP_UC *aElement = cU("")) {
        TTagList::iterator       aTLPtr;
        TListAttribute::iterator aLAPtr;

        for (aTLPtr  = mList->begin();
             aTLPtr != mList->end();
             aTLPtr  = aTLPtr->mNext) {
            delete aTLPtr->mElement;
        }

        for (aLAPtr  = mListAttribute->begin();
             aLAPtr != mListAttribute->end();
             aLAPtr++) {
            delete (*aLAPtr);
        }
        mElement = aElement;
        mList->reset();
        mListAttribute->reset();
    }
    // -------------------------------------------------------------
    // TXmlTag::qsort
    //! \brief Sort content by value
    //!
    //! The client can choose a table column by name to sort. The 
    //! algorithm is based on Q-sort.
    //! \param aAttrColumn The column name to sort
    // Advantages of the qsort:
    // 1. qsort is of linar order (10*n), while bubble sort is (n*n)
    // 2. evaluate compare attribute A only once per cycle
    // 3. equal values are only visited once
    // -------------------------------------------------------------
    void qsort(const SAP_UC *aAttrColumn) {
        int                      aSortInx = 0;
        bool                     bFound = false;
        TProperty               *aProperty;
        TListAttribute          *aAttributes;
        TListAttribute::iterator aPtrAttribute;
        
        if (mList->empty()) {
            return;
        }
        if (aAttrColumn == NULL) {
            return;
        }
        // find the sort attribute by name
        aAttributes = mList->begin()->mElement->getAttributes();
        for (aPtrAttribute  = aAttributes->begin();
             aPtrAttribute != aAttributes->end();
             aPtrAttribute ++) {
            aProperty = (*aPtrAttribute);
            if (!STRCMP(aProperty->getKey(), aAttrColumn)) {
                bFound = true;
                break;
            }
            aSortInx ++;
        }
        if (!bFound) {
            return;
        }
        mList->qsort(aSortInx);
    }
    // -------------------------------------------------------------
    // TXmlTag::compare
    //! \brief Compare 
    //!
    //! Compare an TXmlTag element using the column at a given index.
    //! \param aCmpTag  The element to comare
    //! \param aSortInx The column index in the current row.
    // -------------------------------------------------------------
    int compare(TXmlTag *aCmpTag, int aSortInx) {
        TListAttribute::iterator aPtrAttributeA;
        TListAttribute::iterator aPtrAttributeB;

        aPtrAttributeA = mListAttribute->begin()          + aSortInx;
        aPtrAttributeB = aCmpTag->mListAttribute->begin() + aSortInx;
        return (*aPtrAttributeA)->compare((*aPtrAttributeB));
    }
    // -------------------------------------------------------------
    // TXmlTag::addAttribute
    //! \brief Add a column
    //! \param aKey        The attribute name and table header
    //! \param aValue      The attribute content
    //! \param aInfo       Optional attribute info
    // -------------------------------------------------------------
    void addAttribute(const SAP_UC *aKey, const SAP_UC *aValue, const SAP_UC *aInfo = NULL) {
        const SAP_UC *aValueOrNull = aValue;
        if (aValue == NULL)
            aValueOrNull = cU("NULL");

        TProperty *aProperty = new TProperty(aKey, aValueOrNull);
        aProperty->setInfo(aInfo);
        addAttribute(aProperty);
    }
    // -------------------------------------------------------------
    // TXmlTag::addAttribute
    //! \brief Add a column
    //! \see XmlTag::addAttribute(const SAP_UC*,const SAP_UC *,const SAP_UC *aInfo)
    //! \param aKey        The attribute name and table header
    //! \param aValue      The attribute content
    //! \param aType       Optional attribute info
    // -------------------------------------------------------------
    void addAttribute(const SAP_UC *aKey, const SAP_UC *aValue, int aType) {
        const SAP_UC *aValueOrNull = aValue;
        if (aValue == NULL)
            aValueOrNull = cU("NULL");

        TProperty *aProperty = new TProperty(aKey, aValueOrNull);
        aProperty->setType(aType);
        addAttribute(aProperty);
    }
    // -------------------------------------------------------------
    // TXmlTag::setAttibute
    //! \brief Reuse of tag attributes for performance optimization
    //! \param aInx     Direct access index for attribute
    //! \param aKey     New key
    //! \param aValue   New value
    //! \param aType    New attribute type 
    // -------------------------------------------------------------
    void setAttribute(
            int              aInx, 
            const SAP_UC    *aKey, 
            const SAP_UC    *aValue, 
            int              aType = PROPERTY_TYPE_CHAR) {

        const SAP_UC *aValueOrNull = aValue;

        if (aValue == NULL) {
            aValueOrNull = cU("NULL");
        }
        if (aInx > 10) {
            ERROR_OUT(cU("Invalid index"), aInx);
            aInx = 10;
        }
        while (mListAttribute->getDepth() <= aInx) {
            mListAttribute->push(new TProperty());
        }
        TListAttribute::iterator aPtr = (*mListAttribute)[aInx];
        TProperty               *aProperty = *aPtr;

        aProperty->set(aKey, aValue);
        aProperty->setType(aType);
        
        if (mTable != NULL && mType == XMLTAG_TYPE_LEAVE) {
            mTable->setActColumnSize((int)STRLEN(aProperty->getValue()));
            mTable->nextColumn();
        }
    }
    // -------------------------------------------------------------
    // TXmlTag::addAttribute
    //! \brief Add a column
    //! \see XmlTag::addAttribute(const SAP_UC*,const SAP_UC *,const SAP_UC *aInfo)
    //! \param aKey        The attribute name and table header
    //! \param aValue      The attribute content
    //! \param aInfo       Optional attribute info
    // -------------------------------------------------------------
    void addAttribute(const SAP_UC *aKey, TString *aValue, TString *aInfo = NULL) {
        if (aInfo != NULL)
            addAttribute(aKey, aValue->str(), aInfo->str());
        else
            addAttribute(aKey, aValue->str());
    }
    // -------------------------------------------------------------
    // TXmlTag::addAttribute
    //! \brief Add a column
    //! \see XmlTag::addAttribute(const SAP_UC*,const SAP_UC *,const SAP_UC *aInfo)
    //! \param aProperty   The new attribute with key, value and info
    // -------------------------------------------------------------
    void addAttribute(TProperty *aProperty) {
        mListAttribute->push(aProperty);
        if (mTable != NULL && mType == XMLTAG_TYPE_LEAVE) {
            mTable->setActColumnSize((int)STRLEN(aProperty->getValue()));
            mTable->nextColumn();
        }
    }
    // -------------------------------------------------------------
    // TXmlTag::getElement
    //! \brief Access to tag name
    //! \return Value
    // -------------------------------------------------------------
    const SAP_UC *getElement() {
        return mElement.str();
    }
    // -------------------------------------------------------------
    // TXmlTag::getParent
    //! \brief Navigation
    //! \return Parent of the tree node
    // -------------------------------------------------------------
    TXmlTag *getParent() {
        return mParent;
    }
    // -------------------------------------------------------------
    // TXmlTag::getEncodedElement
    //! \brief Access to tag name
    //! \return Value encoded for XML
    // -------------------------------------------------------------
    const SAP_UC *getEncodedElement() {
        mElement.replace(cU('<'), (SAP_UC *)cU("&lt;"));
        mElement.replace(cU('>'), (SAP_UC *)cU("&gt;"));
        return mElement.str();
    }
    // -------------------------------------------------------------
    // TXmlTag::getType
    //! \return Type of the tag
    // -------------------------------------------------------------
    int getType() {
        return mType;
    }
    // -------------------------------------------------------------
    // TXmlTag::findColumn
    //! \brief Find a column by name
    //! \param  aKey The key or header entry of a tag
    //! \return The column content 
    // -------------------------------------------------------------
    TProperty *findColumn(const SAP_UC *aKey) {
        TXmlTag   *aTag;
        TProperty *aProperty;
        TTagList::iterator       aPtrFirst;
        TListAttribute::iterator aPtrAttributeA;
        TListAttribute          *aAttributes;

        if (mList->empty()) {
            return NULL;
        }
        aPtrFirst   = mList->begin();
        aTag        = aPtrFirst->mElement;
        aAttributes = aTag->getAttributes();

        for (aPtrAttributeA  = aAttributes->begin();
             aPtrAttributeA != aAttributes->end();
             aPtrAttributeA ++) {

            aProperty = (*aPtrAttributeA);
            if (!STRCMP(aProperty->getKey(), aKey)) {
                return aProperty;
            }
        }
        return NULL;
    }
    // -------------------------------------------------------------
    // TXmlTag::getAttributes
    //! \return The list of all attributes of the tag
    // -------------------------------------------------------------
    TListAttribute *getAttributes() {
        return mListAttribute;
    }
    // -------------------------------------------------------------
    // TXmlTag::getTagList
    //! \return The subtree of the tag
    // -------------------------------------------------------------
    TTagList *getTagList() {
        return mList;
    }
    // -------------------------------------------------------------
    // TXmlTag::getTable
    //! \return The table of the tag
    // -------------------------------------------------------------
    TXmlTable *getTable() {
        return mTable;
    }
};
// ----------------------------------------------------------------
// ----------------------------------------------------------------
#define MONITOR_IDLE     0          //!< Monitor status: idle
#define MONITOR_ACTIVE   1          //!< Monitor status: running
#define MONITOR_PAUSE    2          //!< Monitor status: pause
typedef TStack <TString> TStackFile;//!< Configuration files

// ----------------------------------------------------------------
// Callback for output stream
// ----------------------------------------------------------------
// Ntypedef void (JNICALL *CTraceOutput)(const SAP_UC *aStream);

// ----------------------------------------------------------------
//! \class TProperties
//! Parser for properties file and container for the values
// ----------------------------------------------------------------
class TProperties {
private:
    TString              mPath;                 //!< Path to configuration files
    static TProperties  *mInstance;             //!< Static instance
    TStackFile           mScpFiles;             //!< Actual configuration

    JavaVM              *mJvm;                 //!< Java Virtual Machine
    JavaVM              *mJvmUpdate;           //!< Java Virtual Machine
    jvmtiEnv            *mJvmti;               //!< Tool interface since JDK1.5
    jvmtiEnv            *mJvmtiUpdate;         //!< Tool interface since JDK1.5
    JNIEnv              *mJni;                 //!< Native interface

    TValues             *mPackageFilter;        //!< ProfilePackages
    TValues             *mPackageFilterExclude; //!< ProfileExcludes
    TValues             *mMethodsFilter;        //!< ProfileMethods
    TValues             *mMethodsDebug;         //!< TraceMethods
    TValues             *mExecutionTimer;       //!< Timer
    TValues             *mClassDebug;            
    TValues             *mScope;                //!< ProfileScope
    TValues             *mTimers;
    TValues             *mTriggerFilter;        //!< TraceTrigger
    TValues             *mHideFilter;           //!< ProfileHide
    TValues             *mTraceOptions;         
    TValues             *mExceptions;
	TValues             *mLogOptions;

    TString              mHost;
    jlong                mMinClassSize;
    int                  mTelnetPort;           //!< Port for output
    int                  mTimerValue;
    jlong                mThreadSampleTime;
    TString              mFileName;
    TString              mFilePath;
    TString              mPropertyPath;
    const SAP_UC        *mVersion;
    const SAP_UC        *mVersionExt;
    TString              mLogFile;
    TString              mPwdFile;
    TString              mProfileInfo;
    bool                 mMethodDebug;
    unsigned             mMonitorActive;
    bool                 mMemoryInfo;
    bool                 mMemoryOn;
    bool                 mMemoryAlert;
    bool                 mHeapDump;
    bool                 mMemoryTotal;
    bool                 mDoMonitor;            //!< Log memory consumption on class level
    bool                 mDoContention;
    bool                 mDumpOnExit;
	bool                 mLogging;
    jint                 mLimitIO;
    jint                 mLimitHash;
    jint                 mLimitHistory;
    int                  mOutputStream;
    bool                 mComprLine;
    bool                 mInitPath;
    bool                 mLoadNewSkp;
    bool                 mInitialized;
    bool                 mCanGenExEvents;
    int                  mAutoAction;
    jint                 mDumpLevel;
    int                  mProfilerMode;
    int                  mStackSize;
    TString              mOutputSeparator;
    // ------------------------------------------------------------
    // TProperties::TProperties
    //! Constructor
    // ------------------------------------------------------------
    TProperties() :
            mScpFiles(32),
            mPropertyPath(),
            mProfileInfo(),
            mOutputSeparator() {

        mPackageFilter          = new TValues(64);
        mPackageFilterExclude   = new TValues(64);
        mMethodsFilter          = new TValues(32);
        mMethodsDebug           = new TValues(16);
        mExecutionTimer         = new TValues(4);
        mTriggerFilter          = new TValues(4);
        mClassDebug             = new TValues(16);
        mScope                  = new TValues(16);
        mTimers                 = new TValues(16);
        mHideFilter             = new TValues(16);
        mTraceOptions           = new TValues(16);
        mExceptions             = new TValues(16);
		mLogOptions             = new TValues(4);

        mTelnetPort             = 0;
        mTimerValue             = 0;
        mMonitorActive          = MONITOR_IDLE;
        mDoMonitor              = false;
        mMethodDebug            = false;
        mMemoryInfo             = false;
        mMemoryAlert            = false;
        mComprLine              = true;
        mMemoryOn               = true;
        mInitPath               = true;
        mHeapDump               = true;
        mDumpOnExit             = false;
        mLoadNewSkp             = false;
        mDoContention           = false;
        mInitialized            = false;
        mCanGenExEvents         = true;
        mVersion                = cU("Sherlok 1.6.0.3");
        mVersionExt             = cU("Sherlok 1.6.0.3 ((c)21.11.2008/2011 by Albert Zedlitz)");
        mLimitIO                = 1000;
        mLimitHash              = gHashValue;
        mLimitHistory           = 10;
        mDumpLevel              = 0;
        mProfilerMode           = PROFILER_MODE_PROFILE;
        mOutputStream           = XMLWRITER_TYPE_ASCII;
        
        mOutputSeparator        = cU("| ");
        mLogFile                = cU("sherlok.log");
        
        mFileName               = cU("sherlok.properties");
        mFilePath               = cU(".");
        mPwdFile                = cU("sherlok.pwd");
        mHost                   = cU("localhost");
        mStackSize              = 1024;

        mJvmUpdate              = NULL;
        mJvm                    = NULL;
        mJvmti                  = NULL;
        mJvmtiUpdate            = NULL;
    }
    // ------------------------------------------------------------
    // TProperties::~TProperties
    //! Destructor
    // ------------------------------------------------------------
    ~TProperties() {
        delete mPackageFilter;
        delete mPackageFilterExclude;
        delete mMethodsFilter;
        delete mMethodsDebug;
        delete mExecutionTimer;
        delete mClassDebug;
        delete mScope;
        delete mTimers;
        delete mTriggerFilter;
        delete mTraceOptions;
        delete mExceptions;
		delete mLogOptions;
    }    
public:
    // ------------------------------------------------------------
    // TProperties::initialize
    // ------------------------------------------------------------
    void initialize(
            JavaVM   *aJvm,
            jvmtiEnv *aJvmti) {

        if (!mInitialized) {
            mInitialized = true;
            mJvmti = aJvmti;
            mJvm   = aJvm;
        }
    }
    // ------------------------------------------------------------
    // ------------------------------------------------------------
    JavaVM *getJavaVm() {
        return mJvm;
    }
    void setCapaException(bool aCheck) {
        mCanGenExEvents = aCheck;
    }
    bool getCapaException() {
        return mCanGenExEvents;
    }
    // ------------------------------------------------------------
    // ------------------------------------------------------------
    jvmtiEnv *getJvmti() {
        return mJvmti;
    }
    // ------------------------------------------------------------
    // ------------------------------------------------------------
    void setJvmti(jvmtiEnv *aJvmti) {
        mJvmti = aJvmti;
    }
    // ------------------------------------------------------------
    // TProperties::getInstance
    //! Static constructor
    // ------------------------------------------------------------
    static TProperties *getInstance() {
        if (mInstance == NULL) {
            mInstance = new TProperties;
        }
        return mInstance;
    }
    // ------------------------------------------------------------
    // TProperties::mStackSize
    //! \return Current maximum call stack size
    // ------------------------------------------------------------
    int getStackSize() {
        return mStackSize;
    }
    // ------------------------------------------------------------
    // TProperties::setDumpOnExit
    //! \brief Activity at exit
    //! \param aEnable \c TRUE to request dump on exit of JVM
    // ------------------------------------------------------------
    void setDumpOnExit (bool aEnable) {
        mDumpOnExit = aEnable;
    }
    // ------------------------------------------------------------
    // TProperties::getDumpOnExit
    //! \brief Activity at exit
    //! \return \c TRUE if dump on exit of JVM requested
    // ------------------------------------------------------------
    bool getDumpOnExit() {
        return mDumpOnExit;
    }
    void setContention(bool aEnable) {
        mDoContention = aEnable;
    }
    bool doContention() {
        return mDoContention;
    }
    // ------------------------------------------------------------
    // TProperties::parseOptions
    //! \brief Process command line options
    //! \param aOptions The command line
    // ------------------------------------------------------------
    void parseOptions(/*SAPUNICODEOK_CHARTYPE*/ const char *aOptions) {
        TValues::iterator aPtr;
        TValues   aOptionList(16);
        TString   aCmdLine;
        TProperty aCmdProperty;
        bool      aFoundProp = false;

        setDefault();
        if (aOptions == NULL || *aOptions == cR('\0')) {
            loadScpFiles(true);
            parseFile();
            return;
        }        
        aCmdLine.assignR(aOptions, STRLEN_A7(aOptions));
        aCmdLine.split(&aOptionList, cU(','));

        // Load configuration files from configuration path 
        // The program argument for ConfigPath superseeds the default
        for (aPtr  = aOptionList.begin();
             aPtr != aOptionList.end();
             aPtr ++) {            
            aCmdProperty = *aPtr;
            
            if (STRNCMP(aCmdProperty.getKey(), cU("ConfigPath"), 10) == 0) {
                parseProperty(&aCmdProperty);
                aFoundProp = true;
                break;
            }
        }
        
        if (!aFoundProp) {
            aCmdProperty = cU("ConfigPath=.");
            parseProperty(&aCmdProperty);
        }

        // Load configuration from ConfigFile 
        // The program argument for ConfigFile superseeds the default 
        aFoundProp = false;
        for (aPtr  = aOptionList.begin();
             aPtr != aOptionList.end();
             aPtr ++) {            
            aCmdProperty = (*aPtr);
            
            if (STRNCMP(aCmdProperty.getKey(), cU("ConfigFile"), 10) == 0) {
                parseProperty(&aCmdProperty);
                aFoundProp = true;
                break;
            }
        }   

        if (!aFoundProp) {
            aCmdProperty = cU("ConfigFile=default.skp");
            parseProperty(&aCmdProperty);
        }
        loadScpFiles(!aFoundProp);
        parseFile();

        // All other program arguments will supersseed the default settings
        // ConfigFile and ConfigPath from command line superseed any other settings
        for (aPtr  = aOptionList.begin();
             aPtr != aOptionList.end();
             aPtr ++) {    
            aCmdProperty = (*aPtr);
            if (STRNCMP(aCmdProperty.getKey(), cU("ConfigPath"), 10) != 0 &&
                STRNCMP(aCmdProperty.getKey(), cU("ConfigFile"), 10) != 0) {
                parseProperty(&aCmdProperty);
            }
        }
    }
    // ------------------------------------------------------------
    // TProperties::setDefault
    //! \brief Default settings
    //!
    //! Set defaults, if no valid configuration could be found
    // ------------------------------------------------------------
    void setDefault() {
        TProperty aCmdProperty;

        mMinClassSize     = 0;
        mOutputStream     = XMLWRITER_TYPE_ASCII;
        mComprLine        = true;
        mMemoryOn         = true;
        mTimerValue       = 0;

        aCmdProperty  = cU("TelnetPort=2424");
        parseProperty(&aCmdProperty);

        aCmdProperty  = cU("ProfileScope=.");
        parseProperty(&aCmdProperty);

        aCmdProperty  = cU("ProfilePackages= ");
        parseProperty(&aCmdProperty);

        //aCmdProperty = cU("ConfigPath=.");
        //parseProperty(&aCmdProperty);

        //aCmdProperty = cU("ConfigFile=default.skp");
        //parseProperty(&aCmdProperty);
    }
    // ------------------------------------------------------------
    // TProperties::loadScpFiles
    //! \brief Configuration management
    //! \param bFindDefault  \c TRUE to activate default.skp
    // ------------------------------------------------------------
    void loadScpFiles(bool bFindDefault = true) {
#if defined(_WINDOWS)
        HANDLE          aSearch;
        WIN32_FIND_DATA aFileData; 
        BOOL            aHasNext   = true;
#endif 
        TString  aStrPath;
        TString *aName;
        bool     aFoundDefault = false;

        aStrPath = mPath.str();
        aStrPath.concatPathExt(cU("*.skp"));
        mScpFiles.reset();

#if defined (_WINDOWS) 
        aSearch = FindFirstFile(aStrPath.str(), &aFileData);
        if (aSearch == INVALID_HANDLE_VALUE) {
            ERROR_OUT(cU("loadScpFiles"), GetLastError());
            return;
        }
        if (bFindDefault) { 
            mFileName = aFileData.cFileName;
        }
        aHasNext  = true;

        while (aHasNext) {
            if (bFindDefault && !STRCMP(aFileData.cFileName, cU("default.skp"))) {
                mFileName     = aFileData.cFileName;
                aFoundDefault = true;
            }
            aName    = mScpFiles.push();
           *aName    = aFileData.cFileName;
            aHasNext = FindNextFile(aSearch, &aFileData);
        }
        FindClose(aSearch); 
#else
        DIR            *aDir;
        /*SAPUNICODEOK_LIBSTRUCT*/
        struct dirent  *aDirFile;
        TString         aFileName;

        aDir = opendir(mPath.a7_str());
        if (aDir == NULL) {
            ERROR_OUT(cU("loadScpFiles"), 1);
            return;
        }
        while ((aDirFile = readdir(aDir)) != NULL) {
            aFileName.assignR((const char*)aDirFile->d_name, strlenR(aDirFile->d_name));
            if (aFileName.find(cU(".skp")) != aFileName.end()) {
                aName  = mScpFiles.push();
               *aName  = aFileName.str();
            }
            if (bFindDefault && !STRCMP(aFileName.str(), cU("default.skp"))) {
                mFileName     = aFileName.str();
                aFoundDefault = true;
            }
        }
        closedir(aDir);
#endif
        if (!aFoundDefault && bFindDefault) {
            mFileName = mScpFiles.top()->str();
        }
        mPropertyPath = mPath.str();
        mPropertyPath.concatPathExt(mFileName.str());
    }
    // ------------------------------------------------------------
    // TProperties::getFileList
    //! \brief  Configuration management
    //! \return The list of configuration files
    // ------------------------------------------------------------
    TStackFile *getFileList() {
        return &mScpFiles;
    }
    // ------------------------------------------------------------
    // TProperties::getPropertyFile
    //! \brief  Configuration management
    //! \return The current configuration file
    // ------------------------------------------------------------
    const SAP_UC *getPropertyFile() {
        return mFileName.str();
    }
    // ------------------------------------------------------------
    // TProperties::dumpFileList
    //! \brief  Configuration management
    //! 
    //! Dump the list of configuration files
    // ------------------------------------------------------------
    void dumpFileList(TXmlTag *aRootTag) {
        TStackFile::iterator aPtr;
        SAP_UC     aBuffer[128];
        int      aInx = 0;
        TXmlTag *aTag;

        for (aPtr  = mScpFiles.begin();
             aPtr != mScpFiles.end();
             aPtr  = mScpFiles.next()) {
            aInx ++;

            aTag = aRootTag->addTag(cU("File"));
            aTag->addAttribute(cU("FileName"), (*aPtr).str());
            aTag->addAttribute(cU("ID"), TString::parseInt(aInx, aBuffer), PROPERTY_TYPE_HIDDEN);
        }
    }
    // ------------------------------------------------------------
    // TProperties::parseFile
    //! \brief  Configuration management
    //! \param  aFile The new configuration file
    //! Read a new configuration file
    // ------------------------------------------------------------
    void parseFile(const SAP_UC *aFile) {
        mPropertyPath = mPath.str();
        mPropertyPath.concatPathExt(aFile);
        parseFile();
    }
    // ------------------------------------------------------------
    // TProperties::parseFile
    //! \brief  Configuration management
    //! \return Read the current configuration file
    // ------------------------------------------------------------
    void parseFile() {
        /*SAPUNICODEOK_CHARCONST*//*SAPUNICODEOK_LIBFCT*/
        if (ACCESS(mPropertyPath.a7_str(), 0) == -1) {
            ERROR_OUT(mPropertyPath.str(), 100);
            return;
        }
        TProperty aProperty;
        SAP_ifstream aFile;
        aFile.rdbuf()->open(mPropertyPath.a7_str(), ios::in);
        // read all lines
        while (!aFile.eof() && aFile.rdbuf()->is_open()) {
            aProperty.readLine(&aFile, 4098);
            parseProperty(&aProperty);
        }
        aFile.close();
        mLoadNewSkp = false;
    }
    // ------------------------------------------------------------
    // TProperties::parseFile
    //! \brief  Configuration management
    //! \param  aProperty Property to process
    // ------------------------------------------------------------
    bool parseProperty(TProperty *aProperty) {
        bool aValid = true;

        // read all lines
        if (!aProperty->isValid()) {
            return false;
        } 
        else if (aProperty->equalsKey(cU("Debug"))) {
            if (!STRCMP(aProperty->getValue(), cU("yes"))) {
#ifdef _WINDOWS
                DebugBreak();
#else
                int aDebugWait = 0;
                ERROR_OUT(cU("Wait for debugger ...."), 0);
                while (aDebugWait++ < 10) {
                    SLEEP(5);
                }
#endif
            }
        } else if (aProperty->equalsKey(cU("ProfileStart"))) {
            mDoMonitor = STRCMP(aProperty->getValue(), cU("yes"))  == 0 ||
                         STRCMP(aProperty->getValue(), cU("true")) == 0;
        } else if (aProperty->equalsKey(cU("ProfilePackages"))) {
            aProperty->split(mPackageFilter, cU(','));
        } else if (aProperty->equalsKey(cU("ProfileExcludes"))) {
            aProperty->split(mPackageFilterExclude, cU(','));
        } else if (aProperty->equalsKey(cU("ProfileMethods"))) {
            aProperty->split(mMethodsFilter, cU(','));
        } else if (aProperty->equalsKey(cU("ProfileHide"))) {
            aProperty->split(mHideFilter, cU(','));
        } else if (aProperty->equalsKey(cU("ProfileInfo"))) {
            mProfileInfo = aProperty->getInfo();
        } else if (aProperty->equalsKey(cU("TraceMethods"))) {
            aProperty->split(mMethodsDebug, cU(';'));
        } else if (aProperty->equalsKey(cU("ProfileLimitSize"))) {
            mMinClassSize = aProperty->toInteger();
        } else if (aProperty->equalsKey(cU("ProfileOutputType"))) {
            if (!STRCMP(aProperty->getValue(), cU("xml"))) {
                mOutputStream = XMLWRITER_TYPE_XML;
            }
            else {
                mOutputStream = XMLWRITER_TYPE_ASCII;
            }
        } else if (aProperty->equalsKey(cU("ProfileOutputSeparator"))) {
            mOutputSeparator = aProperty->getValue();
        } else if (aProperty->equalsKey(cU("TraceVerbose"))) {
            if (!STRCMP(aProperty->getValue(), cU("no")) ||
                !STRCMP(aProperty->getValue(), cU("off"))) {
                mComprLine = true;
            }
            else {
                mComprLine = false;
            }
        } else if (aProperty->equalsKey(cU("ClassDebug"))) {
            aProperty->split(mClassDebug, cU(','));
        } else if (aProperty->equalsKey(cU("ThreadSampleTime"))) {
            mThreadSampleTime = aProperty->toInteger();
        } else if (aProperty->equalsKey(cU("ProfileScope"))) {
            aProperty->split(mScope, cU(','));
        } else if (aProperty->equalsKey(cU("TraceTrigger"))) {
            aProperty->split(mTriggerFilter, cU(','));
        } else if (aProperty->equalsKey(cU("TraceOutputType"))) {
        } else if (aProperty->equalsKey(cU("TimerMethods"))) {
            aProperty->split(mTimers, cU(','));
        } else if (aProperty->equalsKey(cU("TelnetPort"))) {
            mTelnetPort = (int)aProperty->toInteger();
        } else if (aProperty->equalsKey(cU("Timer"))) {
            mTimerValue = 0;
            if (!STRNCMP(aProperty->getValue(), cU("on"), 2)) {
                mTimerValue = TIMER_THREAD | TIMER_METHOD;
            }
            else if (!STRNCMP(aProperty->getValue(), cU("clock"), 5)) {
                mTimerValue = TIMER_THREAD | TIMER_METHOD | TIMER_CLOCK;
            }
            else if (!STRNCMP(aProperty->getValue(), cU("hpc"), 3)) {
                
                if (TSystem::setHpcTimer()) {
                    mTimerValue = TIMER_HPC | TIMER_THREAD | TIMER_METHOD;
                }
                else {
                    mTimerValue = TIMER_THREAD | TIMER_METHOD;
                }
            }
            else {
                aProperty->split(mExecutionTimer, cU(','));
                if (findEntry(mExecutionTimer, cU("THREAD")) != NULL) {
                    mTimerValue |= TIMER_THREAD;
                }
                if (findEntry(mExecutionTimer, cU("METHOD")) != NULL) {
                    mTimerValue |= TIMER_METHOD;
                }
            }
        } else if (aProperty->equalsKey(cU("ProfileMemory"))) {
            mMemoryOn   = (STRCMP(aProperty->getValue(), cU("off")) != 0);
            mMemoryOn  |= (STRCMP(aProperty->getValue(), cU("all")) == 0);
        } else if (aProperty->equalsKey(cU("ProfilerTriggerMode")) ||
                   aProperty->equalsKey(cU("ProfileMode"))) {
            mProfilerMode = PROFILER_MODE_PROFILE;
            if (!STRCMP(aProperty->getValue(), cU("interrupt"))) {
                mProfilerMode = PROFILER_MODE_TRIGGER;
            }
            if (!STRCMP(aProperty->getValue(), cU("jarm"))) {
                mProfilerMode = PROFILER_MODE_JARM;
                mDoMonitor  = true;
                mTimerValue = TIMER_THREAD | TIMER_METHOD;
                mMemoryOn   = false;
            }
            if (!STRCMP(aProperty->getValue(), cU("ats"))) {
                mProfilerMode = PROFILER_MODE_ATS;
                mDoMonitor  = true;
                mTimerValue = TIMER_THREAD | TIMER_METHOD;
                mMemoryOn   = true;
            }
        } else if (aProperty->equalsKey(cU("MemoryStatistic"))) {
            mMemoryInfo  = false;
            mMemoryAlert = false;
            mMemoryTotal = false;

            if (!STRCMP(aProperty->getValue(), cU("alert"))) {
                mMemoryAlert = true;
                mHeapDump    = true;
            } else if (!STRCMP(aProperty->getValue(), cU("total"))) {
                mMemoryInfo  = true;
                mMemoryTotal = true;
            } else if (!STRCMP(aProperty->getValue(), cU("info")))  {
                mMemoryInfo  = true;
            } else if (!STRCMP(aProperty->getValue(), cU("noheap-alert")))  {
                mMemoryAlert = true;
                mHeapDump    = false;
            }
        } else if (aProperty->equalsKey(cU("Tracer"))) {
            aProperty->split(mTraceOptions, cU(','));
		} else if (aProperty->equalsKey(cU("Logger"))) {
			//aProperty->split(mLogOptions, cU(','));
			//TLogger *aLogger = TLogger::getInstance();
			//if (STRCMP(aProperty->getValue(), cU("on")) == 0) {
			//	aLogger->start();
			//}
			//if (STRCMP(aProperty->getValue(), cU("append")) == 0) {
			//	aLogger->start(true);
			//}
		} else if (aProperty->equalsKey(cU("DumpLevel"))) {
            mDumpLevel = (short)aProperty->toInteger();
            if (mDumpLevel < 0 || mDumpLevel > 1)
                mDumpLevel = 0;
        } else if (aProperty->equalsKey(cU("DumpOnExit"))) {
            mDumpOnExit = STRCMP(aProperty->getValue(), cU("yes")) == 0;
        } else if (aProperty->equalsKey(cU("TelnetHost"))) {
            mHost = aProperty->getValue();
        } else if (aProperty->equalsKey(cU("ConfigFile"))) {
            mFileName = aProperty->getValue();
            mFileName.checkPath();
            mFileName.cut(mFileName.findLastOf(FILESEPARATOR) + 1);

            if (mInitPath) {
                mInitPath = false;
                mPath = aProperty->getValue();
                mPath.checkPath();
                mPath.cut(0, mPath.findLastOf(FILESEPARATOR) + 1);

                mLogFile = mPath.str();
                mLogFile.concatPathExt(cU("sherlok.log"));

                mPwdFile = mPath.str();
                mPwdFile.concatPathExt(cU("sherlok.pwd"));
            }
            else {
                mLoadNewSkp = true;
            }
            mPropertyPath = mPath.str();
            mPropertyPath.concatPathExt(mFileName.str());
        } else if (aProperty->equalsKey(cU("ConfigPath"))) {
            if (mInitPath) {
                mInitPath = false;
                mPath = aProperty->getValue();
                mPath.checkPath();

                mLogFile = mPath.str();
                mLogFile.concatPathExt(cU("sherlok.log"));

                mPwdFile = mPath.str();
                mPwdFile.concatPathExt(cU("sherlok.pwd"));
            }
            else {
                mLoadNewSkp = true;
            }
            loadScpFiles();
        } else if (aProperty->equalsKey(cU("ProfileLimitOutput"))) {
            mLimitIO = (jint)aProperty->toInteger();
            if (mLimitIO < 100)
                mLimitIO = 100;
        } else if (aProperty->equalsKey(cU("ProfileLimitHash"))) {
            mLimitHash = (jint)aProperty->toInteger();
            if (mLimitHash < (jint)gHashValue)
                mLimitHash = gHashValue;
            
        } else if (aProperty->equalsKey(cU("MemoryLimitHistory"))) {
            mLimitHistory = (jint)aProperty->toInteger();
            if (mLimitHistory < 10)
                mLimitHistory = 10;
        } else if (aProperty->equalsKey(cU("StackSize"))) {
            mStackSize    = (int)aProperty->toInteger();
            if (mStackSize < 128 || mStackSize > 2048) {
                mStackSize = 1024;
            }
        } else { 
            TString aMsg;
            aMsg.concat(cU("Unknown property ---"));
            aMsg.concat(aProperty->getKey());
            aMsg.concat(cU("---"));
            ERROR_OUT(aMsg.str(), -1);
            aValid = false;
        }
        return aValid;
    }
    // ------------------------------------------------------------
    // TProperties::getPath
    //! \brief  Configuration management
    //! \return Path to the configuration
    // ------------------------------------------------------------
    const SAP_UC *getPath() {
        if (mPath.pcount() == 0) {
            mPath = cU(".");
        }
        return mPath.str();
    }
    // ------------------------------------------------------------
    // TProperties::doMonitorPackage
    //! \brief  Access to configuration
    //! \param  aPackageName The package to check
    //! \return \c TRUE if the package is in the list of values 
    //!         TProperties::mPackageFilter
    // ------------------------------------------------------------
    inline bool doMonitorPackage(const SAP_UC *aPackageName) {
        bool aFound = findEntry(mPackageFilter, aPackageName) != NULL;
        return aFound;
    }
    // ------------------------------------------------------------
    // TProperties::dontMonitorPackage
    //! \brief  Access to configuration
    //! \param  aPackageName The package to check
    //! \return \c TRUE if the package is in the list of values 
    //!         TProperties::mPackageFilterExclude
    // ------------------------------------------------------------
    inline bool dontMonitorPackage(const SAP_UC *aPackageName) { 
        return 
            (mPackageFilterExclude->getDepth() > 0) &&
            (findEntry(mPackageFilterExclude, aPackageName) != NULL);
    }
    // ------------------------------------------------------------
    // TProperties::doMonitorScope
    //! \brief  Access to configuration
    //! \param  aPackageName The package to check
    //! \return \c TRUE if the package is in the list of values 
    //!         TProperties::mScope
    // ------------------------------------------------------------
    inline bool doMonitorScope(const SAP_UC *aPackageName) {
        return findEntry(mScope, aPackageName) != NULL;
    }
    // ------------------------------------------------------------
    // TProperties::doMonitorVisible
    //! \brief  Access to configuration
    //! \return \c TRUE if the package is in the list of values 
    //!         TProperties::mHideFilter
    // ------------------------------------------------------------
    inline bool doMonitorVisible(const SAP_UC *aPackageName) {
        return findEntry(mHideFilter, aPackageName) == NULL;
    }
    // ------------------------------------------------------------
    // TProperties::doMonitorMethod
    //! \brief  Access to configuration
    //! \param  aClassName  The class/package name to check
    //! \param  aMethodName A method of aClassName
    //! \return \c TRUE if the method is in the list of values 
    //!         TProperties::mMethodsFilter
    // ------------------------------------------------------------
    bool doMonitorMethod(const SAP_UC *aClassName, const SAP_UC *aMethodName) {
        SAP_UC *aEntry = getMonitorMethodEntry(aClassName, aMethodName);
        return aEntry != NULL;
    }
    // ------------------------------------------------------------
    // TProperties::doMonitorTimer
    //! \brief  Access to configuration
    //! \param  aClassName  The class/package name to check
    //! \param  aMethodName A method of aClassName
    //! \return \c TRUE if the method is in the list of values 
    //!         TProperties::mTimers
    // ------------------------------------------------------------
    bool doMonitorTimer(const SAP_UC *aClassName, const SAP_UC *aMethodName) {
        bool  aResult;
        TString aString(aClassName, aMethodName);
        aResult = (findEntry(mTimers, aString.str()) != NULL);
        return aResult;
    }
    // ------------------------------------------------------------
    // TProperties::getMonitorDebugEntry
    //! \brief  Access to configuration
    //! \param  aClassName  The class/package name to check
    //! \param  aMethodName A method of aClassName
    //! \return Pointer to entry in the list of values 
    //!         TProperties::mMethodsDebug
    // ------------------------------------------------------------
    SAP_UC *getMonitorDebugEntry(const SAP_UC *aClassName, const SAP_UC *aMethodName) {
        SAP_UC     *aResult = NULL;
        TString   aString(aClassName, aMethodName);
        aResult = findEntryByName(mMethodsDebug, aString.str());
        return aResult;
    }
    // ------------------------------------------------------------
    // TProperties::getMonitorMethodEntry
    //! \brief  Access to configuration
    //! \param  aClassName  The class/package name to check
    //! \param  aMethodName A method of aClassName
    //! \return Pointer to entry in the list of values 
    //!         TProperties::mMethodsDebug
    // ------------------------------------------------------------
    SAP_UC *getMonitorMethodEntry(const SAP_UC *aClassName, const SAP_UC *aMethodName) {
        SAP_UC     *aResult = NULL;
        TString   aString(aClassName, aMethodName);
        aResult = findEntryByName(mMethodsFilter, aString.str());
        return aResult;
    }
    // ------------------------------------------------------------
    // TProperties::doMonitorClass
    //! \brief  Access to configuration
    //! \param  aClassName  The class/package name to check
    //! \return \c TRUE if class name is in the list of values 
    //!         TProperties::mClassDebug
    // ------------------------------------------------------------
    bool doMonitorClassLoad(const SAP_UC *aClassName) {
        return (findEntry(mClassDebug, aClassName) != NULL);
    }
    // ------------------------------------------------------------
    // TProperties::doExecutionTimer
    //! \brief  Access to configuration
    //! \param  aKind  Timer is one of
    //!         - THREADS
    //!         - METHODS
    //!         - TIMER_HPC
    //! \return \c TRUE if given timer is active
    // ------------------------------------------------------------
    bool doExecutionTimer(const int aKind) {
        return ((mTimerValue & aKind) != 0);
    }
    // ------------------------------------------------------------
    // TProperties::parseExceptions
    //! \brief  Access to configuration
    //! \param  aValues Container for the list of exceptions
    // ------------------------------------------------------------
    void parseExceptions(const SAP_UC *aValues) {
        TString lValues(aValues);
        lValues.split(mExceptions, cU(','));
    }
    // ------------------------------------------------------------
    // TProperties::doTraceException
    //! \brief  Access to configuration
    //! \param  aExceptionSignatur The signatur of the exception to check
    //! \return \c TRUE if the exception name is in the list of values
    //!         TProperties::mExceptions
    // ------------------------------------------------------------
    bool doTraceException(const SAP_UC *aExceptionSignatur) {
        return (findSignature(mExceptions, aExceptionSignatur) != NULL);
    }
    // ------------------------------------------------------------
    // TProperties::doMonitoring
    //! \brief  Access to configuration
    //! \return \c TRUE if profiler is active
    // ------------------------------------------------------------
    bool doMonitoring() {
        return mDoMonitor;
    }
    // ------------------------------------------------------------
    // TProperties::doMonitorMemoryOn
    //! \brief  Access to configuration
    //! \return \c TRUE if profiler for memory is active on class level
    // ------------------------------------------------------------
    bool doMonitorMemoryOn() {
        return mMemoryOn;
    }
    // ------------------------------------------------------------
    // TProperties::setStatus
    //! \brief  Access to configuration
    //! \param aStatus Register profiler state
    // ------------------------------------------------------------
    void setStatus(unsigned aStatus) {
        mMonitorActive = aStatus;
    }
    // ------------------------------------------------------------
    // TProperties::setStatus
    //! \brief  Access to configuration
    //! \return Registered profiler state
    // ------------------------------------------------------------
    unsigned getStatus() {
        return mMonitorActive;
    }
    // ------------------------------------------------------------
    // TProperties::getVersion
    //! \param  aExtVersion \c Dump extended version string
    //! \return The version string of sherlok
    // ------------------------------------------------------------
    const SAP_UC *getVersion(bool aExtVersion = false) {
        if (aExtVersion) {
            return mVersionExt;
        }
        else {
            return mVersion;
        }
    }
    // ------------------------------------------------------------
    // TProperties::doHistoryInfo
    //! \brief Access to configuration
    //! \return \c TRUE if MemoryStatistic == info
    // ------------------------------------------------------------
    inline bool doHistoryInfo() {
        return mMemoryInfo;
    }
    // ------------------------------------------------------------
    // TProperties::doHistoryTotal
    //! \brief Access to configuration
    //! \return \c TRUE if MemoryStatistic == total
    // ------------------------------------------------------------
    bool doHistoryTotal() {
        return mMemoryTotal;
    }
    // ------------------------------------------------------------
    // TProperties::doHistory
    //! \brief Access to configuration
    //! \return \c TRUE if MemoryStatistic == info
    // ------------------------------------------------------------
    bool doHistory() {
        return mMemoryAlert || mMemoryInfo;
    }
    // ------------------------------------------------------------
    // TProperties::doHistoryAlert
    //! \brief Access to configuration
    //! \return \c TRUE if MemoryStatistic == alert
    // ------------------------------------------------------------
    bool doHistoryAlert() {
        return mMemoryAlert;
    }
    // ------------------------------------------------------------
    // TProperties::doAutoAction
    //! \brief Access to configuration
    //! \return \c TRUE if acitvation of agent requested
    // ------------------------------------------------------------
    int doAutoAction() {
        return mAutoAction;
    }
    // ------------------------------------------------------------
    // TProperties::getSeparator
    //! \brief Access to configuration
    //! \return The output separator
    // ------------------------------------------------------------
    inline const SAP_UC *getSeparator() {
        return mOutputSeparator.str();
    }
    // ------------------------------------------------------------
    // TProperties::getLogFile
    //! \brief Access to configuration
    //! \return The path to log file
    // ------------------------------------------------------------
    TString *getLogFile() {
        return &mLogFile;
    }
    // ------------------------------------------------------------
    // TProperties::setLogFile
    //! \brief Access to configuration
    //! \param aFileName path to log file 
    // ------------------------------------------------------------
    void setLogFile(SAP_UC *aFileName) {
        mLogFile = aFileName;
    }
    // ------------------------------------------------------------
    // TProperties::getPasswordFile
    //! \brief  Access to configuration
    //! \return The path to password file 
    // ------------------------------------------------------------
    const SAP_A7 *getPasswordFile() {
        return mPwdFile.a7_str();
    }
    // ------------------------------------------------------------
    // TProperties::getMinMemorySize
    //! \brief  Access to configuration
    //! \return The size of the smallest memory chunck to register
    //!         ProfileLimitSize
    // ------------------------------------------------------------
    jlong getMinMemorySize() {
        return mMinClassSize;
    }
    // ------------------------------------------------------------
    // TProperties::getDumpLevel
    //! \brief  Access to configuration
    //! \return The maximum level for tracing DumpLevel
    // ------------------------------------------------------------
    jint getDumpLevel() {
        return mDumpLevel;
    }
    // ------------------------------------------------------------
    // TProperties::getProfilerMode
    //! \brief  Access to configuration
    //! \return The profiler mode. One of
    //          - PROFILER_MODE_TRIGGER 
    //          - PROFILER_MODE_PROFILE
    //          - PROFILER_MODE_ATS
    //          - PROFILER_MODE_JARM
    // ------------------------------------------------------------
    inline jint getProfilerMode() {
        return mProfilerMode;
    }
    // ------------------------------------------------------------
    // TProperties::setProfilerMode
    //! \brief  Access to configuration
    //! \see Properties::getProfilerMode
    //! \param aMode The new profile mode
    // ------------------------------------------------------------
    void setProfilerMode(jint aMode) {
        if (getStatus() == MONITOR_IDLE) {
            mProfilerMode = aMode;
        }
    }
    // ------------------------------------------------------------
    // TProperties::getTelnetPort
    //! \brief  Access to configuration
    //! \return The current telnet port
    // ------------------------------------------------------------
    unsigned short getTelnetPort() {
        return (unsigned short)mTelnetPort;
    }
    // ------------------------------------------------------------
    // TProperties::getTelnetHost
    //! \brief  Access to configuration
    //! \return The current telnet host
    // ------------------------------------------------------------
    const SAP_A7 *getTelnetHost() {
        return mHost.a7_str();
    }
    // ------------------------------------------------------------
    // TProperties::getInfo
    //! \brief  Access to configuration
    //! \return ProfileInfo
    // ------------------------------------------------------------
    const SAP_UC *getInfo() {
        return mProfileInfo.str();
    }
    // ------------------------------------------------------------
    // TProperties::doTrace
    //! \brief  Access to configuration
    //! \return \c TRUE if the tracer is active
    // ------------------------------------------------------------
    bool doTrace() {
        return mTraceOptions->getDepth() > 0;
    }
    // ------------------------------------------------------------
    // TProperties::getTraceOptions
    //! \brief  Access to configuration
    //! \return The tracer options as list
    // ------------------------------------------------------------
    TValues *getTraceOptions() {
        return mTraceOptions;
    }
    // ------------------------------------------------------------
    // TProperties::getConsoleWriterType
    //! \brief  Access to configuration
    //! \return The output stream type. One of
    //!             - XMLWRITER_TYPE_ASCII
    //!             - XMLWRITER_TYPE_XML
    // ------------------------------------------------------------
    int getConsoleWriterType() {
        return mOutputStream;
    }
    // ------------------------------------------------------------
    // TProperties::getComprLine
    //! \brief  Access to configuration
    //! \return \c TRUE if output should be compressed 
    //!             TraceVerbose=on
    // ------------------------------------------------------------
    bool getComprLine() {
        return mComprLine;
    }
    // ------------------------------------------------------------
    // TProperties::getComprLine
    //! \brief  Access to configuration
    //! \param aEnable \c TRUE if output should be compressed 
    // ------------------------------------------------------------
    void setComprLine(bool aEnable) {
        mComprLine = aEnable;
    }
    // ------------------------------------------------------------
    // TProperties::doTrigger
    //! \brief  Access to configuration
    //! \param  aClassName  The class/package name to check
    //! \param  aMethodName A method of aClassName
    //! \param  aSignature  A signature of the method.
    //! \return \c TRUE if the method is in the list of values
    //!         mTriggerFilter
    // ------------------------------------------------------------
    bool doTrigger(
            const SAP_UC *aClassName, 
            const SAP_UC *aMethodName, 
            const SAP_UC *aSignature) {

        bool      aResult;
        TString   aString(aClassName, aMethodName);
        aResult = (findEntryByName(mTriggerFilter, aString.str()) != NULL);
        if (aResult && mTriggerFilter->getDepth() > 1) {
            aString = aSignature;
            aResult = (findEntryByName(mTriggerFilter, aString.str()) != NULL);
        }
        return aResult;
    }
    // ------------------------------------------------------------
    // TProperties::getLimit
    //! \brief  Manage output limits
    //!
    //! Output limits help to achieve best performance handling with 
    //! mass data.
    //! \param  aLimit The requested section. One of
    //!         - LIMIT_IO
    //!         - LIMIT_HASH
    //!         - LIMIT_HISTORY
    //! \return Integer value as limit for the requested section
    // ------------------------------------------------------------
    jint getLimit(int aLimit) {
        switch (aLimit) {
            case LIMIT_IO:
                return mLimitIO;
            case LIMIT_HASH:
                return mLimitHash;
            case LIMIT_HISTORY:
                return mLimitHistory;
            default:
                ERROR_OUT(cU("TProperties::getLimit"), aLimit);
                break;
        }
        return 0;
    }
    // ------------------------------------------------------------
    // TProperties::reset
    //! \brief Reset all configurations
    //! \param aParseFile \c TRUE if configuration file should be reloaded
    // ------------------------------------------------------------
    bool reset(bool aParseFile = true) {
        loadScpFiles(false);

        if (!aParseFile) {
             aParseFile = mLoadNewSkp;
        }

        if (aParseFile) {
            /*SAPUNICODEOK_CHARCONST*//*SAPUNICODEOK_LIBFCT*/
            if (ACCESS(mPropertyPath.a7_str(), 0) == -1) {
                return false;
            }      

            mPackageFilter->reset();
            mMethodsFilter->reset();
            mMethodsDebug->reset();
            mExecutionTimer->reset();
            mPackageFilterExclude->reset();
            mScope->reset();
            mTimers->reset();
            mClassDebug->reset();
            mHideFilter->reset();
            mThreadSampleTime = 30000;
            parseFile();
        }
        return true;
    }
    // ------------------------------------------------------------
    // TProperties::dump
    //! \brief Dump the configuration to tag list
    //! \param aRootTag The result tag list
    // ------------------------------------------------------------
    void dump(TXmlTag *aRootTag) {

        SAP_UC     aBuffer[128];
        TXmlTag   *aTag;

        aTag = aRootTag->addTag(cU("Properties"));
        aTag->addAttribute(cU("Name"), cU("Version"));
        aTag->addAttribute(cU("Value"), mVersion);

        if (mProfileInfo.pcount() > 0) {
            aTag = aRootTag->addTag(cU("Property"));
            aTag->addAttribute(cU("Name"),        cU("Info"));
            aTag->addAttribute(cU("Value"),       mProfileInfo.str());
        }

        aTag = aRootTag->addTag(cU("Property"));
        aTag->addAttribute(cU("Name"), cU("ConfigFile"));
        aTag->addAttribute(cU("Value"), mPropertyPath.str());

        aTag = aRootTag->addTag(cU("Property"));
        aTag->addAttribute(cU("Name"), cU("Timer"));
        aTag->addAttribute(cU("Value"), TString::parseInt(mTimerValue, aBuffer));

        aTag = aRootTag->addTag(cU("Property"));
        aTag->addAttribute(cU("Name"), cU("ProfileLimitSize"));
        aTag->addAttribute(cU("Value"), TString::parseInt(mMinClassSize, aBuffer));

        aTag = aRootTag->addTag(cU("Property"));
        aTag->addAttribute(cU("Name"), cU("ProfileStart"));
        aTag->addAttribute(cU("Value"), TString::parseBool(mDoMonitor, aBuffer));

        aTag = aRootTag->addTag(cU("Property"));
        aTag->addAttribute(cU("Name"), cU("Logger"));
        aTag->addAttribute(cU("Value"), TString::parseBool(mMemoryAlert, aBuffer));

        aTag = aRootTag->addTag(cU("Property"));
        aTag->addAttribute(cU("Name"), cU("TelnetPort"));
        aTag->addAttribute(cU("Value"), TString::parseInt(mTelnetPort, aBuffer));

        aTag = aRootTag->addTag(cU("Property"));
        aTag->addAttribute(cU("Name"), cU("HistoryStatistic"));
        aTag->addAttribute(cU("Value"), TString::parseBool(mMemoryInfo||mMemoryAlert, aBuffer));
    }
    // ------------------------------------------------------------
    // TProperties::dumpValues
    //! \brief Dump values to string.
    //! \param aValues The source for dump.
    //! \param aString The resultant string.
    // ------------------------------------------------------------
    void dumpValues(TString *aString, TValues *aValues) {
		TValues::iterator aPtr = aValues->begin();
        *aString = cU("");
        if (aPtr == aValues->end()) {
            return;
        }
        *aString = *aPtr;
        aPtr = aValues->next();
        while (aPtr != aValues->end()) {
            aString->concat(cU(","));
            aString->concat(*aPtr);
            aPtr  = aValues->next();
        }
    }
    // ------------------------------------------------------------
    // ------------------------------------------------------------
    const SAP_UC *getPropertyFilePath() {
        return mPropertyPath.str();
    }
    // ------------------------------------------------------------
    // TProperties::dumpProperties
    //! \brief Dump the configuration to tag list
    //!
    //! Extended version to TProperties::dump
    //! \param aRootTag The result tag list
    // ------------------------------------------------------------
    void dumpProperties(TXmlTag *aRootTag) {
        TXmlTag *aTag;
        TXmlTag *aNodeTag;
        TString  aStrValue;
        SAP_UC     aBuffer[128];
        aNodeTag = aRootTag;

        if (mProfileInfo.pcount() > 0) {
            aTag = aNodeTag->addTag(cU("Property"));
            aTag->addAttribute(cU("Type"),        cU("ProfileInfo"));
            aTag->addAttribute(cU("Value"),       mProfileInfo.str());
            aTag->addAttribute(cU("Description"), cU("Info"));
        }

        aTag = aNodeTag->addTag(cU("Property"));
        dumpValues(&aStrValue, mScope);
        aTag->addAttribute(cU("Type"),        cU("ProfileScope"));
        aTag->addAttribute(cU("Value"),       aStrValue.str());
        aTag->addAttribute(cU("Description"), cU("List of classes"));

        aTag = aNodeTag->addTag(cU("Property"));
        switch (getProfilerMode()) {
            case PROFILER_MODE_TRIGGER:  aStrValue = cU("trigger"); break;
            case PROFILER_MODE_PROFILE:  aStrValue = cU("profile"); break;
            case PROFILER_MODE_JARM:     aStrValue = cU("jarm");    break;
            case PROFILER_MODE_ATS:      aStrValue = cU("ats");     break;
        }
        aTag->addAttribute(cU("Type"),        cU("ProfileMode"));
        aTag->addAttribute(cU("Value"),       aStrValue.str());
        aTag->addAttribute(cU("Description"), cU("Profile Mode: [profile|interrupt|jarm|ats]"));

        aTag = aNodeTag->addTag(cU("Property"));
        dumpValues(&aStrValue, mPackageFilter);
        aTag->addAttribute(cU("Type"),        cU("ProfilePackages"));
        aTag->addAttribute(cU("Value"),       aStrValue.str());
        aTag->addAttribute(cU("Description"), cU("List of classes added to profiler"));

        aTag = aNodeTag->addTag(cU("Property"));
        dumpValues(&aStrValue, mPackageFilterExclude);
        aTag->addAttribute(cU("Type"),        cU("ProfileExcludes"));
        aTag->addAttribute(cU("Value"),       aStrValue.str());
        aTag->addAttribute(cU("Description"), cU("List of classes removed from profiler"));

        aTag = aNodeTag->addTag(cU("Property"));
        dumpValues(&aStrValue, mHideFilter);
        aTag->addAttribute(cU("Type"),        cU("ProfileHide"));
        aTag->addAttribute(cU("Value"),       aStrValue.str());
        aTag->addAttribute(cU("Description"), cU("List of classes hidden from profiler"));

        aTag = aNodeTag->addTag(cU("Property"));
        dumpValues(&aStrValue, mMethodsFilter);
        aTag->addAttribute(cU("Type"),        cU("ProfileMethods"));
        aTag->addAttribute(cU("Value"),       aStrValue.str());
        aTag->addAttribute(cU("Description"), cU("List of methods added to profiler"));

        aTag = aNodeTag->addTag(cU("Property"));
        mDoMonitor ? aStrValue = cU("yes") : aStrValue = cU("no");
        aTag->addAttribute(cU("Type"),        cU("ProfileStart"));
        aTag->addAttribute(cU("Value"),       aStrValue.str());
        aTag->addAttribute(cU("Description"), cU("Initial startup: [yes|no]"));

        aTag = aNodeTag->addTag(cU("Property"));
        TString::parseInt(mLimitIO, aBuffer);
        aTag->addAttribute(cU("Type"),       cU("ProfileLimitOutput"));
        aTag->addAttribute(cU("Value"),       aBuffer);
        aTag->addAttribute(cU("Description"), cU("Maximum number of output lines for any command"));

        aTag = aNodeTag->addTag(cU("Property"));
        TString::parseInt(mLimitHash, aBuffer);
        aTag->addAttribute(cU("Type"),        cU("ProfileLimitHash"));
        aTag->addAttribute(cU("Value"),       aBuffer);
        aTag->addAttribute(cU("Description"), cU("Maximum number of objects for profiler"));

        aTag = aNodeTag->addTag(cU("Property"));
        switch (mOutputStream) {
            case XMLWRITER_TYPE_XML: 
                aStrValue = cU("xml");   break;
            default: 
                aStrValue = cU("ascii"); break;
        }
        aTag->addAttribute(cU("Type"),        cU("ProfileOutputType"));
        aTag->addAttribute(cU("Value"),       aStrValue.str());
        aTag->addAttribute(cU("Description"), cU("[xml|ascii] Sets the output type"));

        aTag = aNodeTag->addTag(cU("Property"));
        dumpValues(&aStrValue, mMethodsDebug);
        aTag->addAttribute(cU("Type"),        cU("TraceMethods"));
        aTag->addAttribute(cU("Value"),       aStrValue.str());
        aTag->addAttribute(cU("Description"), cU("Methods to activate for tracing"));

        aTag = aNodeTag->addTag(cU("Property"));
        mComprLine ? aStrValue = cU("on") : aStrValue = cU("off");
        aTag->addAttribute(cU("Type"),        cU("TraceVerbose"));
        aTag->addAttribute(cU("Value"),       aStrValue.str());
        aTag->addAttribute(cU("Description"), cU("trace output for GC"));

        aTag = aNodeTag->addTag(cU("Property"));
        aStrValue = cU("off");
        if (mMemoryOn) {
            aStrValue = cU("on");
        }
        aTag->addAttribute(cU("Type"),        cU("ProfileMemory"));
        aTag->addAttribute(cU("Value"),       aStrValue.str());
        aTag->addAttribute(cU("Description"), cU("Switch memory profiling [on|off|all]"));

        aTag = aNodeTag->addTag(cU("Property"));
        TString::parseInt(mLimitHistory, aBuffer);
        aTag->addAttribute(cU("Type"),        cU("MemoryLimitHistory"));
        aTag->addAttribute(cU("Value"),       aBuffer);
        aTag->addAttribute(cU("Description"), cU("Number of entries for memory history ring buffer"));

        aTag = aNodeTag->addTag(cU("Property"));
        aStrValue = cU("none");
        if (mMemoryAlert) {
            aStrValue = cU("alert");
        }
        else if (mMemoryInfo) {
            aStrValue = cU("info");
        }
        aTag->addAttribute(cU("Type"),        cU("MemoryStatistic"));
        aTag->addAttribute(cU("Value"),       aStrValue.str());
        aTag->addAttribute(cU("Description"), cU("[alert|info] Memory leak detection for alert"));

        aTag = aNodeTag->addTag(cU("Property"));
        mTimerValue != 0 ? aStrValue = cU("on") : aStrValue = cU("off");
        if (mTimerValue & TIMER_HPC) {
            aStrValue = cU("hpc");
        }
        aTag->addAttribute(cU("Type"),         cU("Timer"));
        aTag->addAttribute(cU("Value"),       aStrValue.str());
        aTag->addAttribute(cU("Description"), cU("Activtes the timer for all methods [on|off]"));

        aTag = aNodeTag->addTag(cU("Property"));
        dumpValues(&aStrValue, mTimers);
        aTag->addAttribute(cU("Type"),        cU("TimerMethods"));
        aTag->addAttribute(cU("Value"),       aStrValue.str());
        aTag->addAttribute(cU("Description"), cU("Methods to activate for time measurement"));

        aTag = aNodeTag->addTag(cU("Property"));
        aTag->addAttribute(cU("Type"),        cU("ProfileOutputSeparator"));
        aTag->addAttribute(cU("Value"),       mOutputSeparator.str());
        aTag->addAttribute(cU("Description"), cU("Output separator for traces"));

        aTag = aNodeTag->addTag(cU("Property"));
        aStrValue = cU("");
        if (mTriggerFilter->getSize() > 0) {
            aStrValue = *mTriggerFilter->begin();
        }
        aTag->addAttribute(cU("Type"),        cU("TraceTrigger"));
        aTag->addAttribute(cU("Value"),       aStrValue.str());
        aTag->addAttribute(cU("Description"), cU("Trigger method for trace"));

        aTag = aNodeTag->addTag(cU("Property"));
        TString::parseInt(mTelnetPort, aBuffer);
        aTag->addAttribute(cU("Type"),        cU("TelnetPort"));
        aTag->addAttribute(cU("Value"),       aBuffer);
        aTag->addAttribute(cU("Description"), cU("Port to connect"));

        aTag = aNodeTag->addTag(cU("Property"));
        aTag->addAttribute(cU("Type"),        cU("TelnetHost"));
        aTag->addAttribute(cU("Value"),       mHost.str());
        aTag->addAttribute(cU("Description"), cU("Hostname for remote access"));

        aTag = aNodeTag->addTag(cU("Property"));
        aTag->addAttribute(cU("Type"),        cU("ConfigFile"));
        aTag->addAttribute(cU("Value"),       mPropertyPath.str());
        aTag->addAttribute(cU("Description"), cU("Active config file"));
        aTag = aNodeTag->addTag(cU("Property"));

        dumpValues(&aStrValue, mTraceOptions);
        aTag->addAttribute(cU("Type"),        cU("Tracer"));
        aTag->addAttribute(cU("Value"),       aStrValue.str());
        aTag->addAttribute(cU("Description"), cU("Trace startup"));
    }
private:
    // ------------------------------------------------------------
    // TProperties::findEntry
    //! \brief  Search alowing dots as wildcards
    //!
    //! Wildcards are allowed at the begin and the end of the search value
    //! \param  aValueList The search list
    //! \param  aValue     The search parameter with wildcard
    //! \return The pointer to the entry found or NULL if parameter not in the list
    // ------------------------------------------------------------
    SAP_UC *findEntry(TValues *aValueList, const SAP_UC *aValue) {
        int aPos   = 0;
        TValues::iterator aPtr;
        TString aString(aValue);

        for (aPtr  = aValueList->begin(); 
             aPtr != aValueList->end(); 
             aPtr  = aValueList->next()) {
            aPos   = aString.findWithWildcard(*aPtr, cU('.'));
            if (aPos != -1) {
                return (*aPtr);
            }
        }
        return NULL;
    }
    // ------------------------------------------------------------
    // TProperties::findSignature
    //! \brief  Search signature
    //!
    //! \param  aValueList The search list
    //! \param  aValue     The search parameter 
    //! \return The pointer to the entry found or NULL if parameter not in the list
    // ------------------------------------------------------------
    SAP_UC *findSignature(TValues *aValueList, const SAP_UC *aValue) {
        int aPos   = 0;
        TValues::iterator aPtr;
        TString aString(aValue);

        for (aPtr  = aValueList->begin(); 
             aPtr != aValueList->end(); 
             aPtr  = aValueList->next()) {

            aPos = aString.findWithWildcard(*aPtr, cU('.'));
            if (aPos != -1) {
                return (*aPtr);
            }
        }
        return NULL;
    }
    // ------------------------------------------------------------
    // TProperties::findEntryByName
    //! \brief  Search a name in a list allowing wildcards
    //!
    //! The name expression may contain parameter expression, which is separated with
    //! curled brackets and stack expression. The stack expression will be used to 
    //! find a method in a callstack. An example would be
    //! /.../.MyClass./.../.TestMethod{}
    //!
    //! \param  aValueList The search list
    //! \param  aValue     The search parameter 
    //! \return The pointer to the entry found or NULL if parameter not in the list
    // ------------------------------------------------------------
    SAP_UC *findEntryByName(TValues *aValueList, const SAP_UC *aValue) {
        int   aStart =  0;
        int   aEnd   = -1;
        int   aPos   =  0;

        TValues::iterator aPtr;
        TString aNameString(aValue);
        TString aEntry;

        for (aPtr  = aValueList->begin(); 
             aPtr != aValueList->end(); 
             aPtr  = aValueList->next()) {

            aEntry = (*aPtr);
            aStart = aEntry.findLastOf(cU('/')) + 1;
            aEnd   = aEntry.findFirstOf(cU('{'));
            aEntry.cut(aStart, aEnd);
            aPos = aNameString.findWithWildcard(aEntry.str(), cU('.'));

            if (aPos != -1) {
                return (*aPtr);
            }
        }
        return NULL;
    }
};
#endif

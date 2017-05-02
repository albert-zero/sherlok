// -----------------------------------------------------------------
//
// Author: Albert Rossmann
// File  : types.cpp
// Date  : 27.08.2008
// Abstract:
//    Print types
//
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
#include "cti.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <hash_map>
using namespace std;

// -----------------------------------------------------------------
// -----------------------------------------------------------------
/*SAPUNICODEOK_CHARTYPE*/
extern "C" string JNICALL typeToString(char *aNameList, char *aSignature, ...) {
    ostringstream  aResult;
    istringstream  aSignStream(aSignature);
    istringstream  aNameStream(aNameList);
    string         aToken;
    vector<string> aParams;
    vector<string> aNames;
    hash_map<string, string> aHash;

    while (getline(aSignStream, aToken, ',')) {
        aParams.push_back(aToken);
    }
    va_list  aVarList;
    va_start(aVarList, aSignature);


    for (string xToken : aParams) {
        size_t aPos  = xToken.find(":");
        string aName = xToken.substr(0, aPos);
        string aType = xToken.substr(aPos + 1);

        if (aType.compare("char")) {
            ostringstream aValue;
            aValue << (char)va_arg(aVarList, char);
            aHash[aName] = aValue.str();
            continue;
        }
        if (aType.compare("char*")) {
            ostringstream aValue;
            aValue << (char*)va_arg(aVarList, char*);
            aHash[aName] = aValue.str();
            continue;
        }
        if (aType.compare("char**")) {
            ostringstream aValue;
            char **aTmp = (char**)va_arg(aVarList, char**);
            aValue << aTmp[0];
            aHash[aName] = aValue.str();
            continue;
        }
        if (aType.compare("int")) {
            ostringstream  aValue;
            aValue << (int)va_arg(aVarList, int);
            aHash[aName] = aValue.str();
            continue;
        }

        break;
    }

    va_end(aVarList);
 
    while (getline(aNameStream, aToken, ',')) {
        auto aIt = aHash.find(aToken);
        if (aIt != aHash.end()) {
            aResult << "aToken:" << aIt->second << ",";
        }
    }

    return aResult.str();
}

// -----------------------------------------------------------------
// -----------------------------------------------------------------
extern "C" JNIEXPORT jint JNICALL CtiTypesOnLoad(TCtiInterface **mCti) {
    if (mCti == NULL || *mCti == NULL) {
        return 1;
    }
    if ((*mCti)->mVersion < CTI_VERSION_2) {
        return 2;
    }
    (*mCti)->toString = typeToString;
    return 0;
}


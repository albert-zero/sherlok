#
#    sherlok.unix.mak: 
#    Makefile for gnu-make for
JAVA_PLATFORM = linux
#    
#    
#    Set environment variable JAVA_HOME to the JDK directory (needed to find JVMTI includes)
#    > set JAVA_HOME=\Program Files\Java\jdk

#    Copyright (C) 2015  Albert Zedlitz

#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.

#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.

#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# -------------------------------------------------------------------------

SHERLOK_AGENT = libsherlok.so
SHERLOK_TEST  = profiler.exe
SHERLOK_CTI   = libctilib.so

CPPFLAGS      = -g -fPIC -pthread -Wall -m64 -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/$(JAVA_PLATFORM)

SHERLOK_AGENT_SRC =     \
        cjvmti.cpp      \
        cjvmpi.cpp      \
        system.cpp

SHERLOK_TEST_SRC =      \
        Profiler.cpp    \
        cti.cpp         \
	cjvmti.cpp      \
	cjvmpi.cpp      \
	system.cpp   

SHERLOK_CTI_SRC =       \
        cti.cpp

SYSTEM_LIBS   =

SHERLOK_AGENT_OBJ = $(SHERLOK_AGENT_SRC:.cpp=.o)
SHERLOK_TEST_OBJ  = $(SHERLOK_TEST_SRC:.cpp=.o)
SHERLOK_CTI_OBJ   = $(SHERLOK_CTI_SRC:.cpp=.o)

all    : $(SHERLOK_AGENT) $(SHERLOK_TEST) $(SHERLOK_CTI)

$(SHERLOK_AGENT) : $(SHERLOK_AGENT_OBJ)
        $(CC) $(CPPFLAGS) -shared -o $(SHERLOK_AGENT) $(SHERLOK_AGENT_OBJ)

$(SHERLOK_TEST)  : $(SHERLOK_TEST_OBJ)
        $(CC) $(CPPFLAGS) -o $(SHERLOK_TEST) $(SHERLOK_TEST_OBJ)

$(SHERLOK_CTI) : $(SHERLOK_CTI_OBJ)
        $(CC) $(CPPFLAGS) -shared -o $(SHERLOK_CTI) $(SHERLOK_CTI_OBJ)

.SUFFIXES: .cpp .o
.cpp.o:
        $(CC) $(CPPFLAGS) -o $@ -c $<


~
~
~
                                                                                                                                       1,0-1         All

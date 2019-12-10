#
#    sherlok.mak: 
#    Makefile for windows nmake 
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

SHERLOK_AGENT = sherlok.dll
SHERLOK_TEST  = profiler.exe
SHERLOK_CTI   = ctilib.dll

CPPFLAGS = /GS /W3 /Zc:wchar_t /Zi /Gm /Od /sdl /fp:precise -DWIN32 -D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS -D_WINDOWS /D "_USRDLL_WINDLL" /D "_UNICODE" /D "UNICODE" /Zc:forScope /RTC1 /Gd /MDd /EHsc /nologo -I$(JAVA_HOME)\include -I$(JAVA_HOME)\include\win32

SHERLOK_AGENT_SRC =     \
	cjvmti.cpp      \
	cti.cpp      \
	system.cpp   

SHERLOK_TEST_SRC =      \
	Profiler.cpp    \
	cti.cpp         \
	cjvmti.cpp      \
	system.cpp   

SHERLOK_CTI_SRC =       \
        cti.cpp

SYSTEM_LIBS =           \
        Ws2_32.lib      \
        kernel32.lib    \
        user32.lib      \
        gdi32.lib       \
        winspool.lib    \
        comdlg32.lib    \
        advapi32.lib    \
        shell32.lib     \
        ole32.lib       \
        oleaut32.lib    \
        uuid.lib        \
        odbc32.lib      \
        odbccp32.lib

SHERLOK_AGENT_OBJ = $(SHERLOK_AGENT_SRC:.cpp=.obj) 
SHERLOK_TEST_OBJ  = $(SHERLOK_TEST_SRC:.cpp=.obj) 
SHERLOK_CTI_OBJ   = $(SHERLOK_CTI_SRC:.cpp=.obj)

all         : $(SHERLOK_AGENT) $(SHERLOK_TEST) $(SHERLOK_CTI)

$(SHERLOK_AGENT) : $(SHERLOK_AGENT_OBJ)
	link /OUT:$(SHERLOK_AGENT) $(SHERLOK_AGENT_OBJ) $(SYSTEM_LIBS) /NODEFAULTLIB:LIBCMT /MANIFEST /NXCOMPAT /PDB:sherlok.pdb /DYNAMICBASE /DEF:sherlok.def /IMPLIB:sherlok.lib /DEBUG /DLL /MACHINE:X64 /INCREMENTAL /SUBSYSTEM:WINDOWS /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /ManifestFile:"sherlok.dll.intermediate.manifest" /ERRORREPORT:PROMPT /NOLOGO /TLBID:1

$(SHERLOK_CTI)   : $(SHERLOK_CTI_OBJ)
	link /OUT:$(SHERLOK_CTI)   $(SHERLOK_CTI_OBJ) $(SYSTEM_LIBS)  /NODEFAULTLIB:LIBCMT /MANIFEST /NXCOMPAT /PDB:ctilib.pdb /DYNAMICBASE /IMPLIB:ctilib.lib /DEBUG /DLL /MACHINE:X64 /INCREMENTAL /SUBSYSTEM:WINDOWS /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /ManifestFile:"ctilib.dll.intermediate.manifest" /ERRORREPORT:PROMPT /NOLOGO /TLBID:2


$(SHERLOK_TEST)  : $(SHERLOK_TEST_OBJ)
	link /OUT:$(SHERLOK_TEST) $(SHERLOK_TEST_OBJ) $(SYSTEM_LIBS)  /MANIFEST /NXCOMPAT /PDB:profiler.pdb /DEBUG /MACHINE:X64 /INCREMENTAL /SUBSYSTEM:CONSOLE /ManifestFile:"profiler.exe.intermediate.manifest" /ERRORREPORT:PROMPT /NOLOGO 

*.obj:*.cpp
	$(CPP) $(CPPFLAGS) -c $**
	
    

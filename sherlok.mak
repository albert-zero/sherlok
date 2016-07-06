#
#    sherlok.mak: 
#    sherlok is a high speed java application monitor working with JVM1.6 and newer 
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

AGENTLIB = sherlok.dll
CPPFLAGS = /GS /W3 /Zc:wchar_t /Zi /Gm /Od /sdl /fp:precise -DWIN32 -D_WINDOWS /D "_USRDLL_WINDLL" /D "_UNICODE" /D "UNICODE" /Zc:forScope /RTC1 /Gd /MDd /EHsc /nologo -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/win32

SHERLOK_SRC = \
	cjvmti.cpp   \
	cjvmpi.cpp   \
	system.cpp   

SHERLOK_OBJ = $(SHERLOK_SRC:.cpp=.obj) 

all         : $(AGENTLIB)

$(AGENTLIB) : $(SHERLOK_OBJ)
	link /OUT:$(AGENTLIB) $(SHERLOK_OBJ) /NODEFAULTLIB:LIBCMT /MANIFEST /NXCOMPAT /PDB:sherlok.pdb /DYNAMICBASE Ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /DEF:sherlok.def /IMPLIB:sherlok.lib /DEBUG /DLL /MACHINE:X64 /INCREMENTAL /SUBSYSTEM:WINDOWS /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /ManifestFile:"sherlok.dll.intermediate.manifest" /ERRORREPORT:PROMPT /NOLOGO /TLBID:1

*.obj:*.cpp
	$(CPP) $(CPPFLAGS) -c $<
	
    

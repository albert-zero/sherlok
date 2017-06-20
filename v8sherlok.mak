# @(#) $Id: //Douala/DoualaLayer/dev/v8sherlok/v8sherlok.mak#4 $ SAP
#
#     SAP AG Walldorf
#     Systeme, Anwendungen und Produkte in der Datenverarbeitung
#
#     (C) Copyright SAP AG,  1989 - 1999
#
#-----------------------------------------------------------------------
#responsible:
#	albert.rossmann@sap.com

JAVA_INCLUDE_PATH = $(JAVA5_INCLUDE_PATH) $(JAVA2_INCLUDE_PATH:1.4.2=1.6.0)

#-----------------------------------------------------------------------
#       HEADER FILES
#-----------------------------------------------------------------------
SHARED_HEADERS = \
	$(SY)port.h \
	saptype.h			\
	dlxx.h				\
	sapfstream.hpp		\
	sapstring.hpp		\
	sapsstream.hpp		\
	sapiostrm.hpp		\
	saptypeb.h			\
	sapuc.hpp			\
	sapthr.h			\
	sapiosfwd.hpp		\
	sapuc2.h			\
	rscputf.h			\
	sapuc.h				\
	saptypec.h			\
	saplocale.hpp		\
	cppconfig.hpp		\
	saputsname.hpp		\
	cppconf.$(BASE_PLATFORM) \
	cppconf.stl			\
	$(OSINCLUDE)

SHERLOK_HEADERS =		\
        ccqcovrun.h                     \
        cti.h                           \
	cjvmti.h			\
	javapi.h			\
	console.h			\
	command.h			\
	tracer.h			\
	profiler.h			\
	monitor.h			\
	extended.h			\
	standard.h			\
	ptypes.h

SHERLOK_SOURCES =		\
	Profiler.cpp		\
	system.cpp              \
	cjvmti.cpp			\
	sherlok.def

SHERLOK_LIBS =  \
	$(SY)prtslib$(MT)$(LIBE)	\
	cppnlslib$(LIBA)			\
	dptrrlib$(MT)$(LIBE)		\
	dllib$(MT)$(LIBE)			\
	thrlib$(MT)$(LIBE)			\
	$(STLPORT_SALIBI) 

STATIC_LIBS =  \
	$(SY)prtslib$(MT)$(LIBE)	\
	dptrrlib$(MT)$(LIBE)		\
	dllib$(MT)$(LIBE)			\
	thrlib$(MT)$(LIBE)			\
	cppnlslib$(LIBA)


#-----------------------------------------------------------------------
#       GENRATE FILES
#-----------------------------------------------------------------------
SHERLOK_PRO = \
	Profiler$(EXE)

SHERLOK_OBJECTS = \
	Profiler$(OBJ)		\
	system$(OBJ)            \
	cjvmti$(OBJ)

SHERLOK_LIB = \
	$(LIB_PREFIX)sherlok$(REALLY_SHARED)

#-----------------------------------------------------------------------
#       MAIN TARGETS
#   
# 		On UNIX build MT only
#-----------------------------------------------------------------------
ntexe: 	exe_mt

ntlib: 	lib_mt

uxexe:
	@$(ECHO) nothing to do

uxlib:
	@$(ECHO) nothing to do

exe: 	$(SY)exe

lib: 	$(SY)lib

exe_mt:			\
	synchronize	\
	$(SHERLOK_PRO)
	
lib_mt:			\
	synchronize	\
	$(SHERLOK_LIB) 
	
all: $(SY)exe $(SY)lib

clean:
	$(REMOVE_NAMED) $(SHERLOK_OBJECTS)
	$(REMOVE_NAMED) $(SHERLOK_LIBS)

test_mt:
	
test:

#-----------------------------------------------------------------------
#		Sync files
#-----------------------------------------------------------------------
$(SHERLOK_SOURCES) $(SHERLOK_HEADERS) $(SHARED_HEADERS):
	$(SOFTLINK) $(SOURCE_DIR)$(SEP)$@ $@
	$(CONVSRC) $@

$(SHERLOK_LIBS):
	$(SOFTLINK) $(LIB_DIR)$(SEP)$@ $@

synchronize: $(SHERLOK_SOURCES) $(SHERLOK_HEADERS) $(SHARED_HEADERS) $(SHERLOK_LIBS)
	@$(ECHO) synchronization done


#-----------------------------------------------------------------------
#       Rules
#-----------------------------------------------------------------------
.SUFFIXES: .$(OBJ_EXT) .c .cpp

.cpp.$(OBJ_EXT):
	$(REMOVE)
	$(COMPILE_CPP_SHARED) -DPROFILE_CPP -DPROFILE_MEM $(JAVA_INCLUDE_PATH) -I$(STLPORT_INC_SHARED) -DCPP_USE_STLPORT $<

$(SHERLOK_LIB): $(SHERLOK_OBJECTS) $(SHERLOK_LIBS)
	$(REMOVE)
	$(MAKELIB_SHARED) $(SHERLOK_OBJECTS) $(SOCKET_LIBS) $(SHLIB_LINK_OPTION)$(STLPORT_SALIBI) $(SHERLOK_LIBS) $(OS_LIBS) $(MAKELIB_END)
	$(RANLIB)
	$(EXPORT) $@ $(GEN_DIR)

$(SHERLOK_PRO): $(SHERLOK_OBJECTS) $(SHERLOK_LIBS) 
	$(LINK_CPP_STL) $(SHERLOK_OBJECTS) $(SHERLOK_LIBS) 

$(SHERLOK_OBJECTS): $(SHARED_HEADERS) $(SHERLOK_HEADERS)

responsible:
	albert.zedlitz@sap.com

component:
	@$(ECHO) BC-JAS

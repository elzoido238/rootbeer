#### MAKEFILE FOR THE DRAGON VERSION OF ROOTBEER ####
#### This version of rootbeer is compiled differently from the stock one;
#### namely, the "user" portion is compiled into a separate shared libarary
#### that is then linked into the rootbeer executable.

### Include the user-defined portion of the makefile
include $(PWD)/user/Makefile.user

MIDASLIBFILE=-lmidas

### Variable definitions
SRC=$(PWD)/src
OBJ=$(PWD)/obj
CINT=$(PWD)/cint
USER=$(PWD)/user
RBLIB=$(PWD)/lib

ROOTGLIBS = $(shell root-config --glibs) -lXMLParser -lThread -lTreePlayer
RPATH    += -Wl,-rpath,$(ROOTSYS)/lib -Wl,-rpath,$(PWD)/lib
DYLIB=-shared
FPIC=-fPIC
INCFLAGS=-I$(SRC) -I$(CINT) -I$(USER) $(USER_INCLUDES)
OPTIMIZE=-O3
DEBUG=-ggdb
#-ggdb -O0 -DDEBUG -DRB_LOGGING
CXXFLAGS=$(DEBUG) $(OPTIMIZE) $(INCFLAGS) $(STOCK_BUFFERS) -DBUFFER_TYPE=$(USER_BUFFER_TYPE)


ifdef ROOTSYS
ROOTGLIBS = -L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --glibs --cflags) -lXMLParser -lThread -lTreePlayer
#CXXFLAGS += -L$(ROOTSYS)/lib $(shell $(ROOTSYS)/bin/root-config --cflags) -I$(ROOTSYS)/include
else
ROOTGLIBS = $(shell root-config --glibs --cflags) -lXMLParser -lThread -lTreePlayer
endif

# optional MIDAS libraries
ifdef MIDASSYS
MIDASLIBS = -L$(MIDASSYS)/linux/lib $(MIDASLIBFILE) -lutil -lrt
CXXFLAGS += -DMIDAS_ONLINE -DOS_LINUX -Dextname -I$(MIDASSYS)/include
MIDASONLINE=$(OBJ)/midas/TMidasOnline.o
ifdef MIDAS_SERVER_HOST
CXXFLAGS += -DMIDAS_SERVER_HOST=\"$(MIDAS_SERVER_HOST)\"
endif
ifdef MIDAS_EXPT_NAME
CXXFLAGS += -DMIDAS_EXPT_NAME=\"$(MIDAS_EXPT_NAME)\"
endif
endif

UNAME=$(shell uname)
ifeq ($(UNAME),Darwin)
CXXFLAGS += -DOS_LINUX -DOS_DARWIN
ifdef MIDASSYS
MIDASLIBS = -L$(MIDASSYS)/darwin/lib $(MIDASLIBFILE)
endif
DYLIB=-dynamiclib -single_module -undefined dynamic_lookup 
FPIC=
RPATH=
endif

#CXX=g++ -Wall

DEFAULTS=$(DEF_FILE_DIR) $(DEF_SAVE_DIR) $(DEF_CONFIG_DIR)

COMPILE=$(CXX) $(CXXFLAGS) $(RPATH) $(DEF_EXT) $(DEFAULTS) $(USER_DEFINITIONS) -I$(ROOTSYS)/include
LINK=$(CXX) $(CXXFLAGS) $(ROOTGLIBS) $(RPATH) $(DEFAULTS) $(USER_DEFINITIONS) -L$(PWD)/lib $(USER_LIB_DIRS)
ROOTCINT=rootcint $(USER_DEFINITIONS)


#### MAIN PROGRAM ####
all:  $(RBLIB)/libRootbeer.so $(RBLIB)/libRbMidas.so
## rootbeer rbunpack

# rbunpack: $(RBLIB)/libRootbeer.so $(SRC)/main.cc 
# 	$(LINK) $(USER_LIBS) -lRootbeer -lmidas $(MIDASLIBS) -DRB_UNPACK_ONLY $(SRC)/main.cc -o rbunpack \

# rootbeer: $(RBLIB)/libRootbeer.so $(SRC)/main.cc
# 	$(LINK) $(USER_LIBS) -lRootbeer -lmidas $(MIDASLIBS) $(SRC)/main.cc -o rootbeer \

#### ROOTBEER LIBRARY ####
OBJECTS=$(OBJ)/hist/Hist.o $(OBJ)/hist/Manager.o \
$(OBJ)/Formula.o \
$(OBJ)/Data.o $(OBJ)/Event.o $(OBJ)/Buffer.o $(OBJ)/Canvas.o $(OBJ)/WriteConfig.o \
$(OBJ)/Rint.o $(OBJ)/Signals.o $(OBJ)/Rootbeer.o $(OBJ)/Gui.o $(OBJ)/HistGui.o \
$(OBJ)/TGSelectDialog.o $(OBJ)/TGDivideSelect.o $(OBJ)/Main.o
##$(OBJ)/midas/TMidasEvent.o $(OBJ)/midas/TMidasFile.o $(MIDASONLINE) 


HEADERS=$(SRC)/Main.hxx $(SRC)/Rootbeer.hxx $(SRC)/Rint.hxx $(SRC)/Data.hxx $(SRC)/Buffer.hxx $(SRC)/Event.hxx \
$(SRC)/Signals.hxx $(SRC)/Formula.hxx $(SRC)/utils/LockingPointer.hxx $(SRC)/utils/Mutex.hxx \
$(SRC)/hist/Hist.hxx $(SRC)/hist/Visitor.hxx $(SRC)/hist/Manager.hxx $(SRC)/TGSelectDialog.h $(SRC)/TGDivideSelect.h \
$(SRC)/HistGui.hxx $(SRC)/Gui.hxx $(SRC)/utils/*.h*
#$(SRC)/midas/*.h


RBlib: $(RBLIB)/libRootbeer.so
$(RBLIB)/libRootbeer.so: $(CINT)/RBDictionary.cxx $(USER_SOURCES) $(OBJECTS)
	$(LINK) $(DYLIB) $(FPIC) $(MIDASLIBS) $(OBJECTS) $(CINT)/RBDictionary.cxx \
-o $@ \

$(OBJ)/%.o: $(SRC)/%.cxx $(CINT)/RBDictionary.cxx
	$(COMPILE) $(FPIC) -c $< \
-o $@  \

$(OBJ)/midas/%.o: $(SRC)/midas/%.cxx $(CINT)/RBDictionary.cxx
	$(COMPILE) $(FPIC) -c $< \
-o $@  \

$(OBJ)/hist/%.o: $(SRC)/hist/%.cxx $(CINT)/RBDictionary.cxx
	$(COMPILE) $(FPIC) -c $< \
-o $@ \

testing/%: $(PWD)/testing/%.cxx
	$(LINK) $(USER_LIBS) -lRootbeer -lmidas $(MIDASLIBS) $< \
-o $@ \

RBdict: $(CINT)/RBDictionary.cxx
$(CINT)/RBDictionary.cxx:  $(HEADERS) $(USER)/UserLinkdef.h $(CINT)/Linkdef.h \
$(SRC)/utils/Mutex.hxx $(SRC)/utils/LockingPointer.hxx $(SRC)/utils/ANSort.hxx $(SRC)/hist/*.hxx
	rootcint -f $@ -c $(CXXFLAGS) $(USER_DEFINITIONS) -p $(HEADERS) $(CINT)/Linkdef.h \



### libRbMidas.so ###
MIDAS_SOURCES=$(shell ls $(SRC)/midas/*.cxx)
MIDAS_HEADERS1=$(shell ls $(SRC)/midas/*.h $(SRC)/midas/*.hxx)
MIDAS_HEADERS=$(MIDAS_HEADERS1:$(SRC)/midas/MidasLinkdef.h= )
MIDAS_OBJECTS=$(MIDAS_SOURCES:.cpp=.o)

libRbMidas: $(RBLIB)/libRbMidas.so
$(RBLIB)/libRbMidas.so: $(MIDAS_OBJS) $(CINT)/MidasDict.cxx
	$(LINK) $(DYLIB)$(FPIC) $(MIDAS_OBJECTS) $(CINT)/MidasDict.cxx \
-o $@ \

$(CINT)/MidasDict.cxx:  $(MIDAS_HEADERS) $(SRC)/midas/MidasLinkdef.h
	rootcint -f $@ -c $(CXXFLAGS) $(USER_DEFINITIONS) -p $+ \


#### REMOVE EVERYTHING GENERATED BY MAKE ####

clean:
	rm -f $(RBLIB)/*.so rootbeer $(CINT)/*Dict*.h $(CINT)/*Dict*.cxx $(OBJ)/*.o $(OBJ)/*/*.o



#### FOR DOXYGEN ####

doc:
	cd $(PWD)/doxygen ; doxygen Doxyfile ; cd latex; make; cd $(PWD)


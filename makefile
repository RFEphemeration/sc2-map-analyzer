###############################
## 
##  sc2mapanalyzer makefile
##  for compilation in MinGW
##
###############################


# manually change these for a new version of either the
# executable, or ADDITIONALLY if the core algorithms change,
# meaning running a new executable on an old map gets a different
# result, then roll the algorithms version, too
VEXE=1.4.7
VALG=1.4


CC=g++

# the bit fields and march flags are allegedly to prevent
# crashes of binaries built using MinGW style compilers on
# non-Intel chips... they seem harmless otherwise
FLAGS=-O3 -mms-bitfields -march=i386
LINKFLAGS=--enable-auto-import

VERSIONS=-D VEXE=$(VEXE) -D VALG=$(VALG)

# the order of libraries is apparently important
LIBS=-L./pngwriter/src -lpngwriter -lfreetype -lpng -lz -L. -lStormLib

INCLUDE=-IC:/mingw/include/freetype2 -Itinyxml -IStormLib/src -Ipngwriter/src

# assume tinyxml objects are built and residing in the TXD directory
TXD=tinyxml
TINYXML_OBJS=$(TXD)/tinyxml.o $(TXD)/tinyxmlparser.o $(TXD)/tinyxmlerror.o $(TXD)/tinystr.o


PROGRAM=sc2mapanalyzer
RELEASEDIR=$(PROGRAM)-release-$(VEXE)


OBJECTS=sc2mapanalyzer.o \
	   utility.o \
     config.o \
	   outstreams.o \
	   debug.o \
	   coordinates.o \
     PrioQueue.o \
	   SC2Map.o \
	   bookkeeping.o \
     read.o \
     pathing.o \
     placedobjects.o \
     bases.o \
     openness.o \
     dijkstra.o \
     spreadsheet.o \
     render.o \
     SC2MapAggregator.o \
     sc2ma.res

HEADERS=sc2mapTypes.hpp \
     utility.hpp \
     config.hpp \
     debug.hpp \
	   outstreams.hpp \
	   coordinates.hpp \
	   PrioQueue.hpp \
	   SC2Map.hpp \
	   SC2MapAggregator.hpp
	   
DISTRO_EXE=StormLib.dll \
    FreeSansBold.ttf \
    locales.txt \
	  footprints.txt \
	  constants.txt \
	  colors.txt    
       
DISTRO_CONFIG=configFilesForRelease/to-analyze.txt \
    configFilesForRelease/output.txt
	   


all: $(PROGRAM)


%.o : %.cpp $(HEADERS)
	$(CC) $(FLAGS) $(VERSIONS) $(INCLUDE) -c $<


sc2ma.res: sc2ma.rc
	windres sc2ma.rc -O coff -o sc2ma.res


$(PROGRAM): $(OBJECTS)
	$(CC) $(FLAGS) $(LINKFLAGS) $(VERSIONS) $(OBJECTS) $(TINYXML_OBJS) $(LIBS) -o $(PROGRAM)
	mkdir -p $(RELEASEDIR)
	cp $(PROGRAM).exe $(DISTRO_EXE) $(DISTRO_CONFIG) $(RELEASEDIR)
	

clean:
	rm -f $(PROGRAM).exe
	rm -f $(OBJECTS)
	rm -rf $(RELEASEDIR)

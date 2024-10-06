CXX=g++
CXXFLAGS=-g -std=c++17
SDKDIR=sdk
INCLUDES=-I$(SDKDIR)

# ImageMagick-c++-devel
export PKG_CONFIG_PATH := $(PKG_CONFIG_PATH):/usr/lib64/pkgconfig
MAGICK_CXXFLAGS := $(shell pkg-config --cflags Magick++)
MAGICK_LIBS := $(shell pkg-config --libs Magick++)

CXXFLAGS += $(MAGICK_CXXFLAGS)
LDFLAGS += $(MAGICK_LIBS) -lstdc++fs

TARGETNAME=braw-thumbnailer
LIBRARYNAME=BlackmagicRawAPIDispatch

all: $(TARGETNAME) cleanobj
.PHONY : all

debug: CXXFLAGS += -DDEBUG
debug: all

$(TARGETNAME) : $(TARGETNAME).o $(LIBRARYNAME).o
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(TARGETNAME) $(TARGETNAME).o $(LIBRARYNAME).o -lpthread -ldl $(LDFLAGS)

$(TARGETNAME).o : $(TARGETNAME).cpp $(SDKDIR)/$(LIBRARYNAME).cpp $(SDKDIR)/BlackmagicRawAPI.h $(SDKDIR)/LinuxCOM.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(TARGETNAME).cpp

$(LIBRARYNAME).o : $(SDKDIR)/$(LIBRARYNAME).cpp $(SDKDIR)/BlackmagicRawAPI.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SDKDIR)/$(LIBRARYNAME).cpp

clean:
	$(RM) $(TARGETNAME) $(TARGETNAME).o $(LIBRARYNAME).o

cleanobj:
	$(RM) $(TARGETNAME).o $(LIBRARYNAME).o
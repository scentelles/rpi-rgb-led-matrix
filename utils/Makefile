NDI_SDK_DIRECTORY="/home/pi/Project/NDISDK/NDISDKLINUX"


CXXFLAGS=-Wall -O3 -g -Wextra -Wno-unused-parameter -D_FILE_OFFSET_BITS=64
#for profiling
#CXXFLAGS=-Wall -pg -O3 -g -Wextra -Wno-unused-parameter -D_FILE_OFFSET_BITS=64

OBJECTS=led-image-viewer.o text-scroller.o projectm-image-viewer.o app-launcher.o
BINARIES=led-image-viewer text-scroller projectm-image-viewer app-launcher

OPTIONAL_OBJECTS=video-viewer.o projectm-video-viewer.o
OPTIONAL_BINARIES=video-viewer projectm-video-viewer

# Where our library resides. You mostly only need to change the
# RGB_LIB_DISTRIBUTION, this is where the library is checked out.
RGB_LIB_DISTRIBUTION=..
RGB_INCDIR=$(RGB_LIB_DISTRIBUTION)/include $(NDI_SDK_DIRECTORY)/include
RGB_LIBDIR=$(RGB_LIB_DISTRIBUTION)/lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a

LDFLAGS+=-L$(RGB_LIBDIR) -L$(NDI_SDK_DIRECTORY)/lib/arm-linux-gnueabihf  -l$(RGB_LIBRARY_NAME) -lrt -lm -lpthread -lX11 -lndi

# Imagemagic flags, only needed if actually compiled.
MAGICK_CXXFLAGS?=`GraphicsMagick++-config --cppflags --cxxflags`
MAGICK_LDFLAGS?=`GraphicsMagick++-config --ldflags --libs`
AV_CXXFLAGS=`pkg-config --cflags  libavcodec libavformat libswscale libavutil`

all : $(BINARIES)

$(RGB_LIBRARY): FORCE
	$(MAKE) -C $(RGB_LIBDIR)

text-scroller: text-scroller.o $(RGB_LIBRARY)
	$(CXX) $(CXXFLAGS) text-scroller.o -o $@ $(LDFLAGS)

led-image-viewer: led-image-viewer.o $(RGB_LIBRARY)
	$(CXX) $(CXXFLAGS) led-image-viewer.o -o $@ $(LDFLAGS) $(MAGICK_LDFLAGS)

video-viewer: video-viewer.o $(RGB_LIBRARY)
	$(CXX) $(CXXFLAGS) video-viewer.o -o $@ $(LDFLAGS) `pkg-config --cflags --libs  libavcodec libavformat libswscale libavutil`


app-launcher: app-launcher.o
	$(CXX) $(CXXFLAGS) app-launcher.o -o $@ $(LDFLAGS)

projectm-video-viewer: projectm-video-viewer.o $(RGB_LIBRARY)
	$(CXX) $(CXXFLAGS) projectm-video-viewer.o -o $@ $(LDFLAGS) `pkg-config --cflags --libs  libavcodec libavformat libswscale libavutil`


%.o : %.cc
	$(CXX) -I$(RGB_INCDIR) $(CXXFLAGS) -c -o $@ $<

led-image-viewer.o : led-image-viewer.cc
	$(CXX) -I$(RGB_INCDIR) $(CXXFLAGS) $(MAGICK_CXXFLAGS) -c -o $@ $<

projectm-image-viewer.o : projectm-image-viewer.cc
	$(CXX) -I$(RGB_INCDIR) $(CXXFLAGS) $(MAGICK_CXXFLAGS) -c -o $@ $<


# We're using a couple of deprecated functions. Pull request to update this to
# the latest libraries is welcome.
video-viewer.o: video-viewer.cc
	$(CXX) -I$(RGB_INCDIR) $(CXXFLAGS) -Wno-deprecated-declarations -c -o $@ $<

app-launcher.o: app-launcher.cpp
	$(CXX) $(CXXFLAGS) -Wno-deprecated-declarations -c -o $@ $<


projectm-video-viewer.o: projectm-video-viewer.cc
	$(CXX) -I$(RGB_INCDIR) $(CXXFLAGS) -Wno-deprecated-declarations -c -o $@ $<


clean:
	rm -f $(OBJECTS) $(BINARIES) $(OPTIONAL_OBJECTS) $(OPTIONAL_BINARIES)

FORCE:
.PHONY: FORCE

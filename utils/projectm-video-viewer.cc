// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
//




#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>




#include <string>
#include <cstring>
#include<stdio.h>
#include <iostream>

#define EMPTY_WINDOW NULL
using std::endl;
using std::cout;


#pragma pack (1)
typedef struct BITMAPFILEHEADER 
{
short    bfType;
int    bfSize;
short    bfReserved1;
short    bfReserved2;
int   bfOffBits;
};

typedef struct BITMAPINFOHEADER
{
int  biSize;
int   biWidth;
int   biHeight;
short   biPlanes;
short   biBitCount;
int  biCompression;
int  biSizeImage;
int   biXPelsPerMeter;
int   biYPelsPerMeter;
int  biClrUsed;
int  biClrImportant;
};

void saveXImageToBitmap(XImage *pImage)
{
BITMAPFILEHEADER bmpFileHeader;
BITMAPINFOHEADER bmpInfoHeader;
FILE *fp;
static int cnt = 0;
int dummy;
char filePath[255];
memset(&bmpFileHeader, 0, sizeof(BITMAPFILEHEADER));
memset(&bmpInfoHeader, 0, sizeof(BITMAPINFOHEADER));
bmpFileHeader.bfType = 0x4D42;
bmpFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
bmpFileHeader.bfReserved1 = 0;
bmpFileHeader.bfReserved2 = 0;
int biBitCount =32;
int dwBmpSize = ((pImage->width * biBitCount + 31) / 32) * 4 * pImage->height;
printf("size of short:%d\r\n",(int)sizeof(short));
printf("size of int:%d\r\n",(int)sizeof(int));
printf("size of long:%d\r\n",(int)sizeof(long));
printf("dwBmpSize:%d\r\n",(int)dwBmpSize);
printf("BITMAPFILEHEADER:%d\r\n",(int)sizeof(BITMAPFILEHEADER));
printf("BITMAPINFOHEADER:%d\r\n",(int)sizeof(BITMAPINFOHEADER));
bmpFileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) +  dwBmpSize;

bmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
bmpInfoHeader.biWidth = pImage->width;
bmpInfoHeader.biHeight = pImage->height;
bmpInfoHeader.biPlanes = 1;
bmpInfoHeader.biBitCount = biBitCount;
bmpInfoHeader.biSizeImage = 0;
bmpInfoHeader.biCompression = 0;
bmpInfoHeader.biXPelsPerMeter = 0;
bmpInfoHeader.biYPelsPerMeter = 0;
bmpInfoHeader.biClrUsed = 0;
bmpInfoHeader.biClrImportant = 0;

sprintf(filePath, "/home/pi/Project/rpi-rgb-led-matrix/utils/bitmap%d.bmp", cnt++);
fp = fopen(filePath,"wb");
printf("opening file : %s\n", filePath);
if(fp == NULL)
{
    printf("failed to open file\n");
    return;
}

char * tempData;
tempData = (char*)malloc(dwBmpSize);


for(int h=0; h < pImage->height; h++)
{
  for(int w=0; w < pImage->width; w++)
  {
    int indexB = (h*pImage->width + w)*4;
    int indexG = (h*pImage->width + w)*4+1;
    int indexR = (h*pImage->width + w)*4+2;

    tempData[((pImage->height-1-h)*pImage->width + w) * 4] = pImage->data[indexB];
    tempData[((pImage->height-1-h)*pImage->width + w) * 4+1] = pImage->data[indexG];
    tempData[((pImage->height-1-h)*pImage->width + w) * 4+2] = pImage->data[indexR];

    //printf("index : %d\n", index);
    //pImage->data[indexB] = 0x0;
    //pImage->data[indexG] = 0x0;
    //pImage->data[indexR] = 0xFF;
  
  }	
} 


fwrite(&bmpFileHeader, sizeof(bmpFileHeader), 1, fp);
fwrite(&bmpInfoHeader, sizeof(bmpInfoHeader), 1, fp);
//fwrite(pImage->data, dwBmpSize, 1, fp);
fwrite(tempData, dwBmpSize, 1, fp);
fclose(fp);
printf("file closed\n");
}



Window *getWindowList(Display *disp, unsigned long *len) {
    Atom prop = XInternAtom(disp,"_NET_CLIENT_LIST",False), type;
    int form;
    unsigned long remain;
    unsigned char *list;

    if (XGetWindowProperty(disp,XDefaultRootWindow(disp),prop,0,1024,False,33,
                &type,&form,len,&remain,&list) != Success) {  // XA_WINDOW
        return 0;
    }

    return (Window*)list;
}
char *getWindowName(Display *disp, Window win) {
    Atom prop = XInternAtom(disp,"WM_NAME",False), type;
    int form;
    unsigned long remain, len;
    unsigned char *list;


    if (XGetWindowProperty(disp,win,prop,0,1024,False,AnyPropertyType,
                &type,&form,&len,&remain,&list) != Success) { // XA_STRING

        return NULL;
    }

    return (char*)list;
}


//#==========================================================================
















#include "led-matrix.h"
#include "content-streamer.h"

using rgb_matrix::FrameCanvas;
using rgb_matrix::RGBMatrix;
using rgb_matrix::StreamWriter;
using rgb_matrix::StreamIO;

volatile bool interrupt_received = false;
static void InterruptHandler(int) {
  interrupt_received = true;
}



struct LedPixel {
  uint8_t r, g, b;
};


void CopyFrame(XImage *pXimage, FrameCanvas *canvas) {

char * tempData;
int biBitCount =32;
int dwBmpSize = ((pXimage->width * biBitCount + 31) / 32) * 4 * pXimage->height;

tempData = (char*)malloc(dwBmpSize);


for(int h=0; h < pXimage->height; h++)
{
  for(int w=0; w < pXimage->width; w++)
  {
    int indexB = (h*pXimage->width + w)*4;
    int indexG = (h*pXimage->width + w)*4+1;
    int indexR = (h*pXimage->width + w)*4+2;

    tempData[((pXimage->height-1-h)*pXimage->width + w) * 4] = pXimage->data[indexB];
    tempData[((pXimage->height-1-h)*pXimage->width + w) * 4+1] = pXimage->data[indexG];
    tempData[((pXimage->height-1-h)*pXimage->width + w) * 4+2] = pXimage->data[indexR];
   
     canvas->SetPixel(w, h, pXimage->data[indexR], pXimage->data[indexG], pXimage->data[indexB]);

  }	
}
/*
for(int h=0; h < pXimage->height; h++)
{
  for(int w=0; w < pXimage->width; w++)
  {
    int indexB = (h*pXimage->width + w)*4;
    int indexG = (h*pXimage->width + w)*4+1;
    int indexR = (h*pXimage->width + w)*4+2;

 
    canvas->SetPixel(w, h, tempData[indexR], tempData[indexG], tempData[indexB]);
  
  }	
}*/


}

static int usage(const char *progname, const char *msg = nullptr) {
  if (msg) {
    fprintf(stderr, "%s\n", msg);
  }
  fprintf(stderr, "usage: %s [options] <video>\n", progname);
  fprintf(stderr, "Options:\n"
          "\t-F                 : Full screen without black bars; aspect ratio might suffer\n"
          "\t-O<streamfile>     : Output to stream-file instead of matrix (don't need to be root).\n"
          "\t-s <count>         : Skip these number of frames in the beginning.\n"
          "\t-c <count>         : Only show this number of frames (excluding skipped frames).\n"
          "\t-V<vsync-multiple> : Instead of native video framerate, playback framerate\n"
          "\t                     is a fraction of matrix refresh. In particular with a stable refresh,\n"
          "\t                     this can result in more smooth playback. Choose multiple for desired framerate.\n"
          "\t                     (Tip: use --led-limit-refresh for stable rate)\n"
          "\t-v                 : verbose; prints video metadata and other info.\n"
          "\t-f                 : Loop forever.\n");

  fprintf(stderr, "\nGeneral LED matrix options:\n");
  rgb_matrix::PrintMatrixFlags(stderr);
  return 1;
}

static void add_nanos(struct timespec *accumulator, long nanoseconds) {
  accumulator->tv_nsec += nanoseconds;
  while (accumulator->tv_nsec > 1000000000) {
    accumulator->tv_nsec -= 1000000000;
    accumulator->tv_sec += 1;
  }
}


int main(int argc, char *argv[]) {
  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;
  if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv,
                                         &matrix_options, &runtime_opt)) {
    return usage(argv[0]);
  }

  int vsync_multiple = 1;
  bool use_vsync_for_frame_timing = false;
  bool maintain_aspect_ratio = true;
  bool verbose = false;
  bool forever = false;

  unsigned int frame_skip = 0;
  unsigned int framecount_limit = UINT_MAX;  // even at 60fps, that is > 2yrs

  int opt;
  while ((opt = getopt(argc, argv, "vO:R:Lfc:s:FV:")) != -1) {
    switch (opt) {
    case 'v':
      verbose = true;
      break;
    case 'f':
      forever = true;
      break;

    case 'L':
      fprintf(stderr, "-L is deprecated. Use\n\t--led-pixel-mapper=\"U-mapper\" --led-chain=4\ninstead.\n");
      return 1;
      break;
    case 'R':
      fprintf(stderr, "-R is deprecated. "
              "Use --led-pixel-mapper=\"Rotate:%s\" instead.\n", optarg);
      return 1;
      break;
    case 'c':
      framecount_limit = atoi(optarg);
      break;
    case 's':
      frame_skip = atoi(optarg);
      break;
    case 'F':
      maintain_aspect_ratio = false;
      break;
    case 'V':
      vsync_multiple = atoi(optarg);
      if (vsync_multiple <= 0)
        return usage(argv[0],
                     "-V: VSync-multiple needs to be a positive integer");
      use_vsync_for_frame_timing = true;
      break;
    default:
      return usage(argv[0]);
    }
  }





//  runtime_opt.do_gpio_init = (stream_output_fd < 0);
  RGBMatrix *matrix = CreateMatrixFromOptions(matrix_options, runtime_opt);
  if (matrix == NULL) {
    return 1;
  }

  FrameCanvas *offscreen_canvas = matrix->CreateFrameCanvas();
  StreamIO *stream_io = NULL;





  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  struct timespec next_frame;




//#==================================

        Display *display;
        int screen;
        Window root;
        display = XOpenDisplay(0);
	printf("Display %d\n", display);
        screen = DefaultScreen(display);
        root = RootWindow(display, screen);
    XWindowAttributes gwa;

   //XGetWindowAttributes(display, root, &gwa);
   int width = gwa.width;
   int height = gwa.height;


    Window *list;
    char *name;
    unsigned long len;
    int j;  
    int projectMWindowId = -1;
    
    list = (Window*)getWindowList(display,&len);
    
    for (j=0;j<(int)len;j++) {
        name = getWindowName(display,list[j]);
        cout << j << ": " << name << endl;
	
	std::size_t found = std::string(name).find(std::string("projectM"));
	if(found != std::string::npos)
	{
	  printf("ProjectM window ID is : %d\n", j);
	  projectMWindowId = j;
	}
	else
	{
	  printf("Error : ProjectM window not found\n");
	  
	}
        free(name);
	
        }



        Window target = list[projectMWindowId];
        XGetWindowAttributes(display, target, &gwa);
        width = gwa.width;
        height = gwa.height;


//#==================================



  do {

    clock_gettime(CLOCK_MONOTONIC, &next_frame);
    while (!interrupt_received) {
 	    
		    
        XImage *img = XGetImage(display,target,0,0,width,height,XAllPlanes(),ZPixmap);

        if (img != NULL)
        {
           //saveXImageToBitmap(img);
           //printf("copying\n");
           CopyFrame(img, offscreen_canvas); 
     

        }

        offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas,
                                                   vsync_multiple);

        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_frame, NULL);


    }
  } while (!interrupt_received);

  if (interrupt_received) {
    // Feedback for Ctrl-C, but most importantly, force a newline
    // at the output, so that commandline-shell editing is not messed up.
    fprintf(stderr, "Got interrupt. Exiting\n");
  }

  delete matrix;



  return 0;
}

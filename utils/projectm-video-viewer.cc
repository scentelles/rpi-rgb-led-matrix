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
#include <stdio.h>
#include <iostream>


#include <thread>         // std::thread



using std::endl;
using std::cout;
using std::cerr;


#include "led-matrix.h"
#include "content-streamer.h"

using rgb_matrix::FrameCanvas;
using rgb_matrix::RGBMatrix;
using rgb_matrix::StreamWriter;
using rgb_matrix::StreamIO;


bool mplayerStarted = false;
bool projectMStarted = false;

#define UPDATE_MULTIPLE 4

#define OSCPKT_OSTREAM_OUTPUT
#include "oscpkt/oscpkt.hh"
#include "oscpkt/udp.hh"
using namespace oscpkt;


#define KEYCODE XK_q



const int PORT_NUM = 7700;
        Window targetVideo = 0;
        Window targetProjectM = 0;
        Window currentTarget = 0;
	
	
    Display *display;
	Window winRoot = 0;

        XWindowAttributes gwa;
        int width = 0;
        int height = 0;
	
	float alphaVideo    = 1.0;
	float alphaProjectM = 1.0;


// Function to create a keyboard event
XKeyEvent createKeyEvent(Display *display, Window &win,
                         Window &winRoot, bool press,
                         int keycode, int modifiers)
{
	XKeyEvent event;

	event.display     = display;
	event.window      = win;
	event.root        = winRoot;
	event.subwindow   = None;
	event.time        = CurrentTime;
	event.x           = 1;
	event.y           = 1;
	event.x_root      = 1;
	event.y_root      = 1;
	event.same_screen = True;
	event.keycode     = XKeysymToKeycode(display, keycode);
	event.state       = modifiers;
	if(press)
		event.type = KeyPress;
	else
		event.type = KeyRelease;

	return event;
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




Window getWindowFromName(Display *display, std::string name)
{


    Window *list;

    unsigned long len;
    int j;  
    int windowId = -1;
    
    list = (Window*)getWindowList(display,&len);
    
    for (j=0;j<(int)len;j++) {
        std::string tempName = getWindowName(display,list[j]);
	cout << "\tWindow name : " << tempName << endl;
	
	std::size_t found = tempName.find(name);
	if(found != std::string::npos)
	{
	  windowId = j;
	  cout << "window ID for " << name << " is : " << windowId << endl;
	}


     }
     if ( windowId != -1)
     {
        return list[windowId];  
     }
     else
     {
	cout << "Error : " << name << "window not found" << endl;
	return 0;
     }
     
}



void stopVideo() 
{

	mplayerStarted =  false;

	//Need to declare this here, else it causes weird Xlib failure...
	XKeyEvent event = createKeyEvent(display, currentTarget, winRoot, true, KEYCODE, 0);


	cout << "sending keypress" << endl;
	//XSetInputFocus(display, currentTarget, false, CurrentTime);
	XSelectInput(display, currentTarget, KeyPressMask|KeyReleaseMask);
		
	// Send a fake key press event to the window.

	XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);

	XFlush(display) ;
	XSync(display, false);

}

	
void startVideo(int id) 
{

	if(mplayerStarted == false)
	{
      		//std::string command = "./startVideo.sh " + std::to_string(id);
     	 	//system(command.c_str());
      		mplayerStarted =  true;
		
		targetVideo = getWindowFromName(display, "MPlayer");

		
	}
	else
	{
		stopVideo();
		cout << "Stopping and launching new video" << endl;
     		std::string command = "./startVideo.sh " + std::to_string(id);
     	 	system(command.c_str());
      		mplayerStarted =  true;
		
		
	}
}

void switchTargets() 
{
	cout << "switching targets" << endl;
	if((mplayerStarted == true) && (projectMStarted == true))
	{
		if(currentTarget == targetVideo)
			currentTarget = targetProjectM;
		else
			currentTarget = targetVideo;

	}

}

void startProjectM(int id) 
{

 // system("./startVideo.sh ");

        targetProjectM = getWindowFromName(display, "projectM");
        XWindowAttributes gwa;
        XGetWindowAttributes(display, targetProjectM, &gwa);
        width = gwa.width;
        height = gwa.height;
	projectMStarted =  true;
	currentTarget = targetProjectM;


}


void runOSCServer() {
  UdpSocket sock; 
  sock.bindTo(PORT_NUM);
  if (!sock.isOk()) {
    cerr << "Error opening port " << PORT_NUM << ": " << sock.errorMessage() << "\n";
  } else {
    cout << "Server started, will listen to packets on port " << PORT_NUM << std::endl;
    PacketReader pr;
    PacketWriter pw;
    while (sock.isOk()) {      
      if (sock.receiveNextPacket(30 /* timeout, in ms */)) {
        pr.init(sock.packetData(), sock.packetSize());
        oscpkt::Message *msg;
        while (pr.isOk() && (msg = pr.popMessage()) != 0) {
          int iarg;
	  float tempF;
          cout << "ADDRESS : " << msg->address << endl;

	  
	  //partialMatch
	  
	  
	  if (msg->match("/1/fader1").popFloat(tempF)) {
		
		alphaVideo = tempF;
		printf("New alpha for Video : : %f\n", alphaVideo);
	  }
	  if (msg->match("/1/fader2").popFloat(tempF)) {
		
		alphaProjectM = tempF;
		printf("New alpha for ProjectM : : %f\n", alphaProjectM);
	  }	  
          if (msg->match("/megascreen/video").popInt32(iarg).isOkNoMoreArgs()) {
            cout << "Server: received video request " << iarg << " from " << sock.packetOrigin() << "\n";
	    
          //  Message repl; repl.init("/pong").pushInt32(iarg+1);
          //  pw.init().addMessage(repl);
          //  sock.sendPacketTo(pw.packetData(), pw.packetSize(), sock.packetOrigin());
          if(iarg == 0){
	  	stopVideo();
		if(projectMStarted == true)
		{
		   startProjectM(1);
		}
		continue;
	  }
	  if (iarg < 10)
	  {
	  	startVideo(iarg);
		continue;
	  }
	  if (iarg ==10)
	  {
	   	startProjectM(iarg);
	  	continue;
	  }
	  if (iarg ==11)
	  {
	   	switchTargets();
	  	continue;
	  }
	  //else {
           // cout << "Server: unhandled message: " << *msg << "\n";
          }
        }
      }
    }
  }
}






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
  delete(tempData);

}

//TODO : update usage
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
          "\t-f                 : Loop forever.\n");

  fprintf(stderr, "\nGeneral LED matrix options:\n");
  rgb_matrix::PrintMatrixFlags(stderr);
  return 1;
}



int catcher( Display *disp, XErrorEvent *xe )
{
        printf( "\nSomething bad happened, bruh.\n" );
        return 0;
}




void blendCanvas(XImage *img1, float alpha1, XImage *img2, float alpha2, FrameCanvas *result)
{


//printf ("inside blend function\n");

int biBitCount =32;
int dwBmpSize = ((img1->width * biBitCount + 31) / 32) * 4 * img1->height;


for(int h=0; h < img1->height; h++)
{
  for(int w=0; w < img1->width; w++)
  {
    int indexB = (h*img1->width + w)*4;
    int indexG = (h*img1->width + w)*4+1;
    int indexR = (h*img1->width + w)*4+2;

  
     result->SetPixel(w, h, std::min(int(img1->data[indexR] * alpha1 + img2->data[indexR] * alpha2), 255), 
     			    std::min(int(img1->data[indexG] * alpha1 + img2->data[indexG] * alpha2), 255), 
			    std::min(int(img1->data[indexB] * alpha1 + img2->data[indexB] * alpha2), 255));
 //    result->SetPixel(w, h, 200, 200 , 200);

  }	
}


}




int main(int argc, char *argv[]) {



  int count = 0;

//X11 related

  display = XOpenDisplay(0);
  winRoot = XDefaultRootWindow(display);
  XSetErrorHandler( catcher ); 
	

//Start OSC server
  std::thread ocsThread (runOSCServer);     
  
  
//RGB Matrix
  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;
  if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv,
                                         &matrix_options, &runtime_opt)) {
    return usage(argv[0]);
  }

  int vsync_multiple = 1;

  int opt;
  while ((opt = getopt(argc, argv, "vO:R:Lfc:s:FV:")) != -1) {
    switch (opt) {
    default:
      return usage(argv[0]);
    }
  }



  RGBMatrix *matrix = CreateMatrixFromOptions(matrix_options, runtime_opt);
  if (matrix == NULL) {
    return 1;
  }

  FrameCanvas *offscreen_canvas = matrix->CreateFrameCanvas();
//  FrameCanvas *video_canvas = matrix->CreateFrameCanvas();
//  FrameCanvas *projectm_canvas = matrix->CreateFrameCanvas();



  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);


//Main loop




int count2 = 0;
  do {

    while (!interrupt_received) {
 	count++;
 count2+=5;
 
//processImage(blank_image, offscreen_canvas, count2);
 
	if(count > UPDATE_MULTIPLE and ((mplayerStarted == true) && (projectMStarted == true)))
	{    

cout << "######Condition OK!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
		count = 0;
 		XImage *img = XGetImage(display,currentTarget,0,0,width,height,XAllPlanes(),ZPixmap);


 		XImage *imgv = XGetImage(display,targetVideo,0,0,width,height,XAllPlanes(),ZPixmap);
 		XImage *imgp = XGetImage(display,targetProjectM,0,0,width,height,XAllPlanes(),ZPixmap);
 
        	//if (img != NULL)
        	if (imgv != NULL)

        	{
        	   //saveXImageToBitmap(img);
        	   //printf("copying\n");
        	   //CopyFrame(img, offscreen_canvas); 

		   blendCanvas(imgv, alphaVideo, imgp, alphaProjectM, offscreen_canvas);
			//cout << "blended" << endl;


        	   XDestroyImage(img);
        	   XDestroyImage(imgv);
        	   XDestroyImage(imgp);

        	}
		else
		{
		  printf( "Error : could not get Ximage. Trying to find proper Window\n" );
		  //cout << "Error : could not get Ximage. Trying to find proper Window";
		  if(mplayerStarted == true)
		  {
                     	targetVideo = getWindowFromName(display, "MPlayer");
			XWindowAttributes gwa;
       	 		XGetWindowAttributes(display, targetVideo, &gwa);
        		width = gwa.width;
        		height = gwa.height;
			
			currentTarget = targetVideo;
	
		  }
		}
	}

	
        offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas,
                                                	   vsync_multiple);
	


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

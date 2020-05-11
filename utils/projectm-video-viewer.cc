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
#include <vector>
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
bool imageStarted = false;

#define UPDATE_MULTIPLE 4

#define OSCPKT_OSTREAM_OUTPUT
#include "oscpkt/oscpkt.hh"
#include "oscpkt/udp.hh"
using namespace oscpkt;


#define KEYCODE XK_q

#define NB_MAX_CHANNEL 		8
#define CHANNEL_VIDUALIZER_1 	0
#define CHANNEL_IMAGE_1 	1
#define CHANNEL_IMAGE_2 	2
#define CHANNEL_VIDEO_1 	3
#define CHANNEL_VIDEO_2 	4
#define CHANNEL_CAMERA_1 	5
#define CHANNEL_CAMERA_2 	6



enum MSChannelType { undefined = 0, visualizer = 1, video = 2, image = 3, camera = 4 };

class MegaScreenChannel
{
	public:
	Window m_window 	= 0;
	int m_width 		= 0;
	int m_height 		= 0;
	float m_alpha		= 1.0;
	bool m_started		= false;
	MSChannelType m_type	= undefined;
	XImage * img = 0;
};

MegaScreenChannel MegaScreenChannelArray[NB_MAX_CHANNEL];


const int PORT_NUM = 7700; //TODO : get this from args
    	Display *display;
	Window winRoot = 0;
	



        XWindowAttributes gwa;



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



void stopVideo(Window windowChannel) 
{

	mplayerStarted =  false;

	//Need to declare this here, else it causes weird Xlib failure...
	XKeyEvent event = createKeyEvent(display, windowChannel, winRoot, true, KEYCODE, 0);


	cout << "sending keypress" << endl;
	//XSetInputFocus(display, currentTarget, false, CurrentTime);
	XSelectInput(display, windowChannel, KeyPressMask|KeyReleaseMask);
		
	// Send a fake key press event to the window.

	XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);

	XFlush(display) ;
	XSync(display, false);

}

void stopChannel(int index)
{
	MegaScreenChannel thisChannel = MegaScreenChannelArray[index];
	switch(thisChannel.m_type){
		case video :
			stopVideo(thisChannel.m_window);
		//case:
	
	}
	

}
	
void startVideo(int channelIndex, int videoIndex) 
{
	MegaScreenChannel * thisChannel = &(MegaScreenChannelArray[channelIndex]);

	thisChannel->m_type = video;


	if(thisChannel->m_started == false)
	{
      		std::string command = "./startVideo.sh " + std::to_string(videoIndex);
     	 	//system(command.c_str());
      		thisChannel->m_started =  true;
		
		while(thisChannel->m_window == 0) //TODO : reset window when stopping.
		{ 
		    cout << "trying to find player window" << endl;
		    thisChannel->m_window = getWindowFromName(display, "MPlayer");
		    std::this_thread::sleep_for(std::chrono::milliseconds(100));
		    
		}
        	 XGetWindowAttributes(display, thisChannel->m_window, &gwa);
       		 thisChannel->m_width  = gwa.width;
       		 thisChannel->m_height = gwa.height;

		
	}
}

void startImage(int id, Window * targetWindow) 
{
	if(imageStarted == false)
	{
      		std::string command = "./startImage.sh " + std::to_string(id);
     	 	//system(command.c_str());
      		imageStarted =  true;
		
		while(*targetWindow == 0) //TODO : reset window when stopping.
		{ 
		    cout << "trying to find image window" << endl;
		    *targetWindow = getWindowFromName(display, "gifview");
		    std::this_thread::sleep_for(std::chrono::milliseconds(100));
		    
		}
        	 XGetWindowAttributes(display, *targetWindow, &gwa);
       		 //channel3Width = gwa.width;
       		 //channel3Height = gwa.height;

		
	}
}


void startChannel(int index, MSChannelType channelType, int param)
{
	switch(channelType){
		case video :
			startVideo(index, param);
			cout << "setting up video channel" << endl;
		//case:
	
	}
	

}


/* no longer needed, will be done thru alpha blending
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
*/


/*
void startProjectM(int id) 
{

 // system("./startVideo.sh ");
 //Default channel 1 reserved for projectM
        windowChannel1 = getWindowFromName(display, "projectM");
        XGetWindowAttributes(display, windowChannel1, &gwa);
        channel1Width  = gwa.width;
        channel1Height = gwa.height;
	projectMStarted =  true;

}
*/

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
		
		//alphaChannel2 = tempF;
		//printf("New alpha for Video : : %f\n", alphaChannel2);
	  }
	  if (msg->match("/1/fader2").popFloat(tempF)) {
		
		//alphaChannel1 = tempF;
		//printf("New alpha for ProjectM : : %f\n", alphaChannel1);
	  }	  
          if (msg->match("/megascreen/video").popInt32(iarg).isOkNoMoreArgs()) {
            cout << "Server: received video request " << iarg << " from " << sock.packetOrigin() << "\n";
	    
          //  Message repl; repl.init("/pong").pushInt32(iarg+1);
          //  pw.init().addMessage(repl);
          //  sock.sendPacketTo(pw.packetData(), pw.packetSize(), sock.packetOrigin());
          if(iarg == 0){
	  	//stopVideo(windowChannel2);

		continue;
	  }
	  if (iarg >= 10 && iarg < 20)
	  {
	   	//startProjectM(10 - iarg);
	  	continue;
	  }
	  if (iarg >= 20 && iarg < 30)
	  {
	  	//startVideo(1, 20 - iarg);
		startChannel(CHANNEL_VIDEO_1, video, 20 - iarg);
		continue;
	  }

	  if (iarg >= 30 && iarg < 40)
	  {
	  	//startImage(30 - iarg, &windowChannel3);
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




void CopyFrame(XImage *pXimage, FrameCanvas *canvas, float alpha) {

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
   
     canvas->SetPixel(w, h, pXimage->data[indexR]*alpha, pXimage->data[indexG]*alpha, pXimage->data[indexB]*alpha);

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
	
	
	//TODO : exit to have proper gmon file generation when profiling 
	//exit(0);
        return 0;
}




void blendCanvas(XImage *img1, float alpha1, XImage *img2, float alpha2, FrameCanvas *result)
{


//printf ("inside blend function\n");

int biBitCount =32;
int dwBmpSize = ((img1->width * biBitCount + 31) / 32) * 4 * img1->height;


if(img1 == 0 || img2 == 0)
{
	cout << "ERROR : trying to blend with null pointer images" << endl;

}



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

  }	
}


}

std::vector<MegaScreenChannel*> getActiveChannelList(){
  
  std::vector<MegaScreenChannel*> result; 
  for(int i = 0; i < NB_MAX_CHANNEL; i++){
    MegaScreenChannel* tempChannel = &(MegaScreenChannelArray[i]);
    if(tempChannel->m_started){
      result.push_back(tempChannel);
      
    }
    
  }
  return result;
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


  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);


//Main loop

 
  do {

    while (!interrupt_received) {
 	count++;
 
 
	if(count > UPDATE_MULTIPLE)
	{    

		count = 0;
		std::vector<MegaScreenChannel*> activeChannelList = getActiveChannelList();
		
		for(std::vector<MegaScreenChannel*>::iterator it = activeChannelList.begin() ; it != activeChannelList.end(); ++it)
		{
		    cout << "\nfound " << activeChannelList.size() << " active channel(s)" << endl;
		    (*it)->img = XGetImage(display,(*it)->m_window,0,0,(*it)->m_width,(*it)->m_height,XAllPlanes(),ZPixmap);
		    if((*it)->img != 0)
		    {
		        CopyFrame((*it)->img, offscreen_canvas, (*it)->m_alpha); 
			XDestroyImage((*it)->img );
		    }
		}

/*
        	if (((alphaChannel1 != 0) && (alphaChannel2 != 0)) && ((projectMStarted == true)&&(mplayerStarted == true)))
        	{
        	   
		//cout << "both channels available, trying to blend" << endl;
		   
		   //When video restarting (loop mode), the window may not be available for few frames.
		   if(imgChannel1 != 0 && imgChannel2 != 0){
		   	blendCanvas(imgChannel1, alphaChannel1, imgChannel2, alphaChannel2, offscreen_canvas);
			//cout << "blended" << endl;
		   }

        	}
*/		
	

	}

	
        offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas,
                                                	   vsync_multiple);
	
	//cout << "copied to rgb matrix" << endl;


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

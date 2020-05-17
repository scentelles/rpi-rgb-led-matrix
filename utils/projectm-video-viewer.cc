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

UdpSocket sock; 
UdpSocket sock2; 
const int PORT_NUM = 7701; //TODO : get this from args
const int PORT_NUM2 = 7702; //TODO : get this from args

#define KEYCODE XK_q

#define NB_MAX_CHANNEL 			8
#define CHANNEL_VISUALIZER_1 	0
#define CHANNEL_IMAGE_1 		1
#define CHANNEL_IMAGE_2 		2
#define CHANNEL_VIDEO_1 		3
#define CHANNEL_VIDEO_2 		4
#define CHANNEL_SLIDESWHOW_1	5
#define CHANNEL_SLIDESWHOW_2	6
#define CHANNEL_CAMERA_1 		7
#define CHANNEL_CAMERA_2 		8

//Delay between each attempt to find the Xwindow of channel beeing started
#define RETRY_DELAY 100 

enum MSChannelType { undefined = 0, visualizer = 1, video = 2, image = 3, slideshow = 4, camera = 5 };

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
std::string getWindowName(Display *disp, Window win) {
    Atom prop = XInternAtom(disp,"WM_NAME",False), type;
    int form;
    unsigned long remain, len;
    unsigned char *list;


    if (XGetWindowProperty(disp,win,prop,0,1024,False,AnyPropertyType,
                &type,&form,&len,&remain,&list) != Success) { // XA_STRING

        return "";
    }

	if(len != 0)
	{
		return std::string((char*)list);
	
	}
	else
	{
		//upon creation of window, when app launches, name can be empty.
		cout << "WARNING : Window without name yet ! "<< endl;
		return "";
	}
}




Window getWindowFromName(Display *display, std::string name)
{


    Window *list;

    unsigned long len;
    int j;  
    int windowId = -1;
    
    list = (Window*)getWindowList(display,&len);
    std::string tempName;
		
    for (j=0;j<(int)len;j++) {
        tempName = getWindowName(display,list[j]);

	//cout << "\tWindow name : " << tempName << endl;
	
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
	MegaScreenChannel* thisChannel = &(MegaScreenChannelArray[index]);

	thisChannel->m_started = false;

	switch(thisChannel->m_type){
		case video :
			stopVideo(thisChannel->m_window);


		//case:
	
	}
	thisChannel->m_window = 0;	

}
	
void sendMessageToLauncher(std::string message, int param)
{
  sock2.connectTo("localhost", PORT_NUM2);
  if (!sock.isOk()) {
    cerr << "Error opening port " << PORT_NUM << ": " << sock.errorMessage() << "\n";
  }
  else {
	  cout << "sock2 opened. sending message to start app" << endl;

      Message repl; repl.init(message).pushInt32(param);
	  PacketWriter pw;
      pw.init().addMessage(repl);
      sock2.sendPacketTo(pw.packetData(), pw.packetSize(), sock.packetOrigin());
      cout << "packet OSC sent" << endl;	
  }
}

	
void startVideo(int channelIndex, int videoIndex) 
{
	MegaScreenChannel * thisChannel = &(MegaScreenChannelArray[channelIndex]);

	thisChannel->m_type = video;

   	sendMessageToLauncher("/megascreen/startapp/video", videoIndex); 

		
	while(thisChannel->m_window == 0) 
	{ 
	    cout << "trying to find player window" << endl;
	    thisChannel->m_window = getWindowFromName(display, "MPlayer");
	    std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY));
		XFlush(display);
		XSync(display, false);
		    
	}
	

    XGetWindowAttributes(display, thisChannel->m_window, &gwa);
    thisChannel->m_width  = gwa.width;
    thisChannel->m_height = gwa.height;

	thisChannel->m_started =  true;
		
}

void startImage(int channelIndex, int videoIndex) 
{
	MegaScreenChannel * thisChannel = &(MegaScreenChannelArray[channelIndex]);

	thisChannel->m_type = image;

	std::string command = "./startImage.sh " + std::to_string(videoIndex);
 	cout << "Issued command : " << command << endl;
	system(command.c_str());

		
	while(thisChannel->m_window == 0) 
	{ 
	    cout << "trying to find image window" << endl;
	    thisChannel->m_window = getWindowFromName(display, "gifview");
	    std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY));
		    
	}
	

    XGetWindowAttributes(display, thisChannel->m_window, &gwa);
    thisChannel->m_width  = gwa.width;
    thisChannel->m_height = gwa.height;

	thisChannel->m_started =  true;
}


void startProjectM(int channelIndex, int presetIndex) //TODO : refactor to have generic function start
{
	MegaScreenChannel * thisChannel = &(MegaScreenChannelArray[channelIndex]);

	thisChannel->m_type = visualizer;

   	sendMessageToLauncher("/megascreen/startapp/visualizer", presetIndex); 


	while(thisChannel->m_window == 0) 
	{ 
	    cout << "trying to find projectM window" << endl;
	    thisChannel->m_window = getWindowFromName(display, "projectM");//todo : look for proper name
	    std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY));
		XFlush(display);
		XSync(display, false);
		    
	}
	cout << "Found window : " << thisChannel->m_window << endl;
	

    XGetWindowAttributes(display, thisChannel->m_window, &gwa);
    thisChannel->m_width  = gwa.width;
    thisChannel->m_height = gwa.height;

	thisChannel->m_started =  true;
		
}

void startSlideshow(int channelIndex, int presetIndex) //TODO : refactor to have generic function start
{
	MegaScreenChannel * thisChannel = &(MegaScreenChannelArray[channelIndex]);

	thisChannel->m_type = slideshow;


	sendMessageToLauncher("/megascreen/startapp/slideshow", presetIndex);
  
		
	while(thisChannel->m_window == 0) 
	{ 
	    cout << "trying to find impress window" << endl;
	    thisChannel->m_window = getWindowFromName(display, "slide");//todo : look for proper name
	    std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY));
		XFlush(display);
		XSync(display, false);
		    
	}
	cout << "Found window : " << thisChannel->m_window << endl;
	
	
    XResizeWindow(display, thisChannel->m_window , 128, 128); //TODO set according to config of the screen
    XGetWindowAttributes(display, thisChannel->m_window, &gwa);
    thisChannel->m_width  = gwa.width;
    thisChannel->m_height = gwa.height;

	sleep(1);//TODO resize takes time?
	thisChannel->m_started =  true;
  
		
}


void startChannel(int index, MSChannelType channelType, int param)
{
	MegaScreenChannel * thisChannel = &(MegaScreenChannelArray[index]);
	
	if(thisChannel->m_started == true)
	{
		cout << "CHANNEL ALREADY STARTED. STOPPING IT" << endl;
		stopChannel(index);
		XSync(display, false);
		sleep(1); //todo : use exact window name to check for corretct window. Else, close takes longer than reopen and finds previous window.find a way to sync on proper closure of application.
	}
	switch(channelType){
		case video :
			cout << "setting up video channel" << endl;
			startVideo(index, param);
			break;
		case visualizer:
			cout << "setting up visualizer channel" << endl;
			startProjectM(index, param);
			break;
		case image:
			cout << "setting up image channel" << endl;
			startImage(index, param);
			break;	
		case slideshow:
			cout << "setting up slideshow channel" << endl;
			startSlideshow(index, param);
			break;	
	}
	

}


void runOSCServer() {

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
		
		MegaScreenChannelArray[CHANNEL_VIDEO_1].m_alpha = tempF;
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
		  stopChannel(CHANNEL_VIDEO_1);
		  stopChannel(CHANNEL_IMAGE_1);
  		  stopChannel(CHANNEL_VISUALIZER_1);


		continue;
	  }
	  if (iarg >= 10 && iarg < 20)
	  {
	   	startChannel(CHANNEL_VISUALIZER_1, visualizer, iarg - 10);
		continue;
	  }
	  if (iarg >= 20 && iarg < 30)
	  {
	  	startChannel(CHANNEL_VIDEO_1, video, iarg - 20);
		continue;
	  }

	  if (iarg >= 30 && iarg < 40)
	  {
	  	startChannel(CHANNEL_IMAGE_1, image, iarg - 30);
	  	continue;
	  }
	  if (iarg >= 40 && iarg < 50)
	  {
	  	startChannel(CHANNEL_SLIDESWHOW_1, slideshow, iarg - 40);
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
	
	
	//TODO : look for channel started but window not found
	
	
	
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

//Blends two images of same size
void blendImage(XImage * inputImage, float alpha, XImage * outputImage)
{
	//cout << "pointerin : " << inputImage << " | pointerout : " << outputImage << endl;
	for(int x = 0; x < inputImage->width; x++)
	{
		for(int y = 0; y < inputImage->height; y++)
		{
			XColor colorsInput, colorsOutput;
			colorsInput.pixel  = XGetPixel(inputImage, x, y);
			colorsOutput.pixel = XGetPixel(outputImage, x, y);


			unsigned long red_mask   = inputImage->red_mask;
			unsigned long green_mask = inputImage->green_mask;
			unsigned long blue_mask  = inputImage->blue_mask;						
				  
			int BI = (colorsInput.pixel & blue_mask) * alpha;
			int GI = ((colorsInput.pixel & green_mask) >> 8) * alpha;
			int RI = ((colorsInput.pixel & red_mask) >> 16) * alpha;      
			int BO = (colorsOutput.pixel & blue_mask) * alpha;
			int GO = ((colorsOutput.pixel & green_mask) >> 8) * alpha;
			int RO = ((colorsOutput.pixel & red_mask) >> 16) * alpha;   	
			int BResult =  std::min(BI + BO, 255);
			int GResult =  std::min(GI + GO, 255);
			int RResult =  std::min(RI + RO, 255);
			colorsOutput.pixel = 65536 * RResult + 256 * GResult + BResult ; 
			XPutPixel(outputImage, x, y, colorsOutput.pixel);
	
		}
	}
							

}

XImage *CreateColorImage(Display *display, Visual *visual, unsigned char *image, int width, int height, unsigned char red, unsigned char green, unsigned char blue )
{
    int i, j;
    unsigned char *image32=(unsigned char *)malloc(width*height*4);
    unsigned char *p=image32;
    for(i=0; i<width; i++)
    {
        for(j=0; j<height; j++)
        {
                *p++=red; // blue
                *p++=green; // green
                *p++=blue; // red
   
            p++;
        }
    }
    return XCreateImage(display, visual, DefaultDepth(display,DefaultScreen(display)), ZPixmap, 0, (char*)image32, width, height, 32, 0);
}



int main(int argc, char *argv[]) {

	//Required as multiple thread will acces X
XInitThreads();

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


//Debug Display Window
//TODO : get resolution from config
Window window=XCreateSimpleWindow(display, RootWindow(display, 0), 0, 0, 128, 128, 1, 0, 0);
XStoreName(display, window, "VIRTUAL MS");
XMapWindow(display, window);

Visual *visual=DefaultVisual(display, 0);

XImage blackImage = *(CreateColorImage(display, visual, 0, 128, 128, 0, 0, 0));
XImage whiteImage = *(CreateColorImage(display, visual, 0, 128, 128, 255, 255, 255));
XImage finalImage = blackImage; //TODO get resolution from config

//Main loop

  
  
  do {

    while (!interrupt_received) {
 	count++;
 
 
	if(count > UPDATE_MULTIPLE)
	{    

		count = 0;
		std::vector<MegaScreenChannel*> activeChannelList = getActiveChannelList();
		//cout << "\nfound " << activeChannelList.size() << " active channel(s)" << endl;
		
		int index = 0;
		


		for(std::vector<MegaScreenChannel*>::iterator it = activeChannelList.begin() ; it != activeChannelList.end(); ++it)
		{
			//cout << "looping on active channels : index " << index << endl;
			if((*it)->m_window != 0)
			{
				(*it)->img = XGetImage(display,(*it)->m_window,0,0,(*it)->m_width,(*it)->m_height,XAllPlanes(),ZPixmap);
				if((*it)->img == NULL)
				{
					
					cout << "WARNING : image pointer null while channel started. Resetting channel state" << endl;
					
					(*it)->m_started = false;
					(*it)->m_window = 0;
				}
				else
				{
					if((*it)->img != 0)
					{
						if(index == 0)
						{
						    finalImage = *((*it)->img);								

							for(int x = 0; x < (*it)->img->width; x++)
							{
								for(int y = 0; y < (*it)->img->height; y++)
								{
									XColor colors;
									colors.pixel = XGetPixel((*it)->img, x, y);
									unsigned long red_mask   = finalImage.red_mask;
									unsigned long green_mask = finalImage.green_mask;
									unsigned long blue_mask  = finalImage.blue_mask;						
										  
									int B1 = (colors.pixel & blue_mask) * (*it)->m_alpha;
									int G1 = ((colors.pixel & green_mask) >> 8) * (*it)->m_alpha;
									int R1 = ((colors.pixel & red_mask) >> 16) * (*it)->m_alpha;      
									colors.pixel = 65536 * R1 + 256 * G1 + B1 ; 
									XPutPixel(&finalImage, x, y, colors.pixel);
							
								}
							}
						}
						
						else //for all other indexes, > 0
						{
							blendImage((*it)->img, (*it)->m_alpha, &finalImage);
						}

					}
				}
				
				
			}
			index++;
		}
		
		if(activeChannelList.size() != 0)
		{
		    CopyFrame(&finalImage, offscreen_canvas, 1.0); 
	        //TODO : display virtual screen based on option
			XPutImage(display, window, DefaultGC(display, 0), &finalImage, 0, 0, 0, 0, finalImage.width, finalImage.height); 
		}
		//TODO : warning : there should be memory leak as we do not destry images above

	

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

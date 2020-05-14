#include <unistd.h>
#include <stdio.h>

#include <iostream>
using std::endl;
using std::cout;
using std::cerr;

#include "oscpkt/oscpkt.hh"
#include "oscpkt/udp.hh"
using namespace oscpkt;

const int PORT_NUM = 7702; //TODO : get this from args




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


         if (msg->match("/megascreen/startapp/slideshow").popInt32(iarg).isOkNoMoreArgs()) {
            cout << "Server: received video request " << iarg << " from " << sock.packetOrigin() << "\n";
 
         cout << "Server: received start slidewhow "  << iarg << " app request from " << sock.packetOrigin() << "\n";
	      
	      std::string command = "./startSlideshow.sh " + std::to_string(iarg);
 	      cout << "Issued command : " << command << endl;
	      system(command.c_str());

      
	            }
        }
      }
    }
  }
}








int main(int argc, char *argv[]) {
	printf("Launcher Started\n");
	
	runOSCServer();
	
}
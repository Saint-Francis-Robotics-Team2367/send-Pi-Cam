#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/videoio.hpp"
#include "OpenVideo.hpp"
#include "OpenFilter.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iterator>
#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>

#define PORT 8888
#define BUFFER_SIZE 1*800*600
using namespace cv;
using namespace std;

int main()
{
    //tcp settings
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    const char *hello = "Hello from server";
    
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Forcefully attaching socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
    
    bind(server_fd, (struct sockaddr *)&address,sizeof(address));
    
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    
    Mat image(600,800,CV_8UC3);
    Mat edges(600,800,CV_8UC3);
    int datalen = 0,
    packetSize = 0,
    currPos =0,
    currPacket=0,
    radius1,
    radius2;
    Point2f center1, center2;
    Rect rect1, rect2;
    vector<int> HSV(6);
    
    Size imageSize;
    Filter brita;
    const string filename = "home/pi/send-Pi-Cam.txt";                      //need to get this to save
    
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    
    OpenVideo myVideo(0);
    cout << "Capture is opened" << endl;
    
    if (!brita.readHSV(filename))            //if HSV file is not created
    {
        image = myVideo.getImage();
        imageSize = image.size();
        datalen = imageSize.width * imageSize.height * 3;
        packetSize = imageSize.width;
        
        
        while(currPos < datalen)            //send one image to config values on client side
        {
            if(currPos + packetSize > datalen)
                currPacket = datalen - currPos;
            else
                currPacket = packetSize;
            
            send(new_socket, image.data + currPos, currPacket, 0);
            currPos += currPacket;
        }
        
        read(new_socket, HSV.data(), sizeof(int)*6);    //get HSV values from trackbar on client side
        
        brita.h_min = HSV[0];
        brita.h_max = HSV[1];
        brita.s_min = HSV[2];
        brita.s_max = HSV[3];
        brita.v_min = HSV[4];
        brita.v_max = HSV[5];
        
        cout<<"H MIN  "<< brita.h_min <<endl;
        cout<<"H Max  "<< brita.h_max <<endl;
        cout<<"S MIN  "<< brita.s_min <<endl;
        cout<<"S MAX  "<< brita.s_max <<endl;
        cout<<"V MIN  "<< brita.v_min <<endl;
        cout<<"V MAX  "<< brita.v_max <<endl;
        
        brita.writeHSV(filename);
        
    }
    
    while(waitKey(100) != 'q'){
        
        cout<<"making bounding"<<endl;
        image = myVideo.getImage();
        edges = brita.edgeDetect(&image);
        
        /// Find contours
        findContours(edges, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) ); //external
        
        // Approximate contours to polygons + get bounding rects and circles
        vector<vector<Point> > contours_poly( contours.size() );
        vector<Rect> boundRect( contours.size() );
        vector<Point2f>center( contours.size() );
        vector<float>radius( contours.size() );
        
        for( int i = 0; i < contours.size(); i++ )
        { approxPolyDP( Mat(contours[i]), contours_poly[i], 3, true );
            boundRect[i] = boundingRect( Mat(contours_poly[i]) );
            minEnclosingCircle( (Mat)contours_poly[i], center[i], radius[i] );
        }
        
        /// Draw polygonal contour + bonding rects + circles
        for( int i = 0; i< contours.size(); i++ )
        {
            
            if (radius[i]> radius1)             //gets 2 largest circles
            {
                radius2 = radius1;
                radius1 = radius[i];
                center1 = center[i];
            }
            else if (radius[i] > radius2){
                radius2 = radius[i];
                center2 = center[i];
            }
            
            if (boundRect[i].area()> rect1.area())             //gets 2 largest rect
            {
                rect2 = rect1;
                rect1 = boundRect[i];
                
            }
            else if (boundRect[i].area()> rect2.area()){
                rect2 = boundRect[i];
            }
            
        }

	 cout<<radius1<<" radius"<<endl;
	
        send(new_socket, &center1, sizeof(Point2f),0);     //send info to draw circle
        send(new_socket, &radius1, sizeof(int),0);
        send(new_socket, &center2, sizeof(Point2f),0);
        send(new_socket, &radius2, sizeof(int),0);
        
        send(new_socket, &rect1, sizeof(Rect), 0);          //send info to draw rect
        send(new_socket, &rect2, sizeof(Rect), 0);
        
    }
    return 0;
    
}


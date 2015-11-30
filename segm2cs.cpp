/********************************************
 * Based of OpenCV segmentation_fgbg -example
 * modified by holgafreak for sending OSC
 * OpenCV is BSD-license, so this must be
 * also, feel free to do whatever, but
 * keep (c) Matti Koskinen et OpenCV
 * hacked 2015
 *******************************************/


#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio/videoio.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/background_segm.hpp>
#include "oscsend.h"
#include <stdio.h>
#include <string>
#include <vector>
#include <ctype.h>

using namespace std;
using namespace cv;

static int ivol = 100; // dB gain

// take contour of the segmentated foreground to clean up
// also create OSC-message based on charcteristic of the contour

static void refineSegments(OSC * osc, const Mat& img, Mat& mask, Mat& dst)
{
    int niters = 3;

    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;

    Mat temp;
    // close the holes
    dilate(mask, temp, Mat(), Point(-1,-1), niters);
    erode(temp, temp, Mat(), Point(-1,-1), niters*2);
    dilate(temp, temp, Mat(), Point(-1,-1), niters);
    // find the contour
    findContours( temp, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE );

    dst = Mat::zeros(img.size(), CV_8UC3);
    // sanity check
    if( contours.size() == 0 )
        return;

    //find the outmost of the contours
    int idx = 0, largestComp = 0;
    double maxArea = 0;
    vector<Point> c;
    for( ; idx >= 0; idx = hierarchy[idx][0] )
    {
        c = contours[idx];
        double area = fabs(contourArea(Mat(c)));
        if( area > maxArea )
        {
            maxArea = area;
            largestComp = idx;
        }
    }
    // not found so return and then stry again of the next frame
    if(maxArea == 0)
      return;
    // the outmost contour
    vector<Point> ci = contours[largestComp];
    // max possible area is the frame rows by cols
    const float maximumArea = static_cast<float>(temp.rows*temp.cols);
    // the area out the contour enclosed surface 
    double area = fabs(contourArea(Mat(ci)));
    // length of the contour
    double length = arcLength(Mat(ci),false);
    // boundng box i.e. topleft topright bottomright bottomleft
    Rect r = boundingRect(Mat(ci));
    // the y (row) set as base freuency for the csound csd
    float basefreq = static_cast<float>(r.y);
    // the height is freq shift
    float freqshift = static_cast<float>(r.height);
    // startimng column as pan start (divided by max cols to get into range 0-1
    float panstart = static_cast<float>(r.x) / mask.cols;
    // width pan
    float pan = static_cast<float>(r.width)/mask.cols;
    // lenght of contour for ring modulator
    float modulate = length;
    // volume as dB-like
    float vol = fabs(log(area/maximumArea)/log(10.));
    // if in need to change the gain, use the slider in segmentation  window
    float fvol = ivol/1000.; // scaling
    vol *= fvol;
    char *msg = new char[100];
    // create the OSC-message
    sprintf(msg,"ffffffi %f %f %f %f %f %f %d",fabs(vol),basefreq,freqshift,panstart,pan,modulate,0);
    //in c++14 this is easier
    string message = string(msg);
    delete [] msg;
    if(osc->sendMsg(message,"/img2snd/run")<0)
      return; // can't send
    
    //draw the bounding box and segmentation
    Scalar color( 0, 0, 255 );
    vector<Point> rec = {Point(r.x,r.y),Point(r.x+r.width,r.y),Point(r.x+r.width,r.height+r.y),Point(r.x,r.y+r.height)};
    polylines(dst,rec,true,Scalar(255,255,0));
    drawContours( dst, contours, largestComp, color, FILLED, LINE_8, hierarchy );
}
    
void help() {
  cout << "this is a proof of concept, how to send e.g. gestures to csound/grace etc." << endl;
  cout << "usage is simple: parameters are 1st. camera idx/input video file" << endl;
  cout << " 2nd. OSC communication port" << endl;
  cout << "3rd. optional OSC-host, by default localhost" << endl;
  cout << "4th. optinal video output. XVID has been available, at least in  the past (.avi)" << endl << endl;
  cout << "pressing the spacebar programme goes to background estimation mode" << endl;
  cout << "or pressing again to the segmetation mode" << endl;
  cout << "The programme starts in the learning mode" << endl;
}
  

int main(int argc, char** argv)
{
  help();
  VideoCapture cap; // video capture a number = camera idx;
  //ascii -> video file name to input
  bool update_bg_model = true;
  if(argc < 3) {
    // host is by default localhost
    cout << "usgae: segm2cs [camera idx|video input file] port <host> <video output>" << endl;
    return 1;
  }
  int c = atoi(argv[1]);
  string cam = string(argv[1]);
  cout << "cam " << c << endl;
  try {
    cap.open(c); // try first number
  } catch(...) {
    cerr << "not a  cmaera" << endl;
    cap.open(cam); // try as file
  }
  // if it doesn't succeed, return with error
  if( !cap.isOpened() )
    {
      printf("\nCan not open camera or video file\n");
      return -1;
    }
    string port = string(argv[2]);
    string host="127.0.0.1";
    if(argc >= 4) // optional host
      host = string(argv[3]);
    cerr << port << ":" << host <<endl;
    // create OSC
    OSC *osc = new OSC();
    // if init fails, return with error
    if(osc->init(host,port) < 0) {
      cerr << "error initializing osc" << endl;
      return -1;
    }
    // check for output video
    bool doVideoOut = false;
    VideoWriter vw;
    if(argc >= 5)
      doVideoOut = true;
    Mat tmp_frame, bgmask, out_frame;
    // try input
    cap >> tmp_frame;
    // fails  return with error
    if(tmp_frame.empty())
    {
        printf("can not read data from the video source\n");
        return -1;
    }
    if(doVideoOut) {
      // open video, NB. on some versions of OpenCV, 
      // XVID was the only possible on Linux and OS X anyway some time ago
      vw.open(string(argv[4]),VideoWriter::fourcc('X','V','I','D'),25,tmp_frame.size(),1);
      if(!vw.isOpened()) {
	  cout << "can't open output video" << endl;
	  return -1;
      } // fail
    }
    // open windows to see, what's going on
    namedWindow("video", 1);
    namedWindow("segmented", 1);
    //slider for gain
    createTrackbar("dB gain","segmented",&ivol,1000,NULL);
    // magic to find forgraound, mixture of Gaussians
    Ptr<BackgroundSubtractorMOG2> bgsubtractor=createBackgroundSubtractorMOG2();
    bgsubtractor->setVarThreshold(10);
    // loop forever until stopped
    for(;;)
    {
      cap >> tmp_frame;
      if( tmp_frame.empty() )
	break; // empty frame, break loop 
	
      Mat videoM = tmp_frame.clone(); // create clone of the input for clean
      bgsubtractor->apply(tmp_frame, bgmask, update_bg_model ? -1 : 0);
      // clean up, and send OSC-message
      refineSegments(osc, tmp_frame, bgmask, out_frame);
      Mat scaledFrame;
      resize(out_frame,scaledFrame,Size(),0.33,0.33);
      Rect place(0,0,scaledFrame.cols,scaledFrame.rows);
      scaledFrame.copyTo(tmp_frame(place)); // Picture in Picture
      imshow("video", tmp_frame);
      imshow("segmented", out_frame);
      int keycode = waitKey(20); // get the user pressed key
      if( keycode == 27 || keycode == 'q') // end
	break;
      if( keycode == ' ' ) // change model (learn bg / segmentate)
        {
	  update_bg_model = !update_bg_model;
	  printf("Learn background is in state = %d\n",update_bg_model);
        }
      if(doVideoOut) // do video output
	vw.write(videoM); // write frame
    }
    // send exit OSC
    string termCS = "ffffffi 0. 0. 0. 0. 0. 0. 1";
    osc->sendMsg(termCS,"/img2snd/run");
    return 0; // finally done
}

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <string.h>
#include <sstream>

using namespace cv;
using namespace std;

template <typename T>
  string NumberToString ( T Number )
  {
     ostringstream ss;
     ss << Number;
     return ss.str();
  }

int main()
{
	int index = 0;

	// int a = 10;
	// char *intStr = itoa(a);
	// string str = string(intStr);
	
	string file_name;
    VideoCapture capture(0);
    // VideoWriter writer("VideoTest.avi", CV_FOURCC('M', 'J', 'P', 'G'), 25.0, Size(640, 480));
    Mat frame;

    // set up the image format and the quality
    vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
    compression_params.push_back(95);


    while (capture.isOpened())
    {
        capture >> frame;
        // writer << frame;
        imshow("video", frame);
        // file_name = to_string(index);
        file_name = NumberToString(index);
        imwrite("pics/" + file_name + ".jpeg", frame, compression_params);
        ++index;
        if (cvWaitKey(20) == 27)
        {
            break;
        }
    }

    return 0;
}
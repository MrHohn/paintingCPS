/*
    Author: Wuyang Zhang
    June 17.2015
    ImgMatch.h
*/

// #pragma onc
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <opencv2/legacy/legacy.hpp> 
#include <fstream> 
#include <string>
using namespace cv;
using namespace std;
 
//initiate datebase
//img match
//api: read write back 
 
class ImgMatch
{
public:
    ImgMatch();
    ~ImgMatch();
    static void init_DB(int size_DB, string add_DB, string indexImgAdd, string featureClusterAdd); // minHessian, db add, db size
    static void init_infoDB( string add_DB);
    static long int init_matchImg(string indexImgAdd,string featureClusterAdd,string imgInfoAdd);

    string matchImg(string srcImgAdd, long int initiatePointer);
    string matchImg(char* img, int size);
    void set_minHessian(int minHessian);
    static void set_dbSize(int size_DB);
    static void releaseInitResource(long int initiatePointer  );
    void set_indexImgAdd(string indexImgAdd);
    int getMatchedImgIndex();
    vector<float> calLocation();
    vector<float> matchedLocation;
    void clearLocation();
    void getMatchedImgInfo();
    string getInfo();
    void locateDrawRect(vector<float> location);
    void locateDrawCirle(vector<float> location);
private:
    static int minHessian;
    static string add_DB;  //database address
    static int size_DB;    //database size, number of img
    static string indexImgAdd; // index-img hash table address
    Mat despSRC;
    static Mat despDB; // descriptors of src and db img
    Mat srcImg;
    static Mat dbImg;
    static vector<KeyPoint> dbKeyPoints;
    vector<KeyPoint> keyPoints1;
    vector<KeyPoint> keyPoints2;
    // undefinded 
    static vector<int>index_IMG;
    static Mat featureCluster;
    static flann::Index *flannIndex;
    static string featureClusterAdd; //file address saving feature descriptors of all images
    static string imgInfoAdd;
    Mat indices, dists;
    struct ImgFreq{
        int ImgIndex;
        int Freq;
    };
 
    int matchedImgIndex;
};

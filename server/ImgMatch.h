/*
    Author: Wuyang Zhang
    June 17.2015
 
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
    void init_DB(int size_DB, string add_DB, string indexImgAdd, string featureClusterAdd); // minHessian, db add, db size
    void matchImg(string srcImgAdd);
    void init_matchImg(string indexImgAdd,string featureClusterAdd);
    void set_minHessian(int minHessian);
    void set_dbSize(int size_DB);
    void set_indexImgAdd(string indexImgAdd);
    int getMatchedImgIndex();
    void showMatchImg();
private:
    int minHessian;
    string add_DB;  //database address
    int size_DB;    //database size, number of img
    string indexImgAdd; // index-img hash table address
    Mat despSRC, despDB; // descriptors of src and db img
    Mat dbImg,srcImg;
    vector<KeyPoint> keyPoints1, keyPoints2;
    vector<int>index_IMG;
    SurfDescriptorExtractor extractor;
    Mat featureCluster;
    string featureClusterAdd;
    Mat indices, dists;
    struct ImgFreq{
        int ImgIndex;
        int Freq;
    };
    int matchedImgIndex;
};

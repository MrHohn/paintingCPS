/*
    Author: Wuyang Zhang
    June 17.2015
 
*/
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include "ImgMatch.h"
 
ImgMatch::ImgMatch()
{
    minHessian = 3000;
 
}
 
 
ImgMatch::~ImgMatch()
{
}
//get all descriptor of images in database, save index_img to file;
void ImgMatch::init_DB(int size_DB,string add_DB, string indexImgAdd,string featureClusterAdd)
{
    printf("Start calculating the descriptors\n");
    this->set_dbSize(size_DB);
    SurfFeatureDetector detector(3000);
    for (int i = 1; i <= size_DB; i++){
        char  imgName[100];
         
        sprintf(imgName, "IMG_%d.jpg", i);
        string fileName;
        string fileAdd = add_DB;
        fileName = fileAdd.append( string(imgName));
        dbImg = imread(fileName);
        if (!dbImg.data){
            cout << "Can NOT find img in database!" << endl;
            return;
        }
        detector.detect(dbImg, keyPoints2);
        extractor.compute(dbImg, keyPoints2, despDB);
        featureCluster.push_back(despDB);
 
        for (int j = 0; j < despDB.rows; j++){
            index_IMG.push_back(i);
        }
         
    }
     
    /*
        \save index_IMG to file
    */
    ofstream outFile;
    outFile.open(indexImgAdd);
    for (auto &t : index_IMG){
        outFile << t << "\n";
    }
    /*
        \save feature cluster into file
    */
    cv::FileStorage storage(featureClusterAdd, cv::FileStorage::WRITE);
    storage << "index" << featureCluster;
    storage.release();
    printf("Finished.\n");
}
 
/*
    \read all descriptors of database img, and index-img map hashtable
*/
void ImgMatch:: init_matchImg(string indexImgAdd, string featureClusterAdd)
{
    printf("Start loading the database\n");
    ifstream inFile(indexImgAdd);
    int t;
    while (inFile >> t){
        index_IMG.push_back(t);
    }
    FileStorage storage(featureClusterAdd, FileStorage::READ);
    storage["index"] >> featureCluster;
    storage.release();
    printf("Finished\n");
}
 
void ImgMatch::matchImg(string srcImgAdd){
    Mat srcImg = imread(srcImgAdd);
    if (!srcImg.data){
        cout << "srcImg NOT found" << endl;
        return;
    }
    SurfFeatureDetector detector(minHessian);
    detector.detect(srcImg, keyPoints1);
    extractor.compute(srcImg, keyPoints1, despSRC);
    // const int despImgRows = index_IMG.size();
 
    flann::Index flannIndex(featureCluster, flann::KDTreeIndexParams(), cvflann::FLANN_DIST_EUCLIDEAN);
    const int knn = 2;
    flannIndex.knnSearch(despSRC, indices, dists, knn, flann::SearchParams());
 
    /*
        \find best match img
    */
    float nndrRatio = 0.6f;
    vector<ImgFreq>imgFreq;
    for (int i = 0; i < indices.rows; i++){
        if (dists.at<float>(i, 0) < dists.at<float>(i, 1) * nndrRatio){
 
            const int ImgNum = index_IMG.at(indices.at<int>(i, 0));
            bool findImgNum = false;
            for (auto &t : imgFreq){
                if (t.ImgIndex == ImgNum){
                    t.Freq++;
                    findImgNum = true;
                }
 
            }
            if (!findImgNum){
                ImgFreq nextIMG;
                nextIMG.ImgIndex = ImgNum;
                nextIMG.Freq = 1;
                imgFreq.push_back(nextIMG);
            }
        }
    }
    flannIndex.~Index();
 
    int maxFreq = 1;
    matchedImgIndex = 0;
    for (auto &t : imgFreq){
        if (t.Freq > maxFreq)
            maxFreq = t.Freq;
    }
    for (auto &t : imgFreq){ 
        if (t.Freq == maxFreq)
            matchedImgIndex = t.ImgIndex;
    }
}
 
void ImgMatch::set_minHessian(int m){
    if (m > 0){
        this->minHessian = m;
    }else
        cout << "error:minHessian should bigger than 0!" << endl;
}
 
void ImgMatch::set_dbSize(int s){
    if (s > 0){
        this->size_DB = s;
    }else
        cout << "error: Database Size should bigger than 0!" << endl;
}
int ImgMatch::getMatchedImgIndex(){
    return this->matchedImgIndex;
}
void ImgMatch::showMatchImg(){
    cout << matchedImgIndex;
 
    char filename[100];
    sprintf(filename, "./imgDB/IMG_%d.jpg", matchedImgIndex);
    Mat matchImg = imread(filename);
    imshow("matched img", matchImg);
    waitKey();
 
}

/*
    Author: Wuyang Zhang
    June 17.2015
 
*/
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <sys/time.h>
#include "ImgMatch.h"
 
ImgMatch::ImgMatch()
{
    minHessian = 3000;
 
}
 
ImgMatch::~ImgMatch()
{
}
//get all descriptor of images in database, save index_img to file;
void ImgMatch::init_DB(int size_DB,string add_DB, string indexImgAdd,string featureClusterAdd){
    printf("\nStart calculating the descriptors\n");
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
    printf("Finished.");
}
 
void init_infoDB(string add_DB){
    const int infoDB_Size = 20;
    string infoDB[infoDB_Size] = { "winlab logo", "microware", "file extinguisher", "winlab door", "winlab inner door", "sofa", "working desk", "trash box", "orbit machine", "car", "computer display" };
 
    for (int i = 0; i < sizeof(infoDB) / sizeof(*infoDB); i++){
        if (infoDB[i] != "\0"){
            char filename[100];
            sprintf(filename, "INFO_%d.txt", i);
 
            string fileName;
            string fileAdd = add_DB;
            fileName = fileAdd.append(string(filename));
 
            ofstream outFile;
            outFile.open(filename);
            outFile << infoDB[i];
        }
    }
}
 
/*
    \read all descriptors of database img, and index-img map hashtable
*/
void ImgMatch:: init_matchImg(string indexImgAdd, string featureClusterAdd,string imgInfoAdd){
    ifstream inFile(indexImgAdd);
    printf("\nStart loading the database\n");
    int t;
    while (inFile >> t){
        index_IMG.push_back(t);
    }
    FileStorage storage(featureClusterAdd, FileStorage::READ);
    storage["index"] >> featureCluster;
    storage.release();
    this->imgInfoAdd = imgInfoAdd;
    printf("Finished\n"); 
}
 
void ImgMatch::matchImg(string srcImgAdd){
    struct timeval tpstart,tpend;
    double timeuse;
    gettimeofday(&tpstart,NULL);

    srcImg = imread(srcImgAdd);
    if (!srcImg.data){
        cout << "srcImg NOT found" << endl;
        matchedImgIndex = 0;
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
    float nndrRatio = 0.8f;
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
 
    //display possible matched image index
    /*
    for (auto&t : imgFreq){
    cout << "possible imatch " << t.ImgIndex << " times is: " << t.Freq << endl;
    }
    */
     
 
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

    gettimeofday(&tpend,NULL);
    timeuse=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec;// notice, should include both s and us
    // printf("used time:%fus\n",timeuse);
    printf("used time:%fms\n",timeuse / 1000);
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

vector<float> ImgMatch::calLocation(){
    char filename[100];
    sprintf(filename, "./imgDB/IMG_%d.jpg", matchedImgIndex);
    Mat matchImg = imread(filename);
 
    SurfFeatureDetector detector(minHessian);
    detector.detect(matchImg, keyPoints2);
    extractor.compute(matchImg, keyPoints2, despDB);
    FlannBasedMatcher matcher;
    vector<DMatch>matches;
    matcher.match(despDB, despSRC, matches);
 
 
    double max_dist = 0; double min_dist = 100;
 
    //-- Quick calculation of max and min distances between keypoints
    for (int i = 0; i < despDB.rows; i++)
    {
        double dist = matches[i].distance;
        if (dist < min_dist) min_dist = dist;
        if (dist > max_dist) max_dist = dist;
    }
 
    //-- Draw only "good" matches (i.e. whose distance is less than 3*min_dist )
    std::vector< DMatch > good_matches;
 
    for (int i = 0; i < despDB.rows; i++)
    {
        if (matches[i].distance < 3 * min_dist)
        {
            good_matches.push_back(matches[i]);
        }
    }
 
    Mat img_matches = srcImg;
     
    //-- Localize the object
    vector<Point2f> obj;
    vector<Point2f> scene;
 
    for (int i = 0; i < good_matches.size(); i++)
    {
        //-- Get the keypoints from the good matches
        obj.push_back(keyPoints2[good_matches[i].queryIdx].pt);
        scene.push_back(keyPoints1[good_matches[i].trainIdx].pt);
    }
 
    Mat H = findHomography(obj, scene, CV_RANSAC);
 
    //-- Get the corners from the matchImg ( the object to be "detected" )
    vector<Point2f> obj_corners(4);
    obj_corners[0] = cvPoint(0, 0); obj_corners[1] = cvPoint(matchImg.cols, 0);
    obj_corners[2] = cvPoint(matchImg.cols, matchImg.rows); obj_corners[3] = cvPoint(0, matchImg.rows);
    vector<Point2f> scene_corners(4);
 
    perspectiveTransform(obj_corners, scene_corners, H);
 
    /*
        draw rectangle to localize the object( draw four lines on given points)
        four points postions & type: float
        A:scene_corner[0].x, scene_corner[0].y
        B:scene_corner[1].x, scene_corner[1].y
        C:scene_corner[2].x, scene_corner[2].y
        D:scene_corner[3].x, scene_corner[3].y
 
        draw circle to localize the object ( draw circle based on center and radius
        center: scene_corners[0].x + (scene_corners[1].x - scene_corners[0].x) / 2, scene_corners[0].y + (scene_corners[3].y - scene_corners[0].y) / 2
        radius: sqrt(((scene_corners[1].x - scene_corners[0].x) / 2)*( (scene_corners[1].x - scene_corners[0].x) / 2) + ((scene_corners[3].y - scene_corners[0].y) / 2 )* ((scene_corners[3].y - scene_corners[0].y) / 2));
    */
 
    /*
    line(img_matches, scene_corners[0] , scene_corners[1] , Scalar(0, 0, 0), 2);
    line(img_matches, scene_corners[1] , scene_corners[2] , Scalar(0, 0, 0), 2);
    line(img_matches, scene_corners[2] , scene_corners[3] , Scalar(0, 0, 0), 2);
    line(img_matches, scene_corners[3] , scene_corners[0] , Scalar(0, 0, 0), 2);
 
    circle(img_matches, center, radius, Scalar(0, 0, 0), 2, 0);
    */

    if (matchedLocation.size() >= 8)
    {
        matchedLocation.clear();
    }
    matchedLocation.push_back(scene_corners[0].x);
    matchedLocation.push_back(scene_corners[0].y);
    matchedLocation.push_back(scene_corners[1].x);
    matchedLocation.push_back(scene_corners[1].y);
    matchedLocation.push_back(scene_corners[2].x);
    matchedLocation.push_back(scene_corners[2].y);
    matchedLocation.push_back(scene_corners[3].x);
    matchedLocation.push_back(scene_corners[3].y);
    matchedLocation.push_back(scene_corners[0].x + (scene_corners[1].x - scene_corners[0].x) / 2);
    matchedLocation.push_back(scene_corners[0].y + (scene_corners[3].y - scene_corners[0].y) / 2);
    matchedLocation.push_back(sqrt(((scene_corners[1].x - scene_corners[0].x) / 2)*((scene_corners[1].x - scene_corners[0].x) / 2) + ((scene_corners[3].y - scene_corners[0].y) / 2)* ((scene_corners[3].y - scene_corners[0].y) / 2)));
 
    return matchedLocation;
     
}
 
void ImgMatch::locateDrawRect(vector<float> matchedLocation){
 
    line(srcImg, cvPoint(matchedLocation.at(0), matchedLocation.at(1)), cvPoint(matchedLocation.at(2), matchedLocation.at(3)), Scalar(0, 0, 0), 2);
    line(srcImg, cvPoint(matchedLocation.at(2), matchedLocation.at(3)), cvPoint(matchedLocation.at(4), matchedLocation.at(5)), Scalar(0, 0, 0), 2);
    line(srcImg, cvPoint(matchedLocation.at(4), matchedLocation.at(5)), cvPoint(matchedLocation.at(6), matchedLocation.at(7)), Scalar(0, 0, 0), 2);
    line(srcImg, cvPoint(matchedLocation.at(6), matchedLocation.at(7)), cvPoint(matchedLocation.at(0), matchedLocation.at(1)), Scalar(0, 0, 0), 2);
    imshow("matched image", srcImg);
    moveWindow("matched image", 600, 350 );
    waitKey();
}
 
void ImgMatch::locateDrawCirle(vector<float> matchedLocation){
    circle(srcImg, cvPoint(matchedLocation.at(8), matchedLocation.at(9)), matchedLocation.at(10), Scalar(0, 0, 0), 2, 0);
    imshow("matched image", srcImg);
    waitKey();
}
 
void ImgMatch::getMatchedImgInfo(){
    string add = imgInfoAdd;
    char infoAdd[100];
    int index =  ceil(matchedImgIndex / double(2));
    cout << "matchedImgIndex is: " << matchedImgIndex << endl;
    cout << "info index is: " << index;
    sprintf(infoAdd, "INFO_%d.txt", index);
    add.append(string(infoAdd));
    ifstream inFile(add);
    string info;
    while (inFile >> info){
        cout << info;
    }
 
}

/*
    Author: Wuyang Zhang
    June 17.2015
    ImgMatch.cpp
*/

#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <sys/time.h>
#include "ImgMatch.h"
int deb = 0;
string ImgMatch::add_DB;  //database address
int ImgMatch::size_DB;    //database size, number of img
string ImgMatch::indexImgAdd; // index-img hash table address
Mat ImgMatch::despDB; // descriptors of src and db img
Mat ImgMatch::dbImg;
Mat ImgMatch::featureCluster;
vector<KeyPoint> ImgMatch::dbKeyPoints;
string ImgMatch::featureClusterAdd;
string ImgMatch::imgInfoAdd;
vector<int> ImgMatch::index_IMG;
int ImgMatch::minHessian = 1000;
flann::Index* ImgMatch::flannIndex;
ImgMatch::ImgMatch()
{
    minHessian = 50;
 
}
 
ImgMatch::~ImgMatch()
{
}
//get all descriptor of images in database, save index_img to file;
void ImgMatch::init_DB(int size_DB,string add_DB, string indexImgAdd,string featureClusterAdd){
    printf("\nStart calculating the descriptors\n");
    set_dbSize(size_DB);
    SurfFeatureDetector extractor;
    SurfFeatureDetector detector(minHessian);
    /*
        \loop calculate descriptors of iamges in database
    */
    for (int i = 1; i <= size_DB; i++){
        char  imgName[100]; 
         
        sprintf(imgName, "%d.jpg", i);
        string fileName;
        string fileAdd = add_DB;
        fileName = fileAdd.append( string(imgName));
        dbImg = imread(fileName);

        if (!dbImg.data){
            cout << "Can NOT find img in database!" << endl;
            return;
        }

        detector.detect(dbImg, dbKeyPoints);
        extractor.compute(dbImg, dbKeyPoints, despDB);
    /*
        \put all descripotrs into featureCLuster 
    */

        featureCluster.push_back(despDB);
 
    /*
        \ descriptor index -> image index 
    */
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
    printf("Finished initiate image database!!!");
}
 
void init_infoDB(string add_DB){
    const int infoDB_Size = 20;
    string infoDB[infoDB_Size] = { "winlab logo", "microware", "file extinguisher", "winlab door", "winlab inner door", "sofa", "working desk", "trash box", "orbit machine", "car", "computer display" };
 
    for (uint i = 0; i < sizeof(infoDB) / sizeof(*infoDB); i++){
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
    \read all descriptors of database img, and descripotr index-img index map hashtable
*/
long int ImgMatch:: init_matchImg(string indexImgAdd, string featureClusterAdd,string imgInfoAdd){
    ifstream inFile(indexImgAdd);
    printf("\nStart loading the database\n");
    int t;
    while (inFile >> t){
        index_IMG.push_back(t);
    }
    FileStorage storage(featureClusterAdd, FileStorage::READ);
    storage["index"] >> featureCluster;
    storage.release();
    imgInfoAdd = imgInfoAdd;
    flannIndex = new flann::Index(featureCluster,flann::KDTreeIndexParams(),cvflann::FLANN_DIST_EUCLIDEAN);
    printf("Finished initiate work before image matching\n");
    
    //32 system
    //return ( int )( featureCluster );
    return (long int )( flannIndex );
 
}
 
string ImgMatch::matchImg(string srcImgAdd,long int initiatePointer){
    struct timeval tpstart,tpend;
  
    double timeuse = 0;
    double timeuseDesp;
    double timeuseKnnSearch;
    
    gettimeofday(&tpstart,NULL);

    flannIndex = ( flann::Index * ) initiatePointer;
    srcImg = imread(srcImgAdd);
    if (!srcImg.data){
      printf( "source image NOT found\n");
        matchedImgIndex = 0;
        return NULL;
    }
    // -- default matched index is 0, means no matching image is found
     matchedImgIndex = 0;

     /*
        \calculate input image's descriptor
    */
     if(deb){
       cout<<"\n[INFO] Read source image "<<srcImgAdd<<" successfully! Start matching"<<endl;
       

    gettimeofday(&tpend,NULL);
    timeuse=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec;
    printf("[TIMECOSTING] initiating and reading srcIMG used time :%f ms\n",timeuse / 1000);
     }


    SurfFeatureDetector detector(minHessian);
    SurfDescriptorExtractor extractor;
    detector.detect(srcImg, keyPoints1);
    extractor.compute(srcImg, keyPoints1, despSRC);


    if(deb){
    gettimeofday(&tpend,NULL);
    timeuseDesp=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec;

    printf("[TIMECOSTING] extracting descriptors used time :%f ms\n",(timeuseDesp-timeuse) / 1000);
    }

   
    if(despSRC.empty()){
      if(deb)
    printf("source iamge is empty, quit");
      return NULL;    
    }
   
    
   const int knn = 2; 
   flannIndex->knnSearch(despSRC, indices, dists, knn, flann::SearchParams());
   
   if(deb){
   gettimeofday(&tpend,NULL);
    timeuseKnnSearch=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec;
    printf("[TIMECOSTING] Knnsearch used time: %f ms\n",(timeuseKnnSearch-timeuseDesp) / 1000);
   }
   
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
   
    //display possible matched image index
    /*
    for (auto&t : imgFreq){
    cout << "possible imatch " << t.ImgIndex << " times is: " << t.Freq << endl;
    }
    */
     
 
    int maxFreq = 1;
    for (auto &t : imgFreq){
        if (t.Freq > maxFreq)
            maxFreq = t.Freq;
    }
  
    
    /*
    \if max matched times is smaller than 3, it fails to find a matched object
    */
    // if(deb){
    //   printf("max matched time is%d\n",maxFreq);
    // }
    if (maxFreq < 3){
        // cout << "Can not find matched ojbect" << endl;
        return NULL;
    }
    for (auto &t : imgFreq){ 
        if (t.Freq == maxFreq)
            matchedImgIndex = t.ImgIndex;
    }
    if( deb ){

      gettimeofday(&tpend,NULL);
      timeuse=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec;
      
      printf("[TIMECOSTING] total used time:%fms\n",timeuse / 1000);
    }

    if( deb ){
      printf( "[INFO] Matched image index is%d\n",matchedImgIndex);
    }

      gettimeofday(&tpend,NULL);
      timeuse=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec;
    struct timeval tp;
    gettimeofday( &tp,NULL );
    double ms = tp.tv_sec * 1000 + tp.tv_usec/1000;
    string output = to_string( timeuse / 1000 );
    output.append( " ," ).append( to_string( ms )).append( " ," ).append( to_string(matchedImgIndex) ).append( " ," ).append( srcImgAdd );
    return output;
}

string ImgMatch::matchImg(char* img, int size){
    struct timeval tpstart,tpend;
  
    double timeuse = 0;
    double timeuseDesp;
    double timeuseKnnSearch;
    
    gettimeofday(&tpstart,NULL);

    vector<uchar> jpgbytes;
    for (int i = 0; i < size; ++i) {
        jpgbytes.push_back(img[i]);
    }

    srcImg = imdecode(jpgbytes, CV_LOAD_IMAGE_COLOR);
    if (!srcImg.data){
      printf( "source image NOT found\n");
        matchedImgIndex = 0;
        return NULL;
    }
    // -- default matched index is 0, means no matching image is found
     matchedImgIndex = 0;

     /*
        \calculate input image's descriptor
    */
     if(deb){
       // cout<<"\n[INFO] Read source image "<<srcImgAdd<<" successfully! Start matching"<<endl;
       

    gettimeofday(&tpend,NULL);
    timeuse=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec;
    printf("[TIMECOSTING] initiating and reading srcIMG used time :%f ms\n",timeuse / 1000);
     }


    SurfFeatureDetector detector(minHessian);
    SurfDescriptorExtractor extractor;
    detector.detect(srcImg, keyPoints1);
    extractor.compute(srcImg, keyPoints1, despSRC);


    if(deb){
    gettimeofday(&tpend,NULL);
    timeuseDesp=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec;

    printf("[TIMECOSTING] extracting descriptors used time :%f ms\n",(timeuseDesp-timeuse) / 1000);
    }

   
    if(despSRC.empty()){
      if(deb)
    printf("source iamge is empty, quit");
      return NULL;    
    }
   
    
   const int knn = 2; 
   flannIndex->knnSearch(despSRC, indices, dists, knn, flann::SearchParams());
   
   if(deb){
   gettimeofday(&tpend,NULL);
    timeuseKnnSearch=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec;
    printf("[TIMECOSTING] Knnsearch used time: %f ms\n",(timeuseKnnSearch-timeuseDesp) / 1000);
   }
   
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
   
    //display possible matched image index
    /*
    for (auto&t : imgFreq){
    cout << "possible imatch " << t.ImgIndex << " times is: " << t.Freq << endl;
    }
    */
     
 
    int maxFreq = 1;
    for (auto &t : imgFreq){
        if (t.Freq > maxFreq)
            maxFreq = t.Freq;
    }
  
    
    /*
    \if max matched times is smaller than 3, it fails to find a matched object
    */
    // if(deb){
    //   printf("max matched time is%d\n",maxFreq);
    // }
    if (maxFreq < 3){
        // cout << "Can not find matched ojbect" << endl;
        return NULL;
    }
    for (auto &t : imgFreq){ 
        if (t.Freq == maxFreq)
            matchedImgIndex = t.ImgIndex;
    }
    if( deb ){

      gettimeofday(&tpend,NULL);
      timeuse=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec;
      
      printf("[TIMECOSTING] total used time:%fms\n",timeuse / 1000);
    }

    if( deb ){
      printf( "[INFO] Matched image index is%d\n",matchedImgIndex);
    }

      gettimeofday(&tpend,NULL);
      timeuse=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec;
    struct timeval tp;
    gettimeofday( &tp,NULL );
    double ms = tp.tv_sec * 1000 + tp.tv_usec/1000;
    string output = to_string( timeuse / 1000 );
    output.append( "," ).append(to_string( ms )).append( "," ).append( to_string(matchedImgIndex) ).append( "," );
    return output;
}

void ImgMatch::set_minHessian(int m){
    if (m > 0){
        this->minHessian = m;
    }else{
      
    }
//         cout << "error:minHessian should bigger than 0!" << endl;
}
 
void ImgMatch::set_dbSize(int s){
    if (s > 0){
        size_DB = s;
    }else
        cout << "error: Database Size should bigger than 0!" << endl;
}

int ImgMatch::getMatchedImgIndex(){
    return this->matchedImgIndex;
}

vector<float> ImgMatch::calLocation(){
  
  if( matchedImgIndex == 0)
    return matchedLocation;

    char filename[100];
    // sprintf(filename, "./imgDB/IMG_%d.jpg", matchedImgIndex);
    sprintf(filename, "./MET_IMG/%d.jpg", matchedImgIndex);
    Mat matchImg = imread(filename);
 
    SurfFeatureDetector detector(minHessian);
    SurfDescriptorExtractor extractor;
    detector.detect(matchImg, keyPoints2);
    extractor.compute(matchImg, keyPoints2, despDB);

    FlannBasedMatcher matcher;
    vector<DMatch>matches;
    // -- Rematch matched image in database with source image to check their matched descriptor
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
    /*
        \test size of good_matches, if it is smaller than 4, then it fails to draw an outline of a mathed object
    */
    if(deb)
      printf("number of good match is%zu\n",good_matches.size());
    if (good_matches.size() < 4){
        // cout << "not find matched object" << endl;
        return matchedLocation;
    }
 
    Mat img_matches = srcImg;
     
    //-- Localize the object
    vector<Point2f> obj;
    vector<Point2f> scene;
 
    for (uint i = 0; i < good_matches.size(); i++)
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
    /*
    \when matchedImgIndex equals to 0, it fails to find a object
    */
    if (deb)
        printf("size of Locating Information%zu",matchedLocation.size());
    if (matchedImgIndex == 0){
        return;
    }

    line(srcImg, cvPoint(matchedLocation.at(0), matchedLocation.at(1)), cvPoint(matchedLocation.at(2), matchedLocation.at(3)), Scalar(0, 0, 0), 2);
    line(srcImg, cvPoint(matchedLocation.at(2), matchedLocation.at(3)), cvPoint(matchedLocation.at(4), matchedLocation.at(5)), Scalar(0, 0, 0), 2);
    line(srcImg, cvPoint(matchedLocation.at(4), matchedLocation.at(5)), cvPoint(matchedLocation.at(6), matchedLocation.at(7)), Scalar(0, 0, 0), 2);
    line(srcImg, cvPoint(matchedLocation.at(6), matchedLocation.at(7)), cvPoint(matchedLocation.at(0), matchedLocation.at(1)), Scalar(0, 0, 0), 2);
    imshow("matched image", srcImg);
    moveWindow("matched image", 600, 350 );
    waitKey();
    clearLocation();
}
 
void ImgMatch::locateDrawCirle(vector<float> matchedLocation){
    circle(srcImg, cvPoint(matchedLocation.at(8), matchedLocation.at(9)), matchedLocation.at(10), Scalar(0, 0, 0), 2, 0);
    imshow("matched image", srcImg);
    waitKey();
}

void ImgMatch::clearLocation(){
    matchedLocation.clear();
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

string ImgMatch::getInfo() {
    string line;
    string info;
    char file_name[100];
    sprintf(file_name, "MET_INFO/%d.txt", matchedImgIndex);

    ifstream info_file(file_name);
    if (info_file)
    {
        while (getline(info_file, line)) {
            info = line;
            // cout << line << endl;
        }
    }
    info_file.close();

    return info;
}

void ImgMatch::releaseInitResource(long int initiatePointer  ){
  flannIndex = ( flann::Index * ) initiatePointer;
  delete flannIndex;
}

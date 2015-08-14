   
/* JniImageMatching: encapsulate cpp image processing code by Jni               
   Author: Wuyang Zhang                                                         
   Data: Aug 3 2015                                                             
   Version: 1.0                                                                 
*/

#include "JniImageMatching.h"
#include "ImgMatch.h"

JNIEXPORT void JNICALL Java_storm_winlab_cps_JniImageMatching_initiate_1database( JNIEnv *env,jclass clas, jint totalImageNumber, jstring image_database_addr, jstring indexTable_addr, jstring imgIndex_yml_addr ){
    const char *imgDbAddr = env->GetStringUTFChars(image_database_addr,0);
    const char *indexTableAddr = env->GetStringUTFChars(indexTable_addr,0);
    const char *imgIndexYmlAddr = env->GetStringUTFChars(imgIndex_yml_addr,0);
    
    ImgMatch::init_DB(totalImageNumber,imgDbAddr,indexTableAddr,imgIndexYmlAddr );


    env->ReleaseStringUTFChars(image_database_addr,imgDbAddr);
    env->ReleaseStringUTFChars(indexTable_addr, indexTableAddr);
    env->ReleaseStringUTFChars(imgIndex_yml_addr, imgIndexYmlAddr);

}

JNIEXPORT jlong JNICALL Java_storm_winlab_cps_JniImageMatching_initiate_1imageMatching( JNIEnv *env, jclass cls, jstring indexImgTable_addr, jstring imgIndex_yml_addr, jstring info_database_addr ){
  
    const char *indexImgTableAddr = env->GetStringUTFChars(indexImgTable_addr,0);
    const char *imgIndexYmlAddr = env->GetStringUTFChars(imgIndex_yml_addr,0);
    const char *infoDbAddr = env->GetStringUTFChars(info_database_addr,0);
    
    long int featureClusterAddr =   ImgMatch::init_matchImg(indexImgTableAddr,imgIndexYmlAddr,infoDbAddr);


    env->ReleaseStringUTFChars(indexImgTable_addr,indexImgTableAddr);
    env->ReleaseStringUTFChars(imgIndex_yml_addr, imgIndexYmlAddr);
    env->ReleaseStringUTFChars(info_database_addr, infoDbAddr);

    return featureClusterAddr;


}

JNIEXPORT jstring JNICALL Java_storm_winlab_cps_JniImageMatching_matchingIndex( JNIEnv *env, jclass cla, jstring fileName, jlong initiatePointer){

  const char *str = env->GetStringUTFChars(fileName,0);
  long int iniPointer = ( long int )initiatePointer;

  ImgMatch img;
  img.matchImg( str, iniPointer );

  //release the string when done to avoid memory leak
  env->ReleaseStringUTFChars(fileName,str);
  return env->NewStringUTF("Finished matching");

}

JNIEXPORT void JNICALL Java_storm_winlab_cps_JniImageMatching_releaseInitResource ( JNIEnv *, jclass, jlong initiatePointer){
  long int iniPointer = ( long int )initiatePointer;
  ImgMatch::releaseInitResource(iniPointer );
}

/* JniImageMatching: encapsulate cpp image processing code by Jni

   Author: Wuyang Zhang

   Data: Aug 3 2015

   Version: 1.0
*/
package storm.winlab.cps;
public class JniImageMatching{
    //define image full address

    public static String imgFolderAddr = "/home/hadoop/worksapce/storm/jopencv-copy/src/jvm/storm/winlab/cps/MET_IMG/";
    public static String imgPrefix = "IMG_";
    public static String imgFormat = ".jpg";
    
    public static String indexImgTableAddr = "/home/hadoop/worksapce/storm/jopencv-copy/src/jvm/storm/winlab/cps/indexImgTable";
    public static String imgIndexYmlAddr = "/home/hadoop/worksapce/storm/jopencv-copy/src/jvm/storm/winlab/cps/ImgIndex.yml";
    public static String infoDatabaseAddr = "/home/hadoop/worksapce/storm/jopencv-copy/src/jvm/storm/winlab/cps/MET_INFO/";



    public static int totalInitiateImgNum = 20;
    public static int totalMatchNum = 20;

    // read native library
    static{
	try{
	    System.loadLibrary("JniImageMatching");
	}catch (UnsatisfiedLinkError e){
	    System.out.println("could not find library JniImageMatching");
	}
    }

    //native method

    public static native void initiate_database(int totalImageNumber, String image_database_addr, String indexTable_addr, String imgIndex_yml_addr);

    public static native long initiate_imageMatching(String indexImgTable_addr, String imgIndex_yml_addr, String info_database_addr);

    public static native String matchingIndex(String srcImgAddr, long initiatePointer);

    public static native void releaseInitResource(long initiatePointer);

    public static String combineName(String imgFolderAddr, String imgPrefix, int index, String imgFormat){
	StringBuilder sb = new StringBuilder();

	sb.append(imgFolderAddr);
	sb.append(imgPrefix);
	sb.append(index);
	sb.append(imgFormat);
	String srcImgAddr = sb.toString();
	    
	return srcImgAddr;
    }
    
    public static void main(String args[]){
	//initiate database
	initiate_database(totalInitiateImgNum,imgFolderAddr,indexImgTableAddr,imgIndexYmlAddr);


	//initiate image matching, return initiate parameter memory address on the heap
	long initiatePointer = initiate_imageMatching (indexImgTableAddr,imgIndexYmlAddr,infoDatabaseAddr);


	for(int i = 1; i < totalMatchNum; i++){
	    //build requested images' name
	    String srcImgAddr = combineName(imgFolderAddr, imgPrefix, i, imgFormat);
	    System.out.println(srcImgAddr);
	   String result = matchingIndex(srcImgAddr,initiatePointer);
	   System.out.println("[ Running Reulst:]" + result);
	}

	//release initiate parameter memory resource!!
	releaseInitResource(initiatePointer);
    }
    
}

/* JniImageMatching: encapsulate cpp image processing code by Jni

   Author: Wuyang Zhang, Zihong Zheng(zzhonzi@gmail.com)

   Data: Aug 3 2015

   Version: 1.0
*/
package storm.winlab.cps;

import java.io.*;

public class JniImageMatching{
    //define image full address

    public static String imgFolderAddr = "/demo/img/";
    public static String imgPrefix = "";
    public static String imgFormat = ".jpg";
    
    public static String indexImgTableAddr = "/demo/indexImgTable";
    public static String imgIndexYmlAddr = "/demo/ImgIndex.yml";
    public static String infoDatabaseAddr = "/demo/info/";



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

    public static native String matchingIndex(byte[] img, int size);

    // public static native String getInfo();

    // public static native String getLocation();

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

	// for(int i = 1; i < totalMatchNum; i++){
	//     //build requested images' name
	//     String srcImgAddr = combineName(imgFolderAddr, imgPrefix, i, imgFormat);
	//     System.out.println(srcImgAddr);
	//    String result = matchingIndex(srcImgAddr,initiatePointer);
	//    System.out.println("[ Running Reulst:]" + result);
	// }

    try {
        // now read the img file
        String filePath = "/demo/img/1.jpg";
        File frame = new File(filePath);
        FileInputStream readFile = new FileInputStream(filePath);                
        int size = (int)frame.length();
        byte[] img = new byte[size];
        int length = readFile.read(img, 0, size);
        if (length != size) {
            System.out.println("Read image error!");
            System.exit(1);
        }

        String result = matchingIndex(img, size);
        System.out.println("[ Running Reulst:]" + result);
    }
    catch (Exception e) {
        e.printStackTrace();
    }

	//release initiate parameter memory resource!!
	releaseInitResource(initiatePointer);
    }
    
}

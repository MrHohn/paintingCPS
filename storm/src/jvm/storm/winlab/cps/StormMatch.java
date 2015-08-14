/*
	StormMatch.java: design spout and bolts running image matching on Storm
                                                                                      
	Author: Wuyang Zhang, Zihong Zheng(zzhonzi@gmail.com)

	Data: Aug 8 2015

	Version: 1.0
*/

package storm.winlab.cps;

import backtype.storm.Config;
import backtype.storm.LocalCluster;
import backtype.storm.StormSubmitter;
import backtype.storm.task.OutputCollector;
import backtype.storm.spout.SpoutOutputCollector;
import backtype.storm.task.TopologyContext;
import backtype.storm.testing.TestWordSpout;
import backtype.storm.topology.OutputFieldsDeclarer;
import backtype.storm.topology.TopologyBuilder;
import backtype.storm.topology.base.BaseRichBolt;
import backtype.storm.topology.base.BaseRichSpout;
import backtype.storm.tuple.Fields;
import backtype.storm.tuple.Tuple;
import backtype.storm.tuple.Values;
import backtype.storm.utils.Utils;

import java.util.Map;
import java.util.Random;
import java.io.*;
import java.io.File;
import java.io.Writer;
import java.io.OutputStreamWriter;
import java.net.*;

import storm.winlab.cps.JniImageMatching;

public class StormMatch {
    private static long initiatePointer;
    private static int num = 15;
    private static int index = 1;
    private static boolean found = false;
    private static int monitorPort = 9876;
    private static int spoutFinderPort = 9877;
    private static int requestPort = 9878;
    private static boolean monitor = true;

    public static class RequestedImageSpout extends BaseRichSpout {
	
		SpoutOutputCollector _collector;
		Random _rand;

		@Override
		public void open(Map conf, TopologyContext context, SpoutOutputCollector collector){
		    _collector = collector;
		    // _rand = new Random();
		}

		@Override	
		public void nextTuple(){

			try {
				DatagramSocket clientSocket = new DatagramSocket();
				// InetAddress serverIP = InetAddress.getByName("10.0.0.200");
				InetAddress serverIP = InetAddress.getByName("localhost");
				//send the spout signal
				String buffer = "spout";
				byte[] sendData = new byte[1024];
				sendData = buffer.getBytes();
				DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, serverIP, monitorPort);
				if (monitor) clientSocket.send(sendPacket);
				if (!found) {
					// tell the SpoutFinder where the spout is
					buffer = "here";
					sendData = new byte[1024];
					sendData = buffer.getBytes();
					sendPacket = new DatagramPacket(sendData, sendData.length, serverIP, spoutFinderPort);
					clientSocket.send(sendPacket);
					found = true;
				}

				// now read the img file
				String filePath = "/home/hadoop/worksapce/opencv-CPS/storm/src/jvm/storm/winlab/cps/MET_IMG/IMG_1.jpg";
				File frame = new File(filePath);
				FileInputStream readFile = new FileInputStream(filePath);                
				int size = (int)frame.length();
				byte[] img = new byte[size];
				int length = readFile.read(img, 0, size);
				if (length != size) {
				    System.out.println("Read image error!");
				    System.exit(1);
				}

				_collector.emit(new Values(img, index));
				++index;

				Utils.sleep(200);

				--num;
				if (num == 0) {
					Utils.sleep(10000000);
				}

			}
			catch (Exception e) {
				e.printStackTrace();
			}
		}

		@Override
		public void declareOutputFields(OutputFieldsDeclarer declarer) {
		    declarer.declare(new Fields("img", "index"));
		}
    }


    public static class ImgMatchingBolt extends BaseRichBolt {
		OutputCollector _collector;

		@Override
		public void prepare(Map map, TopologyContext topologyContext, OutputCollector collector) {
		    _collector = collector;
		    initiatePointer = JniImageMatching.initiate_imageMatching (JniImageMatching.indexImgTableAddr,JniImageMatching.imgIndexYmlAddr,JniImageMatching.infoDatabaseAddr);

		    if (monitor) {
			    try {
					DatagramSocket clientSocket = new DatagramSocket();
					// InetAddress serverIP = InetAddress.getByName("10.0.0.200");
					InetAddress serverIP = InetAddress.getByName("localhost");
					//send the prepare signal
					String buffer = "prepare";
					byte[] sendData = new byte[1024];
					sendData = buffer.getBytes();
					DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, serverIP, 9876);
					clientSocket.send(sendPacket);
			    }
			    catch (Exception e) {
			    	e.printStackTrace();
			    }
		    }

		}

		@Override
		public void execute(Tuple tuple) {
			try {
				InetAddress serverIP = InetAddress.getByName("localhost");
				DatagramSocket clientSocket = new DatagramSocket();;
				DatagramPacket sendPacket;
				byte[] sendData;

				if (monitor) {	
					// send the start signal
					sendData = new byte[1024];
					String buf = "start";
					sendData = buf.getBytes();
					sendPacket = new DatagramPacket(sendData, sendData.length, serverIP, 9876);
					clientSocket.send(sendPacket);
				}

				// store the image first
				byte[] img = tuple.getBinaryByField("img");
				int index = tuple.getIntegerByField("index");
				String imgPath = "/home/hadoop/worksapce/opencv-CPS/storm/tmp/";
				imgPath = imgPath + index + ".jpg";
				FileOutputStream saveImg = new FileOutputStream(imgPath);
				saveImg.write(img);
				saveImg.close();

				// start to match
				String result = JniImageMatching.matchingIndex(imgPath,initiatePointer);

				if (monitor) {
					// send the finish signal
					sendData = new byte[1024];
					String delims = "[,]";
					String[] tokens = result.split(delims);
					sendData = tokens[2].getBytes();
					sendPacket = new DatagramPacket(sendData, sendData.length, serverIP, 9876);
					clientSocket.send(sendPacket);
				}

				// delete the tmp img
				File tmpImg = new File(imgPath);
				tmpImg.delete();
			}
		    catch (Exception e) {
		    	e.printStackTrace();
		    }

		    
		}

		@Override
		public void declareOutputFields(OutputFieldsDeclarer declarer){
		    declarer.declare(new Fields ("matching requested images"));
		}
    }


    public static void main(String args[]) throws Exception{
    	// create the topology
        TopologyBuilder builder = new TopologyBuilder();
        //attach the randomSentence to the topology -- parallelism of 1
        builder.setSpout("image-received",new RequestedImageSpout(), 1);
        //attch the exclamationBolt to the topology -- parallelism of 3
        builder.setBolt("image-matching",new ImgMatchingBolt(), 3).shuffleGrouping("image-received");

        Config conf = new Config();

        if(args != null && args.length > 0){
            // Run it on a live storm cluser
            conf.setNumWorkers(3);
            StormSubmitter.submitTopology(args[0],conf,builder.createTopology());
        } else {
            // Run it on a simulated local cluster
            LocalCluster cluster = new LocalCluster();
            //topo name, configuration, builder.topo
            cluster.submitTopology("stormImageMatch",conf, builder.createTopology());

            //let it run 30 seconds;                        
            Thread.sleep(30000);
	   		JniImageMatching.releaseInitResource(initiatePointer);
            cluster.killTopology("stormImageMatch");
            cluster.shutdown();
        }
    }
}

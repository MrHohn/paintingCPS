/*
	StormMatch.java: design KafkaSpout and bolts running image matching on Storm
                                                                                      
	Author: Wuyang Zhang, Zihong Zheng(zzhonzi@gmail.com)

	Data: Nov. 2015

	Version: 1.1
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

import storm.kafka.BrokerHosts;
import storm.kafka.KafkaSpout;
import storm.kafka.SpoutConfig;
import storm.kafka.ZkHosts;
import backtype.storm.spout.RawMultiScheme;
import java.util.Arrays;

public class StormMatch {
    // private static long initiatePointer;
    private static int num = 100;
    private static int index = 1;
    private static boolean found = false;
    private static int monitorPort = 9876;
    private static int spoutFinderPort = 9877;
    private static int spoutPort = 9878;
    private static int serverPort = 9879;
    private static boolean monitor = true;
    private static String monitorHost = "localhost";
    private static boolean testMode = false;
    private static boolean debug = false;

    public static class ImgMatchingBolt extends BaseRichBolt {
		OutputCollector _collector;

		@Override
		public void prepare(Map map, TopologyContext topologyContext, OutputCollector collector) {
		    _collector = collector;
		    JniImageMatching.initiate_imageMatching (JniImageMatching.indexImgTableAddr,JniImageMatching.imgIndexYmlAddr,JniImageMatching.infoDatabaseAddr);

		    if (monitor) {
			    try {
					DatagramSocket clientSocket = new DatagramSocket();
					InetAddress serverIP = InetAddress.getByName(monitorHost);
					//send the prepare signal
					String buffer = "prepare";
					byte[] sendData = new byte[1024];
					sendData = buffer.getBytes();
					DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, serverIP, monitorPort);
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
				InetAddress serverIP = InetAddress.getByName(monitorHost);
				DatagramSocket clientSocket = new DatagramSocket();;
				DatagramPacket sendPacket;
				byte[] sendData;

				if (monitor) {	
					// send the start signal
					sendData = new byte[1024];
					String buf = "start";
					sendData = buf.getBytes();
					sendPacket = new DatagramPacket(sendData, sendData.length, serverIP, monitorPort);
					clientSocket.send(sendPacket);
				}

				// store the image first
				byte[] img = tuple.getBinary(0);
				// byte[] img = tuple.getBinaryByField("img");
				// int index = tuple.getIntegerByField("index");

				// start to match
				String result = JniImageMatching.matchingIndex(img, img.length);

				// send the finish signal
				sendData = new byte[1024];
				// String delims = "[,]";
				// String[] tokens = result.split(delims);
				// sendData = tokens[0].getBytes();
				sendData = result.getBytes();
				// send back the result to server
				sendPacket = new DatagramPacket(sendData, sendData.length, serverIP, serverPort);
				clientSocket.send(sendPacket);
				if (monitor) {
					String delims = "[,]";
					String[] tokens = result.split(delims);
					sendData = tokens[0].getBytes();
					// send back the result to monitor
					sendPacket = new DatagramPacket(sendData, sendData.length, serverIP, monitorPort);
					clientSocket.send(sendPacket);
				}
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


    public static void main(String args[]) throws Exception {

		// Dynamic read from Zookeeper
		String zks = "localhost:2181";
		// Topic name
		String topic = "cps";
		// Spout offset position
		String zkRoot = "/cps";
		String id = "cps";

		// Set read Broker of Kafka from Zookeeper
		BrokerHosts brokerHosts = new ZkHosts(zks);
		
		// Spout configuration
		SpoutConfig spoutConf = new SpoutConfig(brokerHosts, topic, zkRoot, id);
		// Takes the byte[] and returns a tuple with byte[] as is
		spoutConf.scheme = new RawMultiScheme();
		spoutConf.forceFromStart = false;
		spoutConf.zkServers = Arrays.asList(new String[] {"localhost"});
		spoutConf.zkPort = 2181;

    	// Create the topology
        TopologyBuilder builder = new TopologyBuilder();
        // Attach the KafkaSpout to the topology -- parallelism of 1
        builder.setSpout("image-received",new KafkaSpout(spoutConf), 1);
        // Attach the ImageProcessBolt to the topology -- parallelism of 3
        builder.setBolt("image-matching",new ImgMatchingBolt(), 3).shuffleGrouping("image-received");

        Config conf = new Config();

        if(args != null && args.length > 0){
            // Run it on a live storm cluser
            conf.setNumWorkers(3);
            StormSubmitter.submitTopology(args[0],conf,builder.createTopology());
        } else {
            // Run it on a simulated local cluster
            LocalCluster cluster = new LocalCluster();
            // Topo name, configuration, builder.topo
            cluster.submitTopology("stormImageMatch",conf, builder.createTopology());

            //let it run 30 seconds;                        
            Thread.sleep(30000);
	   		// JniImageMatching.releaseInitResource(initiatePointer);
            cluster.killTopology("stormImageMatch");
            cluster.shutdown();
        }
    }
}

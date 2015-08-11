/* StormMatch.java: design spout and bolts running image matching on Storm                     
                                                                                      
   Author: Wuyang Zhang                                                               
                                                                                      
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
public class StormMatch{
    private static long initiatePointer;
    private static DatagramSocket clientSocket;
    private static InetAddress IPAddress;

    public static class RequestedImageSpout extends BaseRichSpout{
	
	SpoutOutputCollector _collector;
	Random _rand;
	@Override
	public void open(Map conf, TopologyContext context, SpoutOutputCollector collector){
	    _collector = collector;
	    _rand = new Random();
	}

	@Override	
	public void nextTuple(){
	       Utils.sleep(500);
	    // int index = _rand.nextInt(100);
	     for( int index = 1; index< 20; index++){
	     String srcImgAddr = JniImageMatching.combineName(JniImageMatching.imgFolderAddr, JniImageMatching.imgPrefix, index, JniImageMatching.imgFormat);
		_collector.emit(new Values(srcImgAddr));
	     }
	     Utils.sleep(1000000);
	}

	@Override
	public void declareOutputFields(OutputFieldsDeclarer declarer){
	    declarer.declare(new Fields("source requested images"));
	}
    }


    public static class ImgMatchingBolt extends BaseRichBolt{
	OutputCollector _collector;

	@Override
	public void prepare(Map map, TopologyContext topologyContext, OutputCollector collector){
	    _collector = collector;
	    initiatePointer = JniImageMatching.initiate_imageMatching(JniImageMatching.indexImgTableAddr,JniImageMatching.imgIndexYmlAddr,JniImageMatching.infoDatabaseAddr);
	    try {
		    clientSocket = new DatagramSocket();
		    IPAddress = InetAddress.getByName("localhost");
	    }
	    catch (Exception e) {
	    	e.printStackTrace();
	    }
	}

	@Override
	public void execute(Tuple tuple){
		byte[] sendData = new byte[1024];
	    String srcImgAddr = tuple.getString(0);
	    String result = JniImageMatching.matchingIndex(srcImgAddr,initiatePointer);

		String temp = "1";
		sendData = temp.getBytes();
		DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, IPAddress, 9876);
		try {
			clientSocket.send(sendPacket);		
		}
	    catch (Exception e) {
	    	e.printStackTrace();
	    }

	    PrintWriter writer;
	    String fileAddr = "/home/hadoop/worksapce/storm/Results.txt";
	    try{
		writer = new PrintWriter(new BufferedWriter(new FileWriter(fileAddr,true)));
		writer.println(result + "\n");
		writer.close();
	    }catch (IOException e){
		e.printStackTrace();
	    }
	    
	}

	@Override
	public void declareOutputFields(OutputFieldsDeclarer declarer){
	    declarer.declare(new Fields ("matching requested images"));
	}
    }


    public static void main(String args[]) throws Exception{
	//crate the topology                                                                        
        TopologyBuilder builder = new TopologyBuilder();
        //attach the randomSentence to the topology -- parallelism of 10                            
        builder.setSpout("rand-image",new RequestedImageSpout(),1);
        //attch the exclamationBolt to the topology -- parallelism of 3                             
        builder.setBolt("image-matching",new ImgMatchingBolt(),5).shuffleGrouping("rand-image");

        Config conf = new Config();

        if(args != null && args.length > 0){
            // Run it on a live storm cluser                                                        
            conf.setNumWorkers(4);
            StormSubmitter.submitTopology(args[0],conf,builder.createTopology());
        }else{
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
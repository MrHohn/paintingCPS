/*************************************************
#
# Purpose: "Kafka Producer" aims to produce tasks for storm
# Author.: Zihong Zheng (zzhonzi@gmail.com)
# Version: 0.1
# License: 
#
*************************************************/

#include "KafkaProducer.h"
#include "kafka.h"

ExampleEventCb ex_event_cb;
ExampleDeliveryReportCb ex_dr_cb;

KafkaProducer::KafkaProducer() {
    brokers = "localhost";
    topic_str = "cps";
    partition = 0;

    /*
    * Create configuration objects
    */
    conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);


    /*
    * Set configuration properties
    */
    conf->set("metadata.broker.list", brokers, errstr);

    conf->set("event_cb", &ex_event_cb, errstr);

    signal(SIGINT, sigterm);
    signal(SIGTERM, sigterm);

    /*
     * Producer mode
     */

    /* Set delivery report callback */
    conf->set("dr_cb", &ex_dr_cb, errstr);

    /*
     * Create producer using accumulated global configuration.
     */
    producer = RdKafka::Producer::create(conf, errstr);
    if (!producer) {
        std::cerr << "Failed to create producer: " << errstr << std::endl;
        exit(1);
    }

    std::cout << "% Created producer " << producer->name() << std::endl;

    /*
     * Create topic handle.
     */
    topic = RdKafka::Topic::create(producer, topic_str, tconf, errstr);
    if (!topic) {
        std::cerr << "Failed to create topic: " << errstr << std::endl;
        exit(1);
    }
}

KafkaProducer::~KafkaProducer() {
    run = true;

    while (run and producer->outq_len() > 0) {
        std::cerr << "Waiting for " << producer->outq_len() << std::endl; producer->poll(1000);
    }

    delete topic;
    delete producer;

    /*
    * Wait for RdKafka to decommission.
    * This is not strictly needed (when check outq_len() above), but
    * allows RdKafka to clean up all its resources before the application
    * exits so that memory profilers such as valgrind wont complain about
    * memory leaks.
    */
    // RdKafka::wait_destroyed(5000);
}

void KafkaProducer::sendString(std::string input, int size) {
    // Read messages from user and produce to broker.
    if (!run) {
        return;
    }
    /*
    * Produce message
    */

    /* Copy payload */
    RdKafka::ErrorCode resp = producer->produce(topic, partition, RdKafka::Producer::RK_MSG_COPY, const_cast<char *>(input.c_str()), input.size(), NULL, NULL);
    if (resp != RdKafka::ERR_NO_ERROR)
        std::cerr << "% Produce failed: " << RdKafka::err2str(resp) << std::endl;
    else
        std::cerr << "% Produced message (" << input.size() << " bytes)" << std::endl;

    producer->poll(0);

}

void KafkaProducer::send(char* message, int size) {
    // Read messages from user and produce to broker.
    if (!run) {
        return;
    }
    /*
    * Produce message
    */

    /* Copy payload */
    RdKafka::ErrorCode resp = producer->produce(topic, partition, RdKafka::Producer::RK_MSG_COPY, message, size, NULL, NULL);
    if (resp != RdKafka::ERR_NO_ERROR)
        std::cerr << "% Produce failed: " << RdKafka::err2str(resp) << std::endl;
    else
        std::cerr << "% Produced message (" << size << " bytes)" << std::endl;

    producer->poll(0);

}

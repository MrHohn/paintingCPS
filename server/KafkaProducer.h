/*************************************************
#
# Purpose: header for "KafkaProducer Class"
# Author.: Zihong Zheng (zzhonzi@gmail.com)
# Version: 0.1
# License: 
#
*************************************************/

#ifndef KAFKAPRODUCER_H
#define KAFKAPRODUCER_H

#include "rdkafkacpp.h"

class KafkaProducer {
public:
    KafkaProducer();
    ~KafkaProducer();
    void send(std::string input, int size);

private:

    std::string brokers;
    std::string errstr;
    std::string topic_str;
    std::string debug;
    int32_t partition;

    /*
    * Create configuration objects
    */
    RdKafka::Conf *conf;
    RdKafka::Conf *tconf;

    RdKafka::Producer *producer;

    /*
     * Create topic handle.
     */
    RdKafka::Topic *topic;

};

#endif /* KAFKAPRODUCER_H */

/*************************************************
#
# Purpose: header for "Metrics Class"
# Author.: Zihong Zheng (zzhonzi@gmail.com)
# Version: 0.1
# License: 
#
*************************************************/

#ifndef METRICS_H
#define METRICS_H

class Metrics {
public:
    Metrics();
    ~Metrics();
    void submit_request();
    void finish_request();
    int get_num_ongoing();
    double get_metrics();

private:
    int request_submitted;
    int request_finished;
    double max_num_of_requests = 100;
};

#endif /* METRICS_H */

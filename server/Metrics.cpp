/*************************************************
#
# Purpose: "Metrics" aims to maintain the metrics for CPS server
# Author.: Zihong Zheng (zzhonzi@gmail.com)
# Version: 0.1
# License: 
#
*************************************************/

#include "Metrics.h"

Metrics::Metrics() {
	request_submitted = 0;
	request_finished = 0;
}

Metrics::~Metrics() {

}

void Metrics::submit_request() {
	++request_submitted;
}

void Metrics::finish_request() {
	++request_finished;
}

int Metrics::get_num_ongoing() {
	return request_submitted - request_finished;
}

double Metrics::get_metrics() {
	if (request_submitted == 0) {
		return 0;
	}
	int curNum = get_num_ongoing();
	if (curNum >= max_num_of_requests) {
		return 1;
	}
	return curNum / max_num_of_requests;
}

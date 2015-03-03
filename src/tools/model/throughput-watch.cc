/*
 * throughput-watch.cc
 *
 *  Created on: Mar 19, 2012
 *      Author: sergi
 */

#include "stat-watch.h"
#include "ns3/simulator.h"
#include "throughput-watch.h"

namespace ns3 {


  void ThroughputWatch::update(uint64_t size, uint64_t time) {
    size_.update (size);
    //std::cout << size << " " << time << " " << time_.current() << " " << (time-time_.current()) << std::endl;
    time_.update (time-time_.current());
    time_.set_current (time);
    //old_average_ = average_;
    //std::cout << size_.average() << " " << time_.average() << " " << (time_.average()/1000000) << std::endl;
    average_ = trAlpha_ *(size_.average()/(time_.average()/1000000)) + (1-trAlpha_)*average_;
    //std::cout << size_.average() << " " << time_.average() << " " << (time_.average()/1000000) << " " << size_.average()/(time_.average()/1000000) << " " << average_ << std::endl;



  }

}


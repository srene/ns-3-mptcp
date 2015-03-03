/*
 * throughput-watch.h
 *
 *  Created on: Mar 19, 2012
 *      Author: sergi
 */

#ifndef THROUGHPUTWATCH_H_
#define THROUGHPUTWATCH_H_

#include "stat-watch.h"
#include "ns3/simulator.h"

namespace ns3 {

class ThroughputWatch : public StatWatch{
public:
  /**
   * Constructor
   */
  ThroughputWatch() { }

  /**
   * Virtual desctructor
   */
  virtual ~ThroughputWatch() {};

  inline void set_alpha(double a) { trAlpha_ = a;}
  inline double get_timer_interval () { return 0.01000000001; }
  void update(uint64_t size,uint64_t time);

 /**
  * Watch for packet size
  */
 StatWatch size_;
 double trAlpha_;

 /**
  * Watch for inter arrival time
  */
 StatWatch time_;

};
}


#endif /* THROUGHPUTWATCH_H_ */

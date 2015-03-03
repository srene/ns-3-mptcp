/*
 * StatWatch.cc
 *
 *  Created on: Mar 19, 2012
 *      Author: sergi
 */

/*
 * NIST: Stats Watch
 * Used to maintain statistics of a particular parameter
 *
 * Positive gradient means that when the value measured increases, it will trigger INIT_ACTION_TH
 * A positive gradient is used for measurements such as packet loss, where as negative gradient
 * is used for RSSI.
 */

#ifndef STAT_WATCH_H
#define STAT_WATCH_H

#include "ns3/assert.h"
#include "ns3/scheduler.h"
#include "ns3/nstime.h"


namespace ns3 {

enum threshold_action_t {
  NO_ACTION_TH,
  INIT_ACTION_TH,
  ROLLBACK_ACTION_TH,
  EXEC_ACTION_TH
};

class StatWatch{
public:
  StatWatch();
  //virtual void expire(Event*);

  /**
   * Reference-counting destructor.
   */
  //~StatWatch();
  inline void reset() {
		average_ = 0;
		instant_ = 0;
		total_ = 0;
		sample_number_ = 0;
	}

	/*inline void set_thresholds (double init_th, double rollback_th, double exec_th) {
	  NS_ASSERT ( (pos_gradient_ && rollback_th <= init_th && init_th <= exec_th)
		   || (!pos_gradient_ && rollback_th >= init_th && init_th >= exec_th));
	  init_th_ = init_th;
	  rollback_th_ = rollback_th;
	  exec_th_ = exec_th;
	  th_set_ = true;
	}*/

	inline void set_timer_interval(int64_t ti) { timer_interval_ = ti; }
	inline void set_alpha(double a) { alpha_= a; }
	inline void set_current(int64_t c) { current_= c; }
	inline void set_pos_gradient(bool b) { pos_gradient_ = b; }
	inline void set_delay (Time d) { delay_ = d; }
	threshold_action_t update(int64_t new_val);


	//inline void schedule_next() { resched(timer_interval_); }

	inline double current() { return current_; }
	inline double total() { return total_; }
	inline double sample_number() { return sample_number_; }
	inline double average() { return average_; }
	//inline int64_t old_average() { return old_average_; }
	inline double instant() { return instant_; }
	inline double simple_average() { return total_/sample_number_; }

protected:
	int64_t timer_interval_;
	double alpha_;
	double current_;
	double average_;
	//int64_t old_average_;
	double instant_;
	double total_;
	int sample_number_;
	bool pos_gradient_;
	Time delay_;
	threshold_action_t pending_action_;
	Time ts_;
	bool th_set_;
	bool init_sent_;
	int64_t init_th_;
	int64_t rollback_th_;
	int64_t exec_th_;
};

}
#endif /* STAT:WATCH_H */

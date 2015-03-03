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

#include "stat-watch.h"
#include "ns3/simulator.h"

namespace ns3 {

  StatWatch::StatWatch() {
		timer_interval_ = 1;
		alpha_ = 1;
		pos_gradient_ = true;
		current_ = 0;
		average_ = 0;
		instant_ = 0;
		total_ = 0;
		sample_number_ = 0;
		th_set_ = false;
		init_sent_ = false;
		init_th_ = 0;
		rollback_th_ = 0;
		exec_th_ = 0;
		//pending_action_ = NO_ACTION_TH;
		delay_ = Time (0);
	}


	//virtual void expire(Event*) { schedule_next(); }

	threshold_action_t StatWatch::update(int64_t new_val) {
	  //old_average_ = average_;
	  average_ = alpha_*new_val + (1-alpha_)*average_;
	  total_ += new_val;
	  sample_number_++;
	  instant_ = new_val;

	  //evaluate if threshold has been crossed
	  /*if (th_set_) {
	    if (pos_gradient_) {
	      //check if threshold is crossed
	      if (old_average_ > rollback_th_ && average_ <= rollback_th_ && init_sent_) {
		pending_action_ = ROLLBACK_ACTION_TH;
		ts_ = Simulator::Now ()+delay_;
	      } else if (old_average_ < exec_th_ && average_ >= exec_th_) {
		pending_action_ = EXEC_ACTION_TH;
		ts_ = Simulator::Now ()+delay_;
	      } else if (old_average_ < init_th_ && average_ >= init_th_) {
		pending_action_ = INIT_ACTION_TH;
		ts_ = Simulator::Now ()+delay_;
	      } else {
		//check if threshold is canceled
		if (((pending_action_ == ROLLBACK_ACTION_TH) && (average_ > rollback_th_))
		    || (pending_action_ == EXEC_ACTION_TH && average_ < exec_th_)
		    || (pending_action_ == INIT_ACTION_TH && average_ < init_th_)) {
		  pending_action_ = NO_ACTION_TH;
		}
	      }
	      //check if action is still valid
	      if (pending_action_ != NO_ACTION_TH && Simulator::Now() >= ts_) {
		threshold_action_t tmp = pending_action_;
		pending_action_ = NO_ACTION_TH;
		return tmp;
	      }

	    } else {
	      //check if threshold is crossed
	      if (old_average_ < rollback_th_ && average_ >= rollback_th_ && init_sent_) {
		pending_action_ = ROLLBACK_ACTION_TH;
		ts_ = Simulator::Now ()+delay_;
	      } else if (old_average_ > exec_th_ && average_ <= exec_th_) {
		pending_action_ = EXEC_ACTION_TH;
		ts_ = Simulator::Now ()+delay_;
	      } else if (old_average_ > init_th_ && average_ <= init_th_) {
		pending_action_ = INIT_ACTION_TH;
		ts_ = Simulator::Now ()+delay_;
	      } else {
		//check if threshold is canceled
		if ((pending_action_ == ROLLBACK_ACTION_TH && average_ < rollback_th_)
		    || (pending_action_ == EXEC_ACTION_TH && average_ > exec_th_)
		    || (pending_action_ == INIT_ACTION_TH && average_ > init_th_)) {
		  pending_action_ = NO_ACTION_TH;
		}
	      }
	      //check if action is still valid
	      if (pending_action_ != NO_ACTION_TH && Simulator::Now() >= ts_) {
		threshold_action_t tmp = pending_action_;
		pending_action_ = NO_ACTION_TH;
		  return tmp;
	      }
	    }
	  }*/
	  return NO_ACTION_TH;
	}

}

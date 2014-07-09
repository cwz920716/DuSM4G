/* -*-	Mode:C++; c-basic-offset:4; tab-width:4; indent-tabs-mode:t -*- */
#ifndef __dusm_routing_h__
#define __dusm_routing_h__

#include <map>
#include <list>
#include <set>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <stdio.h>

#include "dusm_pkt.h"

#include "agent.h"
#include "packet.h"
#include "tclcl.h"
#include "trace.h"
#include "timer-handler.h"
#include "mobilenode.h"
#include "classifier-port.h"
#include "cmu-trace.h"

#define N_REPLICA 3
#define GKPATH "/home/cwz/DuSM4G/rock6461/gkid.txt"
#define STPATH "/home/cwz/DuSM4G/rock6461-ref/ST%d.txt"

/*
 * This is the key for enhanced multicast state. 
 * this is a <src, group> pair
 */
typedef unsigned long hashedKey_t;
std::map< nsaddr_t, int > STrees;

hashedKey_t getKey(nsaddr_t src, nsaddr_t group) {
	hashedKey_t k = src;
	k = (k << 32) | group;
	return k;
}

hashedKey_t hashKey(nsaddr_t src, nsaddr_t group) {
	nsaddr_t hsrc = (src * 2654435761) % (STrees[group]);
	return getKey(hsrc, group);
}

hashedKey_t randomKey(nsaddr_t src, nsaddr_t group, int seq) {
	nsaddr_t hsrc = (seq * 2654435761) % (STrees[group]);
	return getKey(hsrc, group);
}

nsaddr_t getGroupAddr(hashedKey_t k) {
	return (nsaddr_t) (k & 0x00000000ffffffff);
}

nsaddr_t getSrcAddr(hashedKey_t k) {
	return (nsaddr_t) (k >> 32);
}

/*
 * This is the multicast state for multi tree. 
 */
class MtreeState {
public:
	friend class DuSMAgent;
	friend class mtreePIM;

	MtreeState() : states_(), serials_(), len_(0), joinUpdates_(0), leaveUpdates_(0) {}
	inline int len() { return len_; }
	inline int leaveUpdates() { return leaveUpdates_; }
	inline int joinUpdates() { return joinUpdates_; }
	inline void clearAll() {  states_.clear(); len_ = 0; }

	inline void push2(hashedKey_t k, nsaddr_t porta) {  
		if (states_[k].empty())
			len_++;

		states_[k].push_back(porta); 
	}

	inline void remove2(hashedKey_t k, nsaddr_t porta) { 
		states_[k].remove(porta); 
		
		if (states_[k].empty())
			len_--;
	}

	inline void newUpdate(bool join, unsigned long serial) { 
		if (serials_.find(serial) != serials_.end())
			return;

		if (join) 
			joinUpdates_++; 
		else 
			leaveUpdates_++; 
		serials_.insert(serial);
	}
	
	std::map< hashedKey_t, std::list<nsaddr_t> > states_;				/* multicast state for a given group */
	std::set<unsigned long> serials_;
	int len_;
	int joinUpdates_;
	int leaveUpdates_;
	static const int CAPACITY;
};


/*
 * This is the membership controller. 
 */
class GroupController {
public:
	friend class DuSMAgent;
	friend class mtreePIM;

	void subscribe(nsaddr_t node, nsaddr_t group);
	void unsubscribe(nsaddr_t node, nsaddr_t group);
	void post(nsaddr_t, int);
	void receive(nsaddr_t);
	bool inGroup(nsaddr_t node, nsaddr_t group);
	void dumpGroup(nsaddr_t node, nsaddr_t group);
	void init();

	inline bool emptyGroup(nsaddr_t group) {
		return gcs_[group].empty();
	}

	inline bool isElephant(nsaddr_t group) {
		return tfcmtx_[group] >= THRESHOLD && tfcmtx_[group] > 0;
	}

	inline bool preElephant(nsaddr_t group, int len) {
		return !isElephant(group) && tfcmtx_[group] + len >= THRESHOLD;
	}

	inline double fair(nsaddr_t group) {
		double live = finish_[group] - start_[group];
		double send = stop_[group] - start_[group];
		if (live == 0 || finish_[group] == 0)
			return 0;
		if (send == 0)
			return 1;
		else
			return live / send;
	}

	unsigned long nextSerial_;
	std::map< nsaddr_t, std::list<nsaddr_t> > gcs_;
	std::map< nsaddr_t, unsigned long > tfcmtx_;
	std::map< nsaddr_t, double > start_;
	std::map< nsaddr_t, double > stop_;
	std::map< nsaddr_t, double > finish_;
	std::map< nsaddr_t, bool > init_;

	static const unsigned long THRESHOLD;						// the threshold between small/large group

};

class mtreePIM {
public:
	static void build(nsaddr_t group);			
};

/*
 * This is the dusm routing agent. 
 */
class DuSMAgent : public Agent {
public:
	friend class mtreePIM;

	DuSMAgent();
	int command(int, const char*const*);
	void recv(Packet*, Handler*);
	inline void connect2(nsaddr_t nexthop) { 
		dst_.addr_ = nexthop; 
		dst_.port_ = 0;
	}

	static GroupController centralGC_;		// central group controller

protected:
	Trace *logtarget_;						// for trace

private:
	// the node information	
	nsaddr_t addr_;							// addr
	MtreeState mstates_;							// multicast state of this node
	int nextseq_;									// the nextseq to send
	std::map< nsaddr_t, long> tfcmtx_;
	long tfcsum_;

	static std::map< nsaddr_t, DuSMAgent * > agent_pool_;	// agent pool for lookup
	void dumpPacket(Packet*);
	void dumpMcastStates();
	void post(nsaddr_t, int);
	void send2(nsaddr_t, int, nsaddr_t, nsaddr_t, int );
	void send2(nsaddr_t, int, nsaddr_t, nsaddr_t, int, bool, nsaddr_t, nsaddr_t);
	void mcast(Packet*);
	void m2u(Packet*);

};

#endif

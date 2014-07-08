/* -*-	Mode:C++; c-basic-offset:4; tab-width:4; indent-tabs-mode:t -*- */
#include "dusm_routing.h"

using namespace std;

int hdr_dusm::offset_;
std::map< nsaddr_t, DuSMAgent * > DuSMAgent::agent_pool_;
GroupController DuSMAgent::centralGC_;

static class DuSMHeaderClass : public PacketHeaderClass{
public:
	DuSMHeaderClass() : PacketHeaderClass("PacketHeader/DuSM",
										  sizeof(hdr_all_dusm)){
										  bind_offset(&hdr_dusm::offset_);
	}
} class_dusmhdr;

static class DuSMAgentClass : public TclClass {
public:
	DuSMAgentClass() : TclClass("Agent/DuSM") {}
	TclObject *create(int argc, const char*const* argv) {
		return (new DuSMAgent());
	}
} class_DuSMAgent;


/*************************************************************
 *
 *MState Section
 *
 *************************************************************/
const int MtreeState::CAPACITY = 1000;


/*************************************************************
 *
 *DuSM Section
 *
 *************************************************************/

DuSMAgent::DuSMAgent() : Agent(PT_DUSM), addr_(-1) {
	tfcsum_ = 0;
	nextseq_ = 0;
}

int DuSMAgent::command(int argc, const char*const* argv) {
	if (argc == 2) {
		if (strcasecmp(argv[1], "start") == 0) {
			return TCL_OK;
		} else if (strcasecmp(argv[1], "show-port") == 0) {
			fprintf(stderr, "%d \n", here_.port_);
			return TCL_OK;
		} else if (strcasecmp(argv[1], "show-addr") == 0) {
			fprintf(stderr, "%d \n", here_.addr_);
			return TCL_OK;
		} else if (strcasecmp(argv[1], "show-dst-port") == 0) {
			fprintf(stderr, "%d \n", dst_.port_);
			return TCL_OK;
		} else if (strcasecmp(argv[1], "show-dst-addr") == 0) {
			fprintf(stderr, "%d \n", dst_.addr_);
			return TCL_OK;
		} else if (strcasecmp(argv[1], "dump-mcast") == 0) {
			dumpMcastStates();
			return TCL_OK;
		} else if (strcasecmp(argv[1], "dump-tfcsum") == 0) {
			fprintf(stderr, "dump-tfcsum -addr %d %ld \n", addr_, tfcsum_);
			return TCL_OK;
		} else if (strcasecmp(argv[1], "init-gc") == 0) {
			centralGC_.init();
			return TCL_OK;
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "addr") == 0) {
			addr_ = Address::instance().str2addr(argv[2]);
			agent_pool_.insert(map< nsaddr_t, DuSMAgent * >::value_type(addr_, this));
			return TCL_OK;
		} else if (strcmp(argv[1], "subscribe") == 0) {
			nsaddr_t group = Address::instance().str2addr(argv[2]);
			centralGC_.subscribe(addr_, group);
			return TCL_OK;
		} else if (strcmp(argv[1], "unsubscribe") == 0) {
			nsaddr_t group = Address::instance().str2addr(argv[2]);
			centralGC_.unsubscribe(addr_, group);
			return TCL_OK;
		} else if (strcmp(argv[1], "dump-group") == 0) {
			nsaddr_t group = Address::instance().str2addr(argv[2]);
			centralGC_.dumpGroup(addr_, group);
			return TCL_OK;
		} else if (strcmp(argv[1], "dump-fair") == 0) {
			nsaddr_t group = Address::instance().str2addr(argv[2]);
			double fair = centralGC_.fair(group);
			fprintf(stderr, "dump-fair -g %d -f %lf\n", group, fair);
			return TCL_OK;
		} else if (strcmp(argv[1], "dump-link-stat") == 0) {
			fprintf(stderr, "dump-link-stat -from %d -to %d -a %ld -r %lf \n", addr_, Address::instance().str2addr(argv[2]), tfcmtx_[Address::instance().str2addr(argv[2])], tfcmtx_[Address::instance().str2addr(argv[2])] / (Scheduler::instance().clock()));
			return TCL_OK;
		} else if (strcmp(argv[1], "log-target") == 0 || strcmp(argv[1], "tracetarget") == 0) {
			logtarget_ = (Trace*)TclObject::lookup(argv[2]);
			if (logtarget_ == 0) {
				fprintf(stderr, "%s: %s lookup of %s failed \n", __FILE__, argv[1], argv[2]);
				return TCL_ERROR;
			}
			return TCL_OK;
		} 
	} else if (argc == 4) {
		if (strcmp(argv[1], "post") == 0) {
			centralGC_.post( Address::instance().str2addr(argv[2]), atoi(argv[3]) );
			post( Address::instance().str2addr(argv[2]), atoi(argv[3]) );
			return TCL_OK;
		}
	}

	return Agent::command(argc, argv);
}

void DuSMAgent::recv(Packet* p, Handler* h) {
	struct hdr_dusm_data *hdr = HDR_DUSM_DATA(p);
	// struct hdr_ip* ih = HDR_IP(p);

	// fprintf(stderr, "pkt recv at %d, now is %lf.\n", addr_, Scheduler::instance().clock());
	if (centralGC_.inGroup(addr_, hdr->group_)) {
		// recv business
		nsaddr_t group = hdr->group_;
		centralGC_.receive(group);
	}
	if (!hdr->tunnelFg_) {
		// mcast business
		mcast(p);
	}
	Packet::free(p);
}

void DuSMAgent::mcast(Packet *p) {
	struct hdr_dusm_data *hdr = HDR_DUSM_DATA(p);
	hashedKey_t key = randomKey(hdr->source_, hdr->group_, hdr->seq_);

	std::list<nsaddr_t>& ports = mstates_.states_[key];
	if (ports.empty()) {
		return;
	}

	for (std::list<nsaddr_t>::iterator i = ports.begin(); i != ports.end(); ++i) {
		nsaddr_t nexthop = *i;
		if (nexthop != hdr->lasthop_) {
			send2(nexthop, hdr->content_size_, hdr->source_, hdr->group_, hdr->seq_);
		}
	}
}

void DuSMAgent::dumpPacket(Packet *p) {
	struct hdr_dusm_data *hdr = HDR_DUSM_DATA(p);
	fprintf(stderr, "pkt -n %d -n %d -s %d -d %d -h %d",
			 addr_, HDR_CMN(p)->next_hop(), hdr->source_, hdr->group_, (HDR_CMN(p))->num_forwards());
}

void DuSMAgent::dumpMcastStates() {
	fprintf(stderr, "node -addr %d -len(mcast) %d -joinU %d -leaveU %d \n", addr_, mstates_.len(), 
						mstates_.joinUpdates(), mstates_.leaveUpdates());
/*
	for (std::map< hashedKey_t, std::list<nsaddr_t> >::iterator i = mstates_.states_.begin();
			i != mstates_.states_.end(); ++i) {
		fprintf(stderr, "entry -group (%d, %d) [", getSrcAddr((*i).first), getGroupAddr((*i).first));
		std::list<nsaddr_t>& ports = (*i).second;
		for (std::list<nsaddr_t>::iterator j = ports.begin(); j != ports.end(); ++j)
			fprintf(stderr, "%d, ", *j);
		fprintf(stderr, "]\n");
	}
*/
}

void DuSMAgent::post(nsaddr_t group, int size) {
	int seqno = (int) rand();
	if (centralGC_.isElephant(group)) {
		hashedKey_t key = randomKey(addr_, group, seqno);
		std::list<nsaddr_t>& ports = mstates_.states_[key];
		if (ports.empty()) {
			return;
		}
		
		for (std::list<nsaddr_t>::iterator i = ports.begin(); i != ports.end(); ++i) {
			nsaddr_t nexthop = *i;
			send2(nexthop, size, addr_, group, seqno);
		}
	} else {
		// multicast -> unicast tunnel
		std::list<nsaddr_t>& hostlist = centralGC_.gcs_[group];
		for (std::list<nsaddr_t>::iterator i = hostlist.begin(); i != hostlist.end(); ++i) {
			nsaddr_t nexthop = *i;
			if (nexthop == addr_)
				continue;

			// fprintf(stderr, "x%d\n", nexthop);
			send2(nexthop, size, addr_, group, seqno, true, addr_, nexthop);
		}
	}
}

void DuSMAgent::send2(nsaddr_t nexthop, int size, nsaddr_t source, nsaddr_t group, int seq) {
	send2(nexthop, size, source, group, seq, false, -1, -1);
}

void DuSMAgent::send2(nsaddr_t nexthop, int size, nsaddr_t source, nsaddr_t group, int seq, bool tunnelFg, nsaddr_t tsrc, nsaddr_t tdest) {
	// Create a new packet
	connect2(nexthop);
	Packet* p = allocpkt();
	// Access the DuSM header for the new packet:
	struct hdr_dusm_data *hdr = HDR_DUSM_DATA(p);
	hdr->source_ = source;
	hdr->group_ = group;
	hdr->seq_ = seq;
	hdr->tunnelFg_ = tunnelFg;
	hdr->tsrc_ = tsrc;
	hdr->tdest_ = tdest;
	hdr->nexthop_ = nexthop;
	hdr->lasthop_ = addr_;
	hdr->content_size_ = size;
	
	struct hdr_cmn* ch = HDR_CMN(p);
	ch->size() += hdr->content_size_;
	tfcmtx_[nexthop] += size;
	tfcsum_ += size;

	if (tunnelFg) {
		ch->size() += IPTUNNEL_OVERHEAD;
		tfcmtx_[nexthop] += IPTUNNEL_OVERHEAD;
		tfcsum_ += IPTUNNEL_OVERHEAD;
	}
	send(p, 0);
}

/*************************************************************
 *
 *GroupController Section
 *
 *************************************************************/

const unsigned long GroupController::THRESHOLD = 0;

bool GroupController::inGroup(nsaddr_t node, nsaddr_t group) {
	std::map< nsaddr_t, std::list<nsaddr_t> >& c = gcs_;
	return c.find(group) != c.end() && 
				std::find(c[group].begin(), c[group].end(), node) != c[group].end();
}

void GroupController::dumpGroup(nsaddr_t node, nsaddr_t group) {
	std::map< nsaddr_t, std::list<nsaddr_t> >& c = gcs_;
	std::list<nsaddr_t>& hostlist = gcs_[group];

	fprintf(stderr, "dump -edge %d -group %d [", node, group);
	for (std::list<nsaddr_t>::iterator i = hostlist.begin(); i != hostlist.end(); ++i) {
		fprintf(stderr, "%d, ", *i);
	}
	fprintf(stderr, "]\n");
}

void GroupController::subscribe(nsaddr_t node, nsaddr_t group) {
	if (inGroup(node, group))
		return;

	// fprintf(stderr, "%d sub %d\n", node_addr, group);
	std::map< nsaddr_t, std::list<nsaddr_t> >& c = gcs_;
	c[group].push_back(node);
}

void GroupController::unsubscribe(nsaddr_t node, nsaddr_t group) {
	if (!inGroup(node, group))
		return;

	std::map< nsaddr_t, std::list<nsaddr_t> >& c = gcs_;
	c[group].remove(node);
}

void GroupController::post(nsaddr_t group, int len) {
	if (preElephant(group, len)) {
		// fprintf(stderr, "pre Ele: %d %ld", group, tfcmtx_[group] + len);
		nextSerial_++;
		mtreePIM::build(group);
	}
	tfcmtx_[group] += len;

	double now = Scheduler::instance().clock();
	if (!init_[group]) {
		stop_[group] = start_[group] = now;
		init_[group] = true;
		return;
	}

	if (now < start_[group])
		start_[group] = now;

	if (now > stop_[group])
		stop_[group] = now;
		
}

void GroupController::receive(nsaddr_t group) {
	double now = Scheduler::instance().clock();

	if (finish_[group] == 0 || now > finish_[group])
		finish_[group] = now;
}

void GroupController::init() {
	ifstream gkin;
	gkin.open(GKPATH);
	if (!gkin) {
		fprintf(stderr, "open GK error!\n");
		return;
	}

	int npair;
	gkin >> npair;
	for (int i = 0; i < npair; i++) {
		int group, kid;
		gkin >> group >> kid;
		subscribe(kid, group);
	}
	gkin.close();
}


/*************************************************************
 *
 *mtreePIM Section
 *
 *************************************************************/
void mtreePIM::build(nsaddr_t group) {
	char fname[512] = "";
	sprintf(fname, STPATH, group);
	ifstream stin;
	stin.open(fname);
	if (!stin) {
		fprintf(stderr, "open ST error!\n");
		return;
	}

	int ntree;
	stin >> ntree;
	STrees[group] = ntree;
	for (int i = 0; i < ntree; i++) {
		int nbranch;
		stin >> nbranch;
		for (int j = 0; j < nbranch; j++) {
			int v, w;
			stin >> v >> w;
			MtreeState& v_mcast = DuSMAgent::agent_pool_[v]->mstates_;
			MtreeState& w_mcast = DuSMAgent::agent_pool_[w]->mstates_;
			hashedKey_t k = getKey(i, group);
			v_mcast.push2(k, w);
			w_mcast.push2(k, v);
		}
	}
	stin.close();
}

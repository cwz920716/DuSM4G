/* -*-	Mode:C++; c-basic-offset:4; tab-width:4; indent-tabs-mode:t -*- */
#ifndef __dusm_pkt_h__
#define __dusm_pkt_h__

#include "packet.h"

#define HDR_DUSM(p)   ((struct hdr_dusm*)hdr_dusm::access(p))
#define HDR_DUSM_DATA(p) ((struct hdr_dusm_data*)hdr_dusm::access(p))

#define IPTUNNEL_OVERHEAD (4 * 5)

enum DuSMPktType {
	DUSM_PKT_DATA		// data
};

struct hdr_dusm {
	DuSMPktType type_;
	
	static int offset_;
	inline static int& offset() {return offset_;}
	inline static hdr_dusm* access(const Packet* p) {
		return (hdr_dusm*) p->access(offset_);
	}
};

struct hdr_dusm_data {
// App Layer
	DuSMPktType type_;					// the type of pkt
	nsaddr_t source_;						// the source addr
	nsaddr_t group_;						// the group addr
	int seq_;								// the seq number
// GRE/Tunnel Layer
	bool tunnelFg_;							// the tunnel Flag	
	nsaddr_t tsrc_;							// the tunnel source
	nsaddr_t tdest_;						// the tunnel destination
// Routing Layer
	nsaddr_t lasthop_;						// the last hop for this packet
	nsaddr_t nexthop_;						// the next hop for this packet
	int content_size_;						// the content size
};

union hdr_all_dusm {
	hdr_dusm dusmHdr;
	hdr_dusm_data dusmHdrData;
};

#endif

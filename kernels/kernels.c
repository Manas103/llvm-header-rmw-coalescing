// 18 packet-processing kernels, each a small header read-modify-write
// chain on a shared byte buffer, modeled on real per-packet field updates
// (DSCP/ECN, VLAN PCP/DEI, TCP control bits, IPv6 traffic class, MPLS
// TC/S, tunnel flags). 14 of them interleave the RMW steps with a call to
// an opaque, separately-compiled helper (stats/logging/telemetry) that
// never receives the header pointer; clang -O2 cannot prove that call
// doesn't touch the header (see README), so it leaves the redundant
// reload in place. 4 of them (k15-k18) interleave the RMW steps with a
// write through a second, potentially-aliasing header pointer instead;
// no call is involved, and this project's pass must not coalesce these,
// which is what the alias-analysis precondition is for.
//
// Nothing here is the real target's packet format; byte offsets and bit
// masks are illustrative, not drawn from a real header layout.

typedef struct { unsigned char b[32]; } pkt_hdr_t;

extern void stats_bump(void* ctx);
extern void log_event(void* ctx);
extern void telemetry_mark(void* ctx);

// ---- coalescable: call-separated chains (k01-k14) ----

void k01_dscp_ecn(pkt_hdr_t* h, void* ctx, unsigned char ecn, unsigned char dscp) {
    h->b[1] = (h->b[1] & 0xFC) | ecn;           // IPv4 TOS byte: ECN bits
    stats_bump(ctx);
    h->b[1] = (h->b[1] & 0x03) | (dscp << 2);   // IPv4 TOS byte: DSCP bits
}

void k02_vlan_pcp_dei(pkt_hdr_t* h, void* ctx, unsigned char pcp, unsigned char dei) {
    h->b[0] = (h->b[0] & 0x0F) | (pcp << 5);    // VLAN tag: PCP
    log_event(ctx);
    h->b[0] = (h->b[0] & 0xEF) | (dei << 4);    // VLAN tag: DEI
}

void k03_tcp_flags_ack_syn(pkt_hdr_t* h, void* ctx, unsigned char ack, unsigned char syn) {
    h->b[13] = (h->b[13] & 0xEF) | (ack << 4);  // TCP flags byte: ACK
    telemetry_mark(ctx);
    h->b[13] = (h->b[13] & 0xFD) | (syn << 1);  // TCP flags byte: SYN
}

void k04_ipv6_tclass(pkt_hdr_t* h, void* ctx, unsigned char tclass_hi, unsigned char tclass_lo) {
    h->b[0] = (h->b[0] & 0xF0) | (tclass_hi & 0x0F);
    stats_bump(ctx);
    h->b[0] = (h->b[0] & 0x0F) | (tclass_lo << 4);
}

void k05_mpls_tc_s(pkt_hdr_t* h, void* ctx, unsigned char tc, unsigned char s) {
    h->b[2] = (h->b[2] & 0x8F) | (tc << 4);     // MPLS label: traffic class
    log_event(ctx);
    h->b[2] = (h->b[2] & 0xF7) | (s << 3);      // MPLS label: bottom-of-stack bit
}

void k06_gre_flags(pkt_hdr_t* h, void* ctx, unsigned char key_present, unsigned char seq_present) {
    h->b[0] = (h->b[0] & 0x7F) | (key_present << 7);
    telemetry_mark(ctx);
    h->b[0] = (h->b[0] & 0xF7) | (seq_present << 3);
}

void k07_vxlan_flags(pkt_hdr_t* h, void* ctx, unsigned char vni_valid, unsigned char gbp, unsigned char r_bit) {
    h->b[0] = (h->b[0] & 0xF7) | (vni_valid << 3);
    stats_bump(ctx);
    h->b[0] = (h->b[0] & 0x7F) | (gbp << 7);
    log_event(ctx);
    h->b[0] = (h->b[0] & 0xBF) | (r_bit << 6);
}

void k08_gtp_flags(pkt_hdr_t* h, void* ctx, unsigned char pn, unsigned char seq) {
    h->b[0] = (h->b[0] & 0xFE) | pn;
    log_event(ctx);
    h->b[0] = (h->b[0] & 0xFD) | (seq << 1);
}

void k09_ipv4_flags_frag(pkt_hdr_t* h, void* ctx, unsigned char reserved, unsigned char df, unsigned char mf) {
    h->b[6] = (h->b[6] & 0x7F) | (reserved << 7); // IPv4 flags: reserved bit, must be 0
    stats_bump(ctx);
    h->b[6] = (h->b[6] & 0xBF) | (df << 6);     // IPv4 flags: don't-fragment
    telemetry_mark(ctx);
    h->b[6] = (h->b[6] & 0xDF) | (mf << 5);     // IPv4 flags: more-fragments
}

void k10_vlan_tpid_lo(pkt_hdr_t* h, void* ctx, unsigned char lo_hi, unsigned char lo_lo) {
    h->b[4] = (h->b[4] & 0x0F) | (lo_hi << 4);
    stats_bump(ctx);
    h->b[4] = (h->b[4] & 0xF0) | lo_lo;
}

void k11_tcp_ecn_bits(pkt_hdr_t* h, void* ctx, unsigned char ece, unsigned char cwr) {
    h->b[13] = (h->b[13] & 0xBF) | (ece << 6);
    log_event(ctx);
    h->b[13] = (h->b[13] & 0x7F) | (cwr << 7);
}

void k12_ipv6_hop_limit_region(pkt_hdr_t* h, void* ctx, unsigned char hi, unsigned char lo) {
    h->b[7] = (h->b[7] & 0xF0) | (hi & 0x0F);
    telemetry_mark(ctx);
    h->b[7] = (h->b[7] & 0x0F) | (lo << 4);
}

// Three-segment chains (two calls, three RMW steps on the same byte):
// the pass's worklist loop must find and coalesce more than one pair.

void k_three_a_tos_full(pkt_hdr_t* h, void* ctx, unsigned char ecn, unsigned char dscp, unsigned char mark) {
    h->b[1] = (h->b[1] & 0xFC) | ecn;
    stats_bump(ctx);
    h->b[1] = (h->b[1] & 0x03) | (dscp << 2);
    log_event(ctx);
    h->b[1] = h->b[1] | (mark & 0x01);
}

void k_three_b_vlan_full(pkt_hdr_t* h, void* ctx, unsigned char pcp, unsigned char dei, unsigned char idbit) {
    h->b[0] = (h->b[0] & 0x0F) | (pcp << 5);
    telemetry_mark(ctx);
    h->b[0] = (h->b[0] & 0xEF) | (dei << 4);
    stats_bump(ctx);
    h->b[0] = (h->b[0] & 0xFE) | idbit;
}

// ---- guarded: aliasing-pointer-separated chains (k15-k18), must NOT coalesce ----

void k13_dscp_ecn_aliased(pkt_hdr_t* h, pkt_hdr_t* maybe_alias, unsigned char ecn, unsigned char dscp) {
    h->b[1] = (h->b[1] & 0xFC) | ecn;
    maybe_alias->b[3] = 0x55; // a different offset, but the pointer may equal h at runtime
    h->b[1] = (h->b[1] & 0x03) | (dscp << 2);
}

void k14_vlan_aliased(pkt_hdr_t* h, pkt_hdr_t* maybe_alias, unsigned char pcp, unsigned char dei) {
    h->b[0] = (h->b[0] & 0x0F) | (pcp << 5);
    maybe_alias->b[7] = 0xAA;
    h->b[0] = (h->b[0] & 0xEF) | (dei << 4);
}

void k15_tcp_flags_aliased(pkt_hdr_t* h, pkt_hdr_t* maybe_alias, unsigned char ack, unsigned char syn) {
    h->b[13] = (h->b[13] & 0xEF) | (ack << 4);
    maybe_alias->b[1] = 0x01;
    h->b[13] = (h->b[13] & 0xFD) | (syn << 1);
}

void k16_mpls_aliased(pkt_hdr_t* h, pkt_hdr_t* maybe_alias, unsigned char tc, unsigned char s) {
    h->b[2] = (h->b[2] & 0x8F) | (tc << 4);
    maybe_alias->b[9] = 0x02;
    h->b[2] = (h->b[2] & 0xF7) | (s << 3);
}

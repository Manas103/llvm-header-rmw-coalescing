// Execution-level differential test: every kernel is linked in twice, once
// compiled straight (the "k_*" symbols) and once run through this pass
// plus a standard cleanup pipeline (the "opt_k_*" symbols, built from a
// mechanically renamed copy of the same source via tools/run_runtime_check.sh).
// For many random packets and parameter values, both versions must agree
// byte for byte. This is the thing the FileCheck IR tests in test/*.ll
// cannot show on their own: that the transformation is not just
// structurally plausible but actually preserves program behavior.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct { unsigned char b[32]; } pkt_hdr_t;

void stats_bump(void* ctx) { (void)ctx; }
void log_event(void* ctx) { (void)ctx; }
void telemetry_mark(void* ctx) { (void)ctx; }

#define DECL_CTX(name) \
    void name(pkt_hdr_t*, void*, unsigned char, unsigned char); \
    void opt_##name(pkt_hdr_t*, void*, unsigned char, unsigned char);
#define DECL_CTX3(name) \
    void name(pkt_hdr_t*, void*, unsigned char, unsigned char, unsigned char); \
    void opt_##name(pkt_hdr_t*, void*, unsigned char, unsigned char, unsigned char);
#define DECL_ALIAS(name) \
    void name(pkt_hdr_t*, pkt_hdr_t*, unsigned char, unsigned char); \
    void opt_##name(pkt_hdr_t*, pkt_hdr_t*, unsigned char, unsigned char);

DECL_CTX(k01_dscp_ecn)
DECL_CTX(k02_vlan_pcp_dei)
DECL_CTX(k03_tcp_flags_ack_syn)
DECL_CTX(k04_ipv6_tclass)
DECL_CTX(k05_mpls_tc_s)
DECL_CTX(k06_gre_flags)
DECL_CTX3(k07_vxlan_flags)
DECL_CTX(k08_gtp_flags)
DECL_CTX3(k09_ipv4_flags_frag)
DECL_CTX(k10_vlan_tpid_lo)
DECL_CTX(k11_tcp_ecn_bits)
DECL_CTX(k12_ipv6_hop_limit_region)
DECL_CTX3(k_three_a_tos_full)
DECL_CTX3(k_three_b_vlan_full)
DECL_ALIAS(k13_dscp_ecn_aliased)
DECL_ALIAS(k14_vlan_aliased)
DECL_ALIAS(k15_tcp_flags_aliased)
DECL_ALIAS(k16_mpls_aliased)

static int g_checks = 0, g_failures = 0;
static unsigned int g_rng = 0xC0FFEEu;
static unsigned char next_byte(void) {
    g_rng = g_rng * 1103515245u + 12345u;
    return (unsigned char)(g_rng >> 16);
}

static void check_ctx3(const char* name,
                        void (*plain)(pkt_hdr_t*, void*, unsigned char, unsigned char, unsigned char),
                        void (*opt)(pkt_hdr_t*, void*, unsigned char, unsigned char, unsigned char),
                        int trials) {
    for (int t = 0; t < trials; ++t) {
        pkt_hdr_t a, b;
        for (int i = 0; i < 32; ++i) a.b[i] = b.b[i] = next_byte();
        unsigned char v1 = next_byte(), v2 = next_byte(), v3 = next_byte();
        int ctxval = 0;
        plain(&a, &ctxval, v1, v2, v3);
        opt(&b, &ctxval, v1, v2, v3);
        ++g_checks;
        if (memcmp(a.b, b.b, 32) != 0) {
            ++g_failures;
            printf("FAIL %s trial %d: plain/optimized packets differ\n", name, t);
        }
    }
}

static void check_ctx(const char* name,
                       void (*plain)(pkt_hdr_t*, void*, unsigned char, unsigned char),
                       void (*opt)(pkt_hdr_t*, void*, unsigned char, unsigned char),
                       int trials) {
    for (int t = 0; t < trials; ++t) {
        pkt_hdr_t a, b;
        for (int i = 0; i < 32; ++i) a.b[i] = b.b[i] = next_byte();
        unsigned char v1 = next_byte(), v2 = next_byte();
        int ctxval = 0;
        plain(&a, &ctxval, v1, v2);
        opt(&b, &ctxval, v1, v2);
        ++g_checks;
        if (memcmp(a.b, b.b, 32) != 0) {
            ++g_failures;
            printf("FAIL %s trial %d: plain/optimized packets differ\n", name, t);
        }
    }
}

static void check_alias(const char* name,
                         void (*plain)(pkt_hdr_t*, pkt_hdr_t*, unsigned char, unsigned char),
                         void (*opt)(pkt_hdr_t*, pkt_hdr_t*, unsigned char, unsigned char),
                         int trials) {
    for (int t = 0; t < trials; ++t) {
        // Half the trials genuinely alias (same buffer for h and
        // maybe_alias), half don't; the pass must be correct either way
        // since it never attempts to coalesce across this call pattern.
        pkt_hdr_t a, b, other_a, other_b;
        for (int i = 0; i < 32; ++i) {
            a.b[i] = b.b[i] = next_byte();
            other_a.b[i] = other_b.b[i] = next_byte();
        }
        unsigned char v1 = next_byte(), v2 = next_byte();
        int really_alias = (t % 2) == 0;
        plain(&a, really_alias ? &a : &other_a, v1, v2);
        opt(&b, really_alias ? &b : &other_b, v1, v2);
        ++g_checks;
        if (memcmp(a.b, b.b, 32) != 0) {
            ++g_failures;
            printf("FAIL %s trial %d (alias=%d): plain/optimized packets differ\n", name, t, really_alias);
        }
    }
}

int main(void) {
    const int N = 500;
    check_ctx("k01_dscp_ecn", k01_dscp_ecn, opt_k01_dscp_ecn, N);
    check_ctx("k02_vlan_pcp_dei", k02_vlan_pcp_dei, opt_k02_vlan_pcp_dei, N);
    check_ctx("k03_tcp_flags_ack_syn", k03_tcp_flags_ack_syn, opt_k03_tcp_flags_ack_syn, N);
    check_ctx("k04_ipv6_tclass", k04_ipv6_tclass, opt_k04_ipv6_tclass, N);
    check_ctx("k05_mpls_tc_s", k05_mpls_tc_s, opt_k05_mpls_tc_s, N);
    check_ctx("k06_gre_flags", k06_gre_flags, opt_k06_gre_flags, N);
    check_ctx3("k07_vxlan_flags", k07_vxlan_flags, opt_k07_vxlan_flags, N);
    check_ctx("k08_gtp_flags", k08_gtp_flags, opt_k08_gtp_flags, N);
    check_ctx3("k09_ipv4_flags_frag", k09_ipv4_flags_frag, opt_k09_ipv4_flags_frag, N);
    check_ctx("k10_vlan_tpid_lo", k10_vlan_tpid_lo, opt_k10_vlan_tpid_lo, N);
    check_ctx("k11_tcp_ecn_bits", k11_tcp_ecn_bits, opt_k11_tcp_ecn_bits, N);
    check_ctx("k12_ipv6_hop_limit_region", k12_ipv6_hop_limit_region, opt_k12_ipv6_hop_limit_region, N);
    check_ctx3("k_three_a_tos_full", k_three_a_tos_full, opt_k_three_a_tos_full, N);
    check_ctx3("k_three_b_vlan_full", k_three_b_vlan_full, opt_k_three_b_vlan_full, N);
    check_alias("k13_dscp_ecn_aliased", k13_dscp_ecn_aliased, opt_k13_dscp_ecn_aliased, N);
    check_alias("k14_vlan_aliased", k14_vlan_aliased, opt_k14_vlan_aliased, N);
    check_alias("k15_tcp_flags_aliased", k15_tcp_flags_aliased, opt_k15_tcp_flags_aliased, N);
    check_alias("k16_mpls_aliased", k16_mpls_aliased, opt_k16_mpls_aliased, N);

    printf("%d of %d checks passed\n", g_checks - g_failures, g_checks);
    return g_failures == 0 ? 0 : 1;
}

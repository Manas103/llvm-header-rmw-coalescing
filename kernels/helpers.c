// Separately compiled so clang never sees these bodies while optimizing
// kernels.c; that is what makes the calls in kernels.c opaque at -O2.
void stats_bump(void* ctx) { (void)ctx; }
void log_event(void* ctx) { (void)ctx; }
void telemetry_mark(void* ctx) { (void)ctx; }

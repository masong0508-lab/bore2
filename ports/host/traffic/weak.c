// stubs for the audit hooks that an older traffic.c does not have (the real ones in src/traffic.c win)
int __attribute__((weak)) traf_why;
int __attribute__((weak)) traf_dbg_info(int i, char *out) { (void)i; out[0] = 0; return 0; }
int __attribute__((weak)) traf_dbg_gen(int i) { (void)i; return 0; }

#define _GNU_SOURCE
#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
/* On SIGABRT, print the C backtrace to stderr, then die by the signal.
   Preloaded by boil/tools/cstack.sh, for a C stack where gdb is not
   installed.  faulthandler, which regrtest enables, chains to it. */
static void handler(int sig) {
    void *frames[128];
    int n = backtrace(frames, 128);
    static const char head[] = "\n=== C backtrace (abortbt) ===\n";
    write(2, head, sizeof(head) - 1);
    backtrace_symbols_fd(frames, n, 2);
    signal(sig, SIG_DFL);
    raise(sig);
}
__attribute__((constructor)) static void init(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigaction(SIGABRT, &sa, NULL);
}

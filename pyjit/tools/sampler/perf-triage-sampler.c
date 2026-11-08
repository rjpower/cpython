/* LD_PRELOAD SIGPROF sampler.
 * Env: SAMPLER_OUT (default /tmp/sampler.out), SAMPLER_US (interval, default 200),
 *      SAMPLER_DEPTH (frames via frame-pointer walk, default 1).
 * Output: one line per sample: "pc0 pc1 ..." in hex (absolute addresses),
 * plus a "maps" section copied from /proc/self/maps for symbolisation. */
#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <ucontext.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#define MAXS (1 << 22)
#define MAXD 8
static uintptr_t *buf;
static volatile size_t n;
static int depth = 1;

static void handler(int sig, siginfo_t *si, void *uc_) {
    (void)sig; (void)si;
    ucontext_t *uc = uc_;
    if (n + depth > (size_t)MAXS * MAXD) return;
    uintptr_t pc = uc->uc_mcontext.gregs[REG_RIP];
    uintptr_t *out = buf + n;
    out[0] = pc;
    int d = 1;
    uintptr_t fp = uc->uc_mcontext.gregs[REG_RBP];
    uintptr_t sp = uc->uc_mcontext.gregs[REG_RSP];
    while (d < depth) {
        if (fp < sp || fp > sp + (1 << 24) || (fp & 7)) break;
        uintptr_t ret = ((uintptr_t *)fp)[1];
        uintptr_t next = ((uintptr_t *)fp)[0];
        out[d++] = ret;
        if (next <= fp) break;
        fp = next;
    }
    for (; d < depth; d++) out[d] = 0;
    n += depth;
}

static void dump(void) {
    struct itimerval it = {0};
    setitimer(ITIMER_PROF, &it, NULL);
    const char *path = getenv("SAMPLER_OUT");
    if (!path) path = "/tmp/sampler.out";
    char p[512];
    snprintf(p, sizeof p, "%s.%d", path, getpid());
    FILE *f = fopen(p, "w");
    if (!f) return;
    fprintf(f, "depth %d\n", depth);
    for (size_t i = 0; i < n; i += depth) {
        for (int d = 0; d < depth; d++) fprintf(f, "%lx ", (unsigned long)buf[i + d]);
        fputc('\n', f);
    }
    fprintf(f, "maps\n");
    FILE *m = fopen("/proc/self/maps", "r");
    if (m) {
        char line[1024];
        while (fgets(line, sizeof line, m)) fputs(line, f);
        fclose(m);
    }
    fclose(f);
}

__attribute__((constructor)) static void init(void) {
    const char *d = getenv("SAMPLER_DEPTH");
    if (d) depth = atoi(d);
    if (depth < 1) depth = 1;
    if (depth > MAXD) depth = MAXD;
    buf = calloc((size_t)MAXS * MAXD, sizeof *buf);
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_sigaction = handler;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGPROF, &sa, NULL);
    const char *us = getenv("SAMPLER_US");
    long u = us ? atol(us) : 200;
    struct itimerval it = {{0, u}, {0, u}};
    setitimer(ITIMER_PROF, &it, NULL);
    atexit(dump);
}

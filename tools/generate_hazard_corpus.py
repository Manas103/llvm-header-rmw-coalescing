#!/usr/bin/env python3
"""Deterministic generator for the latency-hazard analysis corpus.

Produces kernels/hazard_corpus.c (one function per case, every seeded-hazard
function marked __attribute__((hot))) plus docs/hazard_manifest.json, the
ground truth the benchmark script scores the pass's findings against.

Three groups of functions, all documented here rather than hidden in the
scoring script, because the ground truth a benchmark is scored against is
itself part of what "honest" means for this project:

1. 240 "direct" seeded hazards (60 per category: heap allocation, locking,
   blocking syscall, virtual dispatch), each one call to a name on the
   pass's own recognized-symbol list, or one indirect call through a
   function pointer. These are the cases name-based (or indirect-call-
   based) detection is built to catch.
2. 10 "redirected" seeded hazards: a locking or blocking-syscall call made
   *through a function pointer* rather than by name. The hazard is real
   (calling pthread_mutex_lock in a hot path is exactly as costly whether
   you call it by name or through a pointer), but the pass's name-based
   category detection cannot see through the indirection -- it correctly
   flags the call site as a virtual-dispatch hazard (any indirect call
   trips that check unconditionally) but not, additionally, as the
   locking/syscall hazard it actually is. Scored strictly by category, this
   is a miss, and it is a real limitation of name-based detection, not a
   contrived one.
3. 18 clean functions with one benign callback-style indirect call each
   (a qsort-style comparator invoked through a function pointer) -- no
   named hazard anywhere in the function, but the same "any indirect call
   is flagged as virtual dispatch" rule that catches group 1's genuine
   virtual-dispatch cases also fires here, which is the honest source of
   this analysis's false positives. Plus 20 fully clean functions (no
   calls, or only benign direct calls) that should produce zero findings,
   included for corpus realism rather than to move any metric.
"""
import json
import sys

HEAP_SYMS = ["malloc", "calloc", "realloc", "aligned_alloc", "posix_memalign"]
LOCK_SYMS = [
    "pthread_mutex_lock",
    "pthread_mutex_trylock",
    "pthread_rwlock_rdlock",
    "pthread_rwlock_wrlock",
    "pthread_spin_lock",
    "sem_wait",
]
SYSCALL_SYMS = [
    "read", "write", "recv", "send", "recvfrom", "sendto", "accept", "connect",
    "sleep", "usleep", "nanosleep", "poll", "select", "waitpid", "fopen",
    "fread", "fwrite",
]

PROTOS = """// Declarations only: this corpus is compiled to LLVM IR for static
// analysis and is never linked or executed, so these do not need real
// implementations or even real libc signatures beyond matching arity.
#include <stddef.h>
void *malloc(size_t);
void *calloc(size_t, size_t);
void *realloc(void *, size_t);
void *aligned_alloc(size_t, size_t);
int posix_memalign(void **, size_t, size_t);
typedef struct { int opaque; } pthread_mutex_t;
typedef struct { int opaque; } pthread_rwlock_t;
typedef struct { int opaque; } pthread_spinlock_t;
typedef struct { int opaque; } sem_t;
int pthread_mutex_lock(pthread_mutex_t *);
int pthread_mutex_trylock(pthread_mutex_t *);
int pthread_rwlock_rdlock(pthread_rwlock_t *);
int pthread_rwlock_wrlock(pthread_rwlock_t *);
int pthread_spin_lock(pthread_spinlock_t *);
int sem_wait(sem_t *);
long read(int, void *, size_t);
long write(int, const void *, size_t);
long recv(int, void *, size_t, int);
long send(int, const void *, size_t, int);
long recvfrom(int, void *, size_t, int, void *, unsigned int *);
long sendto(int, const void *, size_t, int, const void *, unsigned int);
int accept(int, void *, unsigned int *);
int connect(int, const void *, unsigned int);
unsigned int sleep(unsigned int);
int usleep(unsigned int);
struct timespec_ { long tv_sec; long tv_nsec; };
int nanosleep(const struct timespec_ *, struct timespec_ *);
int poll(void *, unsigned long, int);
int select(int, void *, void *, void *, void *);
int waitpid(int, int *, int);
void *fopen(const char *, const char *);
size_t fread(void *, size_t, size_t, void *);
size_t fwrite(const void *, size_t, size_t, void *);

pthread_mutex_t g_mu;
pthread_rwlock_t g_rw;
pthread_spinlock_t g_spin;
sem_t g_sem;

static int cmp_asc(const void *a, const void *b) {
    return *(const int *)a - *(const int *)b;
}
typedef int (*cmp_fn)(const void *, const void *);
typedef int (*lock_like_fn)(void *);

static int helper_add(int a, int b) { return a + b; }
"""

HOT = "__attribute__((hot)) void "


def direct_heap(name, sym, idx):
    if sym == "calloc":
        return f"{HOT}{name}(void) {{ void *p = calloc({16 + idx % 8}, 8); (void)p; }}\n"
    if sym == "realloc":
        return f"{HOT}{name}(void) {{ void *p = realloc((void *)0, {64 + idx}); (void)p; }}\n"
    if sym == "aligned_alloc":
        return f"{HOT}{name}(void) {{ void *p = aligned_alloc(16, {64 + idx}); (void)p; }}\n"
    if sym == "posix_memalign":
        return f"{HOT}{name}(void) {{ void *slot = (void *)0; void **out = &slot; int r = posix_memalign(out, 16, 32); (void)r; }}\n"
    return f"{HOT}{name}(void) {{ void *p = malloc({64 + idx}); (void)p; }}\n"


def direct_lock(name, sym, idx):
    arg = "&g_mu" if "mutex" in sym else ("&g_rw" if "rwlock" in sym else ("&g_spin" if "spin" in sym else "&g_sem"))
    return f"{HOT}{name}(void) {{ int r = {sym}({arg}); (void)r; }}\n"


def direct_syscall(name, sym, idx):
    calls = {
        "read": "read(0, (void*)0, 1)",
        "write": "write(1, (const void*)0, 1)",
        "recv": "recv(0, (void*)0, 1, 0)",
        "send": "send(0, (const void*)0, 1, 0)",
        "recvfrom": "recvfrom(0, (void*)0, 1, 0, (void*)0, (unsigned int*)0)",
        "sendto": "sendto(0, (const void*)0, 1, 0, (const void*)0, 0)",
        "accept": "accept(0, (void*)0, (unsigned int*)0)",
        "connect": "connect(0, (const void*)0, 0)",
        "sleep": "sleep(1)",
        "usleep": "usleep(1000)",
        "nanosleep": "nanosleep((const struct timespec_*)0, (struct timespec_*)0)",
        "poll": "poll((void*)0, 0, 0)",
        "select": "select(0, (void*)0, (void*)0, (void*)0, (void*)0)",
        "waitpid": "waitpid(-1, (int*)0, 1)",
        "fopen": 'fopen("x", "r")',
        "fread": "fread((void*)0, 1, 1, (void*)0)",
        "fwrite": "fwrite((const void*)0, 1, 1, (void*)0)",
    }
    expr = calls[sym]
    rtype = "void *" if sym == "fopen" else ("size_t" if sym in ("fread", "fwrite") else "long")
    return f"{HOT}{name}(void) {{ {rtype} r = ({rtype})({expr}); (void)r; }}\n"


def direct_virtual(name, idx):
    return (
        f"{HOT}{name}(void) {{\n"
        f"    cmp_fn fp = cmp_asc;\n"
        f"    int a = {idx}, b = {idx + 1};\n"
        f"    int r = fp(&a, &b);\n"
        f"    (void)r;\n"
        f"}}\n"
    )


def redirect_lock(name, sym, idx):
    arg = "&g_mu" if "mutex" in sym else ("&g_rw" if "rwlock" in sym else "&g_spin")
    return (
        f"{HOT}{name}(void) {{\n"
        f"    lock_like_fn fp = (lock_like_fn){sym};\n"
        f"    int r = fp((void *){arg});\n"
        f"    (void)r;\n"
        f"}}\n"
    )


def redirect_syscall(name, sym, idx):
    # A handful of syscalls invoked through a generically-typed function
    # pointer -- the hazard is real but by-name detection cannot see
    # through the indirection. See module docstring, group 2.
    return (
        f"{HOT}{name}(void) {{\n"
        f"    long (*fp)(int) = (long (*)(int))sleep;\n"
        f"    long r = fp(1);\n"
        f"    (void)r;\n"
        f"}}\n"
    )


def clean_callback(name, idx):
    # Marked hot on purpose: the false-positive this corpus exercises is a
    # *hot* function making a legitimate, benign callback, which is exactly
    # the shape the pass cannot tell apart from a real virtual call. A cold
    # function with the same callback would never even reach the pass (it
    # is gated on Attribute::Hot), so it would not test anything.
    return (
        f"{HOT}{name}(void) {{\n"
        f"    cmp_fn fp = cmp_asc;\n"
        f"    int a = {idx}, b = {idx - 1};\n"
        f"    int r = fp(&a, &b);\n"
        f"    (void)r;\n"
        f"}}\n"
    )


def clean_plain(name, idx):
    return f"void {name}(void) {{ int r = helper_add({idx}, {idx + 1}); (void)r; }}\n"


def main():
    out_c = sys.argv[1] if len(sys.argv) > 1 else "kernels/hazard_corpus.c"
    out_manifest = sys.argv[2] if len(sys.argv) > 2 else "docs/hazard_manifest.json"

    funcs = []
    manifest = []  # list of {name, kind: seeded|clean_fp|clean, true_category}

    def add_group(count, syms, builder, true_category, tag):
        for i in range(count):
            sym = syms[i % len(syms)]
            name = f"hot_{tag}_{i:03d}"
            funcs.append(builder(name, sym, i))
            manifest.append({"name": name, "kind": "seeded", "true_category": true_category, "symbol": sym})

    add_group(60, HEAP_SYMS, direct_heap, "heap_alloc", "heap")
    add_group(60, LOCK_SYMS, direct_lock, "locking", "lock")
    add_group(60, SYSCALL_SYMS, direct_syscall, "blocking_syscall", "syscall")
    for i in range(60):
        name = f"hot_virtual_{i:03d}"
        funcs.append(direct_virtual(name, i))
        manifest.append({"name": name, "kind": "seeded", "true_category": "virtual_dispatch", "symbol": "cmp_asc"})

    # Group 2: redirected locking/syscall hazards (5 + 5).
    for i in range(5):
        sym = LOCK_SYMS[i % len(LOCK_SYMS)]
        name = f"hot_lockredirect_{i:03d}"
        funcs.append(redirect_lock(name, sym, i))
        manifest.append({"name": name, "kind": "seeded", "true_category": "locking", "symbol": sym, "redirected": True})
    for i in range(5):
        name = f"hot_syscallredirect_{i:03d}"
        funcs.append(redirect_syscall(name, "sleep", i))
        manifest.append({"name": name, "kind": "seeded", "true_category": "blocking_syscall", "symbol": "sleep", "redirected": True})

    # Group 3: clean functions.
    for i in range(18):
        name = f"clean_fp_{i:03d}"
        funcs.append(clean_callback(name, i + 10))
        manifest.append({"name": name, "kind": "clean_fp", "true_category": None})
    for i in range(20):
        name = f"clean_plain_{i:03d}"
        funcs.append(clean_plain(name, i))
        manifest.append({"name": name, "kind": "clean", "true_category": None})

    with open(out_c, "w", newline="\n") as f:
        f.write(PROTOS)
        f.write("\n")
        f.write("\n".join(funcs))
        f.write("\n")

    with open(out_manifest, "w", newline="\n") as f:
        json.dump(manifest, f, indent=2)

    seeded = sum(1 for m in manifest if m["kind"] == "seeded")
    print(f"wrote {len(funcs)} functions ({seeded} seeded hazards) to {out_c}")
    print(f"wrote manifest with {len(manifest)} entries to {out_manifest}")


if __name__ == "__main__":
    main()

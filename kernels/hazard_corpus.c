// Declarations only: this corpus is compiled to LLVM IR for static
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

__attribute__((hot)) void hot_heap_000(void) { void *p = malloc(64); (void)p; }

__attribute__((hot)) void hot_heap_001(void) { void *p = calloc(17, 8); (void)p; }

__attribute__((hot)) void hot_heap_002(void) { void *p = realloc((void *)0, 66); (void)p; }

__attribute__((hot)) void hot_heap_003(void) { void *p = aligned_alloc(16, 67); (void)p; }

__attribute__((hot)) void hot_heap_004(void) { void *slot = (void *)0; void **out = &slot; int r = posix_memalign(out, 16, 32); (void)r; }

__attribute__((hot)) void hot_heap_005(void) { void *p = malloc(69); (void)p; }

__attribute__((hot)) void hot_heap_006(void) { void *p = calloc(22, 8); (void)p; }

__attribute__((hot)) void hot_heap_007(void) { void *p = realloc((void *)0, 71); (void)p; }

__attribute__((hot)) void hot_heap_008(void) { void *p = aligned_alloc(16, 72); (void)p; }

__attribute__((hot)) void hot_heap_009(void) { void *slot = (void *)0; void **out = &slot; int r = posix_memalign(out, 16, 32); (void)r; }

__attribute__((hot)) void hot_heap_010(void) { void *p = malloc(74); (void)p; }

__attribute__((hot)) void hot_heap_011(void) { void *p = calloc(19, 8); (void)p; }

__attribute__((hot)) void hot_heap_012(void) { void *p = realloc((void *)0, 76); (void)p; }

__attribute__((hot)) void hot_heap_013(void) { void *p = aligned_alloc(16, 77); (void)p; }

__attribute__((hot)) void hot_heap_014(void) { void *slot = (void *)0; void **out = &slot; int r = posix_memalign(out, 16, 32); (void)r; }

__attribute__((hot)) void hot_heap_015(void) { void *p = malloc(79); (void)p; }

__attribute__((hot)) void hot_heap_016(void) { void *p = calloc(16, 8); (void)p; }

__attribute__((hot)) void hot_heap_017(void) { void *p = realloc((void *)0, 81); (void)p; }

__attribute__((hot)) void hot_heap_018(void) { void *p = aligned_alloc(16, 82); (void)p; }

__attribute__((hot)) void hot_heap_019(void) { void *slot = (void *)0; void **out = &slot; int r = posix_memalign(out, 16, 32); (void)r; }

__attribute__((hot)) void hot_heap_020(void) { void *p = malloc(84); (void)p; }

__attribute__((hot)) void hot_heap_021(void) { void *p = calloc(21, 8); (void)p; }

__attribute__((hot)) void hot_heap_022(void) { void *p = realloc((void *)0, 86); (void)p; }

__attribute__((hot)) void hot_heap_023(void) { void *p = aligned_alloc(16, 87); (void)p; }

__attribute__((hot)) void hot_heap_024(void) { void *slot = (void *)0; void **out = &slot; int r = posix_memalign(out, 16, 32); (void)r; }

__attribute__((hot)) void hot_heap_025(void) { void *p = malloc(89); (void)p; }

__attribute__((hot)) void hot_heap_026(void) { void *p = calloc(18, 8); (void)p; }

__attribute__((hot)) void hot_heap_027(void) { void *p = realloc((void *)0, 91); (void)p; }

__attribute__((hot)) void hot_heap_028(void) { void *p = aligned_alloc(16, 92); (void)p; }

__attribute__((hot)) void hot_heap_029(void) { void *slot = (void *)0; void **out = &slot; int r = posix_memalign(out, 16, 32); (void)r; }

__attribute__((hot)) void hot_heap_030(void) { void *p = malloc(94); (void)p; }

__attribute__((hot)) void hot_heap_031(void) { void *p = calloc(23, 8); (void)p; }

__attribute__((hot)) void hot_heap_032(void) { void *p = realloc((void *)0, 96); (void)p; }

__attribute__((hot)) void hot_heap_033(void) { void *p = aligned_alloc(16, 97); (void)p; }

__attribute__((hot)) void hot_heap_034(void) { void *slot = (void *)0; void **out = &slot; int r = posix_memalign(out, 16, 32); (void)r; }

__attribute__((hot)) void hot_heap_035(void) { void *p = malloc(99); (void)p; }

__attribute__((hot)) void hot_heap_036(void) { void *p = calloc(20, 8); (void)p; }

__attribute__((hot)) void hot_heap_037(void) { void *p = realloc((void *)0, 101); (void)p; }

__attribute__((hot)) void hot_heap_038(void) { void *p = aligned_alloc(16, 102); (void)p; }

__attribute__((hot)) void hot_heap_039(void) { void *slot = (void *)0; void **out = &slot; int r = posix_memalign(out, 16, 32); (void)r; }

__attribute__((hot)) void hot_heap_040(void) { void *p = malloc(104); (void)p; }

__attribute__((hot)) void hot_heap_041(void) { void *p = calloc(17, 8); (void)p; }

__attribute__((hot)) void hot_heap_042(void) { void *p = realloc((void *)0, 106); (void)p; }

__attribute__((hot)) void hot_heap_043(void) { void *p = aligned_alloc(16, 107); (void)p; }

__attribute__((hot)) void hot_heap_044(void) { void *slot = (void *)0; void **out = &slot; int r = posix_memalign(out, 16, 32); (void)r; }

__attribute__((hot)) void hot_heap_045(void) { void *p = malloc(109); (void)p; }

__attribute__((hot)) void hot_heap_046(void) { void *p = calloc(22, 8); (void)p; }

__attribute__((hot)) void hot_heap_047(void) { void *p = realloc((void *)0, 111); (void)p; }

__attribute__((hot)) void hot_heap_048(void) { void *p = aligned_alloc(16, 112); (void)p; }

__attribute__((hot)) void hot_heap_049(void) { void *slot = (void *)0; void **out = &slot; int r = posix_memalign(out, 16, 32); (void)r; }

__attribute__((hot)) void hot_heap_050(void) { void *p = malloc(114); (void)p; }

__attribute__((hot)) void hot_heap_051(void) { void *p = calloc(19, 8); (void)p; }

__attribute__((hot)) void hot_heap_052(void) { void *p = realloc((void *)0, 116); (void)p; }

__attribute__((hot)) void hot_heap_053(void) { void *p = aligned_alloc(16, 117); (void)p; }

__attribute__((hot)) void hot_heap_054(void) { void *slot = (void *)0; void **out = &slot; int r = posix_memalign(out, 16, 32); (void)r; }

__attribute__((hot)) void hot_heap_055(void) { void *p = malloc(119); (void)p; }

__attribute__((hot)) void hot_heap_056(void) { void *p = calloc(16, 8); (void)p; }

__attribute__((hot)) void hot_heap_057(void) { void *p = realloc((void *)0, 121); (void)p; }

__attribute__((hot)) void hot_heap_058(void) { void *p = aligned_alloc(16, 122); (void)p; }

__attribute__((hot)) void hot_heap_059(void) { void *slot = (void *)0; void **out = &slot; int r = posix_memalign(out, 16, 32); (void)r; }

__attribute__((hot)) void hot_lock_000(void) { int r = pthread_mutex_lock(&g_mu); (void)r; }

__attribute__((hot)) void hot_lock_001(void) { int r = pthread_mutex_trylock(&g_mu); (void)r; }

__attribute__((hot)) void hot_lock_002(void) { int r = pthread_rwlock_rdlock(&g_rw); (void)r; }

__attribute__((hot)) void hot_lock_003(void) { int r = pthread_rwlock_wrlock(&g_rw); (void)r; }

__attribute__((hot)) void hot_lock_004(void) { int r = pthread_spin_lock(&g_spin); (void)r; }

__attribute__((hot)) void hot_lock_005(void) { int r = sem_wait(&g_sem); (void)r; }

__attribute__((hot)) void hot_lock_006(void) { int r = pthread_mutex_lock(&g_mu); (void)r; }

__attribute__((hot)) void hot_lock_007(void) { int r = pthread_mutex_trylock(&g_mu); (void)r; }

__attribute__((hot)) void hot_lock_008(void) { int r = pthread_rwlock_rdlock(&g_rw); (void)r; }

__attribute__((hot)) void hot_lock_009(void) { int r = pthread_rwlock_wrlock(&g_rw); (void)r; }

__attribute__((hot)) void hot_lock_010(void) { int r = pthread_spin_lock(&g_spin); (void)r; }

__attribute__((hot)) void hot_lock_011(void) { int r = sem_wait(&g_sem); (void)r; }

__attribute__((hot)) void hot_lock_012(void) { int r = pthread_mutex_lock(&g_mu); (void)r; }

__attribute__((hot)) void hot_lock_013(void) { int r = pthread_mutex_trylock(&g_mu); (void)r; }

__attribute__((hot)) void hot_lock_014(void) { int r = pthread_rwlock_rdlock(&g_rw); (void)r; }

__attribute__((hot)) void hot_lock_015(void) { int r = pthread_rwlock_wrlock(&g_rw); (void)r; }

__attribute__((hot)) void hot_lock_016(void) { int r = pthread_spin_lock(&g_spin); (void)r; }

__attribute__((hot)) void hot_lock_017(void) { int r = sem_wait(&g_sem); (void)r; }

__attribute__((hot)) void hot_lock_018(void) { int r = pthread_mutex_lock(&g_mu); (void)r; }

__attribute__((hot)) void hot_lock_019(void) { int r = pthread_mutex_trylock(&g_mu); (void)r; }

__attribute__((hot)) void hot_lock_020(void) { int r = pthread_rwlock_rdlock(&g_rw); (void)r; }

__attribute__((hot)) void hot_lock_021(void) { int r = pthread_rwlock_wrlock(&g_rw); (void)r; }

__attribute__((hot)) void hot_lock_022(void) { int r = pthread_spin_lock(&g_spin); (void)r; }

__attribute__((hot)) void hot_lock_023(void) { int r = sem_wait(&g_sem); (void)r; }

__attribute__((hot)) void hot_lock_024(void) { int r = pthread_mutex_lock(&g_mu); (void)r; }

__attribute__((hot)) void hot_lock_025(void) { int r = pthread_mutex_trylock(&g_mu); (void)r; }

__attribute__((hot)) void hot_lock_026(void) { int r = pthread_rwlock_rdlock(&g_rw); (void)r; }

__attribute__((hot)) void hot_lock_027(void) { int r = pthread_rwlock_wrlock(&g_rw); (void)r; }

__attribute__((hot)) void hot_lock_028(void) { int r = pthread_spin_lock(&g_spin); (void)r; }

__attribute__((hot)) void hot_lock_029(void) { int r = sem_wait(&g_sem); (void)r; }

__attribute__((hot)) void hot_lock_030(void) { int r = pthread_mutex_lock(&g_mu); (void)r; }

__attribute__((hot)) void hot_lock_031(void) { int r = pthread_mutex_trylock(&g_mu); (void)r; }

__attribute__((hot)) void hot_lock_032(void) { int r = pthread_rwlock_rdlock(&g_rw); (void)r; }

__attribute__((hot)) void hot_lock_033(void) { int r = pthread_rwlock_wrlock(&g_rw); (void)r; }

__attribute__((hot)) void hot_lock_034(void) { int r = pthread_spin_lock(&g_spin); (void)r; }

__attribute__((hot)) void hot_lock_035(void) { int r = sem_wait(&g_sem); (void)r; }

__attribute__((hot)) void hot_lock_036(void) { int r = pthread_mutex_lock(&g_mu); (void)r; }

__attribute__((hot)) void hot_lock_037(void) { int r = pthread_mutex_trylock(&g_mu); (void)r; }

__attribute__((hot)) void hot_lock_038(void) { int r = pthread_rwlock_rdlock(&g_rw); (void)r; }

__attribute__((hot)) void hot_lock_039(void) { int r = pthread_rwlock_wrlock(&g_rw); (void)r; }

__attribute__((hot)) void hot_lock_040(void) { int r = pthread_spin_lock(&g_spin); (void)r; }

__attribute__((hot)) void hot_lock_041(void) { int r = sem_wait(&g_sem); (void)r; }

__attribute__((hot)) void hot_lock_042(void) { int r = pthread_mutex_lock(&g_mu); (void)r; }

__attribute__((hot)) void hot_lock_043(void) { int r = pthread_mutex_trylock(&g_mu); (void)r; }

__attribute__((hot)) void hot_lock_044(void) { int r = pthread_rwlock_rdlock(&g_rw); (void)r; }

__attribute__((hot)) void hot_lock_045(void) { int r = pthread_rwlock_wrlock(&g_rw); (void)r; }

__attribute__((hot)) void hot_lock_046(void) { int r = pthread_spin_lock(&g_spin); (void)r; }

__attribute__((hot)) void hot_lock_047(void) { int r = sem_wait(&g_sem); (void)r; }

__attribute__((hot)) void hot_lock_048(void) { int r = pthread_mutex_lock(&g_mu); (void)r; }

__attribute__((hot)) void hot_lock_049(void) { int r = pthread_mutex_trylock(&g_mu); (void)r; }

__attribute__((hot)) void hot_lock_050(void) { int r = pthread_rwlock_rdlock(&g_rw); (void)r; }

__attribute__((hot)) void hot_lock_051(void) { int r = pthread_rwlock_wrlock(&g_rw); (void)r; }

__attribute__((hot)) void hot_lock_052(void) { int r = pthread_spin_lock(&g_spin); (void)r; }

__attribute__((hot)) void hot_lock_053(void) { int r = sem_wait(&g_sem); (void)r; }

__attribute__((hot)) void hot_lock_054(void) { int r = pthread_mutex_lock(&g_mu); (void)r; }

__attribute__((hot)) void hot_lock_055(void) { int r = pthread_mutex_trylock(&g_mu); (void)r; }

__attribute__((hot)) void hot_lock_056(void) { int r = pthread_rwlock_rdlock(&g_rw); (void)r; }

__attribute__((hot)) void hot_lock_057(void) { int r = pthread_rwlock_wrlock(&g_rw); (void)r; }

__attribute__((hot)) void hot_lock_058(void) { int r = pthread_spin_lock(&g_spin); (void)r; }

__attribute__((hot)) void hot_lock_059(void) { int r = sem_wait(&g_sem); (void)r; }

__attribute__((hot)) void hot_syscall_000(void) { long r = (long)(read(0, (void*)0, 1)); (void)r; }

__attribute__((hot)) void hot_syscall_001(void) { long r = (long)(write(1, (const void*)0, 1)); (void)r; }

__attribute__((hot)) void hot_syscall_002(void) { long r = (long)(recv(0, (void*)0, 1, 0)); (void)r; }

__attribute__((hot)) void hot_syscall_003(void) { long r = (long)(send(0, (const void*)0, 1, 0)); (void)r; }

__attribute__((hot)) void hot_syscall_004(void) { long r = (long)(recvfrom(0, (void*)0, 1, 0, (void*)0, (unsigned int*)0)); (void)r; }

__attribute__((hot)) void hot_syscall_005(void) { long r = (long)(sendto(0, (const void*)0, 1, 0, (const void*)0, 0)); (void)r; }

__attribute__((hot)) void hot_syscall_006(void) { long r = (long)(accept(0, (void*)0, (unsigned int*)0)); (void)r; }

__attribute__((hot)) void hot_syscall_007(void) { long r = (long)(connect(0, (const void*)0, 0)); (void)r; }

__attribute__((hot)) void hot_syscall_008(void) { long r = (long)(sleep(1)); (void)r; }

__attribute__((hot)) void hot_syscall_009(void) { long r = (long)(usleep(1000)); (void)r; }

__attribute__((hot)) void hot_syscall_010(void) { long r = (long)(nanosleep((const struct timespec_*)0, (struct timespec_*)0)); (void)r; }

__attribute__((hot)) void hot_syscall_011(void) { long r = (long)(poll((void*)0, 0, 0)); (void)r; }

__attribute__((hot)) void hot_syscall_012(void) { long r = (long)(select(0, (void*)0, (void*)0, (void*)0, (void*)0)); (void)r; }

__attribute__((hot)) void hot_syscall_013(void) { long r = (long)(waitpid(-1, (int*)0, 1)); (void)r; }

__attribute__((hot)) void hot_syscall_014(void) { void * r = (void *)(fopen("x", "r")); (void)r; }

__attribute__((hot)) void hot_syscall_015(void) { size_t r = (size_t)(fread((void*)0, 1, 1, (void*)0)); (void)r; }

__attribute__((hot)) void hot_syscall_016(void) { size_t r = (size_t)(fwrite((const void*)0, 1, 1, (void*)0)); (void)r; }

__attribute__((hot)) void hot_syscall_017(void) { long r = (long)(read(0, (void*)0, 1)); (void)r; }

__attribute__((hot)) void hot_syscall_018(void) { long r = (long)(write(1, (const void*)0, 1)); (void)r; }

__attribute__((hot)) void hot_syscall_019(void) { long r = (long)(recv(0, (void*)0, 1, 0)); (void)r; }

__attribute__((hot)) void hot_syscall_020(void) { long r = (long)(send(0, (const void*)0, 1, 0)); (void)r; }

__attribute__((hot)) void hot_syscall_021(void) { long r = (long)(recvfrom(0, (void*)0, 1, 0, (void*)0, (unsigned int*)0)); (void)r; }

__attribute__((hot)) void hot_syscall_022(void) { long r = (long)(sendto(0, (const void*)0, 1, 0, (const void*)0, 0)); (void)r; }

__attribute__((hot)) void hot_syscall_023(void) { long r = (long)(accept(0, (void*)0, (unsigned int*)0)); (void)r; }

__attribute__((hot)) void hot_syscall_024(void) { long r = (long)(connect(0, (const void*)0, 0)); (void)r; }

__attribute__((hot)) void hot_syscall_025(void) { long r = (long)(sleep(1)); (void)r; }

__attribute__((hot)) void hot_syscall_026(void) { long r = (long)(usleep(1000)); (void)r; }

__attribute__((hot)) void hot_syscall_027(void) { long r = (long)(nanosleep((const struct timespec_*)0, (struct timespec_*)0)); (void)r; }

__attribute__((hot)) void hot_syscall_028(void) { long r = (long)(poll((void*)0, 0, 0)); (void)r; }

__attribute__((hot)) void hot_syscall_029(void) { long r = (long)(select(0, (void*)0, (void*)0, (void*)0, (void*)0)); (void)r; }

__attribute__((hot)) void hot_syscall_030(void) { long r = (long)(waitpid(-1, (int*)0, 1)); (void)r; }

__attribute__((hot)) void hot_syscall_031(void) { void * r = (void *)(fopen("x", "r")); (void)r; }

__attribute__((hot)) void hot_syscall_032(void) { size_t r = (size_t)(fread((void*)0, 1, 1, (void*)0)); (void)r; }

__attribute__((hot)) void hot_syscall_033(void) { size_t r = (size_t)(fwrite((const void*)0, 1, 1, (void*)0)); (void)r; }

__attribute__((hot)) void hot_syscall_034(void) { long r = (long)(read(0, (void*)0, 1)); (void)r; }

__attribute__((hot)) void hot_syscall_035(void) { long r = (long)(write(1, (const void*)0, 1)); (void)r; }

__attribute__((hot)) void hot_syscall_036(void) { long r = (long)(recv(0, (void*)0, 1, 0)); (void)r; }

__attribute__((hot)) void hot_syscall_037(void) { long r = (long)(send(0, (const void*)0, 1, 0)); (void)r; }

__attribute__((hot)) void hot_syscall_038(void) { long r = (long)(recvfrom(0, (void*)0, 1, 0, (void*)0, (unsigned int*)0)); (void)r; }

__attribute__((hot)) void hot_syscall_039(void) { long r = (long)(sendto(0, (const void*)0, 1, 0, (const void*)0, 0)); (void)r; }

__attribute__((hot)) void hot_syscall_040(void) { long r = (long)(accept(0, (void*)0, (unsigned int*)0)); (void)r; }

__attribute__((hot)) void hot_syscall_041(void) { long r = (long)(connect(0, (const void*)0, 0)); (void)r; }

__attribute__((hot)) void hot_syscall_042(void) { long r = (long)(sleep(1)); (void)r; }

__attribute__((hot)) void hot_syscall_043(void) { long r = (long)(usleep(1000)); (void)r; }

__attribute__((hot)) void hot_syscall_044(void) { long r = (long)(nanosleep((const struct timespec_*)0, (struct timespec_*)0)); (void)r; }

__attribute__((hot)) void hot_syscall_045(void) { long r = (long)(poll((void*)0, 0, 0)); (void)r; }

__attribute__((hot)) void hot_syscall_046(void) { long r = (long)(select(0, (void*)0, (void*)0, (void*)0, (void*)0)); (void)r; }

__attribute__((hot)) void hot_syscall_047(void) { long r = (long)(waitpid(-1, (int*)0, 1)); (void)r; }

__attribute__((hot)) void hot_syscall_048(void) { void * r = (void *)(fopen("x", "r")); (void)r; }

__attribute__((hot)) void hot_syscall_049(void) { size_t r = (size_t)(fread((void*)0, 1, 1, (void*)0)); (void)r; }

__attribute__((hot)) void hot_syscall_050(void) { size_t r = (size_t)(fwrite((const void*)0, 1, 1, (void*)0)); (void)r; }

__attribute__((hot)) void hot_syscall_051(void) { long r = (long)(read(0, (void*)0, 1)); (void)r; }

__attribute__((hot)) void hot_syscall_052(void) { long r = (long)(write(1, (const void*)0, 1)); (void)r; }

__attribute__((hot)) void hot_syscall_053(void) { long r = (long)(recv(0, (void*)0, 1, 0)); (void)r; }

__attribute__((hot)) void hot_syscall_054(void) { long r = (long)(send(0, (const void*)0, 1, 0)); (void)r; }

__attribute__((hot)) void hot_syscall_055(void) { long r = (long)(recvfrom(0, (void*)0, 1, 0, (void*)0, (unsigned int*)0)); (void)r; }

__attribute__((hot)) void hot_syscall_056(void) { long r = (long)(sendto(0, (const void*)0, 1, 0, (const void*)0, 0)); (void)r; }

__attribute__((hot)) void hot_syscall_057(void) { long r = (long)(accept(0, (void*)0, (unsigned int*)0)); (void)r; }

__attribute__((hot)) void hot_syscall_058(void) { long r = (long)(connect(0, (const void*)0, 0)); (void)r; }

__attribute__((hot)) void hot_syscall_059(void) { long r = (long)(sleep(1)); (void)r; }

__attribute__((hot)) void hot_virtual_000(void) {
    cmp_fn fp = cmp_asc;
    int a = 0, b = 1;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_001(void) {
    cmp_fn fp = cmp_asc;
    int a = 1, b = 2;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_002(void) {
    cmp_fn fp = cmp_asc;
    int a = 2, b = 3;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_003(void) {
    cmp_fn fp = cmp_asc;
    int a = 3, b = 4;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_004(void) {
    cmp_fn fp = cmp_asc;
    int a = 4, b = 5;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_005(void) {
    cmp_fn fp = cmp_asc;
    int a = 5, b = 6;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_006(void) {
    cmp_fn fp = cmp_asc;
    int a = 6, b = 7;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_007(void) {
    cmp_fn fp = cmp_asc;
    int a = 7, b = 8;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_008(void) {
    cmp_fn fp = cmp_asc;
    int a = 8, b = 9;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_009(void) {
    cmp_fn fp = cmp_asc;
    int a = 9, b = 10;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_010(void) {
    cmp_fn fp = cmp_asc;
    int a = 10, b = 11;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_011(void) {
    cmp_fn fp = cmp_asc;
    int a = 11, b = 12;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_012(void) {
    cmp_fn fp = cmp_asc;
    int a = 12, b = 13;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_013(void) {
    cmp_fn fp = cmp_asc;
    int a = 13, b = 14;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_014(void) {
    cmp_fn fp = cmp_asc;
    int a = 14, b = 15;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_015(void) {
    cmp_fn fp = cmp_asc;
    int a = 15, b = 16;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_016(void) {
    cmp_fn fp = cmp_asc;
    int a = 16, b = 17;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_017(void) {
    cmp_fn fp = cmp_asc;
    int a = 17, b = 18;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_018(void) {
    cmp_fn fp = cmp_asc;
    int a = 18, b = 19;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_019(void) {
    cmp_fn fp = cmp_asc;
    int a = 19, b = 20;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_020(void) {
    cmp_fn fp = cmp_asc;
    int a = 20, b = 21;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_021(void) {
    cmp_fn fp = cmp_asc;
    int a = 21, b = 22;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_022(void) {
    cmp_fn fp = cmp_asc;
    int a = 22, b = 23;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_023(void) {
    cmp_fn fp = cmp_asc;
    int a = 23, b = 24;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_024(void) {
    cmp_fn fp = cmp_asc;
    int a = 24, b = 25;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_025(void) {
    cmp_fn fp = cmp_asc;
    int a = 25, b = 26;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_026(void) {
    cmp_fn fp = cmp_asc;
    int a = 26, b = 27;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_027(void) {
    cmp_fn fp = cmp_asc;
    int a = 27, b = 28;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_028(void) {
    cmp_fn fp = cmp_asc;
    int a = 28, b = 29;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_029(void) {
    cmp_fn fp = cmp_asc;
    int a = 29, b = 30;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_030(void) {
    cmp_fn fp = cmp_asc;
    int a = 30, b = 31;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_031(void) {
    cmp_fn fp = cmp_asc;
    int a = 31, b = 32;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_032(void) {
    cmp_fn fp = cmp_asc;
    int a = 32, b = 33;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_033(void) {
    cmp_fn fp = cmp_asc;
    int a = 33, b = 34;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_034(void) {
    cmp_fn fp = cmp_asc;
    int a = 34, b = 35;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_035(void) {
    cmp_fn fp = cmp_asc;
    int a = 35, b = 36;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_036(void) {
    cmp_fn fp = cmp_asc;
    int a = 36, b = 37;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_037(void) {
    cmp_fn fp = cmp_asc;
    int a = 37, b = 38;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_038(void) {
    cmp_fn fp = cmp_asc;
    int a = 38, b = 39;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_039(void) {
    cmp_fn fp = cmp_asc;
    int a = 39, b = 40;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_040(void) {
    cmp_fn fp = cmp_asc;
    int a = 40, b = 41;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_041(void) {
    cmp_fn fp = cmp_asc;
    int a = 41, b = 42;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_042(void) {
    cmp_fn fp = cmp_asc;
    int a = 42, b = 43;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_043(void) {
    cmp_fn fp = cmp_asc;
    int a = 43, b = 44;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_044(void) {
    cmp_fn fp = cmp_asc;
    int a = 44, b = 45;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_045(void) {
    cmp_fn fp = cmp_asc;
    int a = 45, b = 46;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_046(void) {
    cmp_fn fp = cmp_asc;
    int a = 46, b = 47;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_047(void) {
    cmp_fn fp = cmp_asc;
    int a = 47, b = 48;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_048(void) {
    cmp_fn fp = cmp_asc;
    int a = 48, b = 49;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_049(void) {
    cmp_fn fp = cmp_asc;
    int a = 49, b = 50;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_050(void) {
    cmp_fn fp = cmp_asc;
    int a = 50, b = 51;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_051(void) {
    cmp_fn fp = cmp_asc;
    int a = 51, b = 52;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_052(void) {
    cmp_fn fp = cmp_asc;
    int a = 52, b = 53;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_053(void) {
    cmp_fn fp = cmp_asc;
    int a = 53, b = 54;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_054(void) {
    cmp_fn fp = cmp_asc;
    int a = 54, b = 55;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_055(void) {
    cmp_fn fp = cmp_asc;
    int a = 55, b = 56;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_056(void) {
    cmp_fn fp = cmp_asc;
    int a = 56, b = 57;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_057(void) {
    cmp_fn fp = cmp_asc;
    int a = 57, b = 58;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_058(void) {
    cmp_fn fp = cmp_asc;
    int a = 58, b = 59;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_virtual_059(void) {
    cmp_fn fp = cmp_asc;
    int a = 59, b = 60;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void hot_lockredirect_000(void) {
    lock_like_fn fp = (lock_like_fn)pthread_mutex_lock;
    int r = fp((void *)&g_mu);
    (void)r;
}

__attribute__((hot)) void hot_lockredirect_001(void) {
    lock_like_fn fp = (lock_like_fn)pthread_mutex_trylock;
    int r = fp((void *)&g_mu);
    (void)r;
}

__attribute__((hot)) void hot_lockredirect_002(void) {
    lock_like_fn fp = (lock_like_fn)pthread_rwlock_rdlock;
    int r = fp((void *)&g_rw);
    (void)r;
}

__attribute__((hot)) void hot_lockredirect_003(void) {
    lock_like_fn fp = (lock_like_fn)pthread_rwlock_wrlock;
    int r = fp((void *)&g_rw);
    (void)r;
}

__attribute__((hot)) void hot_lockredirect_004(void) {
    lock_like_fn fp = (lock_like_fn)pthread_spin_lock;
    int r = fp((void *)&g_spin);
    (void)r;
}

__attribute__((hot)) void hot_syscallredirect_000(void) {
    long (*fp)(int) = (long (*)(int))sleep;
    long r = fp(1);
    (void)r;
}

__attribute__((hot)) void hot_syscallredirect_001(void) {
    long (*fp)(int) = (long (*)(int))sleep;
    long r = fp(1);
    (void)r;
}

__attribute__((hot)) void hot_syscallredirect_002(void) {
    long (*fp)(int) = (long (*)(int))sleep;
    long r = fp(1);
    (void)r;
}

__attribute__((hot)) void hot_syscallredirect_003(void) {
    long (*fp)(int) = (long (*)(int))sleep;
    long r = fp(1);
    (void)r;
}

__attribute__((hot)) void hot_syscallredirect_004(void) {
    long (*fp)(int) = (long (*)(int))sleep;
    long r = fp(1);
    (void)r;
}

__attribute__((hot)) void clean_fp_000(void) {
    cmp_fn fp = cmp_asc;
    int a = 10, b = 9;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void clean_fp_001(void) {
    cmp_fn fp = cmp_asc;
    int a = 11, b = 10;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void clean_fp_002(void) {
    cmp_fn fp = cmp_asc;
    int a = 12, b = 11;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void clean_fp_003(void) {
    cmp_fn fp = cmp_asc;
    int a = 13, b = 12;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void clean_fp_004(void) {
    cmp_fn fp = cmp_asc;
    int a = 14, b = 13;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void clean_fp_005(void) {
    cmp_fn fp = cmp_asc;
    int a = 15, b = 14;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void clean_fp_006(void) {
    cmp_fn fp = cmp_asc;
    int a = 16, b = 15;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void clean_fp_007(void) {
    cmp_fn fp = cmp_asc;
    int a = 17, b = 16;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void clean_fp_008(void) {
    cmp_fn fp = cmp_asc;
    int a = 18, b = 17;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void clean_fp_009(void) {
    cmp_fn fp = cmp_asc;
    int a = 19, b = 18;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void clean_fp_010(void) {
    cmp_fn fp = cmp_asc;
    int a = 20, b = 19;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void clean_fp_011(void) {
    cmp_fn fp = cmp_asc;
    int a = 21, b = 20;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void clean_fp_012(void) {
    cmp_fn fp = cmp_asc;
    int a = 22, b = 21;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void clean_fp_013(void) {
    cmp_fn fp = cmp_asc;
    int a = 23, b = 22;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void clean_fp_014(void) {
    cmp_fn fp = cmp_asc;
    int a = 24, b = 23;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void clean_fp_015(void) {
    cmp_fn fp = cmp_asc;
    int a = 25, b = 24;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void clean_fp_016(void) {
    cmp_fn fp = cmp_asc;
    int a = 26, b = 25;
    int r = fp(&a, &b);
    (void)r;
}

__attribute__((hot)) void clean_fp_017(void) {
    cmp_fn fp = cmp_asc;
    int a = 27, b = 26;
    int r = fp(&a, &b);
    (void)r;
}

void clean_plain_000(void) { int r = helper_add(0, 1); (void)r; }

void clean_plain_001(void) { int r = helper_add(1, 2); (void)r; }

void clean_plain_002(void) { int r = helper_add(2, 3); (void)r; }

void clean_plain_003(void) { int r = helper_add(3, 4); (void)r; }

void clean_plain_004(void) { int r = helper_add(4, 5); (void)r; }

void clean_plain_005(void) { int r = helper_add(5, 6); (void)r; }

void clean_plain_006(void) { int r = helper_add(6, 7); (void)r; }

void clean_plain_007(void) { int r = helper_add(7, 8); (void)r; }

void clean_plain_008(void) { int r = helper_add(8, 9); (void)r; }

void clean_plain_009(void) { int r = helper_add(9, 10); (void)r; }

void clean_plain_010(void) { int r = helper_add(10, 11); (void)r; }

void clean_plain_011(void) { int r = helper_add(11, 12); (void)r; }

void clean_plain_012(void) { int r = helper_add(12, 13); (void)r; }

void clean_plain_013(void) { int r = helper_add(13, 14); (void)r; }

void clean_plain_014(void) { int r = helper_add(14, 15); (void)r; }

void clean_plain_015(void) { int r = helper_add(15, 16); (void)r; }

void clean_plain_016(void) { int r = helper_add(16, 17); (void)r; }

void clean_plain_017(void) { int r = helper_add(17, 18); (void)r; }

void clean_plain_018(void) { int r = helper_add(18, 19); (void)r; }

void clean_plain_019(void) { int r = helper_add(19, 20); (void)r; }


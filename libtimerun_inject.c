#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>

static time_t target_start_sec = 0;
static time_t real_start_sec = 0;
static double speed_factor = 1.0;
static int freeze_flag = 0;
static int fake_mtime_flag = 0;
static int debug_flag = 0;
static int initialized = 0;
static const char *tz_env = "UTC";
static unsigned long long syscall_counter = 0;

static void init_timerun(void) {
    if (initialized) return;
    initialized = 1;

    const char *env_target = getenv("TIMERUN_TARGET_SEC");
    const char *env_speed = getenv("TIMERUN_SPEED");
    const char *env_freeze = getenv("TIMERUN_FREEZE");
    const char *env_mtime = getenv("TIMERUN_FAKE_MTIME");
    const char *env_debug = getenv("TIMERUN_DEBUG");
    const char *env_tz = getenv("TZ");

    if (env_tz) tz_env = env_tz;

    if (env_target) target_start_sec = (time_t)atoll(env_target);
    if (env_speed) speed_factor = atof(env_speed);
    if (env_freeze && strcmp(env_freeze, "1") == 0) freeze_flag = 1;
    if (env_mtime && strcmp(env_mtime, "1") == 0) fake_mtime_flag = 1;
    if (env_debug && strcmp(env_debug, "1") == 0) debug_flag = 1;

    static time_t (*real_time)(time_t *) = NULL;
    if (!real_time) real_time = dlsym(RTLD_NEXT, "time");
    real_start_sec = real_time(NULL);

    if (debug_flag) {
        fprintf(stderr, "%s [DEBUG] : Injected successfully (Target: %ld, Speed: %.2f, Freeze: %d)\n",
                tz_env, (long)target_start_sec, speed_factor, freeze_flag);
    }
}

static time_t calculate_fake_time(time_t current_real_sec) {
    if (freeze_flag) {
        return target_start_sec;
    }
    double elapsed_real = (double)(current_real_sec - real_start_sec);
    return target_start_sec + (time_t)(elapsed_real * speed_factor);
}

/* Intercept: clock_gettime */
int clock_gettime(clockid_t clk_id, struct timespec *tp) {
    init_timerun();
    static int (*real_clock_gettime)(clockid_t, struct timespec *) = NULL;
    if (!real_clock_gettime) real_clock_gettime = dlsym(RTLD_NEXT, "clock_gettime");

    int ret = real_clock_gettime(clk_id, tp);
    if (ret == 0 && (clk_id == CLOCK_REALTIME || clk_id == CLOCK_REALTIME_COARSE)) {
        tp->tv_sec = calculate_fake_time(tp->tv_sec);
        if (debug_flag) {
            fprintf(stderr, "%s [DEBUG] : clock_gettime() -> Fake time: %ld\n", tz_env, (long)tp->tv_sec);
        }
    }

    __sync_fetch_and_add(&syscall_counter, 1);
    return ret;
}

/* Intercept: gettimeofday */
int gettimeofday(struct timeval *tv, void *tz) {
    init_timerun();

    static int (*real_gettimeofday)(struct timeval *, void *) = NULL;
    if (!real_gettimeofday) real_gettimeofday = dlsym(RTLD_NEXT, "gettimeofday");

    int ret = real_gettimeofday(tv, tz);
    if (ret == 0) {
        tv->tv_sec = calculate_fake_time(tv->tv_sec);
        if (debug_flag) {
            fprintf(stderr, "%s [DEBUG] : gettimeofday() -> Fake time: %ld\n", tz_env, (long)tv->tv_sec);
        }
    }
    return ret;

    __sync_fetch_and_add(&syscall_counter, 1);
}

/* Intercept: time */
time_t time(time_t *tloc) {
    init_timerun();
    static time_t (*real_time)(time_t *) = NULL;
    if (!real_time) real_time = dlsym(RTLD_NEXT, "time");

    time_t real_now = real_time(NULL);
    time_t fake_now = calculate_fake_time(real_now);

    if (tloc) *tloc = fake_now;
    if (debug_flag) {
        fprintf(stderr, "%s [DEBUG] : time() -> Fake time: %ld\n", tz_env, (long)fake_now);
    }
    return fake_now;

    __sync_fetch_and_add(&syscall_counter, 1);
}

/* Intercept: stat / fstat / lstat for fake mtime */
int stat(const char *pathname, struct stat *statbuf) {
    init_timerun();
    static int (*real_stat)(const char *, struct stat *) = NULL;
    if (!real_stat) real_stat = dlsym(RTLD_NEXT, "stat");

    int ret = real_stat(pathname, statbuf);
    if (ret == 0 && fake_mtime_flag) {
        time_t fake_now = calculate_fake_time(time(NULL));
        statbuf->st_mtime = fake_now;
        statbuf->st_atime = fake_now;
        statbuf->st_ctime = fake_now;
    }
    return ret;
}

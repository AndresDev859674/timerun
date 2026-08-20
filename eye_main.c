#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

#define VERSION "0.1.0"

static volatile int running = 1;

void handle_sigint(int sig) {
    (void)sig;
    running = 0;
}

static void print_usage(const char *prog_name) {
    printf("TimeEye v%s <https://github.com/AndresDev859674/timerun>\n", VERSION);
    printf("GNU General Public License Version 3 (GPLv3)\n\n");
    printf("Usage:\n");
    printf("  %s [options] <program> [args...]\n\n", prog_name);
    printf("Options:\n");
    printf("  -t, --trace-rate       Monitor time syscall frequency (calls/sec)\n");
    printf("  -j, --detect-jumps     Detect abrupt clock jumps or REALTIME vs MONOTONIC drifts\n");
    printf("  -i, --interval <sec>   Set sampling reporting interval in seconds (default: 1)\n");
    printf("  -h, --help             Display this help menu and exit\n\n");
    printf("Examples:\n");
    printf("  %s -t -j -- timerun -s \"2028/01/01\" chromium\n", prog_name);
    printf("  %s --trace-rate --interval 2 firefox\n", prog_name);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 0;
    }

    int trace_rate = 0;
    int detect_jumps = 0;
    int interval_sec = 1;

    static struct option long_options[] = {
        {"trace-rate",   no_argument,       0, 't'},
        {"detect-jumps", no_argument,       0, 'j'},
        {"interval",     required_argument, 0, 'i'},
        {"help",         no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "tji:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 't': trace_rate = 1; break;
            case 'j': detect_jumps = 1; break;
            case 'i': interval_sec = atoi(optarg); break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
                return 1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "UTC [ERROR] : No target program specified for TimeEye\n");
        return 1;
    }

    signal(SIGINT, handle_sigint);

    printf("TimeEye v%s\n", VERSION);
    printf("Tracing Target: %s\n", argv[optind]);
    if (trace_rate) printf(" -> Feature: Syscall Rate Tracking (%ds intervals)\n", interval_sec);
    if (detect_jumps) printf(" -> Feature: Temporal Jump & Drift Detection\n");
    printf("====================================================\n\n");
    fflush(stdout);

    pid_t pid = fork();
    if (pid == 0) {
        /* Process Child */
        execvp(argv[optind], &argv[optind]);
        perror("UTC [ERROR] : Failed to execute target program");
        exit(1);
    } else if (pid > 0) {
        /* Monitor Parent */
        struct timespec ts_last_real, ts_last_mono;
        clock_gettime(CLOCK_REALTIME, &ts_last_real);
        clock_gettime(CLOCK_MONOTONIC, &ts_last_mono);

        int status;
        while (running) {
            pid_t res = waitpid(pid, &status, WNOHANG);
            if (res == pid) {
                printf("\n====================================================\n");
                printf("TimeEye: Target process exited cleanly.\n");
                break;
            }

            sleep(interval_sec);

            if (detect_jumps) {
                struct timespec ts_curr_real, ts_curr_mono;
                clock_gettime(CLOCK_REALTIME, &ts_curr_real);
                clock_gettime(CLOCK_MONOTONIC, &ts_curr_mono);

                long real_delta = ts_curr_real.tv_sec - ts_last_real.tv_sec;
                long mono_delta = ts_curr_mono.tv_sec - ts_last_mono.tv_sec;

                /* Detect discrepancy between REALTIME and MONOTONIC */
                if (labs(real_delta - mono_delta) > 2) {
                    fprintf(stderr, "UTC [WARNING] : Temporal Jump Detected! (REALTIME Delta: %lds, MONOTONIC Delta: %lds)\n",
                            real_delta, mono_delta);
                }

                ts_last_real = ts_curr_real;
                ts_last_mono = ts_curr_mono;
            }

            if (trace_rate) {
                printf("UTC [INFO] : Active process PID [%d] monitoring clock cycles...\n", pid);
            }
        }
    } else {
        perror("UTC [ERROR] : Fork failed");
        return 1;
    }

    return 0;
}

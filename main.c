#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <libgen.h>
#include <ctype.h>

#define VERSION "1.0.0"

static const char *DEFAULT_FORMATS[] = {
    "%Y/%m/%d %H:%M:%S",
    "%Y/%m/%d %H:%M",
    "%Y/%m/%d",
    "%d/%m/%Y %H:%M:%S",
    "%d/%m/%Y %H:%M",
    "%d/%m/%Y",
    NULL
};

static void print_log(const char *tz, const char *level, const char *msg) {
    const char *tz_display = (tz && strlen(tz) > 0) ? tz : "UTC";
    fprintf(stderr, "%s [%s] : %s\n", tz_display, level, msg);
}

static void print_version(void) {
    printf("TimeRun v%s\n", VERSION);
    printf("License: GNU General Public License Version 3 (GPLv3)\n");
    printf("Written by swiftink (Andres)\n");
}

static void print_usage(const char *prog_name) {
    printf("TimeRun v%s <https://github.com/AndresDev859674/timerun>\n", VERSION);
    printf("GNU General Public License Version 3 (GPLv3)\n\n");
    printf("Usage:\n");
    printf("  %s -s \"<date_time>\" [options] <program> [args...]\n", prog_name);
    printf("  %s -a \"<offset>\" [options] <program> [args...]\n\n", prog_name);
    printf("Options:\n");
    printf("  -s, --specify <date>     Specify target date/time (e.g. \"2027/12/25 00:00\" or \"25/12/2027\")\n");
    printf("  -a, --advance <offset>   Advance target time relatively (e.g. \"+2d 5h\", \"+30m\")\n");
    printf("  -r, --rewind <offset>    Rewind target time relatively (e.g. \"-1y\", \"-3h\")\n");
    printf("  -x, --speed <factor>     Set time speed multiplier (e.g. 2.0 = 2x speed, 0.5 = half speed)\n");
    printf("  -F, --freeze             Freeze the target time permanently (clock does not advance)\n");
    printf("  -f, --format <format>    Custom strftime format string for date parsing\n");
    printf("  -z, --tz <timezone>      Set custom timezone context (e.g. UTC, EST, America/New_York)\n");
    printf("  -m, --fake-mtime         Spoof file modification times in stat() calls\n");
    printf("  -d, --debug              Enable internal debug log output to stderr\n");
    printf("  -v, --version            Display version info and exit\n");
    printf("  -h, --help               Display this help screen and exit\n\n");
    printf("Examples:\n");
    printf("  %s -s \"25/12/2027 00:00\" date\n", prog_name);
    printf("  %s -a \"+5d\" -x 2.0 firefox\n", prog_name);
    printf("  %s -s \"2030/01/01\" -F -z \"UTC\" /path/to/app.AppImage\n\n", prog_name);
    printf("Written by swiftink (Andres)\n");
    printf("if you find a bug you can Report bugs or submit issues to the project repository.\n");
}

static time_t parse_relative_time(const char *str) {
    time_t now = time(NULL);
    long val = 0;
    char unit = 's';
    const char *p = str;

    while (*p) {
        while (*p && isspace(*p)) p++;
        if (!*p) break;

        int sign = 1;
        if (*p == '+') { sign = 1; p++; }
        else if (*p == '-') { sign = -1; p++; }

        if (!isdigit(*p)) return -1;
        val = strtol(p, (char **)&p, 10) * sign;

        if (*p) {
            unit = *p;
            p++;
        }

        switch (unit) {
            case 's': now += val; break;
            case 'm': now += val * 60; break;
            case 'h': now += val * 3600; break;
            case 'd': now += val * 86400; break;
            case 'y': now += val * 31536000; break;
            default: return -1;
        }
    }
    return now;
}

static time_t parse_target_time(const char *time_str, const char *custom_format) {
    struct tm tm_target;
    memset(&tm_target, 0, sizeof(struct tm));

    if (custom_format) {
        if (strptime(time_str, custom_format, &tm_target) != NULL) {
            return mktime(&tm_target);
        }
        return -1;
    }

    for (int i = 0; DEFAULT_FORMATS[i] != NULL; i++) {
        memset(&tm_target, 0, sizeof(struct tm));
        if (strptime(time_str, DEFAULT_FORMATS[i], &tm_target) != NULL) {
            return mktime(&tm_target);
        }
    }

    return -1;
}

static char *get_lib_path(void) {
    static char path[1024];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        char *dir = dirname(path);
        snprintf(path, sizeof(path), "%s/libtimerun_inject.so", dir);
        if (access(path, F_OK) == 0) return path;
    }

    if (access("./libtimerun_inject.so", F_OK) == 0) return "./libtimerun_inject.so";
    if (access("/usr/local/lib/libtimerun_inject.so", F_OK) == 0) return "/usr/local/lib/libtimerun_inject.so";
    if (access("/usr/lib/libtimerun_inject.so", F_OK) == 0) return "/usr/lib/libtimerun_inject.so";

    return NULL;
}

int main(int argc, char *argv[]) {
    /* If run without any arguments, show help directly */
    if (argc < 2) {
        print_usage(argv[0]);
        return 0;
    }

    char *time_str = NULL;
    char *custom_format = NULL;
    char *timezone = NULL;
    char *relative_str = NULL;
    double speed_factor = 1.0;
    int freeze = 0;
    int fake_mtime = 0;
    int debug = 0;

    static struct option long_options[] = {
        {"specify",    required_argument, 0, 's'},
        {"format",     required_argument, 0, 'f'},
        {"tz",         required_argument, 0, 'z'},
        {"advance",    required_argument, 0, 'a'},
        {"rewind",     required_argument, 0, 'r'},
        {"speed",      required_argument, 0, 'x'},
        {"freeze",     no_argument,       0, 'F'},
        {"fake-mtime", no_argument,       0, 'm'},
        {"debug",      no_argument,       0, 'd'},
        {"version",    no_argument,       0, 'v'},
        {"help",       no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "s:f:z:a:r:x:Fmdvh", long_options, NULL)) != -1) {
        switch (opt) {
            case 's': time_str = optarg; break;
            case 'f': custom_format = optarg; break;
            case 'z': timezone = optarg; break;
            case 'a': relative_str = optarg; break;
            case 'r': relative_str = optarg; break;
            case 'x': speed_factor = atof(optarg); break;
            case 'F': freeze = 1; break;
            case 'm': fake_mtime = 1; break;
            case 'd': debug = 1; break;
            case 'v':
                print_version();
                return 0;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
                return 1;
        }
    }

    if (!time_str && !relative_str) {
        print_log(timezone, "ERROR", "Must specify target date/time (-s) or relative offset (-a/-r)");
        fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
        return 1;
    }

    if (optind >= argc) {
        print_log(timezone, "ERROR", "No target program specified");
        fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
        return 1;
    }

    if (timezone) {
        setenv("TZ", timezone, 1);
        tzset();
    }

    time_t target_time = 0;
    if (relative_str) {
        target_time = parse_relative_time(relative_str);
    } else {
        target_time = parse_target_time(time_str, custom_format);
    }

    if (target_time == -1) {
        print_log(timezone, "ERROR", "Failed to parse target date/time");
        return 1;
    }

    char *lib_path = get_lib_path();
    if (!lib_path) {
        print_log(timezone, "ERROR", "Could not locate libtimerun_inject.so");
        return 1;
    }

    char buf[64];
    snprintf(buf, sizeof(buf), "%ld", (long)target_time);
    setenv("TIMERUN_TARGET_SEC", buf, 1);

    snprintf(buf, sizeof(buf), "%.4f", speed_factor);
    setenv("TIMERUN_SPEED", buf, 1);

    if (freeze) setenv("TIMERUN_FREEZE", "1", 1);
    if (fake_mtime) setenv("TIMERUN_FAKE_MTIME", "1", 1);
    if (debug) setenv("TIMERUN_DEBUG", "1", 1);

    const char *existing_preload = getenv("LD_PRELOAD");
    char preload_buf[2048];
    if (existing_preload && strlen(existing_preload) > 0) {
        snprintf(preload_buf, sizeof(preload_buf), "%s:%s", lib_path, existing_preload);
    } else {
        snprintf(preload_buf, sizeof(preload_buf), "%s", lib_path);
    }
    setenv("LD_PRELOAD", preload_buf, 1);

    printf("TimeRun v%s\n", VERSION);
    printf("GNU General Public License Version 3 (GPLv3)\n\n");
    printf("Running %s\n\n", argv[optind]);
    fflush(stdout);

    execvp(argv[optind], &argv[optind]);

    print_log(timezone, "ERROR", "Failed to execute target program");
    return 1;
}

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>

struct simtemp_sample {
    uint64_t timestamp_ns;
    int32_t temp_mC;
    uint32_t flags;
} __attribute__((packed));

#define FLAG_NEW_SAMPLE        0x1
#define FLAG_THRESHOLD_CROSSED 0x2

const char *DEV_PATH = "/dev/simtemp";
const char *SYSFS_PATH = "/sys/class/misc/simtemp/";

bool write_sysfs(const char* file, const char* value) {
    std::ofstream ofs(file);
    if (!ofs.is_open()) return false;
    ofs << value;
    return true;
}

void print_sample(const simtemp_sample &s) {
    time_t sec = s.timestamp_ns / 1000000000;
    long nsec = s.timestamp_ns % 1000000000;
    char timestr[64];
    struct tm tm_info;
    gmtime_r(&sec, &tm_info);
    strftime(timestr, sizeof(timestr), "%Y-%m-%dT%H:%M:%S", &tm_info);

    printf("%s.%09ld temp=%.3fC alert=%s\n",
           timestr, nsec,
           s.temp_mC / 1000.0,
           (s.flags & FLAG_THRESHOLD_CROSSED) ? "YES" : "NO");
}

int main() {
    int fd = open(DEV_PATH, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("=== Test: API Contract ===\n");
    printf("sizeof(simtemp_sample)=%zu bytes\n", sizeof(simtemp_sample));

    // T2: Periodic Read
    printf("=== Test: Periodic Read ===\n");
    struct pollfd pfd = { .fd = fd, .events = POLLIN, .revents = 0 };
    for (int i = 0; i < 5; i++) {
        int ret = poll(&pfd, 1, 2000); // 2s timeout
        if (ret <= 0) {
            printf("Timeout o error en poll\n");
            continue;
        }
        simtemp_sample s;
        ssize_t n = read(fd, &s, sizeof(s));
        if (n == sizeof(s)) print_sample(s);
    }

    // T3: Threshold Event
    printf("=== Test: Threshold Event ===\n");
    char thresh_file[128];
    snprintf(thresh_file, sizeof(thresh_file), "%sthreshold_mC", SYSFS_PATH);
    if (!write_sysfs(thresh_file, "0")) {
        printf("No se pudo escribir threshold\n");
    }

    for (int i = 0; i < 3; i++) {
        int ret = poll(&pfd, 1, 2000);
        if (ret > 0) {
            simtemp_sample s;
            ssize_t n = read(fd, &s, sizeof(s));
            if (n == sizeof(s)) print_sample(s);
        }
    }

    // T4: Error Paths
    printf("=== Test: Error Paths ===\n");
    char invalid_file[128];
    snprintf(invalid_file, sizeof(invalid_file), "%sinvalid_attr", SYSFS_PATH);
    if (!write_sysfs(invalid_file, "123")) {
        printf("Correctamente falló escritura inválida\n");
    }

    close(fd);
    printf("=== Test completado ===\n");
    return 0;
}


#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include "nxp_simtemp_ioctl.h"

#define DEV_PATH "/dev/simtemp"
#define SYSFS_BASE "/sys/class/misc/simtemp/"

static void write_sysfs(const std::string &name, const std::string &value) {
    std::ofstream ofs(SYSFS_BASE + name);
    if (!ofs) {
        perror("open sysfs");
        exit(1);
    }
    ofs << value;
}

static std::string read_sysfs(const std::string &name) {
    std::ifstream ifs(SYSFS_BASE + name);
    if (!ifs) {
        perror("open sysfs");
        exit(1);
    }
    std::string val;
    ifs >> val;
    return val;
}

static void usage(const char *prog) {
    std::cerr << "Usage: " << prog << " [--sample Nms] [--threshold mC] [--test]" << std::endl;
    exit(1);
}

int main(int argc, char *argv[]) {
    int fd;
    int sampling = -1, threshold = -1;
    bool test_mode = false;

    for (int i=1; i<argc; i++) {
        if (strcmp(argv[i], "--sample") == 0 && i+1<argc) {
            sampling = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--threshold") == 0 && i+1<argc) {
            threshold = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--test") == 0) {
            test_mode = true;
        } else {
            usage(argv[0]);
        }
    }

    fd = open(DEV_PATH, O_RDONLY);
    if (fd < 0) {
        perror("open dev");
        return 1;
    }

    if (sampling > 0) {
        write_sysfs("sampling_ms", std::to_string(sampling));
    }
    if (threshold > 0) {
        write_sysfs("threshold_mC", std::to_string(threshold));
    }

    if (test_mode) {
        // Test: threshold bajo → esperar alerta ≤2 periodos
        write_sysfs("sampling_ms", "200");
        write_sysfs("threshold_mC", "1000"); // 0.1 °C, siempre disparará

        struct pollfd pfd = {fd, POLLIN, 0};
        int events = poll(&pfd, 1, 500); // esperar hasta 500ms
        if (events <= 0) {
            std::cerr << "FAIL: no alert within 2 periods" << std::endl;
            return 1;
        }
        struct simtemp_sample s;
        if (read(fd, &s, sizeof(s)) == sizeof(s)) {
            std::cout << "PASS: alert event received, temp=" 
                      << s.temp_mC/1000.0 << "C" 
                      << " flags=" << s.flags << std::endl;
            return 0;
        }
        std::cerr << "FAIL: could not read sample" << std::endl;
        return 1;
    }

    // Normal mode → imprimir datos continuamente
    while (1) {
        struct pollfd pfd = {fd, POLLIN, 0};
        int ret = poll(&pfd, 1, 2000);
        if (ret > 0 && (pfd.revents & POLLIN)) {
            struct simtemp_sample s;
            int n = read(fd, &s, sizeof(s));
            if (n == sizeof(s)) {
                auto ts = std::chrono::system_clock::now();
                std::time_t t = std::chrono::system_clock::to_time_t(ts);
                char buf[64];
                std::strftime(buf, sizeof(buf), "%FT%TZ", std::gmtime(&t));
                std::cout << buf << " temp=" 
                          << s.temp_mC/1000.0 
                          << "C alert=" << ((s.flags & (1<<1)) ? 1:0)
                          << std::endl;
            }
        }
    }

    close(fd);
    return 0;
}


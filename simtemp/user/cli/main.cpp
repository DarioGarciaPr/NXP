// user/cli/main.cpp
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <cstdint>

// Estructura del registro (debe coincidir con kernel)
struct simtemp_sample {
    uint64_t timestamp_ns; // monotonic timestamp
    int32_t temp_mC;       // milli-degree Celsius
    uint32_t flags;        // bit0=NEW_SAMPLE, bit1=THRESHOLD_CROSSED
} __attribute__((packed));

#define FLAG_NEW_SAMPLE        0x1
#define FLAG_THRESHOLD_CROSSED 0x2

int main(int argc, char* argv[]) {
    const char* dev_path = "/dev/simtemp";
    int sampling_ms = 100;
    int threshold_mC = 45000;

    // parse args simples
    for (int i=1; i<argc; i++) {
        if (strcmp(argv[i], "--sampling") == 0 && i+1<argc) sampling_ms = std::stoi(argv[++i]);
        if (strcmp(argv[i], "--threshold") == 0 && i+1<argc) threshold_mC = std::stoi(argv[++i]);
    }

    std::cout << "Opening device: " << dev_path << std::endl;
    int fd = open(dev_path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    std::cout << "Starting poll loop..." << std::endl;

    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;

    while (true) {
        int ret = poll(&pfd, 1, -1); // wait indefinitely
        if (ret < 0) {
            perror("poll");
            break;
        }

        if (pfd.revents & POLLIN) {
            simtemp_sample sample;
            ssize_t n = read(fd, &sample, sizeof(sample));
            if (n == sizeof(sample)) {
                auto ts = std::chrono::nanoseconds(sample.timestamp_ns);
                auto secs = std::chrono::duration_cast<std::chrono::seconds>(ts);
                auto ns = ts - secs;
                std::time_t t_c = secs.count();
                std::tm tm = *std::gmtime(&t_c);

                std::cout << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S")
                          << "." << std::setw(9) << std::setfill('0') << ns.count()
                          << "Z "
                          << "temp=" << sample.temp_mC / 1000.0 << "C"
                          << " alert=" << ((sample.flags & FLAG_THRESHOLD_CROSSED)?1:0)
                          << std::endl;
            }
        }
    }

    close(fd);
    return 0;
}


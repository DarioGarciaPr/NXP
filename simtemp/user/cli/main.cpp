#include <iostream>
#include <fstream>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <ctime>

struct simtemp_sample {
    uint64_t timestamp_ns;
    int32_t temp_mC;
    uint32_t flags;
} __attribute__((packed));

int main() {
    const char* device = "/dev/simtemp";
    int fd = open(device, O_RDONLY | O_NONBLOCK);
    if(fd < 0) {
        std::cerr << "Error: cannot open " << device << std::endl;
        return 1;
    }

    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;

    std::cout << "Waiting for samples...\n";

    while(true) {
        int ret = poll(&pfd, 1, 1000); // 1s timeout
        if(ret > 0 && (pfd.revents & POLLIN)) {
            simtemp_sample s;
            ssize_t r = read(fd, &s, sizeof(s));
            if(r == sizeof(s)) {
                double tempC = s.temp_mC / 1000.0;
                std::time_t t = s.timestamp_ns / 1000000000;
                std::cout << std::ctime(&t) << " temp=" << tempC
                          << "C alert=" << ((s.flags & 0x2) ? 1 : 0) << "\n";
            }
        }
    }

    close(fd);
    return 0;
}

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "nxp_simtemp_ioctl.h"

int main() {
    const char* device = "/dev/nxp_simtemp";
    int fd = open(device, O_RDONLY);
    if (fd < 0) {
        perror("Error abriendo dispositivo");
        return 1;
    }

    int threshold = 40;      // °C
    int sampling = 1000;     // ms

    if (ioctl(fd, NXPSIM_IOCTL_SET_THRESHOLD, &threshold) < 0)
        perror("Error configurando threshold");

    if (ioctl(fd, NXPSIM_IOCTL_SET_SAMPLING_MS, &sampling) < 0)
        perror("Error configurando sampling");

    char buf[128];

    while (true) {
        int n = read(fd, buf, sizeof(buf)-1);
        if (n > 0) {
            buf[n] = '\0';
            std::cout << buf;
        } else {
            perror("Error leyendo temperatura");
        }
        usleep(sampling * 1000); // esperar tiempo de muestreo
    }

    close(fd);
    return 0;
}


#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "nxp_simtemp_ioctl.h"

int main(int argc, char* argv[]) {
    int fd = open("/dev/nxp_simtemp", O_RDWR);
    if (fd < 0) { perror("open"); return 1; }

    int threshold = 40;
    int sampling = 1000;

    printf("Configurando threshold = %d °C\n", threshold);
    if (ioctl(fd, NXPSIM_IOCTL_SET_THRESHOLD, &threshold) < 0) {
        perror("Error configurando threshold");
    }

    printf("Configurando sampling = %d ms\n", sampling);
    if (ioctl(fd, NXPSIM_IOCTL_SET_SAMPLING_MS, &sampling) < 0) {
        perror("Error configurando sampling");
    }

    char tmp[128];

    // Esperar hasta que la primera lectura válida sea distinta de 0
    while (true) {
        int n = read(fd, tmp, sizeof(tmp));
        if (n < 0) { perror("Error leyendo temperatura"); break; }

        float temp;
        int flags;
        if (sscanf(tmp, "Temp: %f °C | flags=0x%x", &temp, &flags) == 2) {
            if (temp > 0) break;  // lectura válida
        }
        usleep(100 * 1000); // 100 ms antes de reintentar
    }

    // Lecturas normales
    for (int i=0; i<10; i++) {
        int n = read(fd, tmp, sizeof(tmp));
        if (n < 0) { perror("Error leyendo temperatura"); continue; }
        printf("%s", tmp);
        usleep(sampling * 1000);
    }

    close(fd);
    return 0;
}


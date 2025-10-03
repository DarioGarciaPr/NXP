#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include "nxp_simtemp_ioctl.h"

/*
 * User-space test application for the NXP simulated temperature driver.
 *
 * Usage:
 *   ./nxp_simtemp_test         -> Normal mode (configure threshold/sampling and print samples)
 *   ./nxp_simtemp_test --test  -> Test mode (check if an alert flag is detected)
 */

int main(int argc, char* argv[]) {
    int fd = open("/dev/nxp_simtemp", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    // ==========================
    // Test Mode (--test)
    // ==========================
    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        int test_threshold = 50;   // °C
        int test_sampling = 500;   // ms
        int alert_detected = 0;

        printf("Test mode: threshold = %d °C, sampling = %d ms\n",
               test_threshold, test_sampling);

        ioctl(fd, NXPSIM_IOCTL_SET_THRESHOLD, &test_threshold);
        ioctl(fd, NXPSIM_IOCTL_SET_SAMPLING_MS, &test_sampling);

        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;

        char tmp[128];

        for (int i = 0; i < 2; i++) {
            int ret = poll(&pfd, 1, -1); // blocking
            if (ret < 0) { perror("poll"); break; }

            if (pfd.revents & POLLIN) {
                int n = read(fd, tmp, sizeof(tmp));
                if (n < 0) { perror("read"); continue; }
                tmp[n] = '\0';
                printf("Sample: %s", tmp);

                if (strstr(tmp, "flags=0x1")) {
                    alert_detected = 1;
                    break;
                }
            }
        }

        close(fd);
        return alert_detected ? 0 : 1;
    }

    // ==========================
    // Normal Mode
    // ==========================
    int threshold = 50;   // °C
    int sampling  = 1000; // ms

    printf("Configuring threshold = %d °C\n", threshold);
    if (ioctl(fd, NXPSIM_IOCTL_SET_THRESHOLD, &threshold) < 0) {
        perror("Failed to configure threshold");
    }

    printf("Configuring sampling = %d ms\n", sampling);
    if (ioctl(fd, NXPSIM_IOCTL_SET_SAMPLING_MS, &sampling) < 0) {
        perror("Failed to configure sampling");
    }

    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;

    char tmp[128];

    printf("Reading samples...\n");
    for (int i = 0; i < 10; i++) {
        int ret = poll(&pfd, 1, -1);
        if (ret < 0) { perror("poll"); break; }

        if (pfd.revents & POLLIN) {
            int n = read(fd, tmp, sizeof(tmp));
            if (n < 0) { perror("read"); continue; }
            tmp[n] = '\0';
            printf("%s", tmp);
        }
    }

    close(fd);
    return 0;
}


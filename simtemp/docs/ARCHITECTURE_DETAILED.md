# nxp_simtemp Detailed Architecture

```mermaid
flowchart TD
    %% User space
    CLI["User CLI / C++ / Python"] 
    DEV["/dev/simtemp/char device"]
    SYSFS["Sysfs config:\nsampling_ms, threshold_mC, stats"]

    %% Kernel driver internals
    DRIVER["nxp_simtemp Kernel Module"]
    RING["Ring Buffer\nsize=128"]
    TIMER["HR Timer → Workqueue"]
    
    %% Flags
    FLAGS["Sample Flags:\nbit0=NEW_SAMPLE\nbit1=THRESHOLD_CROSSED"]

    %% User interactions
    CLI -->|read()| DEV
    CLI -->|poll()/epoll()| DEV
    CLI -->|ioctl()| DEV
    CLI -->|echo/write| SYSFS

    %% Driver -> Kernel internals
    DEV --> DRIVER
    SYSFS --> DRIVER
    DRIVER --> RING
    DRIVER --> FLAGS
    DRIVER --> TIMER
    TIMER --> DRIVER

    %% Ring buffer logic
    RING -->|store new sample| FLAGS
    FLAGS -->|wake_up(wait_queue)| DEV


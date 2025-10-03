# nxp_simtemp Detailed Architecture

```mermaid
flowchart TD
    %% User space
    CLI[User CLI / C++ / Python] 
    DEV[/dev/simtemp/char device/]
    SYSFS[Sysfs config: sampling_ms, threshold_mC, stats]

    %% Kernel driver internals
    DRIVER[nxp_simtemp Kernel Module]
    RING[Ring Buffer<br/>size=128]
    TIMER[HR Timer → Workqueue]
    
    %% Flags
    FLAGS["Sample Flags:<br/>bit0=NEW_SAMPLE<br/>bit1=THRESHOLD_CROSSED"]

    %% User interactions
    CLI -- "read()" --> DEV
    CLI -- "poll()/epoll()" --> DEV
    CLI -- "ioctl()" --> DEV
    CLI -- "echo/write" --> SYSFS

    %% Driver -> Kernel internals
    DEV --> DRIVER
    SYSFS --> DRIVER
    DRIVER --> RING
    DRIVER --> FLAGS
    DRIVER --> TIMER
    TIMER --> DRIVER

    %% Ring buffer logic
    RING -- "store new sample" --> FLAGS
    FLAGS -- "wake_up(wait_queue)" --> DEV

---

### Explicación visual

- **CLI** → lee `/dev/simtemp`, espera eventos con `poll/epoll`.
- **Sysfs** → configura sampling interval y threshold.
- **Kernel driver** → contiene el **ring buffer**, mantiene **flags** y dispara alertas.
- **Timer + Workqueue** → genera nuevas muestras periódicas.
- **Ring buffer + flags** → despierta procesos bloqueados en `poll()` cuando hay **new sample** o **threshold crossed**.



# SimTemp Design

## Block Diagram

```mermaid
graph TD
    KernelModule[nxp_simtemp.ko] -->|Character device| DevNode[/dev/simtemp]
    KernelModule -->|Sysfs interface| Sysfs[/sys/class/misc/simtemp]
    CLI -->|Read/Write| DevNode
    GUI -->|Optional| DevNode
    KernelModule -->|Periodic Timer| Workqueue


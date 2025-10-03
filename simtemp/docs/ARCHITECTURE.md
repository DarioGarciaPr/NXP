# nxp_simtemp Architecture Diagram

```mermaid
flowchart TD
    CLI[User CLI / C++]
    DEV[/dev/simtemp]
    SYSFS[Sysfs config]
    DRIVER[nxp_simtemp Kernel Module]
    TIMER[Timer + Workqueue]
    
    CLI -- ioctl/read/poll --> DEV
    CLI -- echo/write --> SYSFS
    DEV -- read/write --> DRIVER
    SYSFS -- config access --> DRIVER
    DRIVER -- schedules --> TIMER
    TIMER -- pushes sample --> DRIVER

---

### Opción PNG/SVG con Graphviz


```dot
digraph nxp_simtemp {
    rankdir=LR;
    CLI [label="User CLI / C++"];
    DEV [label="/dev/simtemp"];
    SYSFS [label="Sysfs config"];
    DRIVER [label="nxp_simtemp Kernel Module"];
    TIMER [label="Timer + Workqueue"];
    
    CLI -> DEV [label="ioctl/read/poll"];
    CLI -> SYSFS [label="echo/write"];
    DEV -> DRIVER [label="read/write"];
    SYSFS -> DRIVER [label="config access"];
    DRIVER -> TIMER [label="schedules"];
    TIMER -> DRIVER [label="pushes sample"];
}
dot -Tpng architecture.dot -o architecture.png
dot -Tsvg architecture.dot -o architecture.svg


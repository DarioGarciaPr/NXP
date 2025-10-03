flowchart TD
    CLI["User CLI / C++"]
    DEV["/dev/simtemp"]
    SYSFS["Sysfs config"]
    DRIVER["nxp_simtemp Kernel Module"]
    TIMER["Timer + Workqueue"]

    CLI --> DEV
    CLI --> SYSFS
    DEV --> DRIVER
    SYSFS --> DRIVER
    DRIVER --> TIMER
    TIMER --> DRIVER


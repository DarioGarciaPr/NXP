# Test Plan for nxp_simtemp

## T1 — Load/Unload
- Build with `scripts/build.sh`
- Run `sudo insmod nxp_simtemp.ko`
- Check `/dev/simtemp` and `/sys/class/misc/simtemp/`
- Run `sudo rmmod nxp_simtemp.ko`
- Expect no warnings in `dmesg`

## T2 — Periodic Read
- `echo 100 > /sys/class/misc/simtemp/sampling_ms`
- Run CLI → expect ~10 samples/sec

## T3 — Threshold Event
- `echo 42000 > /sys/class/misc/simtemp/threshold_mC`
- Observe CLI prints alert=1 when temp ≥ 42.0 °C
- CLI `--test` must PASS (alert within 2 periods)

## T4 — Error Paths
- `echo -1 > /sys/class/misc/simtemp/sampling_ms` → expect `EINVAL`
- Very small sampling (1ms) → driver should still run (stats increments)

## T5 — Concurrency
- Run CLI continuously + change sysfs values in parallel
- No deadlocks or kernel oops

## T6 — API Contract
- `struct simtemp_sample` size documented in header
- CLI handles partial reads gracefully


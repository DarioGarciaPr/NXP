# Test Plan - simtemp

## Objective
Verify the functionality of the `nxp_simtemp` kernel module and the CLI interface (`user/cli/main.cpp`).

## Test Environment
- Kernel: 6.14.0-33-generic (x86_64)  
- OS: Ubuntu 24.04  
- Module: `nxp_simtemp.ko`  
- CLI: Compiled from `main.cpp` → executable `main`  
- Scripts: `build.sh`, `run_demo.sh`  

## Test Cases

| Case | Action | Input | Expected Result | Observed Result | Status |
|------|--------|-------|----------------|----------------|--------|
| 1    | Load module | `sudo insmod kernel/nxp_simtemp.ko` | Loads without errors | `nxp_simtemp 12288 0` in `lsmod` | ✅ |
| 2    | Configure threshold | CLI: `threshold = 40` | Flags=1 if temp ≥ threshold | Flags=0/1 depending on temp | ✅ |
| 3    | Configure sampling | CLI: `sampling = 1000 ms` | Readings every 1 second | Readings roughly every 1s | ✅ |
| 4    | First reading | Open CLI | Correct temp, flags=0 if below threshold | Temp=0.000 °C, flags=0 | ⚠️ known behavior |
| 5    | Subsequent readings | CLI runs | Correct temp and flags | Temp ≈ 41–49 °C, flags=0x1 | ✅ |
| 6    | Unload module | `sudo rmmod nxp_simtemp` | Module removed | Verified via `lsmod` | ✅ |
| 7    | Run demo | `./run_demo.sh` | CLI outputs readings without error | Demo completed successfully | ✅ |

## Notes
- Initial reading may show 0 °C due to module init.  
- Flags indicate threshold exceedance.  
- CLI handles errors gracefully.  

## Next Steps
- Test thresholds 20–60 °C.  
- Test faster sampling (500 ms, 100 ms).  
- Validate GUI once implemented.


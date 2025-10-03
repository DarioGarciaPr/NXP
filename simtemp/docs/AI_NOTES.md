# AI Notes - simtemp Project

## Objective
Document the prompts, suggestions, and validations performed during development of the `nxp_simtemp` kernel module and CLI.

---

## Kernel Module Development

**Files:**
- `nxp_simtemp.c`
- `nxp_simtemp.h`
- `nxp_simtemp_ioctl.h`
- `dts/nxp-simtemp.dtsi`

**Prompts / Actions:**
- Corrected `struct temp_sample` and sampling variables.
- Fixed missing declarations (`no_llseek` → `noop_llseek`).
- Resolved SSE warnings during compilation.
- Verified module loading/unloading with `insmod` / `rmmod`.
- Checked `dmesg` logs to confirm driver messages.

**Observations:**
- First reading sometimes shows `0.000 °C`.
- Flags indicate threshold exceedance correctly.
- `nxp_simtemp.ko` compiled successfully after cleaning old builds.
- No further kernel errors observed.

---

## CLI Development

**File:** `user/cli/main.cpp`  

**Prompts / Actions:**
- Fixed missing includes (`<cstdio>`, `<fcntl.h>`, `<unistd.h>`).
- Added correct ioctl defines (`NXPSIM_IOCTL_SET_THRESHOLD`, `NXPSIM_IOCTL_SET_SAMPLING_MS`).
- Resolved compilation errors for `open`, `read`, `perror`, `printf`, `usleep`.
- Corrected path to `nxp_simtemp_ioctl.h` for compilation.

**Observations:**
- Initial read can return 0 °C, subsequent reads are correct.
- Flags respond correctly to threshold configuration.
- CLI runs successfully via `run_demo.sh`.

---

## Scripts

**Files:** `scripts/build.sh`, `scripts/run_demo.sh`, `scripts/lint.sh`  

**Prompts / Actions:**
- Ensured `build.sh` compiles kernel module and CLI.
- `run_demo.sh` inserts module, runs CLI, and removes module.
- `.gitignore` updated to exclude `.o`, `.ko`, `.cmd`.

**Observations:**
- Scripts successfully build and run the demo.
- Path corrections needed to avoid missing file errors.
- Verified all scripts execute without errors in current repo layout.

---

## Git / Version Control

**Prompts / Actions:**
- Committed kernel and CLI separately.
- Tags created:
  - `v1.0-kernel` for kernel module
  - `v1.0-cli` for CLI
  - `v1.0` for entire project state
- `.cmd` and temporary files removed via `.gitignore`.

**Observations:**
- Repository is clean and organized.
- Tags reflect project milestones correctly.

---

## Next Steps / TODO

1. Implement GUI (`user/gui/app.py` or `qt_app.cpp`) and connect to kernel module.
2. Extend CLI to handle multiple simultaneous reads.
3. Add automated tests for thresholds and sampling intervals.
4. Complete documentation for GUI and advanced testing scenarios.


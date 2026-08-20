<p align="center">
  <img style="width: 52%; height: auto;" alt="timerun" src="https://github.com/user-attachments/assets/f296e5c0-e0d4-4160-b9e5-e4da223aa860" />
</p>
<p align="center"> Execute applications in a custom system time context </p>

<p align="center">
  <img src="https://theoldnet.com/images/anibar.gif">
</p>

# About
What its this? It's a program written in C that runs applications on a specific date without breaking the system by changing the system time. It communicates with the kernel. For example, if you want to program something that requires a specific date to change things (for events like Christmas, December 24th), you use TimeRun and can test if everything you programmed works (when it's activated on that date).


It works by injecting a shared library via `LD_PRELOAD` to intercept POSIX time-related system functions (`time`, `gettimeofday`, `clock_gettime`, `stat`).

<p align="center">
  <img style="width: 20%; height: auto;" alt="timeeye" src="https://github.com/user-attachments/assets/d2271167-b7a7-4ade-875c-d9bea2391f90" />
</p>

TimeRun its not the Only, Also TimeEye exists and its a division of TimeRun for Advanced Diagnostics
| Product        | Comment                    | Status                                                                                 |
|-----------------|------------------------------|----------------------------------------------------------------------------------------|
| TimeRun  | The Main, The Readme says everything           | Works                                                                                  |
| TimeEye    | its a Temporal Diagnostics & Syscall Analyzer           | In development                                                

### Most Common Features 

- **Absolute Time Control:** Force any application to run on a specific date/time.
- **Relative Time Offsets:** Easily advance (`+2d 5h`) or rewind (`-1m`) time context.
- **Time Acceleration:** Speed up or slow down time progression (`-x 2.0`).
- **Time Freezing:** Freeze time permanently at a specific timestamp.
- **Timezone Spoofing:** Override timezone settings per application.
- **File MTime Spoofing:** Spoof file timestamps in `stat()` calls.

## So HOW this works?

TimeRun intercepts temporal system calls in user space using dynamic library injection before execution reaches the C library or the Linux kernel.
```
+-------------------------------------------------------------+
|                      Target Process                         |
|            (e.g., Firefox, AppImage, custom binary)         |
+-------------------------------------------------------------+
                               |
                               v (time(), clock_gettime, stat)
+-------------------------------------------------------------+
|                    libtimerun_inject.so                     |
|           [Interception Layer via LD_PRELOAD]               |
| Calculates: T_fake = T_start + (T_real - T_real_start) * x  |
+-------------------------------------------------------------+
                               |
                               v (dlsym RTLD_NEXT)
+-------------------------------------------------------------+
|                         GNU C Library                       |
|                          (glibc)                            |
+-------------------------------------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|                        Linux Kernel                         |
+-------------------------------------------------------------+
```

## Installation
The installation its very simple, Just clone, and compile it and install it
```bash
git clone https://github.com/AndresDev859674/timerun.git
cd timerun
make
sudo make install

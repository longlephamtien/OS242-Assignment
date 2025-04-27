# OS242-Assignment

## How to Run the Simulation

This project implements a simple OS and simulates it over virtual hardware.

### 1. Prepare a Configuration File

To start the simulation, create a description file in the `input` directory. The file describes the hardware and environment to be simulated. The format is:

```
[time slice] [N = Number of CPU] [M = Number of Processes to be run]
[mem_ram_size] [mem_swp0_size] [mem_swp1_size] [mem_swp2_size] [mem_swp3_size]
[time 0] [path 0] [priority 0]
[time 1] [path 1] [priority 1]
...
[time M-1] [path M-1] [priority M-1]
```

- **time slice**: Amount of time (in seconds) a process is allowed to run.
- **N**: Number of CPUs available.
- **M**: Number of processes to run.
- **mem_ram_size** and **mem_swp*_size**: Memory sizes for RAM and swap spaces.
- Each process line:  
  - **time**: When to start the process.
  - **path**: Path to the process file (relative to `input/proc/`).
  - **priority**: Priority of the process when invoked (overwrites default).

### 2. Compile the Source Code

Run:
```sh
make all
```

### 3. Run the Simulation

To run the simulation with a specific configuration file:
```sh
./os [configure_file]
```
- `[configure_file]` is the name of the config file in the `input` directory (e.g., `os_test`).

### 4. Run All Configurations Automatically

A helper script `run.sh` is provided to run all config files in the `input` directory (excluding files in `input/proc/`). It stores the output for each config in the `our_output` directory.

To use:
```sh
chmod +x run.sh
./run.sh
```
Each output will be saved as `our_output/<config_name>_output.txt`.

---

**Note:**  
- Make sure your configuration files and process files are correctly placed in the `input` and `input/proc` directories, respectively.
- The simulation will use the parameters and process priorities as specified in your config file.
# Parallel Design Pattern (PDP) Simulation

This repository contains the source code and data for the Parallel Design Pattern (PDP) simulation, which models a traffic simulation using parallel computing techniques. The simulation is written in C, and uses the OpenMP library for parallelization. The simulation is designed to be run on the Cirrus HPC system, and can be built and run using the provided makefile and SLURM script. And this simulation only supports tinyl-scale traffic simulation.

## Directory Structure

- `/bin`: Contains the executable file for the simulation.
- `/include`: Houses all header files, providing function declarations and data structure definitions.
- `/problem_size`: Includes different problem sizes for the simulation, allowing for scalability testing.
- `/result`: Stores the output files generated by the simulation.
- `/src`: Contains source files for the main program functions.
- `makefile`: Used to build the project using the `make` utility.
- `cirrus_run.slurm`: A SLURM script for submitting jobs on the Cirrus HPC system.

## Contents

### Executable

- `bin/actor_parallel`: The compiled simulation executable that runs the traffic model.

### Header Files

- `include/actor_parallel.h`: Declares the setup and main loop functions for the actor parallel pattern.
- `include/data_structures.h`: Defines the data structures used across the simulation, such as vehicles, roads, and junctions, and also defines the tags.
- `include/pool.h`: Contains declarations for functions managing the pool of workers in the simulation.
- `include/utils.h`: Provides utility functions for the simulation, such as random number generation and time handling.

### Problem Sizes

- `/problem_size/large_problem`: Data representing a large-scale simulation scenario.
- `/problem_size/largest_problem`: Data for the largest available simulation scenario.
- `/problem_size/medium_problem`: Medium complexity simulation scenario data.
- `/problem_size/small_problem`: Small-scale simulation data.
- `/problem_size/tiny_problem`: The smallest problem set for quick simulation tests.

### Results

- `/result/results`: Output files generated after running simulations. This includes logs and data regarding the traffic simulation outcomes.

### Source Files

- `src/actor_parallel.c`: Defines the main parallel simulation functions and the three different kinds of actors, and shows the main logic funtion in this file.
- `src/pool.c`: Implements the worker pool management for the simulation actors.
- `src/utils.c`: Provides the implementation for utility functions declared in `utils.h`.

### Build and Run Scripts

- `makefile`: A build script used to compile the simulation into an executable. Use `make` command to build the project.
- `cirrus_run.slurm`: A batch script for submitting the simulation to be run on the Cirrus system using the SLURM workload manager.

## Building the Project

To build the project, navigate to the root directory and run:

```bash
make
```
This will compile the source code and place the executable in the /bin directory.

## Running the Simulation

To run the simulation on Cirrus, use the provided SLURM script:

```bash
sbatch cirrus_run.slurm
```
Make sure to adjust the SLURM script according to the job configuration requirements.


## Output

After running the simulation, the results will be available in the /result directory, which can be used for further analysis.
#!/bin/sh
# Slurm job options (name, compute nodes, job time)
#SBATCH --job-name=PDP_CW2
#SBATCH --time=0:20:0
#SBATCH --exclusive
#SBATCH --nodes=2
#SBATCH --tasks-per-node=36
#SBATCH --cpus-per-task=1

# Replace [budget code] below with your project code (e.g. t01)
#SBATCH --account=m23oc-s2500567
# We use the "standard" partition as we are running on CPU nodes
#SBATCH --partition=standard
# We use the "standard" QoS as our runtime is less than 4 days
#SBATCH --qos=short

# Load the default HPE MPI environment
module load mpt

export OMP_NUM_THREADS=1

mpirun -n 8 ./bin/actor_parallel ./problem_size/tiny_problem


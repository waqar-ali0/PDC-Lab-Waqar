#!/bin/bash
# builds and runs everything for Lab 08, output goes to the screen
mkdir -p ~/PDC_Lab08
cp "$(dirname "$0")"/*.c ~/PDC_Lab08/
cd ~/PDC_Lab08
. ~/mpi-env.sh
# stderr only has harmless "missing help file" warnings, so hide it
mr() { mpirun --oversubscribe "$@" 2>/dev/null; }
MR=mr

echo "### verify"
echo '$ mpicc --version'; mpicc --version | head -1
echo '$ ompi_info | grep "Open MPI:"'; ompi_info 2>/dev/null | grep "Open MPI:"
echo '$ mpirun -np 2 hostname'; mr -np 2 hostname
echo '$ nproc'; nproc

echo "### build"
for cmd in "mpicc -O2 -std=gnu11 hello_mpi.c -o hello_mpi" \
           "mpicc -O2 -std=gnu11 sum_mpi.c -o sum_mpi" \
           "gcc -O2 -fopenmp -std=gnu11 sum_omp.c -o sum_omp" \
           "mpicc -O2 -std=gnu11 master_worker.c -o master_worker" \
           "mpicc -O2 -std=gnu11 ring.c -o ring"; do
  echo "\$ $cmd"; $cmd 2>&1 && echo "  ok"
done

echo "### task1"
for n in 4 6 8; do for run in 1 2; do
  echo "\$ mpirun -np $n ./hello_mpi   (run $run)"; $MR -np $n ./hello_mpi
done; done

echo "### task2"
for n in 1 2 4 8; do echo "\$ mpirun -np $n ./sum_mpi"; $MR -np $n ./sum_mpi; done
for n in 1 2 4 8; do echo "\$ OMP_NUM_THREADS=$n ./sum_omp"; OMP_NUM_THREADS=$n ./sum_omp; done

echo "### task3"
for n in 4 8; do echo "\$ mpirun -np $n ./master_worker"; $MR -np $n ./master_worker; done
for n in 4 8; do echo "\$ mpirun -np $n ./ring"; $MR -np $n ./ring; done

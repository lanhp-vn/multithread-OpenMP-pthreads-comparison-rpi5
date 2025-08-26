# Multithread OpenMP vs PThreads Comparison on Raspberry Pi 5

## Project Overview
Performance comparison of OpenMP and POSIX threads (pthreads) implementations applied to Canny edge detection on Raspberry Pi hardware. Focuses on parallelizing Gaussian blur operations in both x and y directions within the edge detection pipeline.

## Implementation Details
Three versions implemented:
- **Sequential baseline** (`canny_local.c`): Single-threaded Gaussian blur operations
- **OpenMP version** (`canny_local_omp.c`): Parallelized using `#pragma omp parallel for` directives  
- **PThreads version** (`canny_local_pthreads.c`): Manual thread creation with work partitioning

## Hardware Requirements
- Raspberry Pi 5 (quad-core ARM Cortex-A76)
- Camera module (for real-time testing)
- 32GB+ microSD card

## Software Dependencies
```bash
sudo apt update
sudo apt install -y build-essential gcc libomp-dev libpthread-stubs0-dev
```

## Compilation Commands
```bash
# Sequential version
gcc canny_local.c -lm -o canny_local

# OpenMP version  
gcc canny_local_omp.c -lm -fopenmp -o canny_local_omp

# PThreads version
gcc canny_local_pthreads.c -lm -lpthread -o canny_local_pthread
```

## Usage
```bash
# Test with static image (sigma=1.0, low=0.2, high=0.6)
time ./canny_local test.pgm 1.0 0.2 0.6
time ./canny_local_omp test.pgm 1.0 0.2 0.6  
time ./canny_local_pthread test.pgm 1.0 0.2 0.6

# Camera integration
./camera_canny_[variant]
```

## Performance Results
Based on test.pgm processing (wall-clock times):

| Implementation | Real Time | User Time | Sys Time | Speedup |
|---------------|-----------|-----------|----------|---------|
| Sequential    | 0.947s    | 0.881s    | 0.009s   | 1.00x   |
| OpenMP        | 0.569s    | 0.856s    | 0.020s   | 1.66x   |
| PThreads      | 0.675s    | 0.836s    | 0.024s   | 1.40x   |

## Technical Implementation

### OpenMP Approach
- Uses `#pragma omp parallel for` with private variable declarations
- Automatic work distribution across 4 threads
- Thread count set via `omp_set_num_threads(4)`

### PThreads Approach  
- Manual thread creation using `pthread_create()`
- Custom work partitioning via `thread_args_x` structures
- Column-based work distribution for x-direction blur
- Explicit synchronization with `pthread_join()`

## Camera Integration Results
Real-time processing times (capture + process + save):
- Sequential: 0.084252s
- OpenMP: 0.084138s  
- PThreads: 0.085343s

## Analysis
**Static Image Processing**: OpenMP shows 39% better performance than sequential, 19% better than PThreads. Performance difference attributed to compiler optimization and automatic load balancing.

**Camera Pipeline**: Minimal differences due to I/O bottlenecks. Frame capture and disk write operations dominate total execution time, masking computational improvements.

**Thread Overhead**: PThreads shows higher system time (0.024s vs 0.020s) due to manual thread management overhead.

## Limitations Observed
- Small image sizes limit parallelization benefits
- I/O operations mask computational gains in camera applications  
- Thread creation overhead significant for short-duration tasks
- Thermal throttling may affect sustained performance

## Technical Rationale
OpenMP performance advantage stems from:
- Compiler-optimized thread scheduling (Reference: OpenMP 5.0 specification)
- Automatic cache-aware work distribution
- Reduced synchronization overhead compared to manual pthread management

PThreads provides fine-grained control but requires careful workload partitioning to achieve optimal performance.

## Build Optimization
For Pi-specific optimization:
```bash
gcc -mcpu=cortex-a76 -O3 -fopenmp source.c -lm
```

## Monitoring Commands
```bash
# Monitor CPU usage during execution
top -p $(pgrep canny)

# Check thermal status
vcgencmd measure_temp
```
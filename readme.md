## Requirements:
    - GCC 8 or above
    - Google sparsehash. To install
```
git clone https://github.com/sparsehash/sparsehash 
cd sparsehash; ./configure --prefix=`pwd`
make -j8
make install
```

## Directory structure
    - c++
        - utils (general class definitions- flows, parsing of traces etc.)
        - algorithms (inference algorithms: Flock, Sherlock, NetBouncer, 007)
        - analysis (runs inference end-to-end for various gray failure types)
        - collector (host-agent and the collector process for an real/emulated datacenter)

## Compilation
    - "make" from the top level directory

## Running
    - To reproduce paper results:
        - cd analysis; run reproduce_results with appropriate parameters 

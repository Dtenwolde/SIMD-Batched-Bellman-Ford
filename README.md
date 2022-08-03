# Batched Bellman-Ford vectorisation experiments

## Setup

To count the number of cpu cyles, follow the build instructions at https://www.ecrypt.eu.org/ebats/cpucycles.html

In `main.cpp` you can find three different implementations of the batched Bellman-Ford algorithm published by 
Manuel Then (https://doi.org/10.1007/s13222-017-0261-x).

`main.cpp:scalar_bf()` is an implementation using scalar instructions. 

`main.cpp:scalar_modified_bf()` is an implementaiton using scalar instructions and a possibly optimizations to avoid checking unnecessary lanes.

`main.cpp:vectorised_bf()` will make use of vectorised (SIMD) instructions and provide a potential speed-up. 

## Experimental setup

The number of vertices, edges and sources can be adjusted in `main.cpp`.




## Output correctness

The scalar and vectorised functions generate .txt output files (`scalar_output.txt` and `vectorized_output.txt`).
This can be used to verify the functions generate the same correct result using:
```bash
verify_corretness.sh
```


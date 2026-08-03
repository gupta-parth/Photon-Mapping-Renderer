# Progressive photon mapping

# Description
In this project, we extended our basic ray tracer from Assignment 2 to do global illumination using the progressive photon mapping technique of Knaus and Zwicker. The program renders images offline and is fast due to the parallelization of the ray tracing passes. 

# Compilation

The program requires OpenMP. To compile the program use ```cmake --build build``` and to run the program use ```\build\CS488 name.obj``` where ```name.obj``` is the obj file that you would like to render. 

# Implementation

## Data structures and algorithms
### BVH
I implemented the Surface Area Heuristic which estimates the cost of the split as 
$$ 
C = N_L \cdot A_L + N_R \cdot A_R
$$
where $N_L$ is the number of triangles in the left region and $A_L$ is the total area of those triangles. Hence, the cost takes into acccount the probability that a random ray intersects a region. I implemented a full-sweep version which considers all possible primitives on each axis and hence costs $O(n^2)$ to build wehere $n$ is the number of primitives. To do the implementation, I followed a blog series on BVH by Jacco Bikker ([blog](https://jacco.ompf2.com/2022/04/13/how-to-build-a-bvh-part-1-basics/)) and also consulted PBRT's section 4.3. The code in ```bvh.h``` is based on parts 1 and 2 of the blog. The following table compares render times with SAH-BVH, with the naive non-SAH BVH, and without a BVH.

| Model Name | Triangle Count |Time with SAH | Time with naive BVH | Time without BVH |
| ---------- | --------------:| -------------:| -----------------:| -----------------:|
| CornellBox-Original-triangulated | 36 | 0.583798 s | 0.721308 s | 0.924416 s |
| CornellBox-Water | 7089 | 2.702817 s | 5.247191 s | 112.023004 s |

Each configuration was run five times, and the median render-only wall-clock time is reported. The iterations were set to 10, the photon count to 20000, and the depth to 20 for every run. This smaller workload kept the single-threaded brute-force benchmark below 15 minutes. All times are recorded in seconds.

### KD-tree 




## Input/Output and pre-processing
I am still using the obj loading and mesh processing functions that were provided in the base code. For a model, the associated ```.mtl``` file should have ```Ke``` values provided for every object at the end of the description for the object. This is done because of the way the area lights are configured. In addition, the object which should be treated as glass should have ```glass``` prefixed to their name in the ```.mtl``` file. I manually configured these ```.mtl``` files for the models provided in ```media``` folder. Also, the ```.obj``` files should be triangulated.

## Global constants 
To change camera parameters, you can refer to lines 44-45 in ```main.cpp```. The width and height of the output image can be set at lines 36-37 in ```config.h```. The number of iterations for the main loop, trace depth, number of photons per pass, initial radius, alpha can all be set from ```main.cpp```. 

## Caveats and bugs 

## Acknowledgements
Much of the code here is based on 



## Photon tracing

## Monte carlo ray tracing

## Importance sampling 

## SAH-BVH

No Bvh CornellBox-Water took 2 mins 36 seconds (50 iterations, 100000 photons, 20 depth)
Bvh CornellBox-Water took 2 mins and 1 second (50 iterations, 100000 photons, 20 depth)

## Radiance estimate 

## Fresnal reflectance 

## Area light 

## Rendering model

## Multithreading 
-- talk about Open MP here 


# Objectives



# References and acknowledgements 
1. Claude Knaus and Matthias Zwicker. 2011. Progressive photon mapping: A probabilistic approach. ACM Trans. Graph. 30, 3, Article 25 (May 2011), 13 pages. https://doi.org/10.1145/1966394.1966404
2. Jacco Bikker. April 2022. How to build a BVH. https://jacco.ompf2.com/2022/04/13/how-to-build-a-bvh-part-1-basics/
3. https://www.cs.princeton.edu/courses/archive/fall18/cos526/papers/jensen01.pdf
4. PBRT

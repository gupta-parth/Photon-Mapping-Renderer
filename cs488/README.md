# Progressive photon mapping

# Description
In this project, we extended our basic ray tracer from Assignment 2 to do global illumination using the progressive photon mapping technique of Knaus and Zwicker. The program renders images offline and is fast due to the parallelization of the ray tracing passes. 

# Compilation

The program requires OpenMP. To compile the program use ```cmake --build build``` and to run the program use ```\build\CS488 name.obj``` where ```name.obj``` is the obj file that you would like to render. 

# Implementation

## Data structures 

## Input/Output and pre-processing
I am still using the obj loading and mesh processing functions that were provided in the base code. For a model, the associated ```.mtl``` file should have ```Ke``` values provided for every object at the end of the description for the object. This is done because of the way the area lights are configured. In addition, the object which should be treated as glass should have ```glass``` prefixed to their name in the ```.mtl``` file. I manually configured these ```.mtl``` files for the models provided in ```media``` folder. Also, the ```.obj``` files should be triangulated.

## Global constants 

In ```globals.h```, you can change the camera parameters 


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



# References
1. Claude Knaus and Matthias Zwicker. 2011. Progressive photon mapping: A probabilistic approach. ACM Trans. Graph. 30, 3, Article 25 (May 2011), 13 pages. https://doi.org/10.1145/1966394.1966404


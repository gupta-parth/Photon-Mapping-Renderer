#pragma once


#include "config.h"
#include "scene.h"
#include "photon_map.h"


// TODO: I need to move the Ray tracing code from scene.h in the integrator

class Integrator {
private:
    PhotonMap photonMap; 

    // Number of photons traced per pass
    int numPhotons;
    int depth;
    int iterations;                      

    void buildPhotonMap() {
        /*
        This function does photon tracing and stores photons in the map
        when they hit diffuse surface. Note that the direction of the photon ray 
        needs to be randomized according to the light. We also need to use 
        Russian roulette to decide whether to continue tracing photons or not. Of course, we don't
        trace after depth is reached. We need to sample the direction for tracing using 
        BRDF. 
        */

    }

    void radiance() {
        /*
        This function needs to estimate the radiance at a point using the
        photon map. We find all the photons that are within the radius
        around the hitpoint and then sum up the contribution of the photons 
        and multiply with the BRDF. 
        */
    }
    

public:

    void render() {
        /*
        This function will be in a loop for the specified iterations. In each
        iteration, it will first build a photon map and then do a ray tracing pass.
        For each intersection point, we will need to handle the radiance depending on 
        the material of the intersection point. If its diffuse, then we can just compute 
        the radiance easily using the map. For glass and metal, we have to trace more rays and
        handle reflection and refraction. This function is essentially the Fig 3 from the Knaus &
        Zwicker paper.
        */
    }

};
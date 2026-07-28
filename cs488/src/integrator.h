#pragma once


#include "config.h"
#include "scene.h"
#include "photon_map.h"


class Integrator {
private:
    PhotonMap photonMap; 

    // Number of photons traced per pass
    int numPhotons;
    int depth;
    int iterations;
    float radius;
    const float alpha;         

    void tracePhotons(Scene &scene, int seed) {
        /*
        This function does photon tracing and stores photons in the map
        when they hit diffuse surface. Note that the direction of the photon ray 
        needs to be randomized according to the light. We also need to use 
        Russian roulette to decide whether to continue tracing photons or not. Of course, we don't
        trace after depth is reached. We need to sample the direction for tracing using 
        BRDF. 
        */

    }

    float3 getRadiance(const float3 &wo, HitInfo &HitInfo) {
        /*
        This function needs to estimate the radiance at a point using the
        photon map. We find all the photons that are within the radius
        around the hitpoint and then sum up the contribution of the photons 
        and multiply with the BRDF. 
        */
       return float3(1,1,1);
    }
    

public:

    void render(Scene &scene, Image &image) {
        /*
        This function will be in a loop for the specified iterations. In each
        iteration, it will first build a photon map and then do a ray tracing pass.
        For each intersection point, we will need to handle the radiance depending on 
        the material of the intersection point. If its diffuse, then we can just compute 
        the radiance easily using the map. For glass and metal, we have to trace more rays and
        handle reflection and refraction. This function is essentially the Fig 3 from the Knaus &
        Zwicker paper.
        */
        for (int x = 0; x < iterations; x++) {

            // Photon tracing pass
            // TODO: Clear photon map from the previous iter before tracing new photons
            // TODO: Have some random seed pass into the tracePhotons so a new distribution of photons is traced
            int seed;
            tracePhotons(scene, seed);
            
            // Ray tracing pass now (lines 5 - 13 in Fig 3 in the paper)
            for (int j = 0; j < globalHeight; ++j) {
                for (int i = 0; i < globalWidth; ++i) {
                    Ray ray = scene.eyeRay(i, j);
                    float3 weight = float3(1.0f, 1.0f, 1.0f);        // W in the paper. I use beta described in PBRT implementation 
                    float3 radiance = float3(0.0f, 0.0f, 0.0f);

                    // We trace until diffuse surface is hit
                    for (int k = 0; k < depth; k++) {
                        HitInfo hitInfo;
                        if (scene.intersect(hitInfo, ray)) {
                            if (hitInfo.material->type == MAT_LAMBERTIAN) {
                                radiance = weight * getRadiance(-ray.d, hitInfo);
                                break;
                            }
                            // Specular 
                            else if (hitInfo.material->type == MAT_GLASS || hitInfo.material->type == MAT_METAL) {
                                // Need to generate a new ray here using brdf 
                                BSDFSample sample = hitInfo.material->sampleBSDF(-ray.d, hitInfo.N, TransportMode::Camera);
                                if (!sample.valid) break;
                                weight *= (sample.f * abs(dot(sample.wi, hitInfo.N))) / sample.pdf;
                                ray = Ray(hitInfo.P, sample.wi);
                                // TODO : Maybe do offset 
                            }
                            else {
                                break;
                            }
                        }
                        else break;
                    }
                    image.pixel(i,j) += radiance;
                }
            }
            radius = sqrtf((x + alpha) / (x+1)) * radius;
        }
    }
};
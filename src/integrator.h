#pragma once


#include "config.h"
#include "scene.h"
#include "photon_map.h"
#include "light_sampling.h"
#include <omp.h>


class Integrator {
private:
    PhotonMap photonMap; 

    // Number of photons traced per pass
    int numPhotons;
    int depth;
    int iterations;
    float radius;
    float alpha;


    void tracePhotons(Scene &scene, std::vector<RNG> &samplers) {
        /*
        This function does photon tracing and stores photons in the map
        when they hit diffuse surface. Note that the direction of the photon ray 
        needs to be randomized according to the light. We also need to use 
        Russian roulette to decide whether to continue tracing photons or not. Of course, we don't
        trace after depth is reached. We need to sample the direction for tracing using 
        BRDF. 
        */
        if (scene.areaLightSources.empty()) {
            return;
        }
        std::vector<Photon> photons;
        std::vector<std::vector<Photon>> threadPhotons(samplers.size());
        #pragma omp parallel
        {
            const int threadID = omp_get_thread_num();
            RNG &rng = samplers[threadID];
            std::vector<Photon> &localPhotons = threadPhotons[threadID];
        #pragma omp for schedule(dynamic, 32)
        for (int i = 0; i < numPhotons; i++) {
            LightPhotonSample emissionSample;
            if (!sampleAreaLight(scene.areaLightSources, scene.totalLightArea, emissionSample, rng)) {
                continue;
            }

            // TODO: Maybe check that the flux is correct and not infinite or 0 
            for (int k = 0; k < depth; k++) {
                HitInfo hitInfo; 
                if (scene.intersect(hitInfo, emissionSample.ray)) {
                    if (hitInfo.material->type == MAT_LAMBERTIAN) {
                        localPhotons.push_back({emissionSample.flux, hitInfo.P, -emissionSample.ray.d});
                    }
                    float russian_prob = std::min(1.0f, std::max(emissionSample.flux[0], 
                        std::max(emissionSample.flux[1], emissionSample.flux[2])));

                    // terminate path with probability 1 - russian_prob
                    if (rng.next1D() >= russian_prob) {
                        break;
                    }
                    emissionSample.flux /= russian_prob;
                    BSDFSample sample = hitInfo.material->sampleBSDF(-emissionSample.ray.d, hitInfo.N, TransportMode::Light, rng);
                    if (!sample.valid || sample.pdf <= 0.0f) break;

                    // Correction due to non-symmetry, implemented like pbrt sec 16.1.3
                    const float3 wo = -emissionSample.ray.d;
                    const float3 wi = sample.wi;
                    const float3 ns = normalize(hitInfo.N);
                    const float3 ng = normalize(hitInfo.geometricNormal);
                    const float wins = dot(wi, ns);
                    const float wing = dot(wi, ng);
                    const float wons = dot(wo, ns);
                    const float wong = dot(wo, ng);
                    if (wing * wins <= 0.0f || wong * wons <= 0.0f) break;
                    emissionSample.flux *= sample.f * (std::abs(wons) * std::abs(wing) / std::abs(wong)) / sample.pdf;
                    float offsetSide = dot(wi, ng) >= 0.0f ? 1.0f : -1.0f;
                    emissionSample.ray = Ray(hitInfo.P + offsetSide * Epsilon * ng, wi);
                }
                else break;
            }
            
        }
        }
        std::size_t totalPhotons = 0;
        for (const std::vector<Photon> &localPhotons : threadPhotons) {
            totalPhotons += localPhotons.size();
        }
        photons.reserve(totalPhotons);
        for (std::vector<Photon> &localPhotons : threadPhotons) {
            photons.insert(photons.end(), localPhotons.begin(), localPhotons.end());
            std::vector<Photon>().swap(localPhotons);
        }
        photonMap.setPhotons(std::move(photons));
        photonMap.buildTree();
    }

    float3 getRadiance(const float3 &wo, HitInfo &HitInfo) {
        /*
        This function needs to estimate the radiance at a point using the
        photon map. We find all the photons that are within the radius
        around the hitpoint and then sum up the contribution of the photons 
        and multiply with the BRDF. 
        */
       const float3 normal = normalize(HitInfo.N);
       if (dot(normal, normalize(wo)) <= 0.0f) {
           return float3(0.0f);
       }
       const float3 flux = photonMap.sumFlux(HitInfo.P, radius, normal);
       float3 L = (HitInfo.material->Kd / PI) * flux;
       L /= (numPhotons * PI * radius * radius);
       return L;
    }
    

public:

    Integrator(int iterations, int photons, float alpha, float initRadius, float depth) {
        this->iterations = iterations;
        this->numPhotons = photons;
        this->alpha = alpha;
        this->radius = initRadius;
        this->depth = depth;
    }

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
        const uint64_t baseSeed = 12345u;
        const int maxThreads = omp_get_max_threads();
        std::vector<RNG> samplers;
        for (int threadID = 0; threadID < maxThreads; threadID++) {
            samplers.emplace_back(baseSeed, 0u, static_cast<uint64_t>(threadID));
        }

        for (int x = 1; x <= iterations; x++) {
            std::cout << "Iteration: " << x << std::endl;


            // Photon tracing pass
            photonMap.reset();
            tracePhotons(scene, samplers);
            
            #pragma omp parallel
            {
                RNG &rng = samplers[omp_get_thread_num()];
            
            #pragma omp for schedule(dynamic, 1)
            // Ray tracing pass now (lines 5 - 13 in Fig 3 in the paper)
            for (int j = 0; j < globalHeight; ++j) {
                for (int i = 0; i < globalWidth; ++i) {

                    const float sampleX = float(i) + rng.next1D();
                    const float sampleY = float(j) + rng.next1D();
                    Ray ray = scene.eyeRay(sampleX, sampleY);
                    float3 weight = float3(1.0f, 1.0f, 1.0f);        // W in the paper. I use beta described in PBRT implementation 
                    float3 radiance = float3(0.0f, 0.0f, 0.0f);
                    
                    // We trace until diffuse surface is hit
                    for (int k = 0; k < depth; k++) {
                        HitInfo hitInfo;

                        if (scene.intersect(hitInfo, ray)) {
                            if (dot(hitInfo.geometricNormal, -ray.d) > 0.0f) {
                                radiance += weight * hitInfo.material->Ke;
                            }
                            if (hitInfo.material->type == MAT_LAMBERTIAN) {
                                radiance += weight * getRadiance(-ray.d, hitInfo);
                                break;
                            }
                            // Specular 
                            else if (hitInfo.material->type == MAT_GLASS || hitInfo.material->type == MAT_METAL) {
                                // Need to generate a new ray here using brdf 
                                BSDFSample sample = hitInfo.material->sampleBSDF(-ray.d, hitInfo.N, TransportMode::Camera, rng);
                                if (!sample.valid || sample.pdf <= 0.0f) break;
                                weight *= (sample.f * abs(dot(sample.wi, hitInfo.N))) / sample.pdf;

                                float offset_side = dot(sample.wi, normalize(hitInfo.geometricNormal)) >= 0.0f ? 1.0f : -1.0f;
                                ray = Ray(hitInfo.P + offset_side * 1e-6f * hitInfo.geometricNormal, sample.wi);
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
        }
            radius = sqrtf((x + alpha) / (x+1)) * radius;
        }
        for (int i = 0; i < globalWidth; i++) {
            for (int j = 0; j < globalHeight; j++) {
                image.pixel(i,j) /= iterations;
            }
        }
    }
};

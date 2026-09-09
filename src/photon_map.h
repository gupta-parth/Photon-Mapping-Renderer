#pragma once 

#include "config.h"
#include <utility>

struct Photon {
    float3 flux;                       
    float3 position;
    float3 wi;                                // incident direction
};

/* The photon map will be stored as a k-d tree using the ideas in Section 2 in these notes 
https://graphics.stanford.edu/courses/cs348b-00/course8.pdf*/

class PhotonMap {
  private: 
    struct Node {
      int index;        // tells you which photon the node is representing
      int leftChild;
      int rightChild;
      int dimension;    // splitting dimension
    };

    std::vector<Photon> photons;
    std::vector<Node> nodes;
    int root = -1;

    int balance(std::vector<int> &indices, int start, int end) {
        if (start >= end) return -1;
        // We first need to find the separation axis using bounding box
        int axis = 0; 
        float3 minp = float3(FLT_MAX); 
        float3 maxp = float3(-FLT_MAX);
        for (int i = start; i < end; i++) {
            const float3& p = photons[indices[i]].position;
            if (minp.x > p.x) minp.x = p.x;
            if (minp.y > p.y) minp.y = p.y;
            if (minp.z > p.z) minp.z = p.z;

            if (maxp.x < p.x) maxp.x = p.x;
            if (maxp.y < p.y) maxp.y = p.y;
            if (maxp.z < p.z) maxp.z = p.z;
        }
        const float3& diff = maxp - minp;
        if (diff.y > diff.x && diff.y > diff.z) {
            axis = 1;
        } 

        // in this case, y <= x OR y <= z
        else if (diff.z > diff.x) {
            axis = 2;
        }
        // otherwise x is the biggest axis since (y <= z <= x)
        
        // now we find the median of the photons within that axis
        int median = start + (end - start) / 2;
        std::nth_element(
            indices.begin() + start,
            indices.begin() + median,
            indices.begin() + end,
            [this, axis](int a, int b) {
                return photons[a].position[axis] < photons[b].position[axis];
            });
        
        // index of last node is one less than the size due to 0-indexing
        const int nodeIdx = (int)nodes.size(); 
        nodes.push_back({indices[median], -1, -1, axis});
        nodes[nodeIdx].leftChild = balance(indices, start, median);
        nodes[nodeIdx].rightChild = balance(indices, median+1, end);
        return nodeIdx;
    }

    /*We only care about finding photons within a certain search radius
    since we will use a global reference radius and reduce it in each iteration.*/

    // Returns all photons in accumulate within the radius around the point
    void search(int nodeId, const float3 &point, float r, std::vector<int> &accumulate) const {
        if (nodeId == -1) return;
        const Node &node = nodes[nodeId];
        int photonIdx = node.index;
        float3 photonPosition = photons[photonIdx].position;
        float dist2 = linalg::distance2(photonPosition, point);
        if (dist2 < r * r) {
            accumulate.push_back(photonIdx);
        }
        
        // We have to search the side where the query point lies and 
        // we also check if the sphere with radius r crosses the splitting plane
        int axis = node.dimension;
        float axisDist = point[axis] - photonPosition[axis];
        if (axisDist < 0.0f) {
            search(node.leftChild, point, r, accumulate);
            if (axisDist * axisDist <= r * r) {
                search(node.rightChild, point, r, accumulate);
            }
        }
        else {
            search(node.rightChild, point, r, accumulate);
            if (axisDist * axisDist <= r * r) {
                search(node.leftChild, point, r, accumulate);
            }
        }
    }

    void accumulateFlux(int nodeId, const float3 &point, float radiusSquared,
        const float3 &normal, float3 &flux) const {
        if (nodeId == -1) return;
        const Node &node = nodes[nodeId];
        const Photon &photon = photons[node.index];
        const float3 &photonPosition = photon.position;
        const float dist2 = linalg::distance2(photonPosition, point);
        if (dist2 < radiusSquared && dot(photon.wi, normal) > 0.0f) {
            flux += photon.flux;
        }

        const int axis = node.dimension;
        const float axisDist = point[axis] - photonPosition[axis];
        if (axisDist < 0.0f) {
            accumulateFlux(node.leftChild, point, radiusSquared, normal, flux);
            if (axisDist * axisDist <= radiusSquared) {
                accumulateFlux(node.rightChild, point, radiusSquared, normal, flux);
            }
        }
        else {
            accumulateFlux(node.rightChild, point, radiusSquared, normal, flux);
            if (axisDist * axisDist <= radiusSquared) {
                accumulateFlux(node.leftChild, point, radiusSquared, normal, flux);
            }
        }
    }



  public:
    void setPhotons(const std::vector<Photon> &photons) {
        this->photons = photons;
    }

    void setPhotons(std::vector<Photon> &&photons) {
        this->photons = std::move(photons);
    }

    void buildTree() {
        std::vector<int> indices(photons.size());
        this->nodes.clear();
        for (int i = 0; i < indices.size(); i++) {
            indices[i] = i;
        }
        root = this->balance(indices, 0, (int) indices.size());
    }

    std::vector<int> rangeSearch(const float3 &point, float radius) const {
        std::vector<int> res;
        if (this->root == -1) {
            return res;
        }
        this->search(this->root, point, radius, res);
        return res;
    }

    float3 sumFlux(const float3 &point, float radius, const float3 &normal) const {
        float3 flux = float3(0.0f);
        if (this->root != -1) {
            this->accumulateFlux(this->root, point, radius * radius, normal, flux);
        }
        return flux;
    }

    Photon getPhoton (int &index) {
        return this->photons[index];
    }

    void reset() {
        photons.clear();
        nodes.clear();
        root = -1;
    }





    // For DEBUG/TESTING purposes 
    int getRoot() const {
        return root;
    }

    int getNodeCount() const {
        return (int)nodes.size();
    }

    int getNodePhotonIndex(int nodeIdx) const {
        return nodes[nodeIdx].index;
    }

    int getNodeLeftChild(int nodeIdx) const {
        return nodes[nodeIdx].leftChild;
    }

    int getNodeRightChild(int nodeIdx) const {
        return nodes[nodeIdx].rightChild;
    }

    int getNodeDimension(int nodeIdx) const {
        return nodes[nodeIdx].dimension;
    }

};

#include "photon_map.h"

#include <assert.h>
#include <cmath>

Photon makePhoton(float x, float y, float z) {
    Photon photon;
    photon.flux = float3(1.0f);
    photon.position = float3(x, y, z);
    photon.wi = float3(0.0f, 1.0f, 0.0f);
    return photon;
}

/*
I implemented all the helper functions with the help of Codex. The test case 
was handwritten by me. The exact query that was used to generate the code was:

I want to create a new file called test_photon_map under tests/ and in that file I
want to design a test case with 10 photons to make sure that the PhotonMap is building
correctly and the photons are in the right split. Here are the 10 photons that I want to use:
....
Use helper functions to check the nodes are in the correct split as the Photon map is implemented
as a kdtree. 
*/


std::vector<Photon> makeTenPhotons() {
    std::vector<Photon> photons;

    photons.push_back(makePhoton(0.0f, 0.0f, 0.0f));
    photons.push_back(makePhoton(1.0f, 2.0f, 0.0f));
    photons.push_back(makePhoton(2.0f, 4.0f, 0.0f));
    photons.push_back(makePhoton(3.0f, 6.0f, 0.0f));
    photons.push_back(makePhoton(4.0f, 8.0f, 0.0f));
    photons.push_back(makePhoton(5.0f, 0.0f, 0.0f));
    photons.push_back(makePhoton(6.0f, 0.0f, 0.0f));
    photons.push_back(makePhoton(7.0f, 0.0f, 2.0f));
    photons.push_back(makePhoton(8.0f, 0.0f, 5.0f));
    photons.push_back(makePhoton(9.0f, 0.0f, 8.0f));

    return photons;
}

float getPhotonPositionValue(const std::vector<Photon>& photons, int photonIndex, int dimension) {
    return photons[photonIndex].position[dimension];
}

char getAxisName(int dimension) {
    if (dimension == 0) {
        return 'x';
    }
    if (dimension == 1) {
        return 'y';
    }
    return 'z';
}

void printIndent(int depth) {
    for (int i = 0; i < depth; i++) {
        std::cout << "  ";
    }
}

void printPhotonInfo(
    const PhotonMap& map,
    const std::vector<Photon>& photons,
    int nodeIdx,
    const char* label,
    int depth) {
    int photonIndex = map.getNodePhotonIndex(nodeIdx);
    int dimension = map.getNodeDimension(nodeIdx);
    const float3& position = photons[photonIndex].position;

    printIndent(depth);
    std::cout << label
              << " node=" << nodeIdx
              << " photon=" << photonIndex
              << " split=" << getAxisName(dimension)
              << " position=(" << position.x << ", " << position.y << ", " << position.z << ")"
              << std::endl;
}

void printKdTree(
    const PhotonMap& map,
    const std::vector<Photon>& photons,
    int nodeIdx,
    const char* label,
    int depth) {
    if (nodeIdx == -1) {
        return;
    }

    printPhotonInfo(map, photons, nodeIdx, label, depth);

    printKdTree(map, photons, map.getNodeLeftChild(nodeIdx), "L", depth + 1);
    printKdTree(map, photons, map.getNodeRightChild(nodeIdx), "R", depth + 1);
}

void checkChildIndicesAreValid(const PhotonMap& map) {
    int nodeCount = map.getNodeCount();

    for (int i = 0; i < nodeCount; i++) {
        int leftChild = map.getNodeLeftChild(i);
        int rightChild = map.getNodeRightChild(i);

        assert(leftChild == -1 || (leftChild >= 0 && leftChild < nodeCount));
        assert(rightChild == -1 || (rightChild >= 0 && rightChild < nodeCount));
    }
}

void checkEveryPhotonAppearsOnce(const PhotonMap& map, int photonCount) {
    std::vector<int> timesSeen(photonCount);

    for (int i = 0; i < map.getNodeCount(); i++) {
        int photonIndex = map.getNodePhotonIndex(i);
        assert(photonIndex >= 0);
        assert(photonIndex < photonCount);
        timesSeen[photonIndex] = timesSeen[photonIndex] + 1;
    }

    for (int i = 0; i < photonCount; i++) {
        assert(timesSeen[i] == 1);
    }
}

void checkSubtreeLessOrEqual(
    const PhotonMap& map,
    const std::vector<Photon>& photons,
    int nodeIdx,
    int dimension,
    float splitValue) {
    if (nodeIdx == -1) {
        return;
    }

    int photonIndex = map.getNodePhotonIndex(nodeIdx);
    float value = getPhotonPositionValue(photons, photonIndex, dimension);
    assert(value <= splitValue);

    checkSubtreeLessOrEqual(map, photons, map.getNodeLeftChild(nodeIdx), dimension, splitValue);
    checkSubtreeLessOrEqual(map, photons, map.getNodeRightChild(nodeIdx), dimension, splitValue);
}

void checkSubtreeGreaterOrEqual(
    const PhotonMap& map,
    const std::vector<Photon>& photons,
    int nodeIdx,
    int dimension,
    float splitValue) {
    if (nodeIdx == -1) {
        return;
    }

    int photonIndex = map.getNodePhotonIndex(nodeIdx);
    float value = getPhotonPositionValue(photons, photonIndex, dimension);
    assert(value >= splitValue);

    checkSubtreeGreaterOrEqual(map, photons, map.getNodeLeftChild(nodeIdx), dimension, splitValue);
    checkSubtreeGreaterOrEqual(map, photons, map.getNodeRightChild(nodeIdx), dimension, splitValue);
}

void checkKdTreeSplits(const PhotonMap& map, const std::vector<Photon>& photons, int nodeIdx) {
    if (nodeIdx == -1) {
        return;
    }

    int photonIndex = map.getNodePhotonIndex(nodeIdx);
    int dimension = map.getNodeDimension(nodeIdx);
    float splitValue = getPhotonPositionValue(photons, photonIndex, dimension);

    checkSubtreeLessOrEqual(map, photons, map.getNodeLeftChild(nodeIdx), dimension, splitValue);
    checkSubtreeGreaterOrEqual(map, photons, map.getNodeRightChild(nodeIdx), dimension, splitValue);

    checkKdTreeSplits(map, photons, map.getNodeLeftChild(nodeIdx));
    checkKdTreeSplits(map, photons, map.getNodeRightChild(nodeIdx));
}

void checkExpectedTopSplits(const PhotonMap& map) {
    int root = map.getRoot();
    int leftChild = map.getNodeLeftChild(root);
    int rightChild = map.getNodeRightChild(root);

    assert(map.getNodePhotonIndex(root) == 5);
    assert(map.getNodeDimension(root) == 0);

    assert(leftChild != -1);
    assert(map.getNodePhotonIndex(leftChild) == 2);
    assert(map.getNodeDimension(leftChild) == 1);

    assert(rightChild != -1);
    assert(map.getNodePhotonIndex(rightChild) == 8);
    assert(map.getNodeDimension(rightChild) == 2);
}

void checkFloat3Equal(const float3& actual, const float3& expected) {
    const float epsilon = 1e-6f;
    assert(std::abs(actual.x - expected.x) < epsilon);
    assert(std::abs(actual.y - expected.y) < epsilon);
    assert(std::abs(actual.z - expected.z) < epsilon);
}

Photon makeFluxPhoton(
    const float3& position,
    const float3& flux,
    const float3& wi) {
    Photon photon;
    photon.position = position;
    photon.flux = flux;
    photon.wi = wi;
    return photon;
}

void checkSumFluxMatchesLegacySearch() {
    std::vector<Photon> photons;
    photons.push_back(makeFluxPhoton(
        float3(0.0f, 0.0f, 0.0f), float3(1.0f, 2.0f, 3.0f), float3(0.0f, 1.0f, 0.0f)));
    photons.push_back(makeFluxPhoton(
        float3(0.2f, 0.0f, 0.0f), float3(4.0f, 5.0f, 6.0f), float3(0.0f, 2.0f, 0.0f)));
    photons.push_back(makeFluxPhoton(
        float3(0.0f, 0.2f, 0.0f), float3(8.0f, 8.0f, 8.0f), float3(0.0f, -1.0f, 0.0f)));
    photons.push_back(makeFluxPhoton(
        float3(0.0f, 0.0f, 0.2f), float3(16.0f, 16.0f, 16.0f), float3(1.0f, 0.0f, 0.0f)));
    photons.push_back(makeFluxPhoton(
        float3(2.0f, 0.0f, 0.0f), float3(32.0f, 32.0f, 32.0f), float3(0.0f, 1.0f, 0.0f)));

    PhotonMap map;
    map.setPhotons(photons);
    map.buildTree();

    const float3 point(0.1f, 0.0f, 0.0f);
    const float radius = 0.5f;
    const float3 normal(0.0f, 1.0f, 0.0f);
    float3 legacyFlux(0.0f);
    std::vector<int> photonIndices = map.rangeSearch(point, radius);
    for (int photonIndex : photonIndices) {
        const Photon photon = map.getPhoton(photonIndex);
        if (dot(photon.wi, normal) > 0.0f) {
            legacyFlux += photon.flux;
        }
    }

    checkFloat3Equal(map.sumFlux(point, radius, normal), legacyFlux);
    checkFloat3Equal(legacyFlux, float3(5.0f, 7.0f, 9.0f));
    checkFloat3Equal(
        map.sumFlux(point, radius, -normal),
        float3(8.0f, 8.0f, 8.0f));
}

void checkSumFluxUsesStrictRadius() {
    const float justInside = std::nextafter(1.0f, 0.0f);
    std::vector<Photon> photons;
    photons.push_back(makeFluxPhoton(
        float3(1.0f, 0.0f, 0.0f), float3(1.0f, 0.0f, 0.0f), float3(0.0f, 1.0f, 0.0f)));
    photons.push_back(makeFluxPhoton(
        float3(justInside, 0.0f, 0.0f), float3(0.0f, 2.0f, 0.0f), float3(0.0f, 1.0f, 0.0f)));

    PhotonMap map;
    map.setPhotons(photons);
    map.buildTree();

    checkFloat3Equal(
        map.sumFlux(float3(0.0f), 1.0f, float3(0.0f, 1.0f, 0.0f)),
        float3(0.0f, 2.0f, 0.0f));
}

int main() {
    std::vector<Photon> photons = makeTenPhotons();

    PhotonMap map;
    map.setPhotons(photons);
    map.buildTree();

    std::cout << "KD tree:" << std::endl;
    printKdTree(map, photons, map.getRoot(), "root", 0);

    assert(map.getRoot() != -1);
    assert(map.getNodeCount() == 10);

    checkChildIndicesAreValid(map);
    checkEveryPhotonAppearsOnce(map, (int)photons.size());
    checkExpectedTopSplits(map);
    checkKdTreeSplits(map, photons, map.getRoot());
    checkSumFluxMatchesLegacySearch();
    checkSumFluxUsesStrictRadius();

    std::cout << "test_photon_map passed" << std::endl;
    return 0;
}

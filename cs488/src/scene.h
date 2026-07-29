#pragma once

#include "bvh.h"
#include "globals.h"
#include "image.h"
#include "mesh.h"

// scene definition
class Scene {
public:
	std::vector<TriangleMesh*> objects;
	std::vector<PointLightSource*> pointLightSources;
	std::vector<BVH> bvhs;

	void addObject(TriangleMesh* pObj) {
		objects.push_back(pObj);
	}
	void addLight(PointLightSource* pObj) {
		pointLightSources.push_back(pObj);
	}

	void preCalc() {
		bvhs.resize(objects.size());
		for (int i = 0; i < objects.size(); i++) {
			objects[i]->preCalc();
			bvhs[i].build(objects[i]);
		}
	}

	// ray-scene intersection
	bool intersect(HitInfo& minHit, const Ray& ray, float tMin = 0.0f, float tMax = FLT_MAX) const {
		bool hit = false;
		HitInfo tempMinHit;
		minHit.t = FLT_MAX;

		for (int i = 0, i_n = (int)objects.size(); i < i_n; i++) {
			//if (objects[i]->bruteforceIntersect(tempMinHit, ray, tMin, tMax)) { // for debugging
			if (bvhs[i].intersect(tempMinHit, ray, tMin, tMax)) {
				if (tempMinHit.t < minHit.t) {
					hit = true;
					minHit = tempMinHit;
				}
			}
		}
		return hit;
	}

	// camera -> screen matrix (given to you for A1)
	float4x4 perspectiveMatrix(float fovy, float aspect, float zNear, float zFar) const {
		float4x4 m;
		const float f = 1.0f / (tan(fovy * DegToRad / 2.0f));
		m[0] = { f / aspect, 0.0f, 0.0f, 0.0f };
		m[1] = { 0.0f, f, 0.0f, 0.0f };
		m[2] = { 0.0f, 0.0f, (zFar + zNear) / (zNear - zFar), -1.0f };
		m[3] = { 0.0f, 0.0f, (2.0f * zFar * zNear) / (zNear - zFar), 0.0f };

		return m;
	}

	// model -> camera matrix (given to you for A1)
	float4x4 lookatMatrix(const float3& _eye, const float3& _center, const float3& _up) const {
		// transformation to the camera coordinate
		float4x4 m;
		const float3 f = normalize(_center - _eye);
		const float3 upp = normalize(_up);
		const float3 s = normalize(cross(f, upp));
		const float3 u = cross(s, f);

		m[0] = { s.x, s.y, s.z, 0.0f };
		m[1] = { u.x, u.y, u.z, 0.0f };
		m[2] = { -f.x, -f.y, -f.z, 0.0f };
		m[3] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m = transpose(m);

		// translation according to the camera location
		const float4x4 t = float4x4{ {1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, { -_eye.x, -_eye.y, -_eye.z, 1.0f} };

		m = mul(m, t);
		return m;
	}


	// eye ray generation (given to you for A2)
	Ray eyeRay(int x, int y) const {
		// compute the camera coordinate system 
		const float3 wDir = normalize(float3(-globalViewDir));
		const float3 uDir = normalize(cross(globalUp, wDir));
		const float3 vDir = cross(wDir, uDir);

		// compute the pixel location in the world coordinate system using the camera coordinate system
		// trace a ray through the center of each pixel
		const float imPlaneUPos = (x + 0.5f) / float(globalWidth) - 0.5f;
		const float imPlaneVPos = (y + 0.5f) / float(globalHeight) - 0.5f;

		const float3 pixelPos = globalEye + float(globalAspectRatio * globalFilmSize * imPlaneUPos) * uDir + float(globalFilmSize * imPlaneVPos) * vDir - globalDistanceToFilm * wDir;

		return Ray(globalEye, normalize(pixelPos - globalEye));
	}


};
static Scene globalScene;

#pragma once

#include "config.h"
#include "geometry.h"
#include "image.h"


class EnvironmentMap {
public:
	Image *img;
	bool isLoaded = false;

	bool load(const char* filename) {
		img = new Image();
		img->load(filename);
		isLoaded = true;
		std::cout << "Image loaded true " << std::endl;
		return true;
	}
	float3 fetch(const Ray* ray) {

		if (!isLoaded) {
			return float3(0.0f);
		}

		float3 dir = normalize(ray->d);
		float u = dir.x * (1/PI) * acosf(dir.z) / sqrt(dir.x * dir.x + dir.y * dir.y);
		float v = dir.y * (1/PI) * acosf(dir.z) / sqrt(dir.x * dir.x + dir.y * dir.y);
		int x = int(((u + 1)/ 2) * img->width);
		int y = int(((v+1)/ 2) * img->height);
		return img->pixel(x, y);
	}

};
static EnvironmentMap globalEnvironmentMap;

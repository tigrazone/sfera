/***************************************************************************
 *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt)                  *
 *                                                                         *
 *   This file is part of Sfera.                                           *
 *                                                                         *
 *   Sfera is free software; you can redistribute it and/or modify         *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Sfera is distributed in the hope that it will be useful,              *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#include "renderer/cpu/singlecpurenderer.h"

SingleCPURenderer::SingleCPURenderer(const GameLevel *level) :
	LevelRenderer(level), rnd(1) {
	sampleFrameBuffer = new SampleFrameBuffer(
			gameLevel->gameConfig->GetScreenWidth(),
			gameLevel->gameConfig->GetScreenHeight());
	frameBuffer = new FrameBuffer(
			gameLevel->gameConfig->GetScreenWidth(),
			gameLevel->gameConfig->GetScreenHeight());

	sampleFrameBuffer->Clear();
	frameBuffer->Clear();
}

SingleCPURenderer::~SingleCPURenderer() {
	delete sampleFrameBuffer;
	delete frameBuffer;
}

Spectrum SingleCPURenderer::SampleImage(const float u0, const float u1) {
	Ray ray;
	gameLevel->camera->GenerateRay(
		u0, u1,
		sampleFrameBuffer->GetWidth(), sampleFrameBuffer->GetHeight(),
		&ray, rnd.floatValue(), rnd.floatValue());

	const Scene &scene(*(gameLevel->scene));
	Spectrum throughput(1.f, 1.f, 1.f);
	Spectrum radiance(0.f, 0.f, 0.f);

	unsigned int diffuseBounces = 0;
	const unsigned int maxDiffuseBounces = gameLevel->maxPathDiffuseBounces;
	unsigned int specularGlossyBounces = 0;
	const unsigned int maxSpecularGlossyBounces = gameLevel->maxPathSpecularGlossyBounces;

	for(;;) {
		// Check for intersection with objects
		const vector<GameSphere> &spheres(scene.spheres);
		bool hit = false;
		size_t sphereIndex = 0;
		for (size_t s = 0; s < spheres.size(); ++s) {
			if (spheres[s].sphere.Intersect(&ray)) {
				hit = true;
				sphereIndex = s;
			}
		}

		const Sphere *hitSphere = NULL;
		const Material *hitMat = NULL;
		const TexMapInstance *texMap = NULL;
		const BumpMapInstance *bumpMap = NULL;
		if (hit) {
			hitSphere = &spheres[sphereIndex].sphere;
			hitMat = scene.sphereMaterials[sphereIndex];
			texMap = scene.sphereTexMaps[sphereIndex];
			bumpMap = scene.sphereBumpMaps[sphereIndex];
		}

		// Check for intersection with the player body
		for (size_t s = 0; s < GAMEPLAYER_PUPPET_SIZE; ++s) {
			const Sphere *puppet = &(gameLevel->player->puppet[s]);

			if (puppet->Intersect(&ray)) {
				hit = true;
				hitSphere = puppet;
				hitMat = gameLevel->player->puppetMaterial[s];
				texMap = NULL;
				bumpMap = NULL;
			}
		}

		if (hit) {
			const Point hitPoint(ray(ray.maxt));
			Normal N(Normalize(hitPoint - hitSphere->center));

			// Check if I have to flip the normal
			Normal ShadeN = (Dot(Vector(N), ray.d) > 0.f) ? (-N) : N;
			if (bumpMap) {
				// Apply bump mapping
				const Vector dir = Normalize(hitPoint - hitSphere->center);

				ShadeN = bumpMap->SphericalMap(dir, ShadeN);
			}

			radiance += throughput * hitMat->GetEmission();

			Vector wi;
			float pdf;
			bool diffuseBounce;
			Spectrum f = hitMat->Sample_f(-ray.d, &wi, N, ShadeN,
					rnd.floatValue(), rnd.floatValue(), rnd.floatValue(),
					&pdf, diffuseBounce);
			if ((pdf <= 0.f) || f.Black())
				return radiance;

			// Check if there is a texture map to apply
			if (texMap) {
				const Vector dir = Normalize(hitPoint - hitSphere->center);
				f *= texMap->SphericalMap(dir);
			}

			if (diffuseBounce) {
				++diffuseBounces;

				if (diffuseBounces > maxDiffuseBounces)
					return radiance;
			} else {
				++specularGlossyBounces;

				if (specularGlossyBounces > maxSpecularGlossyBounces)
					return radiance;
			}

			throughput *= f / pdf;

			ray = Ray(hitPoint, wi);
		} else
			return radiance + throughput * scene.infiniteLight->Le(ray.d);
	}
}

void SingleCPURenderer::DrawFrame() {
	const unsigned int width = gameLevel->gameConfig->GetScreenWidth();
	const unsigned int height = gameLevel->gameConfig->GetScreenHeight();

	// Render
	for (unsigned int y = 0; y < height; ++y) {
		for (unsigned int x = 0; x < width; ++x) {
			Spectrum s = SampleImage(x + rnd.floatValue() - .5f, y + rnd.floatValue() - .5f);

			sampleFrameBuffer->BlendPixel(x, y, s, .33f);
			//sampleFrameBuffer->SetPixel(x, y, s, 1.f);
			//sampleFrameBuffer->AddPixel(x, y, s, 1.f);
		}
	}

	// Tone mapping
	gameLevel->toneMap->Map(sampleFrameBuffer, frameBuffer);

	glDrawPixels(width, height, GL_RGB, GL_FLOAT, frameBuffer->GetPixels());
}

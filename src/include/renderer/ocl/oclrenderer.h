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

#ifndef _SFERA_OCLRENDERER_H
#define	_SFERA_OCLRENDERER_H

#if !defined(SFERA_DISABLE_OPENCL)

#include "gamelevel.h"
#include "renderer/levelrenderer.h"
#include "pixel/framebuffer.h"

namespace ocl_kernels {

typedef struct {
	unsigned int s1, s2, s3;
} Seed;

typedef struct {
	// The task seed
	Seed seed;
} GPUTask;

typedef struct {
	float lensRadius;
	float focalDistance;
	float yon, hither;

	float rasterToCameraMatrix[4][4];
	float cameraToWorldMatrix[4][4];
} Camera;

#define MAT_MATTE 0
#define MAT_MIRROR 1
#define MAT_GLASS 2
#define MAT_METAL 3
#define MAT_ALLOY 4

typedef struct {
    float r, g, b;
} MatteParam;

typedef struct {
    float r, g, b;
} MirrorParam;

typedef struct {
    float refl_r, refl_g, refl_b;
    float refrct_r, refrct_g, refrct_b;
    float ousideIor, ior;
    float R0;
} GlassParam;

typedef struct {
    float r, g, b;
    float exponent;
} MetalParam;

typedef struct {
    float diff_r, diff_g, diff_b;
    float refl_r, refl_g, refl_b;
    float exponent;
    float R0;
} AlloyParam;

typedef struct {
	unsigned int type;
	union {
		MatteParam matte;
		MirrorParam mirror;
        GlassParam glass;
        MetalParam metal;
        AlloyParam alloy;
	} param;
} Material;

}

class OCLRenderer : public LevelRenderer {
public:
	OCLRenderer(const GameLevel *level);
	~OCLRenderer();

	size_t DrawFrame(const EditActionList &editActionList);

protected:
	void AllocOCLBufferRO(cl::Buffer **buff, void *src, const size_t size, const string &desc);
	void AllocOCLBufferRW(cl::Buffer **buff, const size_t size, const string &desc);
	void FreeOCLBuffer(cl::Buffer **buff);

	cl::Device dev;
	cl::Context *ctx;
	cl::CommandQueue *cmdQueue;

	cl::Kernel *kernelInit;
	cl::Kernel *kernelInitToneMapFB;
	cl::Kernel *kernelUpdatePixelBuffer;
	cl::Kernel *kernelPathTracing;

	cl::Buffer *toneMapFrameBuffer;
	cl::Buffer *bvhBuffer;
	cl::Buffer *gpuTaskBuffer;
	cl::Buffer *cameraBuffer;
	cl::Buffer *infiniteLightBuffer;

	GLuint pbo;
	cl::BufferGL *pboBuff;

	size_t usedDeviceMemory;

	FrameBuffer *frameBuffer;

	ocl_kernels::Camera camera;

	double timeSinceLastCameraEdit, timeSinceLastNoCameraEdit;
};

#endif

#endif	/* _SFERA_OCLRENDERER_H */
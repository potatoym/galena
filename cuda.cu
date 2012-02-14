#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <cuda_gl_interop.h>
#include <cutil.h>

extern "C" bool initialize_cuda(int argc, char **argv);
extern "C" void pbo_register_cuda(int pbo);
extern "C" void pbo_unregister_cuda(int pbo);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool initialize_cuda(int argc, char **argv)
{
    CUT_DEVICE_INIT(argc, argv);
    return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void pbo_register_cuda(int pbo)
{
    CUDA_SAFE_CALL(cudaGLRegisterBufferObject(pbo));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void pbo_unregister_cuda(int pbo)
{
    CUDA_SAFE_CALL(cudaGLUnregisterBufferObject(pbo));
}



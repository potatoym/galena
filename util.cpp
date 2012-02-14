// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// galena utility routines
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <cutil_inline.h>
#include "my_cutil_gl_error.h"

#include "util.h"

// cuda.cu calls
extern "C" void pbo_register_cuda(int pbo);
extern "C" void pbo_unregister_cuda(int pbo);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
create_pbo(GLuint* pbo, unsigned int width, unsigned int height, void *data_in)
{
//    printf("I AM HERE\n");
    unsigned int size_data = sizeof(GLubyte) * width * height * 4;
    void *data;
    if(data_in == NULL) {
        data = malloc(size_data);
    } else {
        data = data_in;
    }

    glGenBuffers(1, pbo);
    glBindBuffer(GL_ARRAY_BUFFER, *pbo);

    glBufferData(GL_ARRAY_BUFFER, size_data, data, GL_DYNAMIC_DRAW);
    if(data_in == NULL) {
        free(data);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // attach this Buffer Object to CUDA
    pbo_register_cuda(*pbo);
    CUT_CHECK_ERROR_GL();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
create_texture(GLuint *tex_name, unsigned int width, unsigned int height, GLubyte *data)
{
    glGenTextures(1, tex_name);
    glBindTexture(GL_TEXTURE_2D, *tex_name);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    CUT_CHECK_ERROR_GL();
}


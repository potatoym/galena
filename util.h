#ifndef GALENA_UTIL_H
#define GALENA_UTIL_H
#include <GL/gl.h>
void create_pbo(GLuint* pbo, unsigned int width, unsigned int height, void *data_in);
void create_texture(GLuint* tex, unsigned int width, unsigned int height, GLubyte *data);
#endif

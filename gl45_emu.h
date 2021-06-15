#pragma once

#include "gl.h"

#define USE_GL3 1

void gl3CopyImageSubData(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ,
	GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ,
	GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth);

void gl3GetTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
	GLsizei width, GLsizei height, GLsizei depth,
	GLenum format, GLenum type, GLsizei bufSize, void *pixels);

void gl3TextureStorage2D(GLuint tex, GLint levels, GLenum ifmt, GLsizei w, GLsizei h);
void gl3TextureStorage3D(GLuint tex, GLint levels, GLenum ifmt, GLsizei w, GLsizei h, GLsizei d);
void gl3CreateTextures(GLenum target, GLsizei num, GLuint* arr);
void gl3TextureSubImage2D(GLuint tex, GLint level, GLint x, GLint y, GLsizei w, GLsizei h, GLenum fmt, GLenum type, const void *pix);
void gl3BindTextureUnit2D(GLuint unit, GLuint tex);
void gl3BindTextureUnit3D(GLuint unit, GLuint tex);
void gl3TextureParameteri2D(GLuint tex, GLenum param, GLint val);
void gl3TextureParameteri3D(GLuint tex, GLenum param, GLint val);
void gl3TextureParameterfv2D(GLuint tex, GLenum param, GLfloat* val);
void gl3TextureParameterfv3D(GLuint tex, GLenum param, GLfloat* val);
void gl3CreateBuffers(GLsizei num, GLuint* arr);
void gl3NamedBufferStorage(GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags);
void gl3NamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data);
void gl3CreateVertexArrays(GLsizei num, GLuint* arr);

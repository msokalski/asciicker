
#include <stdio.h>
#include "gl45_emu.h"

void gl3CopyImageSubData(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ,
						GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ,
						GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth)
{
#if USE_GL3
	// all calls ported
	static bool warn_once = true;
	if (warn_once)
	{
		warn_once = false;
		printf("WARNING: GL3 calling unimplemented glCopyImageSubData()\n");
	}
#else
	glCopyImageSubData(srcName, srcTarget, srcLevel, srcX, srcY, srcZ,
		dstName, dstTarget, dstLevel, dstX, dstY, dstZ,
		srcWidth, srcHeight, srcDepth);
#endif
}

void gl3GetTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, 
						   GLsizei width, GLsizei height, GLsizei depth,
						   GLenum format, GLenum type, GLsizei bufSize, void *pixels)
{
#if USE_GL3
	// TODO later
	// used by font editor (get pixel) & glyphs coverage calculator (not needed for end user)
	static bool warn_once = true;
	if (warn_once)
	{
		warn_once = false;
		printf("WARNING: GL3 calling unimplemented glGetTextureSubImage()\n");
	}
#else
	glGetTextureSubImage(texture, level, xoffset, yoffset, zoffset,
		width, height, depth, format, type, bufSize, pixels);
#endif
}

void gl3TextureStorage2D(GLuint tex, GLint levels, GLenum ifmt, GLsizei w, GLsizei h)
{
#if USE_GL3
	int t;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &t);
	glBindTexture(GL_TEXTURE_2D, tex);
	for (int lev = 0; lev < levels; lev++)
	{
		if (ifmt == GL_RGBA8UI)
			glTexImage2D(GL_TEXTURE_2D, lev, ifmt, w, h, 0, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, 0);
		else
		if (ifmt == GL_R16UI)
			glTexImage2D(GL_TEXTURE_2D, lev, ifmt, w, h, 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, 0);
		else
			glTexImage2D(GL_TEXTURE_2D, lev, ifmt, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		w >>= 1;
		h >>= 1;
		if (!w)
			w = 1;
		if (!h)
			h = 1;
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, levels - 1);
	glBindTexture(GL_TEXTURE_2D, t);
#else
	glTextureStorage2D(tex, levels, ifmt, w, h);
#endif
}

void gl3TextureStorage3D(GLuint tex, GLint levels, GLenum ifmt, GLsizei w, GLsizei h, GLsizei d)
{
#if USE_GL3
	int t;
	glGetIntegerv(GL_TEXTURE_BINDING_3D, &t);
	glBindTexture(GL_TEXTURE_3D, tex);
	for (int lev = 0; lev < levels; lev++)
	{
		glTexImage3D(GL_TEXTURE_3D, lev, ifmt, w, h, d, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		w >>= 1;
		h >>= 1;
		d >>= 1;
		if (!w)
			w = 1;
		if (!h)
			h = 1;
		if (!d)
			d = 1;
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, levels - 1);
	glBindTexture(GL_TEXTURE_3D, t);
#else
	glTextureStorage3D(tex, levels, ifmt, w, h, d);
#endif
}


void gl3CreateTextures(GLenum target, GLsizei num, GLuint* arr)
{
#if USE_GL3
	glGenTextures(num, arr);
#else
	glCreateTextures(target, num, arr);
#endif
}

void gl3TextureSubImage2D(GLuint tex, GLint level, GLint x, GLint y, GLsizei w, GLsizei h, GLenum fmt, GLenum type, const void *pix)
{
#if USE_GL3
	int t;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &t);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexSubImage2D(GL_TEXTURE_2D, level, x, y, w, h, fmt, type, pix);
	glBindTexture(GL_TEXTURE_2D, t);
#else
	glTextureSubImage2D(tex, level, x, y, w, h, fmt, type, pix);
#endif
}

void gl3BindTextureUnit2D(GLuint unit, GLuint tex)
{
#if USE_GL3
	int u;
	glGetIntegerv(GL_ACTIVE_TEXTURE, &u);
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, tex);
	if (!tex)
		glBindTexture(GL_TEXTURE_3D, 0);
	glActiveTexture(u);
#else
	glBindTextureUnit(unit, tex);
#endif
}

void gl3BindTextureUnit3D(GLuint unit, GLuint tex)
{
#if USE_GL3
	int u;
	glGetIntegerv(GL_ACTIVE_TEXTURE, &u);
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_3D, tex);
	if (!tex)
		glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(u);
#else
	glBindTextureUnit(unit, tex);
#endif
}

void gl3TextureParameteri2D(GLuint tex, GLenum param, GLint val)
{
#if USE_GL3
	int t;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &t);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, param, val);
	glBindTexture(GL_TEXTURE_2D, t);
#else
	glTextureParameteri(tex, param, val);
#endif
}

void gl3TextureParameteri3D(GLuint tex, GLenum param, GLint val)
{
#if USE_GL3
	int t;
	glGetIntegerv(GL_TEXTURE_BINDING_3D, &t);
	glBindTexture(GL_TEXTURE_3D, tex);
	glTexParameteri(GL_TEXTURE_3D, param, val);
	glBindTexture(GL_TEXTURE_3D, t);
#else
	glTextureParameteri(tex, param, val);
#endif
}

void gl3TextureParameterfv2D(GLuint tex, GLenum param, GLfloat* val)
{
#if USE_GL3
	int t;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &t);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameterfv(GL_TEXTURE_2D, param, val);
	glBindTexture(GL_TEXTURE_2D, t);
#else
	glTextureParameterfv(tex, param, val);
#endif
}

void gl3TextureParameterfv3D(GLuint tex, GLenum param, GLfloat* val)
{
#if USE_GL3
	int t;
	glGetIntegerv(GL_TEXTURE_BINDING_3D, &t);
	glBindTexture(GL_TEXTURE_3D, tex);
	glTexParameterfv(GL_TEXTURE_3D, param, val);
	glBindTexture(GL_TEXTURE_3D, t);
#else
	glTextureParameterfv(tex, param, val);
#endif
}

void gl3CreateBuffers(GLsizei num, GLuint* arr)
{
#if USE_GL3
	glGenBuffers(num, arr);
#else
	glCreateBuffers(num, arr);
#endif
}

void gl3NamedBufferStorage(GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags)
{
#if USE_GL3
	glBindBuffer(GL_COPY_WRITE_BUFFER, buffer);
	GLenum usage = GL_STATIC_DRAW;
	if (flags & GL_DYNAMIC_STORAGE_BIT)
		usage = GL_DYNAMIC_DRAW;
	glBufferData(GL_COPY_WRITE_BUFFER, size, data, GL_DYNAMIC_DRAW);
#else
	glNamedBufferStorage(buffer, size, data, flags);
#endif
}

void gl3NamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data)
{
#if USE_GL3
	glBindBuffer(GL_COPY_WRITE_BUFFER, buffer);
	glBufferSubData(GL_COPY_WRITE_BUFFER, offset, size, data);
#else
	glNamedBufferSubData(buffer, offset, size, data);
#endif
}

void gl3CreateVertexArrays(GLsizei num, GLuint* arr)
{
#if USE_GL3
	glGenVertexArrays(num, arr);
#else
	glCreateVertexArrays(num, arr);
#endif
}

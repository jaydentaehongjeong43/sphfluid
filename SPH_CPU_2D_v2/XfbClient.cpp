#include "XfbClient.h"
#include <GL/glew.h>

#ifdef QUERY_XFB_PRIMS_WRITTEN
#include <iostream>
#endif

#ifdef QUERY_XFB_PRIMS_WRITTEN
unsigned query;
#endif

XfbClient::XfbClient()
{
	glGenBuffers(1, &buffer_);
	glGenTransformFeedbacks(1, &xfb_);
#ifdef QUERY_XFB_PRIMS_WRITTEN
	glGenQueries(1, &query);
#endif
}

XfbClient::~XfbClient()
{
	glDeleteBuffers(1, &buffer_);
	glDeleteTransformFeedbacks(1, &xfb_);
#ifdef QUERY_XFB_PRIMS_WRITTEN
	glDeleteQueries(1, &query);
#endif
}

void XfbClient::BeginTransformFeedback(unsigned const xfb, int const primitiveMode)
{
	glEnable(GL_RASTERIZER_DISCARD);
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, xfb);
#ifdef QUERY_XFB_PRIMS_WRITTEN
	glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, query);
#endif
	glBeginTransformFeedback(primitiveMode);
}

void XfbClient::EndTransformFeedback()
{
	glEndTransformFeedback();
#ifdef QUERY_XFB_PRIMS_WRITTEN
	glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
#endif
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
	glDisable(GL_RASTERIZER_DISCARD);
#ifdef QUERY_XFB_PRIMS_WRITTEN
	GLint primitives;
	glGetQueryObjectiv(query, GL_QUERY_RESULT, &primitives);
	std::cout << "[XfbClient] primitives written = " << primitives << std::endl;
#endif
}

bool XfbClient::needInitBuffer() const
{
	return lastBufferSize_ != getBufferSize();
}

unsigned XfbClient::initBuffer() const
{
	auto const bufferSize{getBufferSize()};
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, getBuffer());
	glBufferData(GL_SHADER_STORAGE_BUFFER, bufferSize, nullptr, GL_STREAM_COPY);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	return bufferSize;
}

void XfbClient::initXfb() const
{
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, getXfb());
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, getBuffer());
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
}

const unsigned& XfbClient::getBuffer() const
{
	return buffer_;
}

const unsigned& XfbClient::getXfb() const
{
	return xfb_;
}
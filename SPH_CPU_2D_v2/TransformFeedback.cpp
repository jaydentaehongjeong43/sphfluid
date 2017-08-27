#include "stdafx.h"
#include "TransformFeedback.h"

TransformFeedback::TransformFeedback()
{
	glGenTransformFeedbacks(1, &xfb_);
}

TransformFeedback::~TransformFeedback()
{
	glDeleteTransformFeedbacks(1, &xfb_);
}

void TransformFeedback::Begin(unsigned const xfb, int const primitiveMode)
{
	glEnable(GL_RASTERIZER_DISCARD);
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, xfb);
	glBeginTransformFeedback(primitiveMode);
}

void TransformFeedback::End()
{
	glEndTransformFeedback();
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
	glDisable(GL_RASTERIZER_DISCARD);
}

void TransformFeedback::init(unsigned const buffer) const
{
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, get());
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, buffer);
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
}

const unsigned& TransformFeedback::get() const
{
	return xfb_;
}

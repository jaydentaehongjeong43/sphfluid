#pragma once

struct TransformFeedback
{
	explicit TransformFeedback();
	virtual ~TransformFeedback();

	static void Begin(unsigned const xfb, int const primitiveMode);
	static void End();

	virtual void init(unsigned const buffer) const;
	unsigned const& get() const;
private:
	unsigned xfb_ = 0;
};

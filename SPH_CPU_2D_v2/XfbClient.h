#pragma once

struct XfbClient
{
	explicit XfbClient();
	virtual ~XfbClient();

	static void BeginTransformFeedback(unsigned const xfb, int const primitiveMode);
	static void EndTransformFeedback();

	virtual unsigned getPrims() const = 0;
	virtual unsigned getVerts() const = 0;
	virtual unsigned getBufferSize() const = 0;
	bool needInitBuffer() const;
	virtual unsigned initBuffer() const;
	virtual void initXfb() const;
	unsigned const& getBuffer() const;
	unsigned const& getXfb() const;
protected:
	unsigned lastBufferSize_ = 0;
private:
	unsigned buffer_ = 0;
	unsigned xfb_ = 0;
};

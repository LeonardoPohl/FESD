#pragma once

#include "VertexBuffer.h"

class VertexBufferLayout;

class VertexArray
{
private:
	unsigned int m_RendererID;
	unsigned int m_Count;
public:
	VertexArray();
	VertexArray(unsigned int count);
	~VertexArray();

	void AddBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout);
	void Bind() const;
	void Unbind() const;

	inline unsigned int GetCount() const
	{
		return m_Count;
	};
};
#pragma once

#include "VertexArray.h"
#include "IndexBuffer.h"
#include "Shader.h"

class Renderer
{
public:
    void DrawTriangles(const VertexArray& va, const IndexBuffer &ib, const Shader& shader) const;
    void DrawPoints(const VertexArray& va, const IndexBuffer &ib, const Shader& shader) const;
    void Clear() const;
};
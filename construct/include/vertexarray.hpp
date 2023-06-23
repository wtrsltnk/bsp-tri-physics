#ifndef VERTEXARRAY_H
#define VERTEXARRAY_H

#include <glm/glm.hpp>
#include <tuple>
#include <vector>

enum class VertexArrayRenderModes
{
    Points,
    Lines,
    Triangles,
};

class VertexArray
{
public:
    VertexArray();

    void reset();

    std::tuple<size_t, size_t> add(
        const float vertexData[],
        unsigned int vertexCount,
        const glm::vec3 &size = glm::vec3(1.0f),
        const glm::vec3 &transform = glm::vec3(0.0f));

    void upload();

    void bind();

    void cleanup();

    void render(
        VertexArrayRenderModes mode,
        size_t first = 0,
        size_t count = 0);

private:
    unsigned int _vao = 0, _vbo = 0;
    std::vector<float> _vertexData;
};

#endif // VERTEXARRAY_H

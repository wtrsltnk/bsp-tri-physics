#include <vertexarray.hpp>

#include <glad/glad.h>

VertexArray::VertexArray()
{
    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
}

glm::vec3 hsv2rgb(glm::vec3 c)
{
    glm::vec4 K = glm::vec4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
    glm::vec3 p = glm::abs(glm::fract(glm::vec3(c.x) + glm::vec3(K)) * 6.0f - glm::vec3(K.w));
    return c.z * mix(glm::vec3(K.x), glm::clamp(p - glm::vec3(K.x), 0.0f, 1.0f), c.y);
}

void VertexArray::reset()
{
    _vertexData.clear();
}

std::tuple<size_t, size_t> VertexArray::add(
    const float vertexData[],
    unsigned int vertexCount,
    const glm::vec3 &size,
    const glm::vec3 &transform)
{
    size_t start = _vertexData.size() / 11;

    for (unsigned int i = 0; i < vertexCount; i++)
    {
        _vertexData.push_back((vertexData[(i * 6) + 0] * size.x) + transform.x);
        _vertexData.push_back((vertexData[(i * 6) + 1] * size.y) + transform.y);
        _vertexData.push_back((vertexData[(i * 6) + 2] * size.z) + transform.z);

        auto rgb = hsv2rgb(glm::vec3(vertexData[(i * 6) + 3], vertexData[(i * 6) + 4], 1.0f));
        _vertexData.push_back(rgb.x);
        _vertexData.push_back(rgb.y);
        _vertexData.push_back(rgb.z);

        // uvs
        _vertexData.push_back(0.0f);
        _vertexData.push_back(0.0f);
        _vertexData.push_back(0.0f);
        _vertexData.push_back(0.0f);

        // bone
        _vertexData.push_back(-1.0f);
    }

    return std::tuple<size_t, size_t>(start, (_vertexData.size() / 11) - start);
}

void VertexArray::upload()
{
    bind();

    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * _vertexData.size(), _vertexData.data(), GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    // bone attribute
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void *)(10 * sizeof(float)));
    glEnableVertexAttribArray(3);
}

void VertexArray::bind()
{
    glBindVertexArray(_vao);
}

void VertexArray::cleanup()
{
    glDeleteBuffers(1, &_vbo);
    glDeleteVertexArrays(1, &_vao);

    _vbo = 0;
    _vao = 0;
}

void VertexArray::render(
    VertexArrayRenderModes mode,
    size_t first,
    size_t count)
{
    if (count == 0)
    {
        count = _vertexData.size() / 6;
    }

    bind();

    switch (mode)
    {
        case VertexArrayRenderModes::Points:
            glDrawArrays(GL_POINTS, (int)first, (int)count);
            break;
        case VertexArrayRenderModes::Lines:
            glDrawArrays(GL_LINES, (int)first, (int)count);
            break;
        case VertexArrayRenderModes::Triangles:
            glDrawArrays(GL_TRIANGLES, (int)first, (int)count);
            break;
    }
}

#ifndef GLBUFFER_H
#define GLBUFFER_H

#include "glad/glad.h"
#include "glshader.h"

#include <glm/glm.hpp>
#include <map>
#include <vector>

class VertexType
{
public:
    glm::vec3 pos;
    glm::vec3 col;
    glm::vec4 uvs;
    int bone;
};

class BufferType
{
public:
    BufferType() = default;

    virtual ~BufferType() = default;

    std::vector<VertexType> &verts()
    {
        return _verts;
    }

    BufferType &operator<<(
        VertexType const &vertex)
    {
        _verts.push_back(vertex);
        _vertexCount = static_cast<GLsizei>(_verts.size());

        return *this;
    }

    int vertexCount() const
    {
        return _vertexCount;
    }

    BufferType &vertex_and_col(
        float const *arr,
        glm::vec3 const &scale = glm::vec3(1.0f))
    {
        return col(&arr[3])
            .vertex(glm::vec3(arr[0] * scale.x, arr[1] * scale.y, arr[2] * scale.z));
    }

    BufferType &vertex(
        float const *position)
    {
        return vertex(glm::vec3(position[0], position[1], position[2]));
    }

    BufferType &vertex(
        glm::vec3 const &position)
    {
        VertexType v;

        v.pos = position;
        v.uvs = _nextUvs;
        v.col = _nextCol;
        v.bone = _nextBone;

        _verts.push_back(v);

        _vertexCount = static_cast<GLsizei>(_verts.size());

        return *this;
    }

    BufferType &col(
        glm::vec3 const &col)
    {
        _nextCol = col;

        return *this;
    }

    BufferType &col(
        float const *col)
    {
        _nextCol = glm::vec3(col[0], col[1], col[2]);

        return *this;
    }

    BufferType &uvs(
        glm::vec4 const &uvs)
    {
        _nextUvs = uvs;

        return *this;
    }

    BufferType &uvs(
        glm::vec2 const uvs[2])
    {
        _nextUvs = glm::vec4(uvs[0].x, uvs[0].y, uvs[1].x, uvs[1].y);

        return *this;
    }

    BufferType &uvs(
        float const *uvs)
    {
        _nextUvs = glm::vec4(uvs[0], uvs[1], uvs[2], uvs[3]);

        return *this;
    }

    BufferType &bone(
        int bone)
    {
        _nextBone = bone;

        return *this;
    }

    bool upload(
        ShaderType &shader)
    {
        _vertexCount = static_cast<GLsizei>(_verts.size());

        if (_vertexCount == 0)
        {
            return false;
        }

        glGenVertexArrays(1, &_vertexArrayId);
        glGenBuffers(1, &_vertexBufferId);

        glBindVertexArray(_vertexArrayId);
        glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferId);

        glBufferData(
            GL_ARRAY_BUFFER,
            GLsizeiptr(_verts.size() * sizeof(VertexType)),
            0,
            GL_STATIC_DRAW);

        glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            GLsizeiptr(_verts.size() * sizeof(VertexType)),
            reinterpret_cast<const GLvoid *>(&_verts[0]));

        shader.setupAttributes(sizeof(VertexType));

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        _verts.clear();

        return true;
    }

    void bind()
    {
        glBindVertexArray(_vertexArrayId);
    }

    void unbind()
    {
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void cleanup()
    {
        if (_vertexBufferId != 0)
        {
            glDeleteBuffers(1, &_vertexBufferId);
            _vertexBufferId = 0;
        }
        if (_vertexArrayId != 0)
        {
            glDeleteVertexArrays(1, &_vertexArrayId);
            _vertexArrayId = 0;
        }
    }

private:
    int _vertexCount = 0;
    std::vector<VertexType> _verts;
    glm::vec4 _nextUvs;
    glm::vec3 _nextCol = glm::vec3(1.0f, 1.0f, 1.0f);
    int _nextBone = 0;
    unsigned int _vertexArrayId = 0;
    unsigned int _vertexBufferId = 0;
};

#endif // GLBUFFER_H

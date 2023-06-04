#include "glbuffer.h"

#include "glad/glad.h"
#include "glshader.h"
#include <map>

BufferType::BufferType() = default;

BufferType::~BufferType() = default;

std::vector<VertexType> &BufferType::verts()
{
    return _verts;
}

BufferType &BufferType::operator<<(
    VertexType const &vertex)
{
    _verts.push_back(vertex);
    _vertexCount = static_cast<GLsizei>(_verts.size());

    return *this;
}

int BufferType::vertexCount() const
{
    return _vertexCount;
}

BufferType &BufferType::vertex_and_col(
    float const *arr,
    glm::vec3 const &scale)
{
    return col(&arr[3])
        .vertex(glm::vec3(arr[0] * scale.x, arr[1] * scale.y, arr[2] * scale.z));
}

BufferType &BufferType::vertex(
    float const *position)
{
    return vertex(glm::vec3(position[0], position[1], position[2]));
}

BufferType &BufferType::vertex(
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

BufferType &BufferType::col(
    glm::vec3 const &col)
{
    _nextCol = col;

    return *this;
}

BufferType &BufferType::col(
    float const *col)
{
    _nextCol = glm::vec3(col[0], col[1], col[2]);

    return *this;
}

BufferType &BufferType::uvs(
    glm::vec4 const &uvs)
{
    _nextUvs = uvs;

    return *this;
}

BufferType &BufferType::uvs(
    glm::vec2 const uvs[2])
{
    _nextUvs = glm::vec4(uvs[0].x, uvs[0].y, uvs[1].x, uvs[1].y);

    return *this;
}

BufferType &BufferType::uvs(
    glm::vec2 const uvs)
{
    _nextUvs = glm::vec4(uvs.x, uvs.y, uvs.x, uvs.y);

    return *this;
}

BufferType &BufferType::uvs(
    float const *uvs)
{
    _nextUvs = glm::vec4(uvs[0], uvs[1], uvs[2], uvs[3]);

    return *this;
}

BufferType &BufferType::bone(
    int bone)
{
    _nextBone = bone;

    return *this;
}

bool BufferType::upload()
{
    _vertexCount = static_cast<GLsizei>(_verts.size());

    if (_vertexCount == 0)
    {
        return true;
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

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);

    _verts.clear();

    return true;
}

void BufferType::bind()
{
    glBindVertexArray(_vertexArrayId);
}

void BufferType::unbind()
{
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void BufferType::cleanup()
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

#ifndef GLBUFFER_H
#define GLBUFFER_H

#include "glshader.h"

#include <glm/glm.hpp>
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
    BufferType();

    virtual ~BufferType();

    std::vector<VertexType> &verts();

    BufferType &operator<<(
        VertexType const &vertex);

    int vertexCount() const;

    BufferType &vertex_and_col(
        float const *arr,
        glm::vec3 const &scale = glm::vec3(1.0f));

    BufferType &vertex(
        float const *position);

    BufferType &vertex(
        glm::vec3 const &position);

    BufferType &col(
        glm::vec3 const &col);

    BufferType &col(
        float const *col);

    BufferType &uvs(
        glm::vec4 const &uvs);

    BufferType &uvs(
        glm::vec2 const uvs[2]);

    BufferType &uvs(
        glm::vec2 const uvs);

    BufferType &uvs(
        float const *uvs);

    BufferType &bone(
        int bone);

    bool upload();

    void bind();

    void unbind();

    void cleanup();

private:
    int _vertexCount = 0;
    std::vector<VertexType> _verts;
    glm::vec4 _nextUvs;
    glm::vec3 _nextCol = glm::vec3(1.0f, 1.0f, 1.0f);
    int _nextBone = -1;
    unsigned int _vertexArrayId = 0;
    unsigned int _vertexBufferId = 0;
};

#endif // GLBUFFER_H

#ifndef GLSHADER_H
#define GLSHADER_H

#include "glad/glad.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>

#define GLSL(src) "#version 330 core\n" #src

class ShaderType
{
public:
    ShaderType();

    virtual ~ShaderType();

    GLuint id() const;

    void use() const;

    GLuint compile(
        const std::string &vertShaderStr,
        const std::string &fragShaderStr,
        int boneCount = 0);

    GLuint compileDefaultShader();
    GLuint compileBspShader();
    GLuint compileMdlShader();
    GLuint compileSprShader();
    GLuint compileSkyShader();

    void setupMatrices(
        const glm::mat4 &proj,
        const glm::mat4 &view,
        const glm::mat4 &model);

    void setupColor(
        const glm::vec4 &color);

    void setupBrightness(
        float brightness);

    void setupAttributes(
        GLsizei vertexSize,
        bool includeBone = false);

    void BindBones(
        const glm::mat4 m[],
        size_t count);

    void UnbindBones();

private:
    GLuint _shaderId = 0;
    GLuint _projUniformId = 0;
    GLuint _viewUniformId = 0;
    GLuint _modelUniformId = 0;
    GLuint _colorUniformId = 0;
    GLuint _brightnessUniformId = 0;
    GLuint _bonesUniformId = 0;
    unsigned int _bonesBuffer = 0;
};

#endif // GLSHADER_H

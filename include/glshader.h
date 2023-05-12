#ifndef GLSHADER_H
#define GLSHADER_H

#include "glad/glad.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

#define GLSL(src) "#version 330 core\n" #src

class ShaderType
{
public:
    ShaderType() = default;

    virtual ~ShaderType() = default;

    GLuint id() const
    {
        return _shaderId;
    }

    void use() const
    {
        glUseProgram(_shaderId);
    }

    bool compileDefaultShader()
    {
        static GLuint defaultShader = 0;

        if (defaultShader == 0)
        {
            std::string const vshader(GLSL(
                in vec3 a_vertex;
                in vec3 a_color;
                in vec4 a_texcoords;

                uniform mat4 u_matrix;
                uniform vec4 u_color;

                out vec2 v_uv_tex;
                out vec2 v_uv_light;
                out vec4 v_color;

                void main() {
                    gl_Position = u_matrix * vec4(a_vertex.xyz, 1.0);
                    v_uv_light = a_texcoords.xy;
                    v_uv_tex = a_texcoords.zw;
                    v_color = vec4(a_color, 1.0) * u_color;
                }));

            std::string const fshader(GLSL(
                uniform sampler2D u_tex0;
                uniform sampler2D u_tex1;

                in vec2 v_uv_tex;
                in vec2 v_uv_light;
                in vec4 v_color;

                out vec4 color;

                void main() {
                    vec4 texel0;
                    vec4 texel1;
                    texel0 = texture2D(u_tex0, v_uv_tex);
                    texel1 = texture2D(u_tex1, v_uv_light);
                    color = texel0 * texel1 * v_color;
                }));

            if (compile(vshader, fshader))
            {
                defaultShader = _shaderId;

                return true;
            }

            return false;
        }

        return true;
    }

    virtual bool compile(
        const std::string &vertShaderStr,
        const std::string &fragShaderStr)
    {
        GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
        GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
        const char *vertShaderSrc = vertShaderStr.c_str();
        const char *fragShaderSrc = fragShaderStr.c_str();

        GLint result = GL_FALSE;
        GLint logLength;

        // Compile vertex shader
        glShaderSource(vertShader, 1, &vertShaderSrc, NULL);
        glCompileShader(vertShader);

        // Check vertex shader
        glGetShaderiv(vertShader, GL_COMPILE_STATUS, &result);
        if (result == GL_FALSE)
        {
            glGetShaderiv(vertShader, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<char> vertShaderError(static_cast<size_t>((logLength > 1) ? logLength : 1));
            glGetShaderInfoLog(vertShader, logLength, NULL, &vertShaderError[0]);
            spdlog::error("error compiling vertex shader: {}", vertShaderError.data());

            return false;
        }

        // Compile fragment shader
        glShaderSource(fragShader, 1, &fragShaderSrc, NULL);
        glCompileShader(fragShader);

        // Check fragment shader
        glGetShaderiv(fragShader, GL_COMPILE_STATUS, &result);
        if (result == GL_FALSE)
        {
            glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<char> fragShaderError(static_cast<size_t>((logLength > 1) ? logLength : 1));
            glGetShaderInfoLog(fragShader, logLength, NULL, &fragShaderError[0]);
            spdlog::error("error compiling fragment shader: {}", fragShaderError.data());

            return false;
        }

        _shaderId = glCreateProgram();
        glAttachShader(_shaderId, vertShader);
        glAttachShader(_shaderId, fragShader);
        glLinkProgram(_shaderId);

        glGetProgramiv(_shaderId, GL_LINK_STATUS, &result);
        if (result == GL_FALSE)
        {
            glGetProgramiv(_shaderId, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<char> programError(static_cast<size_t>((logLength > 1) ? logLength : 1));
            glGetProgramInfoLog(_shaderId, logLength, NULL, &programError[0]);
            spdlog::error("error linking shader: {}", programError.data());

            return false;
        }

        glDeleteShader(vertShader);
        glDeleteShader(fragShader);

        _matrixUniformId = glGetUniformLocation(_shaderId, "u_matrix");
        _colorUniformId = glGetUniformLocation(_shaderId, "u_color");

        return true;
    }

    void setupMatrices(
        const glm::mat4 &matrix)
    {
        use();

        glUniformMatrix4fv(_matrixUniformId, 1, false, glm::value_ptr(matrix));
    }

    void setupColor(
        const glm::vec4 &color)
    {
        use();

        glUniform4f(_colorUniformId, color.r, color.g, color.b, color.a);
    }

    void setupAttributes(
        GLsizei vertexSize,
        bool includeBone = false) const
    {
        glUseProgram(_shaderId);

        auto vertexAttrib = glGetAttribLocation(_shaderId, "a_vertex");
        if (vertexAttrib < 0)
        {
            spdlog::error("failed to get attribute location for \"a_vertex\" ({})", vertexAttrib);
            return;
        }

        glVertexAttribPointer(
            GLuint(vertexAttrib),
            sizeof(glm::vec3) / sizeof(float),
            GL_FLOAT,
            GL_FALSE,
            vertexSize, 0);

        glEnableVertexAttribArray(GLuint(vertexAttrib));

        auto colorAttrib = glGetAttribLocation(_shaderId, "a_color");
        if (colorAttrib < 0)
        {
            spdlog::error("failed to get attribute location for \"a_color\" ({})", colorAttrib);
            return;
        }

        glVertexAttribPointer(
            GLuint(colorAttrib),
            sizeof(glm::vec3) / sizeof(float),
            GL_FLOAT,
            GL_FALSE,
            vertexSize,
            reinterpret_cast<const GLvoid *>(sizeof(glm::vec3)));

        glEnableVertexAttribArray(GLuint(colorAttrib));

        auto texcoordsAttrib = glGetAttribLocation(_shaderId, "a_texcoords");
        if (texcoordsAttrib < 0)
        {
            spdlog::error("failed to get attribute location for \"a_texcoords\" ({})", texcoordsAttrib);
            return;
        }

        glVertexAttribPointer(
            GLuint(texcoordsAttrib),
            sizeof(glm::vec4) / sizeof(float),
            GL_FLOAT,
            GL_FALSE,
            vertexSize,
            reinterpret_cast<const GLvoid *>(sizeof(glm::vec3) + sizeof(glm::vec3)));

        glEnableVertexAttribArray(GLuint(texcoordsAttrib));

        if (includeBone)
        {
            auto boneAttrib = glGetAttribLocation(_shaderId, "a_bone");
            if (boneAttrib < 0)
            {
                spdlog::error("failed to get attribute location for \"a_bone\" ({})", boneAttrib);
                return;
            }

            glVertexAttribPointer(
                GLuint(boneAttrib),
                1,
                GL_INT,
                GL_FALSE,
                vertexSize,
                reinterpret_cast<const GLvoid *>(sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec4)));

            glEnableVertexAttribArray(GLuint(boneAttrib));
        }

        auto textLocation = glGetUniformLocation(_shaderId, "u_tex0");
        auto ligtmapLocation = glGetUniformLocation(_shaderId, "u_tex1");

        glUniform1i(textLocation, 0);
        glUniform1i(ligtmapLocation, 1);
    }

private:
    GLuint _shaderId = 0;
    GLuint _matrixUniformId = 0;
    GLuint _colorUniformId = 0;
};

#endif // GLSHADER_H

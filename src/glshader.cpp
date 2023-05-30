#include "glshader.h"

#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <spdlog/spdlog.h>
#include <string>

ShaderType::ShaderType() = default;

ShaderType::~ShaderType() = default;

GLuint ShaderType::id() const
{
    return _shaderId;
}

void ShaderType::use() const
{
    glUseProgram(_shaderId);
}

GLuint ShaderType::compile(
    const std::string &vertShaderStr,
    const std::string &fragShaderStr,
    int boneCount)
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

        return 0;
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

        return 0;
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

        return 0;
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    use();

    _projUniformId = glGetUniformLocation(_shaderId, "u_proj");
    _viewUniformId = glGetUniformLocation(_shaderId, "u_view");
    _modelUniformId = glGetUniformLocation(_shaderId, "u_model");
    _colorUniformId = glGetUniformLocation(_shaderId, "u_color");
    _u_spritetypeId = glGetUniformLocation(_shaderId, "u_spritetype");
    glUniform1i(_u_spritetypeId, 2);
    _brightnessUniformId = glGetUniformLocation(_shaderId, "u_brightness");

    GLint i;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &i);

    _bonesUniformId = 0;
    if (boneCount > 0)
    {
        GLuint uniform_block_index = glGetUniformBlockIndex(_shaderId, "u_bones");

        glUniformBlockBinding(_shaderId, uniform_block_index, _bonesUniformId);

        glGenBuffers(1, &_bonesBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, _bonesBuffer);
        glBufferData(GL_UNIFORM_BUFFER, boneCount * sizeof(glm::mat4), nullptr, GL_STREAM_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    return _shaderId;
}

void ShaderType::setupMatrices(
    const glm::mat4 &proj,
    const glm::mat4 &view,
    const glm::mat4 &model)
{
    use();

    glUniformMatrix4fv(_projUniformId, 1, false, glm::value_ptr(proj));
    glUniformMatrix4fv(_viewUniformId, 1, false, glm::value_ptr(view));
    glUniformMatrix4fv(_modelUniformId, 1, false, glm::value_ptr(model));
}

void ShaderType::setupColor(
    const glm::vec4 &color)
{
    use();

    glUniform4f(_colorUniformId, color.r, color.g, color.b, color.a);
}

void ShaderType::setupBrightness(
    float brightness)
{
    use();

    glUniform1f(_brightnessUniformId, brightness);
}

void ShaderType::setupSpriteType(
    int type)
{
    use();

    glUniform1i(_u_spritetypeId, type);
}

void ShaderType::setupAttributes(
    GLsizei vertexSize,
    bool includeBone)
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
            GL_FLOAT,
            GL_FALSE,
            vertexSize,
            reinterpret_cast<const GLvoid *>(sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec4)));

        glEnableVertexAttribArray(GLuint(boneAttrib));
    }

    auto textLocation = glGetUniformLocation(_shaderId, "u_tex0");
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(textLocation, 0);

    auto ligtmapLocation = glGetUniformLocation(_shaderId, "u_tex1");
    glActiveTexture(GL_TEXTURE1);
    glUniform1i(ligtmapLocation, 1);
}

void ShaderType::BindBones(
    const glm::mat4 m[],
    size_t count)
{
    glBindBuffer(GL_UNIFORM_BUFFER, _bonesBuffer);
    if (count > 0)
    {
        glBufferSubData(GL_UNIFORM_BUFFER, 0, count * sizeof(glm::mat4), glm::value_ptr(m[0]));
        glBindBufferRange(GL_UNIFORM_BUFFER, _bonesUniformId, _bonesBuffer, 0, count * sizeof(glm::mat4));
    }
}

void ShaderType::UnbindBones()
{
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

GLuint ShaderType::compileDefaultShader()
{
    std::string const vshader(GLSL(
        in vec3 a_vertex;
        in vec3 a_color;
        in vec4 a_texcoords;

        uniform mat4 u_proj;
        uniform mat4 u_view;
        uniform mat4 u_model;
        uniform vec4 u_color;

        out vec2 v_uv_tex;
        out vec2 v_uv_light;
        out vec4 v_color;

        void main() {
            mat4 m = u_proj * u_view * u_model;
            gl_Position = m * vec4(a_vertex.xyz, 1.0);
            v_uv_light = a_texcoords.xy;
            v_uv_tex = a_texcoords.zw;
            v_color = vec4(a_color, 1.0) * u_color;
        }));

    std::string const fshader(GLSL(
        uniform sampler2D u_tex0;
        uniform sampler2D u_tex1;
        uniform float u_brightness;

        in vec2 v_uv_tex;
        in vec2 v_uv_light;
        in vec4 v_color;

        out vec4 color;

        void main() {
            vec4 texel0;
            vec4 texel1;
            texel0 = texture2D(u_tex0, v_uv_tex);
            texel1 = texture2D(u_tex1, v_uv_light) + vec4(u_brightness, u_brightness, u_brightness, u_brightness);
            color = texel0 * texel1 * v_color;
        }));

    static GLuint defaultShader = compile(vshader, fshader);

    return defaultShader;
}

GLuint ShaderType::compileBspShader()
{
    spdlog::debug("compiling BSP shader");

    const char *vshader = GLSL(
        in vec3 a_vertex;
        in vec3 a_color;
        in vec4 a_texcoords;

        uniform mat4 u_proj;
        uniform mat4 u_view;
        uniform mat4 u_model;

        out vec4 v_color;
        out vec2 v_uv_tex;
        out vec2 v_uv_light;

        void main() {
            mat4 m = u_proj * u_view * u_model;
            gl_Position = m * vec4(a_vertex.xyz, 1.0);
            v_color = vec4(a_color, 1.0f);
            v_uv_light = a_texcoords.xy;
            v_uv_tex = a_texcoords.zw;
        });

    const char *fshader = GLSL(
        uniform sampler2D u_tex0;
        uniform sampler2D u_tex1;
        uniform float u_brightness;

        in vec2 v_uv_tex;
        in vec2 v_uv_light;
        in vec4 v_color;

        out vec4 color;

        void main() {
            vec4 texel0;
            vec4 texel1;
            texel0 = texture2D(u_tex0, v_uv_tex);
            texel1 = texture2D(u_tex1, v_uv_light);
            if (texel0.a < 0.1)
                discard;
            else
                color = texel0 * texel1 * vec4(1.0, 1.0, 1.0, u_brightness) * v_color;
        });

    static GLuint defaultShader = compile(vshader, fshader);

    return defaultShader;
}

GLuint ShaderType::compileMdlShader()
{
    spdlog::debug("compiling MDL shader");

    const char *mdlvshader = GLSL(
        in vec3 a_vertex;
        in vec3 a_color;
        in vec4 a_texcoords;
        in int a_bone;

        uniform mat4 u_proj;
        uniform mat4 u_view;
        uniform mat4 u_model;
        uniform vec4 u_color;
        layout(std140) uniform BonesBlock {
            mat4 u_bones[64];
        };

        out vec2 v_uv_tex;
        out vec2 v_uv_light;
        out vec4 v_color;

        void main() {
            mat4 m = u_proj * u_view * u_model;
            if (a_bone >= 0) m = m * u_bones[a_bone];
            gl_Position = m * vec4(a_vertex.xyz, 1.0);
            v_uv_light = a_texcoords.zw;
            v_uv_tex = a_texcoords.xy;
            v_color = vec4(a_color, 1.0) * u_color;
        });

    const char *mdlfshader = GLSL(
        uniform sampler2D u_tex0;
        uniform sampler2D u_tex1;
        uniform float u_brightness;

        in vec2 v_uv_tex;
        in vec2 v_uv_light;
        in vec4 v_color;

        out vec4 color;

        void main() {
            vec4 texel0;
            vec4 texel1;
            texel0 = texture2D(u_tex0, v_uv_tex);
            texel1 = texture2D(u_tex1, v_uv_light) * u_brightness;
            vec4 tempcolor = texel0 * v_color;
            if (texel0.a < 0.1)
                discard;
            else
                color = texel0 * v_color;
        });

    static GLuint defaultShader = compile(mdlvshader, mdlfshader, 64);

    return defaultShader;
}

GLuint ShaderType::compileSprShader()
{
    spdlog::debug("compiling SPR shader");

    const char *vshader = GLSL(
        in vec3 a_vertex;
        in vec3 a_color;
        in vec4 a_texcoords;

        uniform mat4 u_proj;
        uniform mat4 u_view;
        uniform mat4 u_model;
        uniform vec4 u_color;
        uniform int u_spritetype; // 0 for cylindrical; 1 for spherical; 2 for Oriented

        out vec2 v_uv_tex;
        out vec4 v_color;

        void main() {
            mat4 viewmodel = u_view * u_model;

            if (u_spritetype < 3)
            {
                // First colunm.
                viewmodel[0][0] = length(viewmodel[0]);
                viewmodel[0][1] = 0.0;
                viewmodel[0][2] = 0.0;

                // Second colunm.
                viewmodel[1][0] = 0.0;
                viewmodel[1][1] = 0.0;
                viewmodel[1][2] = length(viewmodel[1]);

                if (u_spritetype == 2)
                {
                    // Thrid colunm.
                    viewmodel[2][0] = 0.0;
                    viewmodel[2][1] = length(viewmodel[2]);
                    viewmodel[2][2] = 0.0;
                }
            }

            mat4 m = u_proj * viewmodel;
            gl_Position = m * vec4(a_vertex.xyz, 1.0);

            v_uv_tex = a_texcoords.xy;
            v_color = vec4(a_color, 1.0) * u_color;
        });

    const char *fshader = GLSL(
        uniform sampler2D u_tex0;
        uniform float u_brightness;

        in vec2 v_uv_tex;
        in vec4 v_color;

        out vec4 color;

        void main() {
            vec4 texel0;
            vec4 texel1;
            texel0 = texture2D(u_tex0, v_uv_tex) * u_brightness;
            if (texel0.a < 0.1)
                discard;
            else
                color = texel0 * v_color;
        });

    static GLuint defaultShader = compile(vshader, fshader);

    return defaultShader;
}

GLuint ShaderType::compileSkyShader()
{
    const char *vshader = GLSL(
        in vec3 a_vertex;
        in vec3 a_color;
        in vec4 a_texcoords;

        uniform mat4 u_proj;
        uniform mat4 u_view;
        uniform mat4 u_model;

        out vec3 v_color;
        out vec2 v_texCoord;

        void main() {
            mat4 m = u_proj * u_view * u_model;
            gl_Position = m * vec4(a_vertex, 1.0);
            v_color = a_color;
            v_texCoord = a_texcoords.xy;
        });

    const char *fshader = GLSL(
        in vec3 v_color;
        in vec2 v_texCoord;

        uniform sampler2D tex;

        out vec4 color;

        void main() {
            color = texture(tex, v_texCoord) * vec4(v_color, 1.0f);
        });

    static GLuint defaultShader = compile(vshader, fshader);

    return defaultShader;
}

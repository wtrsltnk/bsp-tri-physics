#ifndef OPENGLRENDERER_H
#define OPENGLRENDERER_H

#include <irenderer.hpp>

class OpenGlRenderer : public IRenderer
{
public:
    virtual ~OpenGlRenderer();

    virtual unsigned int LoadTexture(
        int width,
        int height,
        int bpp,
        bool repeat,
        unsigned char *data);
};

#endif // OPENGLRENDERER_H

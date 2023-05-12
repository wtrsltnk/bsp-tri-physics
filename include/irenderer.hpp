#ifndef IRENDERER_H
#define IRENDERER_H

class IRenderer
{
public:
    virtual ~IRenderer() {}

    virtual unsigned int LoadTexture(
        int width,
        int height,
        int bpp,
        bool repeat,
        unsigned char *data) = 0;
};

#endif // IRENDERER_H

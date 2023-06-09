#include <valve/hltexture.h>

#include <cstring>
#include <glm/glm.hpp>

using namespace valve;

Texture::Texture() = default;

Texture::Texture(
    const std::string &name)
    : _name(name)
{}

Texture::~Texture()
{
    ClearData();
}

void Texture::ClearData()
{
    if (_data != nullptr)
    {
        delete[] _data;
        _data = nullptr;
    }
}

Texture *Texture::Copy() const
{
    auto result = new Texture();

    result->CopyFrom(*this);

    return result;
}

void Texture::CopyFrom(
    const Texture &from)
{
    _name = from._name;

    SetData(from._width, from._height, from._bpp, from._data, from._repeat);
}

void Texture::SetDimentions(
    int width,
    int height,
    int bpp)
{
    SetData(width, height, bpp, nullptr);
}

void Texture::SetData(
    int w,
    int h,
    int bpp,
    unsigned char *data,
    bool repeat)
{
    _width = w;
    _height = h;
    _bpp = bpp;
    _repeat = repeat;

    ClearData();

    int dataSize = DataSize();

    if (dataSize == 0)
    {
        return;
    }

    _data = new unsigned char[dataSize];

    if (data != 0)
    {
        memcpy(_data, data, sizeof(unsigned char) * dataSize);
    }
    else
    {
        memset(_data, 0, dataSize);
    }
}

void Texture::DefaultTexture()
{
    int value;

    for (int row = 0; row < _width; row++)
    {
        for (int col = 0; col < _height; col++)
        {
            // Each cell is 8x8, value is 0 or 255 (black or white)
            value = (((row & 0x8) == 0) ^ ((col & 0x8) == 0)) * 255;
            SetPixelAt(glm::vec4(float(value), float(value), float(value), float(value)), row, col);
        }
    }
}

glm::vec4 Texture::PixelAt(
    int x,
    int y) const
{
    glm::vec4 r(1.0f, 1.0f, 1.0f, 1.0f);

    int p = x + (y * _width);

    for (int i = 0; i < _bpp; i++)
    {
        r[i] = _data[(p * _bpp) + i];
    }

    return r;
}

void Texture::SetPixelAt(
    const glm::vec4 &pixel,
    int x,
    int y)
{
    int p = x + (y * _width);

    for (int i = 0; i < _bpp; i++)
    {
        _data[(p * _bpp) + i] = static_cast<unsigned char>(pixel[i]);
    }
}

void Texture::Fill(
    const glm::vec4 &color)
{
    for (int y = 0; y < _height; y++)
    {
        for (int x = 0; x < _width; x++)
        {
            SetPixelAt(color, x, y);
        }
    }
}

void Texture::Fill(
    const Texture &from)
{
    int x = 0, y = 0;

    while (x < Width())
    {
        while (y < Height())
        {
            FillAtPosition(from, glm::vec2(x, y));

            y += from.Height();
        }

        x += from.Width();
    }
}

void Texture::FillAtPosition(
    const Texture &from,
    const glm::vec2 &pos,
    bool expandBorder)
{
    if (pos.x > Width() || pos.y > Height())
    {
        return;
    }

    int w = from.Width();
    int h = from.Height();

    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            SetPixelAt(from.PixelAt(x, y), int(pos.x + x), int(pos.y + y));

            if (!expandBorder)
            {
                continue;
            }

            if (y == 0)
            {
                SetPixelAt(from.PixelAt(x, y), int(pos.x + x), int(pos.y + y) - 1);
                if (x == 0) SetPixelAt(from.PixelAt(x, y), int(pos.x + x) - 1, int(pos.y + y) - 1);
                if (x == w - 1) SetPixelAt(from.PixelAt(x, y), int(pos.x + x) + 1, int(pos.y + y) - 1);
            }
            else if (y == h - 1)
            {
                SetPixelAt(from.PixelAt(x, y), int(pos.x + x), int(pos.y + y + 1));
                if (x == 0) SetPixelAt(from.PixelAt(x, y), int(pos.x + x) - 1, int(pos.y + y) + 1);
                if (x == w - 1) SetPixelAt(from.PixelAt(x, y), int(pos.x + x) + 1, int(pos.y + y) + 1);
            }

            if (x == 0)
            {
                SetPixelAt(from.PixelAt(x, y), int(pos.x + x) - 1, int(pos.y + y));
            }
            else if (x == w - 1)
            {
                SetPixelAt(from.PixelAt(x, y), int(pos.x + x) + 1, int(pos.y + y));
            }
        }
    }
}

void Texture::CorrectGamma(
    float gamma)
{
    // Only images with rgb colors
    if (_bpp < 3)
    {
        return;
    }

    for (int j = 0; j < (_width * _height); ++j)
    {
        float r, g, b;
        r = _data[j * _bpp + 0];
        g = _data[j * _bpp + 1];
        b = _data[j * _bpp + 2];

        r *= gamma / 255.0f;
        g *= gamma / 255.0f;
        b *= gamma / 255.0f;

        // find the value to scale back up
        float scale = 1.0f;
        float temp;
        if (r > 1.0f && (temp = (1.0f / r)) < scale) scale = temp;
        if (g > 1.0f && (temp = (1.0f / g)) < scale) scale = temp;
        if (b > 1.0f && (temp = (1.0f / b)) < scale) scale = temp;

        // scale up color values
        scale *= 255.0f;
        r *= scale;
        g *= scale;
        b *= scale;

        // fill data back in
        _data[j * _bpp + 0] = (unsigned char)r;
        _data[j * _bpp + 1] = (unsigned char)g;
        _data[j * _bpp + 2] = (unsigned char)b;
    }
}

void Texture::EnsureFullOpacity()
{
    // Only images with rgba colors
    if (_bpp < 4)
    {
        return;
    }

    for (int j = 0; j < (_width * _height); ++j)
    {
        _data[j * _bpp + 3] = 255;
    }
}

void Texture::SetName(
    const std::string &name)
{
    _name = name;
}

void Texture::SetRepeat(
    bool repeat)
{
    _repeat = repeat;
}

const std::string &Texture::Name() const
{
    return _name;
}

int Texture::Width() const
{
    return _width;
}

int Texture::Height() const
{
    return _height;
}

int Texture::Bpp() const
{
    return _bpp;
}

bool Texture::Repeat() const
{
    return _repeat;
}

int Texture::DataSize() const
{
    return _width * _height * _bpp;
}

unsigned char *Texture::Data()
{
    return _data;
}

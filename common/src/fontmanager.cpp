#include "fontmanager.h"

#include <filesystem>
#include <fstream>
#include <glad/glad.h>
#include <stb_truetype.h>

class LoadedFont : public IFont
{
public:
    LoadedFont(
        const std::wstring &fontName,
        float fontSize,
        const std::vector<char> &ttfData);

    virtual void Print(
        const wchar_t *text,
        float x,
        float y);

private:
    GLuint _ftex = 0;
    stbtt_bakedchar _cdata[96];
};

FontManager::FontManager() = default;

#define OS_FONT_PATH std::filesystem::path(L"c:\\windows\\fonts")

std::shared_ptr<IFont> FontManager::LoadFont(
    const wchar_t *font,
    float fontSize)
{
    std::filesystem::path fontPath;

    if (std::filesystem::exists(font))
    {
        fontPath = std::filesystem::path(font);
    }
    else if (std::filesystem::exists(OS_FONT_PATH / font))
    {
        fontPath = OS_FONT_PATH / font;
    }
    else if (std::filesystem::exists(OS_FONT_PATH / (std::wstring(font) + L".ttf")))
    {
        fontPath = OS_FONT_PATH / (std::wstring(font) + L".ttf");
    }

    std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(fileSize);
    if (!file.read(buffer.data(), fileSize))
    {
        return nullptr;
    }

    auto font_ptr = new LoadedFont(fontPath.filename(), fontSize, buffer);
    auto fnt = std::shared_ptr<IFont>(font_ptr);

    _loadedFonts.push_back(fnt);

    return fnt;
}

LoadedFont::LoadedFont(
    const std::wstring &fontName,
    float fontSize,
    const std::vector<char> &ttfData)
{
    this->fontName = fontName;
    this->fontSize = fontSize;

    unsigned char temp_bitmap[512 * 512];

    stbtt_BakeFontBitmap(
        reinterpret_cast<const unsigned char *>(ttfData.data()),
        0,
        fontSize,
        temp_bitmap, // bitmap data buffer
        512, 512,    // output image size (no guarantee this fits!)
        32, 96,      // codepoint range
        _cdata);

    glGenTextures(1, &_ftex);
    glBindTexture(GL_TEXTURE_2D, _ftex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 512, 512, 0, GL_ALPHA, GL_UNSIGNED_BYTE, temp_bitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void LoadedFont::Print(
    const wchar_t *text,
    float x,
    float y)
{
    while (*text)
    {
        if (*text >= 32 && *text < 128)
        {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(_cdata, 512, 512, *text - 32, &x, &y, &q, 1); // 1=opengl & d3d10+,0=d3d9
            // glTexCoord2f(q.s0,q.t0); glVertex2f(q.x0,q.y0);
            // glTexCoord2f(q.s1,q.t0); glVertex2f(q.x1,q.y0);
            // glTexCoord2f(q.s1,q.t1); glVertex2f(q.x1,q.y1);
            // glTexCoord2f(q.s0,q.t1); glVertex2f(q.x0,q.y1);
        }
        ++text;
    }
}

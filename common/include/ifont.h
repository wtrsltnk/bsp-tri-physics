#ifndef IFONT_H
#define IFONT_H

#include <string>

struct IFont
{
    std::wstring fontName;
    float fontSize;

    virtual void Print(
        const wchar_t *text,
        float x,
        float y) = 0;
};

#endif // IFONT_H

#ifndef IFONT_H
#define IFONT_H

#include <string>

struct IFont
{
    std::wstring fonName;
    float fontSize = 12.0f;

    virtual void DrawText(
        const wchar_t *text,
        int x,
        int y) = 0;
};

#endif // IFONT_H

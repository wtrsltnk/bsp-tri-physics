#ifndef FONTMANAGER_H
#define FONTMANAGER_H

#include <ifont.h>
#include <memory>
#include <vector>

class FontManager
{
public:
    FontManager();

    std::shared_ptr<IFont> LoadFont(
        const wchar_t *font,
        float size);

public:
    std::vector<std::shared_ptr<IFont>> _loadedFonts;
};

#endif // FONTMANAGER_H

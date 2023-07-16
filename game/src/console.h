#ifndef CONSOLE_H
#define CONSOLE_H

#include <chrono>
#include <ifont.h>

class Console
{
public:
    Console(
        std::shared_ptr<IFont> &font);

    void Resize(
        int width,
        int height);

    bool Tick(
        std::chrono::microseconds time,
        const struct InputState &inputState);

    void Render();

private:
    std::shared_ptr<IFont> _font;
    bool _consoleOpen = false;
    int _width = 0;
    int _height = 0;
};

#endif // CONSOLE_H

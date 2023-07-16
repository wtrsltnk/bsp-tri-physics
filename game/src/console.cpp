#include "console.h"

#include <inputstate.h>

Console::Console(
    std::shared_ptr<IFont> &font)
    : _font(font)
{}

void Console::Resize(
    int width,
    int height)
{
    _width = width;
    _height = height;
}

bool Console::Tick(
    std::chrono::microseconds time,
    const struct InputState &inputState)
{
    if (IsKeyboardButtonPushed(inputState, KeyboardButtons::KeyGraveAccent))
    {
        _consoleOpen = !_consoleOpen;
    }

    return _consoleOpen;
}

void Console::Render() {}

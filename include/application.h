#ifndef APPLICATION_H
#define APPLICATION_H

#include "inputstate.h"
#include <chrono>
#include <functional>
#include <spdlog/spdlog.h>

class IApplication
{
public:
    virtual ~IApplication() {}

    virtual bool Startup() = 0;

    virtual void Resize(
        int width,
        int height) = 0;

    virtual void Destroy() = 0;

    virtual bool Tick(
        std::chrono::microseconds time,
        const struct InputState &inputState) = 0;

protected:
    IApplication() {}
};

int Run(IApplication *t);

#endif // APPLICATION_H

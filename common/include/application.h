#ifndef APPLICATION_H
#define APPLICATION_H

#include "inputstate.h"
#include <chrono>
#include <functional>

class IApplication
{
public:
    virtual ~IApplication() {}

    virtual bool Startup(
        void *windowHandle) = 0;

    virtual void Resize(
        int width,
        int height) = 0;

    virtual void Destroy() = 0;

    virtual bool Tick(
        std::chrono::microseconds time,
        const struct InputState &inputState) = 0;

    virtual bool RawEvent(
        void *eventPackage) = 0;

protected:
    IApplication() {}
};

struct RunOptions
{
    bool hideMouse = false;
};

int Run(IApplication *t, const RunOptions &runOptions = {});

#endif // APPLICATION_H

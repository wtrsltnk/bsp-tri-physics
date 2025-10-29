
#include "application.h"
#include "goldsrcviewerapp.h"

#include <print>

int main(
    int argc,
    char *argv[])
{
    GoldSrcViewerApp t;

    if (argc > 2)
    {
        std::println("[DBG] loading {} from {}", argv[1], argv[2]);
        t.SetFilename(argv[1], argv[2]);
    }
    else if (argc > 1)
    {
        std::println("[DBG] loading map {}", argv[1]);
        t.SetFilename(argv[1], nullptr);
    }

    struct RunOptions runOptions =
        {
            .hideMouse = false,
        };

    return Run(&t, runOptions);
}

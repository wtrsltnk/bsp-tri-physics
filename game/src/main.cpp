
#include "application.h"
#include "genmapapp.h"

int main(
    int argc,
    char *argv[])
{
    spdlog::set_level(spdlog::level::debug); // Set global log level to debug

    GenMapApp t;

    if (argc > 2)
    {
        spdlog::debug("loading {1} from {0}", argv[1], argv[2]);
        t.SetFilename(argv[1], argv[2]);
    }
    else if (argc > 1)
    {
        spdlog::debug("loading map {0}", argv[1]);
        t.SetFilename(argv[1], nullptr);
    }

    return Run(&t);
}

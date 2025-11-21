#include "application.hpp"

int main(int argc, char **argv)
{
    IM::Application app;
    if (app.init(argc, argv))
    {
        return app.run();
    }
    return 0;
}
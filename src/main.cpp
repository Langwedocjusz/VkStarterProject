#include "Application.h"

#include <iostream>

int main()
{
    try
    {
        Application app;
        app.Run();
    }

    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return -1;
    }
}

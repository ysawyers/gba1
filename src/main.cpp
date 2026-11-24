#include "emulator/emulator.hpp"

#include <print>


int main(int argc, char** argv)
{
    try
    {
        emulator::Emulator emulator;

        emulator.readFile(argv[1]);
        emulator.renderFrame();

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        std::println(stderr, "ERROR: {}", e.what());
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::println(stderr, "ERROR: unknown exception");
        return EXIT_FAILURE;
    }
}
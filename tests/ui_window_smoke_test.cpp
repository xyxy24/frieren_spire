#include "presentation/SfmlApplication.hpp"

#include <filesystem>

int main(const int argc, const char* const argv[])
{
    if (argc > 0)
    {
        const std::filesystem::path executable = std::filesystem::absolute(argv[0]);
        std::filesystem::current_path(executable.parent_path());
    }
    arcane::presentation::SfmlApplication application;
    return application.runVisualSmokeTest();
}

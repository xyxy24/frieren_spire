#pragma once

namespace arcane::presentation
{
class SfmlApplication
{
public:
    int run();
    int runVisualSmokeTest();

private:
    int runImpl(bool visualSmokeTest);
};
}

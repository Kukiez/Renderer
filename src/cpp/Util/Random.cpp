#include <util/Random.h>

std::mt19937 Random::gen(static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count()));


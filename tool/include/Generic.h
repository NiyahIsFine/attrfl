#pragma once
#include <chrono>
#include <optional>

long long GetTimestamp()
{
    const auto now = std::chrono::system_clock::now();
    const auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()
    ).count();
    return timestamp;
}

static std::optional<std::string> GetArgValue(int argc, char **argv, const char *key)
{
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], key) == 0)
        {
            if (i + 1 < argc) return std::string(argv[i + 1]);
            return std::nullopt;
        }
    }
    return std::nullopt;
}

static bool HasFlag(int argc, char **argv, const char *key)
{
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], key) == 0) return true;
    }
    return false;
}

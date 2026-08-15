#pragma once


namespace forge
{


enum class LogLoadResult
{
    Ok,
    Missing,
    Corrupted,
    UnsupportedVersion,
    IoError
};


constexpr const char* log_load_result_name(
    LogLoadResult result
) noexcept
{
    switch(result)
    {
    case LogLoadResult::Ok:
        return "ok";

    case LogLoadResult::Missing:
        return "missing";

    case LogLoadResult::Corrupted:
        return "corrupted";

    case LogLoadResult::UnsupportedVersion:
        return "unsupported-version";

    case LogLoadResult::IoError:
        return "io-error";
    }


    return "unknown";
}


}
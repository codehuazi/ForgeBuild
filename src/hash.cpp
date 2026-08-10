#include "forge/hash.hpp"

#include <array>
#include <fstream>

namespace forge
{


namespace
{


constexpr std::uint64_t fnv_offset_basis =
    14695981039346656037ULL;


constexpr std::uint64_t fnv_prime =
    1099511628211ULL;


} // namespace


Hash64::Hash64()
    :
    value_(
        fnv_offset_basis
    )
{
}


void Hash64::update(
    std::string_view text
)
{
    for (unsigned char byte :
         text)
    {
        value_ ^=
            byte;


        value_ *=
            fnv_prime;
    }
}


bool Hash64::update_file(
    const std::string& path
)
{
    std::ifstream input(
        path,
        std::ios::binary
    );


    if (!input)
    {
        return false;
    }


    std::array<char, 4096> buffer;


    while (input)
    {
        input.read(
            buffer.data(),
            static_cast<std::streamsize>(
                buffer.size()
            )
        );


        const std::streamsize count =
            input.gcount();


        if (count > 0)
        {
            update(
                std::string_view(
                    buffer.data(),
                    static_cast<std::size_t>(
                        count
                    )
                )
            );
        }
    }


    return input.eof();
}


std::uint64_t Hash64::value() const
{
    return value_;
}


std::uint64_t hash_string(
    std::string_view text
)
{
    Hash64 hasher;


    hasher.update(
        text
    );


    return hasher.value();
}


} // namespace forge
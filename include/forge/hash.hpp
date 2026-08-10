#pragma once

#include <cstdint>
#include <string>
#include <string_view>


namespace forge
{


class Hash64
{

public:

    Hash64();


    /*
     * 将一段字节继续加入当前哈希状态。
     *
     * 可以连续调用多次，不会自动重置。
     */
    void update(
        std::string_view text
    );

    bool update_file(
        const std::string& path
    );



    /*
     * 返回当前哈希结果。
     */
    std::uint64_t value() const;


private:

    std::uint64_t value_;

};


std::uint64_t hash_string(
    std::string_view text
);


} // namespace forge
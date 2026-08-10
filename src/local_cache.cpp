#include "forge/local_cache.hpp"

#include "forge/hash.hpp"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

#include <unistd.h>


namespace forge
{


namespace
{


std::atomic<std::uint64_t>
    next_temp_id{0};


std::string temporary_path(
    const std::string& final_path
)
{
    const std::uint64_t temp_id =
        next_temp_id.fetch_add(
            1,
            std::memory_order_relaxed
        );


    return final_path
        + ".tmp."
        + std::to_string(
            static_cast<long long>(
                ::getpid()
            )
        )
        + "."
        + std::to_string(
            temp_id
        );
}


void remove_if_exists(
    const std::string& path
)
{
    std::error_code error;


    std::filesystem::remove(
        path,
        error
    );
}


} // namespace


LocalCache::LocalCache(
    std::string root
)
    :
    root_(
        std::move(root)
    )
{
}


bool LocalCache::contains(
    std::uint64_t key
) const
{
    std::error_code error;


    const bool object_exists =
        std::filesystem::exists(
            object_path(key),
            error
        );


    if(error || !object_exists)
    {
        return false;
    }


    const bool metadata_exists =
        std::filesystem::exists(
            metadata_path(key),
            error
        );


    return !error
        && metadata_exists;
}


bool LocalCache::store(
    std::uint64_t key,
    const std::string& output
)
{
    std::error_code error;


    const auto output_size =
        std::filesystem::file_size(
            output,
            error
        );


    if(error)
    {
        return false;
    }


    Hash64 output_hasher;


    if(!output_hasher.update_file(
            output
        ))
    {
        return false;
    }


    const std::uint64_t output_hash =
        output_hasher.value();


    std::filesystem::create_directories(
        root_,
        error
    );


    if(error)
    {
        return false;
    }


    const std::string final_object =
        object_path(key);


    const std::string final_metadata =
        metadata_path(key);


    const std::string temp_object =
        temporary_path(
            final_object
        );


    const std::string temp_metadata =
        temporary_path(
            final_metadata
        );


    remove_if_exists(
        temp_object
    );


    remove_if_exists(
        temp_metadata
    );


    std::filesystem::copy_file(
        output,
        temp_object,
        std::filesystem::copy_options::
            overwrite_existing,
        error
    );


    if(error)
    {
        remove_if_exists(
            temp_object
        );

        return false;
    }


    {
        std::ofstream metadata(
            temp_metadata,
            std::ios::trunc
        );


        if(!metadata)
        {
            remove_if_exists(
                temp_object
            );

            return false;
        }


        metadata
            << output_size
            << '\n'
            << output_hash
            << '\n';


        metadata.close();


        if(!metadata)
        {
            remove_if_exists(
                temp_object
            );


            remove_if_exists(
                temp_metadata
            );


            return false;
        }
    }


    //
    // 先提交 object，
    // metadata 最后提交，作为缓存完整性的标记。
    //
    std::filesystem::rename(
        temp_object,
        final_object,
        error
    );


    if(error)
    {
        remove_if_exists(
            temp_object
        );


        remove_if_exists(
            temp_metadata
        );


        return false;
    }


    std::filesystem::rename(
        temp_metadata,
        final_metadata,
        error
    );


    if(error)
    {
        remove_if_exists(
            temp_metadata
        );


        remove_if_exists(
            final_object
        );


        return false;
    }


    return true;
}


bool LocalCache::restore(
    std::uint64_t key,
    const std::string& output
) const
{
    const std::string cached_object =
        object_path(key);


    const std::string cached_metadata =
        metadata_path(key);


    std::ifstream metadata(
        cached_metadata
    );


    std::uintmax_t expected_size = 0;

    std::uint64_t expected_hash = 0;


    if(!(metadata
        >> expected_size
        >> expected_hash))
    {
        remove_if_exists(
            cached_object
        );


        remove_if_exists(
            cached_metadata
        );


        return false;
    }


    std::error_code error;


    const auto actual_size =
        std::filesystem::file_size(
            cached_object,
            error
        );


    if(error
        || actual_size
           != expected_size)
    {
        remove_if_exists(
            cached_object
        );


        remove_if_exists(
            cached_metadata
        );


        return false;
    }


    Hash64 object_hasher;


    if(!object_hasher.update_file(
            cached_object
        )
        || object_hasher.value()
           != expected_hash)
    {
        remove_if_exists(
            cached_object
        );


        remove_if_exists(
            cached_metadata
        );


        return false;
    }


    //
    // restore 也先写临时文件，
    // 再 rename 到真正 output。
    //
    const std::string temp_output =
        temporary_path(
            output
        );


    remove_if_exists(
        temp_output
    );


    std::filesystem::copy_file(
        cached_object,
        temp_output,
        std::filesystem::copy_options::
            overwrite_existing,
        error
    );


    if(error)
    {
        remove_if_exists(
            temp_output
        );


        return false;
    }


    std::filesystem::rename(
        temp_output,
        output,
        error
    );


    if(error)
    {
        remove_if_exists(
            temp_output
        );


        return false;
    }


    return true;
}


std::string LocalCache::object_path(
    std::uint64_t key
) const
{
    std::ostringstream stream;


    stream
        << std::hex
        << key;


    return root_
        + "/"
        + stream.str();
}


std::string LocalCache::metadata_path(
    std::uint64_t key
) const
{
    return object_path(key)
        + ".meta";
}


} // namespace forge
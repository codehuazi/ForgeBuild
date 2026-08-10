#include "forge/executor.hpp"

#include "forge/build_plan.hpp"
#include "forge/build_log.hpp"
#include "forge/depfile.hpp"
#include "forge/deps_log.hpp"
#include "forge/hash.hpp"
#include "forge/edge.hpp"
#include "forge/node.hpp"
#include "forge/cache_key.hpp"
#include "forge/local_cache.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>


namespace forge
{

Executor::Executor()
    :
    build_log_(nullptr),
    deps_log_(nullptr),
    local_cache_(nullptr)
{
}


Executor::Executor(
    BuildLog& build_log
)
    :
    build_log_(&build_log),
    deps_log_(nullptr),
    local_cache_(nullptr)
{
}


Executor::Executor(
    BuildLog& build_log,
    DepsLog& deps_log
)
    :
    build_log_(&build_log),
    deps_log_(&deps_log),
    local_cache_(nullptr)
{
}


Executor::Executor(
    BuildLog& build_log,
    DepsLog& deps_log,
    LocalCache& local_cache
)
    :
    build_log_(&build_log),
    deps_log_(&deps_log),
    local_cache_(&local_cache)
{
}


bool Executor::execute(
    const BuildPlan& plan
)
{

    std::cout
        << "===== Execute Build =====\n";


    for(auto* edge :
        plan.edges())
    {

        if(!execute_edge(
            edge
        ))
        {
            return false;
        }

    }


    return true;

}


bool Executor::execute_edge(
    Edge* edge
)
{
    if(edge == nullptr)
    {
        {
            std::lock_guard<std::mutex> lock(
                output_mutex_
            );


            std::cerr
                << "cannot execute null edge\n";
        }

        return false;
    }


    const std::string command =
        edge->command();


    const std::string depfile_path =
        edge->depfile();


    const bool cacheable =
        local_cache_ != nullptr
        && deps_log_ != nullptr
        && !depfile_path.empty()
        && edge->outputs().size() == 1;


    std::uint64_t cache_key = 0;


    //
    // 编译前尝试 cache lookup。
    //
    // 第一次构建没有 DepsLog，所以自然跳过。
    //
    if(cacheable)
    {
        const std::string& output_path =
            edge->outputs()[0]->path();


        if(deps_log_->contains(
                output_path
            ))
        {
            CacheKeyBuilder key_builder;


            key_builder.add_command(
                command
            );


            bool dependencies_ok = true;


            for(const auto& dependency :
                deps_log_->inputs(
                    output_path
                ))
            {
                if(!key_builder.add_dependency(
                        dependency
                    ))
                {
                    dependencies_ok = false;

                    break;
                }
            }


            if(dependencies_ok)
            {
                cache_key =
                    key_builder.value();


                if(local_cache_->contains(
                        cache_key
                    ))
                {
                    if(local_cache_->restore(
                            cache_key,
                            output_path
                        ))
                    {
                        {
                            std::lock_guard<std::mutex> lock(
                                output_mutex_
                            );


                            std::cout
                                << "cache hit: "
                                << output_path
                                << "\n";
                        }


                        const std::uint64_t command_hash =
                            hash_string(
                                command
                            );


                        auto* output =
                            edge->outputs()[0];


                        output->refresh();

                        output->mark_clean();


                        if(build_log_ != nullptr)
                        {
                            build_log_->record(
                                output->path(),
                                command_hash
                            );
                        }


                        return true;
                    }
                }
            }
        }
    }


    {
        std::lock_guard<std::mutex> lock(
            output_mutex_
        );


        if(cacheable)
        {
            std::cout
                << "cache miss\n";
        }


        std::cout
            << command
            << "\n";
    }


    const int result =
        std::system(
            command.c_str()
        );


    if(result != 0)
    {
        std::lock_guard<std::mutex> lock(
            output_mutex_
        );


        std::cerr
            << "command failed: "
            << command
            << "\n";


        return false;
    }


    //
    // 编译成功后读取新的 depfile。
    //
    if(!depfile_path.empty())
    {
        forge::Depfile depfile;


        if(!depfile.load(
                depfile_path
            ))
        {
            std::lock_guard<std::mutex> lock(
                output_mutex_
            );


            std::cerr
                << "failed to load depfile: "
                << depfile_path
                << "\n";


            return false;
        }


        if(deps_log_ != nullptr)
        {
            deps_log_->record(
                depfile.output(),
                depfile.inputs()
            );
        }
    }


    const std::uint64_t command_hash =
        hash_string(
            command
        );


    for(auto* output :
        edge->outputs())
    {
        output->refresh();

        output->mark_clean();


        if(build_log_ != nullptr)
        {
            build_log_->record(
                output->path(),
                command_hash
            );
        }
    }


    //
    // 编译完成以后已经有完整 depfile，
    // 此时重新计算最终 Cache Key 并 store。
    //
    if(cacheable)
    {
        const std::string& output_path =
            edge->outputs()[0]->path();


        if(deps_log_->contains(
                output_path
            ))
        {
            CacheKeyBuilder key_builder;


            key_builder.add_command(
                command
            );


            bool dependencies_ok = true;


            for(const auto& dependency :
                deps_log_->inputs(
                    output_path
                ))
            {
                if(!key_builder.add_dependency(
                        dependency
                    ))
                {
                    dependencies_ok = false;

                    break;
                }
            }


            if(dependencies_ok)
            {
                cache_key =
                    key_builder.value();


                if(local_cache_->store(
                        cache_key,
                        output_path
                    ))
                {
                    std::lock_guard<std::mutex> lock(
                        output_mutex_
                    );


                    std::cout
                        << "cache store: "
                        << output_path
                        << "\n";
                }
            }
        }
    }


    return true;
}

}
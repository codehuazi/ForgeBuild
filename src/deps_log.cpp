#include "forge/deps_log.hpp"

#include <fstream>


namespace forge
{


void DepsLog::record(
    const std::string& output,
    const std::vector<std::string>& inputs
)
{
    std::lock_guard<std::mutex> lock(
        mutex_
    );


    entries_[output] =
        inputs;
}


bool DepsLog::save(
    const std::string& file_path
) const
{
    std::unordered_map<
        std::string,
        std::vector<std::string>
    > snapshot;


    {
        std::lock_guard<std::mutex> lock(
            mutex_
        );


        snapshot =
            entries_;
    }


    std::ofstream output_file(
        file_path
    );


    if (!output_file)
    {
        return false;
    }


    for (const auto& [output, inputs] :
         snapshot)
    {
        output_file
            << output
            << '\n';


        output_file
            << inputs.size()
            << '\n';


        for (const std::string& input :
             inputs)
        {
            output_file
                << input
                << '\n';
        }
    }


    return true;
}


bool DepsLog::load(
    const std::string& file_path
)
{
    std::ifstream input_file(
        file_path
    );


    if (!input_file)
    {
        return false;
    }


    std::unordered_map<
        std::string,
        std::vector<std::string>
    > loaded_entries;


    std::string output_path;


    while (std::getline(
        input_file,
        output_path
    ))
    {
        std::size_t input_count = 0;


        if (!(input_file >> input_count))
        {
            return false;
        }


        /*
         * operator>> 读取数字后，
         * 换行符仍然留在输入流中。
         *
         * ignore() 用来丢弃这个换行符，
         * 否则下一次 getline() 会读到空字符串。
         */
        input_file.ignore();


        std::vector<std::string> inputs;


        for (std::size_t i = 0;
             i < input_count;
             ++i)
        {
            std::string input_path;


            if (!std::getline(
                    input_file,
                    input_path
                ))
            {
                return false;
            }


            inputs.push_back(
                std::move(input_path)
            );
        }


        loaded_entries[
            std::move(output_path)
        ] = std::move(inputs);
    }


    {
        std::lock_guard<std::mutex> lock(
            mutex_
        );


        entries_ =
            std::move(loaded_entries);
    }

    return true;
}


bool DepsLog::contains(
    const std::string& output
) const
{
    std::lock_guard<std::mutex> lock(
        mutex_
    );


    return entries_.find(output)
        != entries_.end();
}


std::vector<std::string> DepsLog::inputs(
    const std::string& output
) const
{
    std::lock_guard<std::mutex> lock(
        mutex_
    );


    return entries_.at(output);
}


}
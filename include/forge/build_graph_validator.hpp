#pragma once


namespace forge
{


class BuildGraph;


class BuildGraphValidator
{
public:

    void validate(
        const BuildGraph& graph
    ) const;
};


}
#pragma once

#include <string>
#include <vector>

namespace forge {

class Edge;

class Node
{
public:
    explicit Node(std::string path);

    const std::string& path() const;

    Edge* in_edge() const;

    void set_in_edge(Edge* edge);

    const std::vector<Edge*>& out_edges() const;

    void add_out_edge(Edge* edge);

private:
    std::string path_;

    // 生成当前 Node 的 Edge。
    // 源文件没有生产者，所以允许为 nullptr。
    Edge* in_edge_{nullptr};

    // 使用当前 Node 作为输入的所有 Edge。
    std::vector<Edge*> out_edges_;
};

} // namespace forge
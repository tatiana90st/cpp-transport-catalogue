#pragma once

#include "ranges.h"
#include <graph.pb.h>
#include <cstdlib>
#include <vector>

namespace graph {

using VertexId = size_t;
using EdgeId = size_t;

template <typename Weight>
struct Edge {
    VertexId from;
    VertexId to;
    Weight weight;
};

template <typename Weight>
class DirectedWeightedGraph {
private:
    using IncidenceList = std::vector<EdgeId>;
    using IncidentEdgesRange = ranges::Range<typename IncidenceList::const_iterator>;

public:
    DirectedWeightedGraph() = default;
    explicit DirectedWeightedGraph(size_t vertex_count);
    explicit DirectedWeightedGraph(std::vector<Edge<Weight>> edges, std::vector<IncidenceList> incidence_lists);//for serialization purposes

    EdgeId AddEdge(const Edge<Weight>& edge);

    size_t GetVertexCount() const;
    size_t GetEdgeCount() const;
    const Edge<Weight>& GetEdge(EdgeId edge_id) const;
    IncidentEdgesRange GetIncidentEdges(VertexId vertex) const;

    tc_serialize::Graph SerializeGraph() const;

private:
    std::vector<Edge<Weight>> edges_;
    std::vector<IncidenceList> incidence_lists_;
};

template <typename Weight>
DirectedWeightedGraph<Weight>::DirectedWeightedGraph(std::vector<Edge<Weight>> edges, std::vector<IncidenceList> incidence_lists)
    :edges_(std::move(edges))
    ,incidence_lists_(std::move(incidence_lists)) {

}

template <typename Weight>
DirectedWeightedGraph<Weight>::DirectedWeightedGraph(size_t vertex_count)
    : incidence_lists_(vertex_count) {
}

template <typename Weight>
EdgeId DirectedWeightedGraph<Weight>::AddEdge(const Edge<Weight>& edge) {
    edges_.push_back(edge);
    const EdgeId id = edges_.size() - 1;
    incidence_lists_.at(edge.from).push_back(id);
    return id;
}

template <typename Weight>
size_t DirectedWeightedGraph<Weight>::GetVertexCount() const {
    return incidence_lists_.size();
}

template <typename Weight>
size_t DirectedWeightedGraph<Weight>::GetEdgeCount() const {
    return edges_.size();
}

template <typename Weight>
const Edge<Weight>& DirectedWeightedGraph<Weight>::GetEdge(EdgeId edge_id) const {
    return edges_.at(edge_id);
}

template <typename Weight>
typename DirectedWeightedGraph<Weight>::IncidentEdgesRange
DirectedWeightedGraph<Weight>::GetIncidentEdges(VertexId vertex) const {
    return ranges::AsRange(incidence_lists_.at(vertex));
}

template <typename Weight>
tc_serialize::Graph DirectedWeightedGraph<Weight>::SerializeGraph() const {
    tc_serialize::Graph serial_graph;
    for (const Edge<Weight>& edge : edges_) {
        tc_serialize::Edge* s_edge = serial_graph.add_edges();
        s_edge->set_weight(edge.weight);
        s_edge->set_vertex_from(edge.from);
        s_edge->set_vertex_to(edge.to);
    }
    for (const IncidenceList& inc_list : incidence_lists_) {
        tc_serialize::IncidenceList* list = serial_graph.add_incidence_lists();
        for (EdgeId edge_id : inc_list) {
            list->add_edge_id_incidence_list(edge_id); //wow, i could do that?
        }
    }
    return serial_graph;
}

}  // namespace graph
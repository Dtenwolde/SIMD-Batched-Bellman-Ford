#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include "cpucycles.h"

using DistT = uint8_t;

const int number_of_vertices = 10;
const int number_of_edges = number_of_vertices * 5;
const int sources = 1;

DistT vertex_csr[number_of_vertices + 1];
DistT edge_csr[number_of_edges];
DistT weight_csr[number_of_edges];

void create_csr(DistT *vertex_csr, DistT *edge_csr, DistT *weight_csr, int number_of_vertices, int number_of_edges) {
    for (int i = 0; i < number_of_edges; i++) {
        edge_csr[i] = rand() % number_of_vertices;
    }
    std::cout << "Initialized edges" << std::endl;
    for (int i = 0; i < number_of_edges; i++) {
        weight_csr[i] = rand() % 10 + 1;
    }
    std::cout << "Initialized weight" << std::endl;

    for (int i = 0; i < number_of_vertices + 1; i++) {
        int j = i * (number_of_edges / number_of_vertices);
        vertex_csr[i] = j;
    }

    std::cout << "Initialized vertices" << std::endl;
}


DistT update_one_lane(DistT &n_dist, DistT v_dist, DistT weight) {
    DistT new_dist = v_dist + weight;
    bool better = new_dist < n_dist;
    DistT min = better ? new_dist : n_dist;
    DistT diff = n_dist ^ min;
    n_dist = min;
    return diff;
}


bool update_lanes(std::unordered_map<size_t, std::vector<DistT>> &dists, size_t v, DistT n, DistT weight) {
    std::vector<DistT> &v_dists = dists[v];
    std::vector<DistT> &n_dists = dists[n];

    size_t num_lanes = dists[v].size();
    size_t lane_idx = 0;
    DistT xor_diff = 0;
    while (lane_idx < num_lanes) {
        xor_diff |= update_one_lane(n_dists[lane_idx], v_dists[lane_idx], weight);
        ++lane_idx;
    }
    return xor_diff != 0;
}


long long
vectorised_bf(const DistT *vertex_csr, const DistT *edge_csr, const DistT *weight_csr, int number_of_vertices) {
    DistT source_vertex[sources];
    DistT number_of_sources = sizeof(source_vertex) / sizeof(source_vertex[0]);

    for (int i = 0; i < number_of_sources; i++) {
        source_vertex[i] = i;
    }
    std::unordered_map<size_t, std::vector<DistT>> modified;
    std::unordered_map<size_t, std::vector<DistT>> dists;

    for (int64_t i = 0; i < number_of_vertices; i++) {
        modified[i] = std::vector<DistT>(number_of_sources, false);
        dists[i] = std::vector<DistT>(number_of_sources, INT8_MAX);
    }
    int64_t lanes = 0;
    for (int64_t i = 0; i < number_of_sources; i++) {
        modified[source_vertex[i]][lanes] = true;
        dists[source_vertex[i]][lanes] = 0;
        lanes++;
    }
    bool changed = true;

    auto start = cpucycles();
    while (changed) {
        changed = false;
        for (int64_t v = 0; v < number_of_vertices; v++) {
            for (int64_t index = vertex_csr[v]; index < vertex_csr[v + 1]; index++) {
                changed = update_lanes(dists, v, edge_csr[index], weight_csr[index]) | changed;
            }
        }

    }
    auto stop = cpucycles();

    std::ofstream scalar_output_file("/home/daniel/git/SIMD-Batched-Bellman-Ford/vectorized_output.txt");
    for (int i = 0; i < number_of_vertices; i++) {
        for (int j = 0; j < number_of_sources; j++) {
            scalar_output_file << (int) dists[i][j] << " ";
        }
        scalar_output_file << std::endl;
    }

    return stop - start;
}

long long scalar_bf(const DistT *vertex_csr, const DistT *edge_csr, const DistT *weight_csr, int number_of_vertices) {
    DistT source_vertex[sources];
    DistT number_of_sources = sizeof(source_vertex) / sizeof(source_vertex[0]);

    for (int i = 0; i < number_of_sources; i++) {
        source_vertex[i] = i;
    }

    std::unordered_map<int64_t, std::vector<bool>> modified;
    std::unordered_map<int64_t, std::vector<DistT>> dists;

    for (int64_t i = 0; i < number_of_vertices; i++) {
        modified[i] = std::vector<bool>(number_of_sources, false);
        dists[i] = std::vector<DistT>(number_of_sources, INT8_MAX);
    }
    int64_t lanes = 0;
    for (int64_t i = 0; i < number_of_sources; i++) {
        modified[source_vertex[i]][lanes] = true;
        dists[source_vertex[i]][lanes] = 0;
        lanes++;
    }

    auto start = cpucycles();

    bool changed = true;
    while (changed) {
        changed = false;
        for (DistT v = 0; v < number_of_vertices; v++) {
            for (int64_t index = vertex_csr[v]; index < vertex_csr[v + 1]; index++) {
                DistT n = edge_csr[index];
                for (int64_t i = 0; i < modified[v].size(); i++) {
                    DistT new_dist = dists[v][i] + weight_csr[index];
                    DistT min_dist = std::min(dists[n][i], new_dist);
                    if (min_dist != dists[n][i]) {
                        dists[n][i] = min_dist;
                        modified[n][i] = true;
                        changed = true;
                    }
                }
            }
        }
    }

    auto stop = cpucycles();

    std::ofstream scalar_output_file("/home/daniel/git/SIMD-Batched-Bellman-Ford/scalar_output.txt");
    for (int i = 0; i < number_of_vertices; i++) {
        for (int j = 0; j < number_of_sources; j++) {
            scalar_output_file << (int) dists[i][j] << " ";
        }
        scalar_output_file << std::endl;
    }

    return stop - start;
}

long long scalar_modified_bf(const DistT *vertex_csr, const DistT *edge_csr, const DistT *weight_csr, int number_of_vertices) {

    DistT source_vertex[sources];
    DistT number_of_sources = sizeof(source_vertex) / sizeof(source_vertex[0]);

    for (int i = 0; i < number_of_sources; i++) {
        source_vertex[i] = i;
    }

    std::unordered_map<int64_t, std::vector<bool>> modified;
    std::unordered_map<int64_t, std::vector<DistT>> dists;

    for (int64_t i = 0; i < number_of_vertices; i++) {
        modified[i] = std::vector<bool>(number_of_sources, false);
        dists[i] = std::vector<DistT>(number_of_sources, INT8_MAX);
    }
    int64_t lanes = 0;
    for (int64_t i = 0; i < number_of_sources; i++) {
        modified[source_vertex[i]][lanes] = true;
        dists[source_vertex[i]][lanes] = 0;
        lanes++;
    }


    auto start = cpucycles();

    bool changed = true;
    while (changed) {
        changed = false;
        for (DistT v = 0; v < number_of_vertices; v++) {
            if (!std::all_of(modified[v].begin(), modified[v].end(), [](bool v) { return !v; })) {
                for (int64_t index = vertex_csr[v]; index < vertex_csr[v + 1]; index++) {
                    DistT n = edge_csr[index];
                    for (int64_t i = 0; i < modified[v].size(); i++) {
                        if (modified[v][i]) {
                            DistT new_dist = dists[v][i] + weight_csr[index];
                            DistT min_dist = std::min(dists[n][i], new_dist);
                            if (min_dist != dists[n][i]) {
                                dists[n][i] = min_dist;
                                modified[n][i] = true;
                                changed = true;
                            }
                        }
                    }
                }
            }
        }
    }

    auto stop = cpucycles();

    std::ofstream scalar_output_file("/home/daniel/git/SIMD-Batched-Bellman-Ford/scalar_output.txt");
    for (int i = 0; i < number_of_vertices; i++) {
        for (int j = 0; j < number_of_sources; j++) {
            scalar_output_file << (int) dists[i][j] << " ";
        }
        scalar_output_file << std::endl;
    }

    return stop - start;
}


int main() {
    create_csr(vertex_csr, edge_csr, weight_csr, number_of_vertices, number_of_edges);
    std::ofstream experiment_output_file("/home/daniel/git/SIMD-Batched-Bellman-Ford/experiment_output.csv",
                                         std::ios::app);
    for (int _ = 0; _ < 10; _++) {
        std::cout << _ << std::endl;
        auto scalar_cycles = scalar_bf(vertex_csr, edge_csr, weight_csr, number_of_vertices);
        std::cout << "Scalar cycles: " << scalar_cycles << std::endl;

        auto scalar_modified_cycles = scalar_modified_bf(vertex_csr, edge_csr, weight_csr, number_of_vertices);
        std::cout << "Scalar cycles: " << scalar_modified_cycles << std::endl;

        auto vectorised_cycles = vectorised_bf(vertex_csr, edge_csr, weight_csr, number_of_vertices);
        std::cout << "Vectorized cycles: " << vectorised_cycles << std::endl;

        std::cout << "Speedup: " << scalar_cycles / vectorised_cycles << std::endl;

        experiment_output_file << scalar_cycles << ", " << vectorised_cycles << ", " << scalar_modified_cycles << ", " << sources << ", " << number_of_vertices << ", " << number_of_edges << ", SSE2" << std::endl;
    }
}
#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include "cpucycles.h"

using DistT = uint32_t;

const int number_of_vertices = 1024;
const int number_of_edges = 1024 * 100;

DistT vertex_csr[number_of_vertices + 1];
DistT edge_csr[number_of_edges];
DistT weight_csr[number_of_edges];

void create_csr(DistT *vertex_csr, DistT* edge_csr, DistT* weight_csr, int number_of_vertices, int number_of_edges) {
//    vertex_csr[number_of_vertices + 1];
//    edge_csr[number_of_edges];
//    weight_csr[number_of_edges];
    for (int i = 0; i < number_of_edges; i++) {
        edge_csr[i] = rand() % number_of_vertices;
    }
    std::cout << "Initialized edges" << std::endl;
    for (int i = 0; i < number_of_edges; i++) {
        weight_csr[i] = rand() % 10;
    }
    std::cout << "Initialized weight" << std::endl;


    for (int i = 0; i < number_of_vertices + 1; i++) {
        int j = i * (number_of_edges / number_of_vertices);
        vertex_csr[i] = j;
    }

    std::cout << "Initialized vertices" << std::endl;



    //! Hardcoded examples if needed
//    int8_t vertex_csr[8]{0, 2, 4, 5, 7, 8, 9, 12};
//    int8_t edge_csr[12]{1, 3, 4, 6, 5, 0, 2, 5, 2, 2, 3, 4};
//    int8_t weight_csr[12]{1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4};
}




DistT update_one_lane(DistT& n_dist, DistT v_dist, DistT weight) {
    DistT new_dist = v_dist + weight;
    bool better = new_dist < n_dist;
    DistT min = better ? new_dist : n_dist;
    DistT diff = n_dist ^ min;
    n_dist = min;
    return diff;
}


bool update_lanes(std::unordered_map<size_t, std::vector<DistT>>& dists, size_t v, size_t index) {
    DistT n = edge_csr[index];
    DistT weight = weight_csr[index];
    std::vector<DistT>& v_dists = dists[v];
    std::vector<DistT>& n_dists = dists[n];

    size_t num_lanes = dists[v].size();
    size_t lane_idx = 0;
    DistT xor_diff = 0;
    while (lane_idx < num_lanes) {
        xor_diff |= update_one_lane(n_dists[lane_idx], v_dists[lane_idx], weight); ++lane_idx;
    }
    return xor_diff != 0;
}




// int8_t checkUpdate(int i, uint8_t weight, bool &changed, std::unordered_map<int8_t, std::vector<uint8_t>> &dists, int8_t n, int8_t v) {
//     const int8_t vector_width = 16;
//     uint8_t new_dists[vector_width];
//     uint8_t has_changed[vector_width];
//     #pragma clang loop vectorize(enable) // Pragma does not change anything afaik
//     for (int8_t j = 0; j < vector_width; j++) {
//         // Calculate the new distance from the vertex we are currently looking with the weight of the edge
//         new_dists[j] = (dists[v][i + j] + weight);
//         // Take the minimal of the two. Either the newly calculated distance or the current known distance
//         new_dists[j] = (new_dists[j] < dists[n][i + j]) ? new_dists[j] : dists[n][i + j];
//         // If the new distance is smaller than the known distance, set changed to true. If changed was
//         // Already true (i.e. a change was already made), stay true.
//         has_changed[j] = (new_dists[j] < dists[n][i + j]);
//         // Set the minimal distance to be the known distance.
//         dists[n][i + j] = new_dists[j];
//     }
//     for (int i = 0; i < vector_width; i++) {
//         if (has_changed[i]) {
//             return 1;
//         }
//     }
//     return 0;
// }



long long vectorized_bf(const DistT *vertex_csr, const DistT* edge_csr, const DistT* weight_csr, int number_of_vertices) {
//    int8_t vertex_csr[8]{0, 2, 4, 5, 7, 8, 9, 12};
//    int8_t edge_csr[12]{1, 3, 4, 6, 5, 0, 2, 5, 2, 2, 3, 4};
//
//    int8_t weight_csr[12]{1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4};
    int64_t source_vertex[16]{0, 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    int64_t number_of_sources = sizeof(source_vertex) / sizeof(source_vertex[0]);

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

    auto start = cpucycles_amd64cpuinfo();
    while (changed) {
        changed = false;
        // Loop through all vertices
        for (int64_t v = 0; v < number_of_vertices; v++) {
            // If non of them have been modified (i.e. they are all set to INF) we can skip the vertex as we know there will
            // not be a cheaper path found through this vertex than we currently know
            // if (!std::all_of(modified[v].begin(), modified[v].end(), [](bool v) { return !v; })) {
            // Loop through all edges (n) of this vertex
            for (int64_t index = vertex_csr[v]; index < vertex_csr[v + 1]; index++) {
                // int8_t n = edge_csr[index];
                // uint8_t weight = weight_csr[index];
                // For every entry that has been modified (again if it has not been modified, we know it will not give a cheaper path)
                // Increment this with the vector width. For SSE there are 128-bit wide registers (XMM), we have 8-bit integers
                // Therefore there can be 128 / 8 = 16 values loaded in this register (hence vector width = 16).
                // With AVX it would be 32 values and AVX-512 64.
                // for (int8_t i = 0; i < (int8_t) modified[v].size(); i += vector_width) {
                changed = update_lanes(dists, v, index) | changed;
                // #pragma clang loop vectorize(enable) // Pragma does not change anything afaik
                // for (int8_t j = i; j < i + vector_width; j++) {
                //     // Calculate the new distance from the vertex we are currently looking with the weight of the edge
                //     new_dists[j-i] = (dists[v][i + j] + weight);
                //     // Take the minimal of the two. Either the newly calculated distance or the current known distance
                //     new_dists[j-i] = (new_dists[j-i] < dists[n][i + j]) ? new_dists[j-i] : dists[n][i + j];
                //     // If the new distance is smaller than the known distance, set changed to true. If changed was
                //     // Already true (i.e. a change was already made), stay true.
                //     changed = ((new_dists[j-i] < dists[n][i + j]) | changed);
                //     // Set the minimal distance to be the known distance.
                //     dists[n][i + j] = new_dists[j-i];
                // }
                // }
            }
            // }
        }
        // After we have looped through all the vertices print some debug information.
//        for (int i = 0; i < number_of_vertices; i++) {
//            std::cout << "Vertex " << i << ": ";
//            for (int j = 0; j < number_of_sources; j++) {
//                std::cout << (int)dists[i][j] << " ";
//            }
//            std::cout << std::endl;
//        }
//        std::cout << "-" << std::endl;
    }
    auto stop = cpucycles_amd64cpuinfo();

//    std::cout << "Cycles: " << stop - start << std::endl;


    return stop - start;
}


// int vectorized_bf(const int8_t *vertex_csr, const int8_t* edge_csr, const int8_t* weight_csr, int number_of_vertices) {
// //    int8_t vertex_csr[8]{0, 2, 4, 5, 7, 8, 9, 12};
// //    int8_t edge_csr[12]{1, 3, 4, 6, 5, 0, 2, 5, 2, 2, 3, 4};
// //
// //    int8_t weight_csr[12]{1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4};
//     int8_t source_vertex[16]{0, 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
//     int8_t number_of_sources = sizeof(source_vertex) / sizeof(source_vertex[0]);

//     std::unordered_map<int8_t, std::vector<bool>> modified;
//     std::unordered_map<int8_t, std::vector<uint8_t>> dists;

//     for (int8_t i = 0; i < number_of_vertices; i++) {
//         modified[i] = std::vector<bool>(number_of_sources, false);
//         dists[i] = std::vector<uint8_t>(number_of_sources, INT8_MAX);
//     }
//     int8_t lanes = 0;
//     for (int8_t i = 0; i < number_of_sources; i++) {
//         modified[source_vertex[i]][lanes] = true;
//         dists[source_vertex[i]][lanes] = 0;
//         lanes++;
//     }
//     int8_t vector_width = 16;

//     bool changed = true;
//     int8_t new_dists[vector_width];
//     bool has_changed;

//     while (changed) {
//         changed = false;
//         for (int8_t v = 0; v < number_of_vertices; v++) {
//             for (int8_t index = vertex_csr[v]; index < vertex_csr[v + 1]; index++) {
//                 int8_t n = edge_csr[index];
//                 for (int8_t i = 0; i < (int8_t) 16; i += vector_width) {
//                     #pragma clang loop vectorize(enable)
//                     for (int8_t j = i; j < i + vector_width; j++) {
//                         new_dists[j-i] = dists[v][j] + weight_csr[index];
//                         dists[n][j] = dists[n][j] < new_dists[j-i] ? dists[n][j] : new_dists[j-i];
//                         has_changed |= dists[n][j] < new_dists[j-i];
//                     }
//                     changed = has_changed;
//                 }
//             }
//         }
//     }
//     return 0;
// }


long long scalar_bf(const DistT *vertex_csr, const DistT* edge_csr, const DistT* weight_csr, int number_of_vertices) {
//    int8_t input_size = 7;
//    int8_t vertex_csr[8]{0, 2, 4, 5, 7, 8, 9, 12};
//    int8_t edge_csr[12]{1, 3, 4, 6, 5, 0, 2, 5, 2, 2, 3, 4};
//    int8_t weight_csr[12]{1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4};
    int64_t source_vertex[16]{0, 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    int64_t number_of_sources = 16;

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



    auto start = cpucycles_amd64cpuinfo();

    bool changed = true;
    while (changed) {
        changed = false;
        for (DistT v = 0; v < number_of_vertices; v++) {
//            if (!std::all_of(modified[v].begin(), modified[v].end(), [](bool v) { return !v; })) {
                for (int64_t index = vertex_csr[v]; index < vertex_csr[v + 1]; index++) {
                    DistT n = edge_csr[index];
                    for (int64_t i = 0; i < modified[v].size(); i++) {
//                        if (modified[v][i]) {
                            DistT new_dist = dists[v][i] + weight_csr[index];
                            DistT min_dist = std::min(dists[n][i], new_dist);
                            if (min_dist != dists[n][i]) {
                                dists[n][i] = min_dist;
                                modified[n][i] = true;
                                changed = true;
                            }
//                        }
                    }
//                }
            }
        }
//        for (int i = 0; i < number_of_vertices; i++) {
//            std::cout << "Vertex " << i << ": ";
//            for (int j = 0; j < number_of_sources; j++) {
//                std::cout << (int)dists[i][j] << " ";
//            }
//            std::cout << std::endl;
//        }
//        std::cout << "-" << std::endl;
    }

    auto stop = cpucycles_amd64cpuinfo();

//    std::cout << "Cycles: " << stop - start << std::endl;

    return stop - start;
}



int main() {


    create_csr(vertex_csr, edge_csr, weight_csr, number_of_vertices, number_of_edges);
    for (int i = 0; i < sizeof(vertex_csr)/sizeof(vertex_csr[0]); i++) {
        std::cout << i << " Vertex: " << (int)vertex_csr[i] << std::endl;
    }
    for (int i = 0; i < sizeof(edge_csr)/sizeof(edge_csr[0]); i++) {
        std::cout << i << " Edge: " << (int)edge_csr[i] << std::endl;
    }


    auto scalar_cycles = scalar_bf(vertex_csr, edge_csr, weight_csr, number_of_vertices);
    std::cout << "Scalar cycles: " << scalar_cycles  << std::endl;

    auto vectorized_cycles = vectorized_bf(vertex_csr, edge_csr, weight_csr, number_of_vertices);
    std::cout << "Vectorized cycles: " << vectorized_cycles  << std::endl;

    std::cout << "Speedup: " << scalar_cycles / vectorized_cycles << std::endl;
}
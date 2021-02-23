/*
    smoothing.cpp: smoothing & reprojection

    This file is part of the implementation of

        Instant Field-Aligned Meshes
        Wenzel Jakob, Daniele Panozzo, Marco Tarini, and Olga Sorkine-Hornung
        In ACM Transactions on Graphics (Proc. SIGGRAPH Asia 2015)

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include "smoothing.h"

#include "bvh.h"

void smoothing(const int smooth_iterations, 
               MatrixXu& F, MatrixXf& O, MatrixXf& N,
               const std::set<uint32_t>& crease,
               const uint32_t nV, const uint32_t nF,
               Float scale, BVH* bvh, 
               const int posy, const bool pure_quad) {
    auto withNormals = N.size() > 0;
	
    std::vector<std::set<uint32_t>> adj_new(nV);
    std::vector<tbb::spin_mutex> locks(nV);
    tbb::parallel_for(
        tbb::blocked_range<uint32_t>(0u, nF, GRAIN_SIZE),
        [&](const tbb::blocked_range<uint32_t>& range) {
            for(uint32_t f = range.begin(); f != range.end(); ++f) {
                if(posy == 4 && F(2, f) == F(3, f)) {
                    /* Irregular face */
                    if(pure_quad) /* Should never get these when subdivision is on */
                        throw std::runtime_error("Internal error in extraction");
                    uint32_t i0 = F(0, f), i1 = F(1, f);
                    if(i0 < i1)
                        std::swap(i1, i0);
                    if(i0 == i1)
                        continue;
                    tbb::spin_mutex::scoped_lock lock1(locks[i0]);
                    tbb::spin_mutex::scoped_lock lock2(locks[i1]);
                    adj_new[i0].insert(i1);
                    adj_new[i1].insert(i0);
                }
                else {
                    for(int j = 0; j < F.rows(); ++j) {
                        uint32_t i0 = F(j, f), i1 = F((j + 1) % F.rows(), f);
                        if(i0 < i1)
                            std::swap(i1, i0);
                        if(i0 == i1)
                            continue;
                        tbb::spin_mutex::scoped_lock lock1(locks[i0]);
                        tbb::spin_mutex::scoped_lock lock2(locks[i1]);
                        adj_new[i0].insert(i1);
                        adj_new[i1].insert(i0);
                    }
                }
            }
        }
    );

    for(int it = 0; it < smooth_iterations; ++it) {
        MatrixXf O_prime(O.rows(), O.cols());
        MatrixXf N_prime(N.rows(), N.cols());
        cout << ".";
        cout.flush();

        tbb::parallel_for(
            tbb::blocked_range<uint32_t>(0u, (uint32_t)O.cols(), GRAIN_SIZE),
            [&](const tbb::blocked_range<uint32_t>& range) {
                std::set<uint32_t> temp;
                for(uint32_t i = range.begin(); i != range.end(); ++i) {
                    bool is_crease = crease.find(i) != crease.end();
                    if(adj_new[i].size() > 0 && !is_crease) {
                        Vector3f centroid = Vector3f::Zero(), avgNormal = Vector3f::Zero();
                        for(auto j : adj_new[i]) {
                            centroid += O.col(j);
                        	if(withNormals)
								avgNormal += N.col(j);
                        }
                        if(withNormals)
							avgNormal += N.col(i);
                        centroid /= adj_new[i].size();
                        Matrix3f cov = Matrix3f::Zero();
                        for(auto j : adj_new[i])
                            cov += (O.col(j) - centroid) * (O.col(j) - centroid).transpose();
                        Vector3f n = cov.jacobiSvd(Eigen::ComputeFullU).matrixU().col(2).normalized();
                        n *= signum(avgNormal.dot(n));

                        if(bvh && bvh->F()->size() > 0) {
                            Ray ray1(centroid, n, 0, scale / 2);
                            Ray ray2(centroid, -n, 0, scale / 2);
                            uint32_t idx1 = 0, idx2 = 0;
                            Float t1 = 0, t2 = 0;
                            bvh->rayIntersect(ray1, idx1, t1);
                            bvh->rayIntersect(ray2, idx2, t2);
                            if(std::min(t1, t2) < scale * 0.5f)
                                centroid = t1 < t2 ? ray1(t1) : ray2(t2);
                        }
                        O_prime.col(i) = centroid;
                        if(withNormals)
							N_prime.col(i) = n;
                    }
                    else {
                        O_prime.col(i) = O.col(i);
                        if(withNormals)
							N_prime.col(i) = N.col(i);
                    }
                }
            }
        );
        O_prime.swap(O);
        if(withNormals)
			N_prime.swap(N);
    }
}
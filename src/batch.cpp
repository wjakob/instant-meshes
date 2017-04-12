/*
    batch.cpp -- command line interface to Instant Meshes

    This file is part of the implementation of

        Instant Field-Aligned Meshes
        Wenzel Jakob, Daniele Panozzo, Marco Tarini, and Olga Sorkine-Hornung
        In ACM Transactions on Graphics (Proc. SIGGRAPH Asia 2015)

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include "batch.h"
#include "meshio.h"
#include "dedge.h"
#include "subdivide.h"
#include "meshstats.h"
#include "hierarchy.h"
#include "field.h"
#include "normal.h"
#include "extract.h"
#include "bvh.h"

Mesh batch_process(const Mesh& in_mesh,
                   int rosy, int posy, Float scale, int face_count,
                   int vertex_count, Float creaseAngle, bool extrinsic,
                   bool align_to_boundaries, int smooth_iter, int knn_points,
                   bool pure_quad, bool deterministic) {
    MatrixXu F;
    MatrixXf V, N;
    VectorXf A;
    std::set<uint32_t> crease_in, crease_out;
    BVH *bvh = nullptr;
    AdjacencyMatrix adj = nullptr;

    F = in_mesh.F;
    V = in_mesh.V;
    N = in_mesh.N;

    bool pointcloud = F.size() == 0;

    Timer<> timer;
    MeshStats stats = compute_mesh_stats(F, V, deterministic);

    if (pointcloud) {
        bvh = new BVH(&F, &V, &N, stats.mAABB);
        bvh->build();
        adj = generate_adjacency_matrix_pointcloud(V, N, bvh, stats, knn_points, deterministic);
        A.resize(V.cols());
        A.setConstant(1.0f);
    }

    if (scale < 0 && vertex_count < 0 && face_count < 0) {
        vertex_count = V.cols() / 16;
    }

    if (scale > 0) {
        Float face_area = posy == 4 ? (scale*scale) : (std::sqrt(3.f)/4.f*scale*scale);
        face_count = stats.mSurfaceArea / face_area;
        vertex_count = posy == 4 ? face_count : (face_count / 2);
    } else if (face_count > 0) {
        Float face_area = stats.mSurfaceArea / face_count;
        vertex_count = posy == 4 ? face_count : (face_count / 2);
        scale = posy == 4 ? std::sqrt(face_area) : (2*std::sqrt(face_area * std::sqrt(1.f/3.f)));
    } else if (vertex_count > 0) {
        face_count = posy == 4 ? vertex_count : (vertex_count * 2);
        Float face_area = stats.mSurfaceArea / face_count;
        scale = posy == 4 ? std::sqrt(face_area) : (2*std::sqrt(face_area * std::sqrt(1.f/3.f)));
    }

    MultiResolutionHierarchy mRes;

    if (!pointcloud) {
        /* Subdivide the mesh if necessary */
        VectorXu V2E, E2E;
        VectorXb boundary, nonManifold;
        if (stats.mMaximumEdgeLength*2 > scale || stats.mMaximumEdgeLength > stats.mAverageEdgeLength * 2) {
            build_dedge(F, V, V2E, E2E, boundary, nonManifold);
            subdivide(F, V, V2E, E2E, boundary, nonManifold, std::min(scale/2, (Float) stats.mAverageEdgeLength*2), deterministic);
        }

        /* Compute a directed edge data structure */
        build_dedge(F, V, V2E, E2E, boundary, nonManifold);

        /* Compute adjacency matrix */
        adj = generate_adjacency_matrix_uniform(F, V2E, E2E, nonManifold);

        /* Compute vertex/crease normals */
        if (creaseAngle >= 0)
            generate_crease_normals(F, V, V2E, E2E, boundary, nonManifold, creaseAngle, N, crease_in);
        else
            generate_smooth_normals(F, V, V2E, E2E, nonManifold, N);

        /* Compute dual vertex areas */
        compute_dual_vertex_areas(F, V, V2E, E2E, nonManifold, A);

        mRes.setE2E(std::move(E2E));
    }

    /* Build multi-resolution hierarrchy */
    mRes.setAdj(std::move(adj));
    mRes.setF(std::move(F));
    mRes.setV(std::move(V));
    mRes.setA(std::move(A));
    mRes.setN(std::move(N));
    mRes.setScale(scale);
    mRes.build(deterministic);
    mRes.resetSolution();

    if (align_to_boundaries && !pointcloud) {
        mRes.clearConstraints();
        for (uint32_t i=0; i<3*mRes.F().cols(); ++i) {
            if (mRes.E2E()[i] == INVALID) {
                uint32_t i0 = mRes.F()(i%3, i/3);
                uint32_t i1 = mRes.F()((i+1)%3, i/3);
                Vector3f p0 = mRes.V().col(i0), p1 = mRes.V().col(i1);
                Vector3f edge = p1-p0;
                if (edge.squaredNorm() > 0) {
                    edge.normalize();
                    mRes.CO().col(i0) = p0;
                    mRes.CO().col(i1) = p1;
                    mRes.CQ().col(i0) = mRes.CQ().col(i1) = edge;
                    mRes.CQw()[i0] = mRes.CQw()[i1] = mRes.COw()[i0] =
                        mRes.COw()[i1] = 1.0f;
                }
            }
        }
        mRes.propagateConstraints(rosy, posy);
    }

    if (bvh) {
        bvh->setData(&mRes.F(), &mRes.V(), &mRes.N());
    } else if (smooth_iter > 0) {
        bvh = new BVH(&mRes.F(), &mRes.V(), &mRes.N(), stats.mAABB);
        bvh->build();
    }

    Optimizer optimizer(mRes, false);
    optimizer.setRoSy(rosy);
    optimizer.setPoSy(posy);
    optimizer.setExtrinsic(extrinsic);

    optimizer.optimizeOrientations(-1);
    optimizer.notify();
    optimizer.wait();

    std::map<uint32_t, uint32_t> sing;
    compute_orientation_singularities(mRes, sing, extrinsic, rosy);
    timer.reset();

    optimizer.optimizePositions(-1);
    optimizer.notify();
    optimizer.wait();

    //std::map<uint32_t, Vector2i> pos_sing;
    //compute_position_singularities(mRes, sing, pos_sing, extrinsic, rosy, posy);
    //timer.reset();

    optimizer.shutdown();

    MatrixXf O_extr, N_extr, Nf_extr;
    std::vector<std::vector<TaggedLink>> adj_extr;
    extract_graph(mRes, extrinsic, rosy, posy, adj_extr, O_extr, N_extr,
                  crease_in, crease_out, deterministic);

    MatrixXu F_extr;
    extract_faces(adj_extr, O_extr, N_extr, Nf_extr, F_extr, posy,
            mRes.scale(), crease_out, true, pure_quad, bvh, smooth_iter);

    if (bvh)
        delete bvh;

    return {F_extr, O_extr, Nf_extr};
}

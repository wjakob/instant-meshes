/*
    smoothing.h: smoothing & reprojection

    This file is part of the implementation of

        Instant Field-Aligned Meshes
        Wenzel Jakob, Daniele Panozzo, Marco Tarini, and Olga Sorkine-Hornung
        In ACM Transactions on Graphics (Proc. SIGGRAPH Asia 2015)

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#pragma once

#include "common.h"
#include <set>

class BVH;

extern void smoothing(int smooth_iterations, 
                      MatrixXu& F, MatrixXf& O, MatrixXf& N,
                      const std::set<uint32_t>& crease,
                      uint32_t nV, uint32_t nF, 
                      Float scale = 1.0, BVH* bvh = nullptr,
                      int posy = 4, bool pure_quad = true);
# Instant field-aligned meshes: minimal Python implementation in 100 LOC. 4-(R/P)oSy, uses randomization approach
import numpy as np
from itertools import product as all_combinations

def lattice_op(p, o, n, target, scale, op = np.floor):
    """ 4-PoSy lattice floor/rounding operation -- see the paper appendix for details """
    t, d = np.cross(n, o), target - p
    return p + scale * (o * op(np.dot(o, d) / scale) + t * op(np.dot(t, d) / scale))

def intermediate_pos(p0, n0, p1, n1):
    """ Find an intermediate position between two vertices -- see the paper appendix """
    n0p0, n0p1, n1p0, n1p1, n0n1 = np.dot(n0, p0), np.dot(n0, p1), np.dot(n1, p0), np.dot(n1, p1), np.dot(n0, n1)
    denom = 1.0 / (1.0 - n0n1*n0n1 + 1e-4)
    lambda_0 = 2.0*(n0p1 - n0p0 - n0n1*(n1p0 - n1p1))*denom
    lambda_1 = 2.0*(n1p0 - n1p1 - n0n1*(n0p1 - n0p0))*denom
    return 0.5 * (p0 + p1) - 0.25 * (n0 * lambda_0 + n1 * lambda_1)

def compat_orientation_extrinsic(o0, n0, o1, n1):
    """ Find compatible versions of two representative orientations (with specified normals) """
    return max(all_combinations([o0, np.cross(n0, o0), -o0, -np.cross(n0, o0)], [o1, np.cross(n1, o1)]),
        key = lambda x: np.dot(x[0], x[1]))

def compat_position_extrinsic(o0, p0, n0, v0, o1, p1, n1, v1, scale):
    """ Find compatible versions of two representative positions (with specified normals and orientations) """
    t0, t1, middle = np.cross(n0, o0), np.cross(n1, o1), intermediate_pos(v0, n0, v1, n1)
    p0, p1 = lattice_op(p0, o0, n0, middle, scale), lattice_op(p1, o1, n1, middle, scale)
    x = min(all_combinations([0, 1], [0, 1], [0, 1], [0, 1]),
        key = lambda x : np.linalg.norm((p0 + scale * (o0 * x[0] + t0 * x[1])) - (p1 + scale * (o1 * x[2] + t1 * x[3]))))
    result = (p0 + scale * (o0 * x[0] + t0 * x[1]), p1 + scale * (o1 * x[2] + t1 * x[3]))
    return result

class Mesh:
    def __init__(self, filename):
        print('Loading \"%s\" ..' % filename)
        with open(filename) as f:
            if f.readline().strip() != 'OFF': raise Exception("Invalid format")
            self.nverts, self.nfaces, _ = map(int, f.readline().split())
            self.vertices, self.faces = np.zeros((self.nverts, 3)), np.zeros((self.nfaces, 3), np.uint32)
            for i in range(self.nverts):
                self.vertices[i, :] = np.fromstring(f.readline(), sep=' ')
            for i in range(self.nfaces):
                self.faces[i, :] = np.fromstring(f.readline(), sep=' ', dtype=np.uint32)[1:]
        print('Computing face and vertex normals ..')
        v = [self.vertices[self.faces[:, i], :] for i in range(3)]
        face_normals = np.cross(v[2] - v[0], v[1] - v[0])
        face_normals /= np.linalg.norm(face_normals, axis=1)[:, None]
        self.normals = np.zeros((self.nverts, 3))
        for i, j in np.ndindex(self.faces.shape):
            self.normals[self.faces[i, j], :] += face_normals[i, :]
        self.normals /= np.linalg.norm(self.normals, axis=1)[:, None]
        print('Building adjacency matrix ..')
        self.adjacency = [set() for _ in range(self.nfaces)]
        for i, j in np.ndindex(self.faces.shape):
            e0, e1 = self.faces[i, j], self.faces[i, (j+1)%3]
            self.adjacency[e0].add(e1)
            self.adjacency[e1].add(e0)
        print('Randomly initializing fields ..')
        self.o_field = np.zeros((self.nverts, 3))
        self.p_field = np.zeros((self.nverts, 3))
        min_pos, max_pos = self.vertices.min(axis=0), self.vertices.max(axis=0)
        np.random.seed(0)
        for i in range(self.nverts):
            d, p = np.random.standard_normal(3), np.random.random(3)
            d -= np.dot(d, self.normals[i]) * self.normals[i]
            self.o_field[i] = d / np.linalg.norm(d)
            self.p_field[i] = (1-p) * min_pos + p * max_pos

    def smooth_orientations(self, iterations = 100):
        for i in range(iterations):
            print('Smoothing orientations (%i/%i) ..' % (i+1, iterations))
            for i in np.random.permutation(np.arange(self.nverts)):
                o_i, n_i, weight = self.o_field[i], self.normals[i], 0
                for j in np.random.permutation(list(self.adjacency[i])):
                    o_compat = compat_orientation_extrinsic(o_i, n_i, self.o_field[j], self.normals[j])
                    o_i = weight*o_compat[0] + o_compat[1]
                    o_i -= n_i * np.dot(o_i, n_i)
                    o_i /= np.linalg.norm(o_i)
                    weight += 1
                self.o_field[i] = o_i

    def smooth_positions(self, scale, iterations = 100):
        for i in range(iterations):
            print('Smoothing positions (%i/%i) ..' % (i+1, iterations))
            for i in np.random.permutation(np.arange(self.nverts)):
                o_i, p_i, n_i, v_i, weight = self.o_field[i], self.p_field[i], self.normals[i], self.vertices[i], 0
                for j in self.adjacency[i]:
                    p_compat = compat_position_extrinsic(o_i, p_i, n_i, v_i,
                        self.o_field[j], self.p_field[j], self.normals[j], self.vertices[j], scale)
                    p_i = (weight*p_compat[0] + p_compat[1]) / (weight + 1)
                    p_i -= n_i * np.dot(p_i - v_i, n_i)
                    weight += 1
                self.p_field[i] = lattice_op(p_i, o_i, n_i, v_i, scale, op = np.round)

    def save_position_field(self, filename):
        np.savetxt(filename, self.p_field, header="OFF\n%i 0 0" % self.nverts, comments='')

mesh = Mesh("bunny.off")
mesh.smooth_orientations()
mesh.smooth_positions(scale = 0.01)
mesh.save_position_field("output.off")

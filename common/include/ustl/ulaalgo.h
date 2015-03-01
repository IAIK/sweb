// This file is part of the uSTL library, an STL implementation.
//
// Copyright (c) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "umatrix.h"

namespace ustl {

/// \brief Creates an identity matrix in \p m
/// \ingroup NumericAlgorithms
template <size_t NX, size_t NY, typename T>
void load_identity (matrix<NX,NY,T>& m)
{
    fill_n (m.begin(), NX * NY, 0);
    for (typename matrix<NX,NY,T>::iterator i = m.begin(); i < m.end(); i += NX + 1)
  *i = 1;
}

/// \brief Multiplies two matrices
/// \ingroup NumericAlgorithms
template <size_t NX, size_t NY, typename T>
matrix<NY,NY,T> operator* (const matrix<NX,NY,T>& m1, const matrix<NY,NX,T>& m2)
{
    matrix<NY,NY,T> mr;
    for (uoff_t ry = 0; ry < NY; ++ ry) {
  for (uoff_t rx = 0; rx < NY; ++ rx) {
      T dpv (0);
      for (uoff_t x = 0; x < NX; ++ x)
    dpv += m1[ry][x] * m2[x][rx];
      mr[ry][rx] = dpv;
  }
    }
    return mr;
}

/// \brief Transforms vector \p t with matrix \p m
/// \ingroup NumericAlgorithms
template <size_t NX, size_t NY, typename T>
tuple<NX,T> operator* (const tuple<NY,T>& t, const matrix<NX,NY,T>& m)
{
    tuple<NX,T> tr;
    for (uoff_t x = 0; x < NX; ++ x) {
  T dpv (0);
  for (uoff_t y = 0; y < NY; ++ y)
      dpv += t[y] * m[y][x];
  tr[x] = dpv;
    }
    return tr;
}

/// \brief Transposes (exchanges rows and columns) matrix \p m.
/// \ingroup NumericAlgorithms
template <size_t N, typename T>
void transpose (matrix<N,N,T>& m)
{
    for (uoff_t x = 0; x < N; ++ x)
  for (uoff_t y = x; y < N; ++ y)
      swap (m[x][y], m[y][x]);
}

/// Specialization for 4-component vector transform, the slow part of 3D graphics.
template <> inline tuple<4,float> operator* (const tuple<4,float>& t, const matrix<4,4,float>& m)
{
    tuple<4,float> tr;
    for (uoff_t i = 0; i < 4; ++ i)
  tr[i] = t[0] * m[0][i] + t[1] * m[1][i] + t[2] * m[2][i] + t[3] * m[3][i];
    return tr;
}

} // namespace ustl

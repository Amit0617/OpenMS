// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2022
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
// https://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// Version: 6.0.2022.01.06

#pragma once

#include <Mathematics/Matrix.h>
#include <Mathematics/Vector3.h>
#include <Mathematics/Hypersphere.h>
#include <Mathematics/SymmetricEigensolver.h>

namespace gte
{
    // The quadratic fit is
    //   0 = C[0] + C[1]*x + C[2]*y + C[3]*z + C[4]*x^2 + C[5]*x*y
    //       + C[6]*x*z + C[7]*y^2 + C[8]*y*z + C[9]*z^2
    // which has one degree of freedom in the coefficients. Eliminate the
    // degree of freedom by minimizing the quadratic form E(C) = C^T M C
    // subject to Length(C) = 1 with M = (sum_i V_i)(sum_i V_i)^t where
    //   V = (1, x, y, z, x^2, x*y, x*z, y^2, y*z, z^2)
    // The minimum value is the smallest eigenvalue of M and C is a
    // corresponding unit length eigenvector.
    //
    // Input:
    //   n = number of points to fit
    //   P[0..n-1] = array of points to fit
    //
    // Output:
    //   C[0..9] = coefficients of quadratic fit (the eigenvector)
    //   return value of function is nonnegative and a measure of the fit
    //   (the minimum eigenvalue; 0 = exact fit, positive otherwise)
    //
    // Canonical forms. The quadratic equation can be factored into
    // P^T A P + B^T P + K = 0 where P = (x,y,z), K = C[0],
    // B = (C[1],C[2],C[3]) and A is a 3x3 symmetric matrix with
    // A00 = C[4], A01 = C[5]/2, A02 = C[6]/2, A11 = C[7], A12 = C[8]/2 and
    // A22 = C[9]. Using an eigendecomposition, matrix A = R^T D R where
    // R is orthogonal and D is diagonal. Define V = R*P = (v0,v1,v2),
    // E = R*B = (e0,e1,e2), D = diag(d0,d1,d2) and f = K to obtain
    //   d0 v0^2 + d1 v1^2 + d2 v^2 + e0 v0 + e1 v1 + e2 v2 + f = 0
    // The classification depends on the signs of the d_i. See the file
    // QuadricSurface.h for determining the type of quadric surface. 

    template <typename Real>
    class ApprQuadratic3
    {
    public:
        Real operator()(int32_t numPoints, Vector3<Real> const* points,
            std::array<Real, 10>& coefficients)
        {
            Matrix<10, 10, Real> M{};  // constructor sets M to zero
            for (int32_t i = 0; i < numPoints; ++i)
            {
                Real x = points[i][0];
                Real y = points[i][1];
                Real z = points[i][2];
                Real x2 = x * x;
                Real y2 = y * y;
                Real z2 = z * z;
                Real xy = x * y;
                Real xz = x * z;
                Real yz = y * z;
                Real x3 = x * x2;
                Real xy2 = x * y2;
                Real xz2 = x * z2;
                Real x2y = x * xy;
                Real x2z = x * xz;
                Real xyz = x * yz;
                Real y3 = y * y2;
                Real yz2 = y * z2;
                Real y2z = y * yz;
                Real z3 = z * z2;
                Real x4 = x * x3;
                Real x2y2 = x * xy2;
                Real x2z2 = x * xz2;
                Real x3y = x * x2y;
                Real x3z = x * x2z;
                Real x2yz = x * xyz;
                Real y4 = y * y3;
                Real y2z2 = y * yz2;
                Real xy3 = x * y3;
                Real xy2z = x * y2z;
                Real y3z = y * y2z;
                Real z4 = z * z3;
                Real xyz2 = x * yz2;
                Real xz3 = x * z3;
                Real yz3 = y * z3;

                // M(0, 0) += 1
                M(0, 1) += x;
                M(0, 2) += y;
                M(0, 3) += z;
                M(0, 4) += x2;
                M(0, 5) += xy;
                M(0, 6) += xz;
                M(0, 7) += y2;
                M(0, 8) += yz;
                M(0, 9) += z2;

                // M(1, 1) += x2    [M(0,4)]
                // M(1, 2) += xy    [M(0,5)]
                // M(1, 3) += xz    [M(0,6)]
                M(1, 4) += x3;
                M(1, 5) += x2y;
                M(1, 6) += x2z;
                M(1, 7) += xy2;
                M(1, 8) += xyz;
                M(1, 9) += xz2;

                // M(2, 2) += y2    [M(0,7)]
                // M(2, 3) += yz    [M(0,8)]
                // M(2, 4) += x2y   [M(1,5)]
                M(2, 5) += xy2;
                // M(2, 6) += xyz   [M(1,8)]
                M(2, 7) += y3;
                M(2, 8) += y2z;
                M(2, 9) += yz2;

                // M(3, 3) += z2    [M(0,9)]
                // M(3, 4) += x2z   [M(1,6)]
                // M(3, 5) += xyz   [M(1,8)]
                // M(3, 6) += xz2   [M(1,9)]
                // M(3, 7) += y2z   [M(2,8)]
                // M(3, 8) += yz2   [M(2,9)]
                M(3, 9) += z3;

                M(4, 4) += x4;
                M(4, 5) += x3y;
                M(4, 6) += x3z;
                M(4, 7) += x2y2;
                M(4, 8) += x2yz;
                M(4, 9) += x2z2;

                // M(5, 5) += x2y2  [M(4,7)]
                // M(5, 6) += x2yz  [M(4,8)]
                M(5, 7) += xy3;
                M(5, 8) += xy2z;
                M(5, 9) += xyz2;

                // M(6, 6) += x2z2  [M(4,9)]
                // M(6, 7) += xy2z  [M(5,8)]
                // M(6, 8) += xyz2  [M(5,9)]
                M(6, 9) += xz3;

                M(7, 7) += y4;
                M(7, 8) += y3z;
                M(7, 9) += y2z2;

                // M(8, 8) += y2z2  [M(7,9)]
                M(8, 9) += yz3;

                M(9, 9) += z4;
            }

            Real const rNumPoints = static_cast<Real>(numPoints);
            M(0, 0) = rNumPoints;
            M(1, 1) = M(0, 4);  // x2
            M(1, 2) = M(0, 5);  // xy
            M(1, 3) = M(0, 6);  // xz
            M(2, 2) = M(0, 7);  // y2
            M(2, 3) = M(0, 8);  // yz
            M(2, 4) = M(1, 5);  // x2y
            M(2, 6) = M(1, 8);  // xyz
            M(3, 3) = M(0, 9);  // z2
            M(3, 4) = M(1, 6);  // x2z
            M(3, 5) = M(1, 8);  // xyz
            M(3, 6) = M(1, 9);  // xz2
            M(3, 7) = M(2, 8);  // y2z
            M(3, 8) = M(2, 9);  // yz2
            M(5, 5) = M(4, 7);  // x2y2
            M(5, 6) = M(4, 8);  // x2yz
            M(6, 6) = M(4, 9);  // x2z2
            M(6, 7) = M(5, 8);  // xy2z
            M(6, 8) = M(5, 9);  // xyz2
            M(8, 8) = M(7, 9);  // y2z2

            for (int32_t row = 0; row < 10; ++row)
            {
                for (int32_t col = 0; col < row; ++col)
                {
                    M(row, col) = M(col, row);
                }
            }

            for (int32_t row = 0; row < 10; ++row)
            {
                for (int32_t col = 0; col < 10; ++col)
                {
                    M(row, col) /= rNumPoints;
                }
            }

            SymmetricEigensolver<Real> es(10, 1024);
            es.Solve(&M[0], +1);
            es.GetEigenvector(0, coefficients.data());

            // For an exact fit, numeric round-off errors might make the
            // minimum eigenvalue just slightly negative. Return the clamped
            // value because the application might rely on the return value
            // being nonnegative.
            return std::max(es.GetEigenvalue(0), static_cast<Real>(0));
        }
    };


    // If you believe your points are nearly spherical, use this. The sphere
    // is of the form
    //   C'[0] + C'[1]*x + C'[2]*y + C'[3]*z + C'[4]*(x^2 + y^2 + z^2) = 0
    // where Length(C') = 1. The function returns
    //   C = (C'[0] / C'[4], C'[1] / C'[4], C'[2] / C'[4], C'[3] / C'[4])
    //     = (C[0], C[1], C[2], C[3])
    // so the fitted sphere is
    //   C[0] + C[1]*x + C[2]*y + C[3]*z + x^2 + y^2 + z^2 = 0
    // The center is (xc,yc,zc) = -0.5*(C[1],C[2],C[3]) and the radius is
    // r = sqrt(xc * xc + yc * yc + zc * zc - C[0]).

    template <typename Real>
    class ApprQuadraticSphere3
    {
    public:
        Real operator()(int32_t numPoints, Vector3<Real> const* points, Sphere3<Real>& sphere)
        {
            Matrix<5, 5, Real> M{};  // constructor sets M to zero
            for (int32_t i = 0; i < numPoints; ++i)
            {
                Real x = points[i][0];
                Real y = points[i][1];
                Real z = points[i][2];
                Real x2 = x * x;
                Real y2 = y * y;
                Real z2 = z * z;
                Real xy = x * y;
                Real xz = x * z;
                Real yz = y * z;
                Real r2 = x2 + y2 + z2;
                Real xr2 = x * r2;
                Real yr2 = y * r2;
                Real zr2 = z * r2;
                Real r4 = r2 * r2;

                // M(0, 0) += 1
                M(0, 1) += x;
                M(0, 2) += y;
                M(0, 3) += z;
                M(0, 4) += r2;

                M(1, 1) += x2;
                M(1, 2) += xy;
                M(1, 3) += xz;
                M(1, 4) += xr2;

                M(2, 2) += y2;
                M(2, 3) += yz;
                M(2, 4) += yr2;

                M(3, 3) += z2;
                M(3, 4) += zr2;

                M(4, 4) += r4;
            }

            Real const rNumPoints = static_cast<Real>(numPoints);
            M(0, 0) = rNumPoints;

            for (int32_t row = 0; row < 5; ++row)
            {
                for (int32_t col = 0; col < row; ++col)
                {
                    M(row, col) = M(col, row);
                }
            }

            for (int32_t row = 0; row < 5; ++row)
            {
                for (int32_t col = 0; col < 5; ++col)
                {
                    M(row, col) /= rNumPoints;
                }
            }

            M(0, 0) = static_cast<Real>(1);

            SymmetricEigensolver<Real> es(5, 1024);
            es.Solve(&M[0], +1);
            Vector<5, Real> evector{};
            es.GetEigenvector(0, &evector[0]);

            std::array<Real, 4> coefficients{};
            for (int32_t row = 0; row < 4; ++row)
            {
                coefficients[row] = evector[row] / evector[4];
            }

            // Clamp the radius to nonnegative values in case rounding errors
            // cause sqrRadius to be slightly negative.
            Real const negHalf = static_cast<Real>(-0.5);
            sphere.center[0] = negHalf * coefficients[1];
            sphere.center[1] = negHalf * coefficients[2];
            sphere.center[2] = negHalf * coefficients[3];
            Real sqrRadius = Dot(sphere.center, sphere.center) - coefficients[0];
            sphere.radius = std::sqrt(std::max(sqrRadius, static_cast<Real>(0)));

            // For an exact fit, numeric round-off errors might make the
            // minimum eigenvalue just slightly negative. Return the clamped
            // value because the application might rely on the return value
            // being nonnegative.
            return std::max(es.GetEigenvalue(0), static_cast<Real>(0));
        }
    };
}

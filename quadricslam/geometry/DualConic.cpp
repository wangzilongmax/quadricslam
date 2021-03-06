/* ----------------------------------------------------------------------------

 * QuadricSLAM Copyright 2020, ARC Centre of Excellence for Robotic Vision, Queensland University of Technology (QUT)
 * Brisbane, QLD 4000
 * All Rights Reserved
 * Authors: Lachlan Nicholson, et al. (see THANKS for the full author list)
 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file DualConic.cpp
 * @date Apr 14, 2020
 * @author Lachlan Nicholson
 * @brief a dual conic
 */

#include <quadricslam/geometry/DualConic.h>
#include <quadricslam/geometry/QuadricCamera.h>
#include <quadricslam/base/Utilities.h>
#include <quadricslam/base/QuadricProjectionException.h>

#include <gtsam/base/numericalDerivative.h>

#include <iostream>
#include <iomanip>
#include <cmath>

using namespace std;

namespace gtsam {

/* ************************************************************************* */
DualConic::DualConic() {
  dC_ = (Matrix33() << 1.0, 0.0, 0.0, 
                        0.0, 1.0, 0.0,
                        0.0, 0.0, -1.0).finished();
}

/* ************************************************************************* */
DualConic::DualConic(const Pose2& pose, const Vector2& radii) {
  Matrix33 Z = pose.matrix();
  Matrix33 Cc = (Vector3() << (radii).array().pow(2), -1.0).finished().asDiagonal();
  Matrix33 dC = Z * Cc * Z.transpose();
  dC_ = dC;
}

/* ************************************************************************* */
DualConic DualConic::normalize(void) const {
  return DualConic(dC_/dC_(2,2));
}

/* ************************************************************************* */
/// TODO: assert conic is closed (eccentricity)
/// assert bounds are real-valued
/// normalize conic
AlignedBox2 DualConic::bounds(OptionalJacobian<4,9> H) const {
  double xmin = (dC_(0,2) + sqrt(dC_(0,2)*dC_(0,2)-dC_(2,2)*dC_(0,0))) / dC_(2,2);
  double xmax = (dC_(0,2) - sqrt(dC_(0,2)*dC_(0,2)-dC_(2,2)*dC_(0,0))) / dC_(2,2);
  double ymin = (dC_(1,2) + sqrt(dC_(1,2)*dC_(1,2)-dC_(2,2)*dC_(1,1))) / dC_(2,2);
  double ymax = (dC_(1,2) - sqrt(dC_(1,2)*dC_(1,2)-dC_(2,2)*dC_(1,1))) / dC_(2,2);

  if (H) {
    Eigen::Matrix<double, 4,9> db_dC = Matrix::Zero(4,9);
    double f = sqrt(dC_(0,2)*dC_(0,2)-dC_(0,0)*dC_(2,2));
    double g = sqrt(dC_(1,2)*dC_(1,2)-dC_(1,1)*dC_(2,2));
    db_dC(0,0) = 1.0/f*(-1.0/2.0);
    db_dC(0,6) = (dC_(0,2)*1.0/f+1.0)/dC_(2,2);
    db_dC(0,8) = -1.0/(dC_(2,2)*dC_(2,2))*(dC_(0,2)+f)-(dC_(0,0)*1.0/f*(1.0/2.0))/dC_(2,2);
    db_dC(1,4) = 1.0/g*(-1.0/2.0);
    db_dC(1,7) = (dC_(1,2)*1.0/g+1.0)/dC_(2,2);
    db_dC(1,8) = -1.0/(dC_(2,2)*dC_(2,2))*(dC_(1,2)+g)-(dC_(1,1)*1.0/g*(1.0/2.0))/dC_(2,2);
    db_dC(2,0) = 1.0/f*(1.0/2.0);
    db_dC(2,6) = -(dC_(0,2)*1.0/f-1.0)/dC_(2,2);
    db_dC(2,8) = -1.0/(dC_(2,2)*dC_(2,2))*(dC_(0,2)-f)+(dC_(0,0)*1.0/f*(1.0/2.0))/dC_(2,2);
    db_dC(3,4) = 1.0/g*(1.0/2.0);
    db_dC(3,7) = -(dC_(1,2)*1.0/g-1.0)/dC_(2,2);
    db_dC(3,8) = -1.0/(dC_(2,2)*dC_(2,2))*(dC_(1,2)-g)+(dC_(1,1)*1.0/g*(1.0/2.0))/dC_(2,2);
    *H = db_dC;
  }

  return AlignedBox2(xmin, ymin, xmax, ymax);
}

/* ************************************************************************* */
bool DualConic::isDegenerate(void) const {
  Matrix33 C = dC_.inverse();
  if (C.determinant() == 0.0 || C.array().isInf().any() || C.array().isNaN().any()) {
    return true;
  }
  return false;
}

/* ************************************************************************* */
bool DualConic::isEllipse(void) const {
  Matrix33 C = dC_.inverse();
  C = C/C(2,2);
  bool isDegenerate = C.determinant() == 0.0;
  if (!isDegenerate) {
    Matrix22 A33 = C.block(0,0,2,2);
    return (A33.determinant() > 0);
  }
  return false;
}

/* ************************************************************************* */
void DualConic::print(const string& s) const {
  cout << s << " : \n" << dC_ << endl;
}

/* ************************************************************************* */
bool DualConic::equals(const DualConic& other, double tol) const {
  return this->normalize().matrix().isApprox(other.normalize().matrix(), tol);
}
    
} // namespace gtsam

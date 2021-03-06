#include "curve.h"
#include "vertexrecorder.h"
using namespace std;

const float c_pi = 3.14159265358979323846f;

namespace {
// Approximately equal to.  We don't want to use == because of
// precision issues with floating point.
inline bool approx(const Vector3f &lhs, const Vector3f &rhs) {
  const float eps = 1e-8f;
  return (lhs - rhs).absSquared() < eps;
}

const Matrix4f bez_mat{
    1, -3, 3, -1,
    0, 3, -6, 3,
    0, 0, 3, -3,
    0, 0, 0, 1
};

const auto bez_mat_inverse = bez_mat.inverse();

const auto bsp_mat = 1.0f / 6.0f * Matrix4f{
    1, -3, 3, -1,
    4, 0, -6, 3,
    1, 3, 3, -3,
    0, 0, 0, 1
};

const Matrix4f derivative_mat{
    0, 0, 0, 0,
    1, 0, 0, 0,
    0, 2, 0, 0,
    0, 0, 3, 0
};

}

Curve evalBezier(const vector<Vector3f> &P, unsigned steps) {
  // Check
  if (P.size() < 4 || P.size() % 3 != 1) {
    cerr << "evalBezier must be called with 3n+1 control points." << endl;
    exit(0);
  }

  // TODO:
  // You should implement this function so that it returns a Curve
  // (e.g., a vector< CurvePoint >).  The variable "steps" tells you
  // the number of points to generate on each piece of the spline.
  // At least, that's how the sample solution is implemented and how
  // the SWP files are written.  But you are free to interpret this
  // variable however you want, so long as you can control the
  // "resolution" of the discretized spline curve with it.

  // Make sure that this function computes all the appropriate
  // Vector3fs for each CurvePoint: V,T,N,B.
  // [NBT] should be unit and orthogonal.

  // Also note that you may assume that all Bezier curves that you
  // receive have G1 continuity.  Otherwise, the TNB will not be
  // be defined at points where this does not hold.

  cerr << "\t>>> evalBezier has been called with the following input:" << endl;

  cerr << "\t>>> Control points (type vector< Vector3f >): " << endl;
  for (int i = 0; i < (int)P.size(); ++i) {
    cerr << "\t>>> " << P[i] << endl;
  }

  cerr << "\t>>> Steps (type steps): " << steps << endl;
//  cerr << "\t>>> Returning empty curve." << endl;

  Curve bez_curve{};

  Vector3f N{};
  Vector3f B{};
  auto n_ctr_points = P.size();
  for (auto piece = 0; piece + 3 < n_ctr_points; piece += 3) {
    Matrix4f geometry_mat{};
    for (int i = 0; i < 4; i++) {
      // Since we don't have Matrix3x4.
      geometry_mat.setCol(i, Vector4f{P[piece + i], 0});
    }

    auto trans_mat = geometry_mat * bez_mat;
    // Make sure to draw the first curve point.
    unsigned start_step = piece == 0 ? 0 : 1;
    for (unsigned step = start_step; step <= steps; ++step) {
      auto t = float(step) / float(steps);
      Vector4f basis_vec{1.f, t, t * t, t * t * t};
      Vector4f basis_derivative_vec = derivative_mat * basis_vec;

      auto V_tmp = trans_mat * basis_vec;
      auto T_tmp = trans_mat * basis_derivative_vec;

      Vector3f V{V_tmp[0], V_tmp[1], V_tmp[2]};
      Vector3f T{T_tmp[0], T_tmp[1], T_tmp[2]};
      T = T.normalized();

      if (piece == 0 && step == 0) {
        // To make sure that when the curve is on xy plane, the N will also be on xy plane.
        B = Vector3f{0.f, 0.f, 1.f};
        if (T == B) {
          B = Vector3f{0.f, 1.f, 0.f};
        }
      }

      N = Vector3f::cross(B, T).normalized();
      B = Vector3f::cross(T, N).normalized();

      bez_curve.emplace_back(CurvePoint{V, T, N, B});
    }
  }

  return bez_curve;
}

Curve evalBspline(const vector<Vector3f> &P, unsigned steps) {
  // Check
  if (P.size() < 4) {
    cerr << "evalBspline must be called with 4 or more control points." << endl;
    exit(0);
  }

  // TODO:
  // It is suggested that you implement this function by changing
  // basis from B-spline to Bezier.  That way, you can just call
  // your evalBezier function.

  cerr << "\t>>> evalBSpline has been called with the following input:" << endl;

  cerr << "\t>>> Control points (type vector< Vector3f >): " << endl;
  for (int i = 0; i < (int)P.size(); ++i) {
    cerr << "\t>>> " << P[i] << endl;
  }

  cerr << "\t>>> Steps (type steps): " << steps << endl;
//  cerr << "\t>>> Returning empty curve." << endl;

  auto n_ctr_points = P.size();
  vector<Vector3f> bez_ctrl_points;
  for (auto piece = 0; piece + 3 < n_ctr_points; ++piece) {
    Matrix4f geometry_mat{};
    for (int i = 0; i < 4; i++) {
      // Since we don't have Matrix3x4.
      geometry_mat.setCol(i, Vector4f{P[piece + i], 0});
    }
    geometry_mat = geometry_mat * bsp_mat * bez_mat_inverse;
    if (piece == 0) {
      Vector3f ctrl_point
          {geometry_mat(0, 0), geometry_mat(1, 0), geometry_mat(2, 0)};
      bez_ctrl_points.emplace_back(ctrl_point);
    }
    for (int i = 1; i < 4; i++) {
      Vector3f ctrl_point
          {geometry_mat(0, i), geometry_mat(1, i), geometry_mat(2, i)};
      bez_ctrl_points.emplace_back(ctrl_point);
    }
  }

  return evalBezier(bez_ctrl_points, steps);
}

Curve evalCircle(float radius, unsigned steps) {
  // This is a sample function on how to properly initialize a Curve
  // (which is a vector< CurvePoint >).

  // Preallocate a curve with steps+1 CurvePoints
  Curve R(steps + 1);

  // Fill it in counterclockwise
  for (unsigned i = 0; i <= steps; ++i) {
    // step from 0 to 2pi
    float t = 2.0f * c_pi * float(i) / steps;

    // Initialize position
    // We're pivoting counterclockwise around the y-axis
    R[i].V = radius * Vector3f(cos(t), sin(t), 0);

    // Tangent vector is first derivative
    R[i].T = Vector3f(-sin(t), cos(t), 0);

    // Normal vector is second derivative
    R[i].N = Vector3f(-cos(t), -sin(t), 0);

    // Finally, binormal is facing up.
    R[i].B = Vector3f(0, 0, 1);
  }

  return R;
}

void recordCurve(const Curve &curve, VertexRecorder *recorder) {
  const Vector3f WHITE(1, 1, 1);
  for (int i = 0; i < (int)curve.size() - 1; ++i) {
    recorder->record_poscolor(curve[i].V, WHITE);
    recorder->record_poscolor(curve[i + 1].V, WHITE);
  }
}
void recordCurveFrames(const Curve &curve,
                       VertexRecorder *recorder,
                       float framesize) {
  Matrix4f T;
  const Vector3f RED(1, 0, 0);
  const Vector3f GREEN(0, 1, 0);
  const Vector3f BLUE(0, 0, 1);

  const Vector4f ORGN(0, 0, 0, 1);
  const Vector4f AXISX(framesize, 0, 0, 1);
  const Vector4f AXISY(0, framesize, 0, 1);
  const Vector4f AXISZ(0, 0, framesize, 1);

  for (int i = 0; i < (int)curve.size(); ++i) {
    T.setCol(0, Vector4f(curve[i].N, 0));
    T.setCol(1, Vector4f(curve[i].B, 0));
    T.setCol(2, Vector4f(curve[i].T, 0));
    T.setCol(3, Vector4f(curve[i].V, 1));

    // Transform orthogonal frames into model space
    Vector4f MORGN = T * ORGN;
    Vector4f MAXISX = T * AXISX;
    Vector4f MAXISY = T * AXISY;
    Vector4f MAXISZ = T * AXISZ;

    // Record in model space
    recorder->record_poscolor(MORGN.xyz(), RED);
    recorder->record_poscolor(MAXISX.xyz(), RED);

    recorder->record_poscolor(MORGN.xyz(), GREEN);
    recorder->record_poscolor(MAXISY.xyz(), GREEN);

    recorder->record_poscolor(MORGN.xyz(), BLUE);
    recorder->record_poscolor(MAXISZ.xyz(), BLUE);
  }
}


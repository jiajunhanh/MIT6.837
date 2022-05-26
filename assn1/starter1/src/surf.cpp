#include "surf.h"
#include "vertexrecorder.h"
using namespace std;

namespace {

// We're only implenting swept surfaces where the profile curve is
// flat on the xy-plane.  This is a check function.
static bool checkFlat(const Curve &profile) {
  for (unsigned i = 0; i < profile.size(); i++)
    if (profile[i].V[2] != 0.0 ||
        profile[i].T[2] != 0.0 ||
        profile[i].N[2] != 0.0)
      return false;

  return true;
}
}

// DEBUG HELPER
Surface quad() {
  Surface ret;
  ret.VV.push_back(Vector3f(-1, -1, 0));
  ret.VV.push_back(Vector3f(+1, -1, 0));
  ret.VV.push_back(Vector3f(+1, +1, 0));
  ret.VV.push_back(Vector3f(-1, +1, 0));

  ret.VN.push_back(Vector3f(0, 0, 1));
  ret.VN.push_back(Vector3f(0, 0, 1));
  ret.VN.push_back(Vector3f(0, 0, 1));
  ret.VN.push_back(Vector3f(0, 0, 1));

  ret.VF.push_back(Tup3u(0, 1, 2));
  ret.VF.push_back(Tup3u(0, 2, 3));
  return ret;
}

Surface makeSurfRev(const Curve &profile, unsigned steps) {
  Surface surface;
  if (profile.empty()) return surface;

  if (!checkFlat(profile)) {
    cerr << "surfRev profile curve must be flat on xy plane." << endl;
    exit(0);
  }

  auto n_points = unsigned(profile.size());
  for (unsigned step = 0; step < steps; ++step) {
    auto m_v = Matrix3f::rotateY(float(M_PI * 2.0 * step / steps));
    auto m_n = m_v.inverse().transposed();
    for (unsigned i = 0; i < n_points; ++i) {
      surface.VV.push_back(m_v * profile[i].V);
      surface.VN.push_back(-1.0 * m_n * profile[i].N);
    }
  }

  for (unsigned step = 0; step < steps; ++step) {
    auto offset1 = step ? (step - 1) * n_points : (steps - 1) * n_points;
    auto offset2 = step * n_points;
    for (unsigned i = 0; i + 1 < n_points; ++i) {
      surface.VF.emplace_back(offset1 + i, offset2 + i + 1, offset2 + i);
      surface.VF.emplace_back(offset1 + i, offset1 + i + 1, offset2 + i + 1);
    }
  }
  return surface;
}

Surface makeGenCyl(const Curve &profile, const Curve &sweep) {
  Surface surface;
  surface = quad();

  if (!checkFlat(profile)) {
    cerr << "genCyl profile curve must be flat on xy plane." << endl;
    exit(0);
  }

  // TODO: Here you should build the surface.  See surf.h for details.

  cerr
      << "\t>>> makeGenCyl called (but not implemented).\n\t>>> Returning empty surface."
      << endl;

  return surface;
}

void recordSurface(const Surface &surface, VertexRecorder *recorder) {
  const Vector3f WIRECOLOR(0.4f, 0.4f, 0.4f);
  for (int i = 0; i < (int)surface.VF.size(); i++) {
    recorder->record(surface.VV[surface.VF[i][0]],
                     surface.VN[surface.VF[i][0]],
                     WIRECOLOR);
    recorder->record(surface.VV[surface.VF[i][1]],
                     surface.VN[surface.VF[i][1]],
                     WIRECOLOR);
    recorder->record(surface.VV[surface.VF[i][2]],
                     surface.VN[surface.VF[i][2]],
                     WIRECOLOR);
  }
}

void recordNormals(const Surface &surface,
                   VertexRecorder *recorder,
                   float len) {
  const Vector3f NORMALCOLOR(0, 1, 1);
  for (int i = 0; i < (int)surface.VV.size(); i++) {
    recorder->record_poscolor(surface.VV[i], NORMALCOLOR);
    recorder->record_poscolor(surface.VV[i] + surface.VN[i] * len, NORMALCOLOR);
  }
}

void outputObjFile(ostream &out, const Surface &surface) {

  for (int i = 0; i < (int)surface.VV.size(); i++)
    out << "v  "
        << surface.VV[i][0] << " "
        << surface.VV[i][1] << " "
        << surface.VV[i][2] << endl;

  for (int i = 0; i < (int)surface.VN.size(); i++)
    out << "vn "
        << surface.VN[i][0] << " "
        << surface.VN[i][1] << " "
        << surface.VN[i][2] << endl;

  out << "vt  0 0 0" << endl;

  for (int i = 0; i < (int)surface.VF.size(); i++) {
    out << "f  ";
    for (unsigned j = 0; j < 3; j++) {
      unsigned a = surface.VF[i][j] + 1;
      out << a << "/" << "1" << "/" << a << " ";
    }
    out << endl;
  }
}

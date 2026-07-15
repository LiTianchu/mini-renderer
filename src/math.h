#ifndef __MATH_H__
#define __MATH_H__
#include "geometry.h"
#include "he_model.h"

class Math {
public:
  static Vec3f get_barycentric(const Vec3f *pts, const Vec2i &P) {
    //(ABx ACx PAx) X (ABy ACy PAy)
    Vec3f cross_product =
        Vec3f(pts[1].x - pts[0].x, pts[2].x - pts[0].x, pts[0].x - P.x)
            .cross(Vec3f(pts[1].y - pts[0].y, pts[2].y - pts[0].y,
                         pts[0].y - P.y)); // obtain (u*z, v*z, 1*z)

    if (cross_product.z ==
        0) // if z is 0, it is a line, return a vector with negative number
      return Vec3f(-1, 1, 1);

    Vec3f c = Vec3f(cross_product.x / cross_product.z,
                    cross_product.y / cross_product.z, 1.0); // obtain (u,v,1)
    if (c.x < 0 || c.y < 0 || c.x + c.y > 1)
      return Vec3f(-1, 1, 1); // if u or v is negative or their sum greater than
                              // 1, it is outside of triangle, return (-1,1,1)
    return Vec3f(1 - c.x - c.y, c.x,
                 c.y); // calculate and return the alpha, beta and gamma
  }

  static Vec3f get_face_normal(const std::vector<Vertex> &vertices) {
    Vec3f face_normal = (vertices[0].pos - vertices[2].pos)
                            .cross(vertices[1].pos - vertices[2].pos);
    face_normal.normalize();
    return face_normal;
  }

  template <typename t> static Vec3<t> get_homogeneous(const Vec2<t> &v) {
    return Vec3<t>(v.x, v.y, 1.0f);
  }

  template <typename t> static Vec4<t> get_homogeneous(const Vec3<t> &v) {
    return Vec4<t>(v.x, v.y, v.z, 1.0f);
  }

  template <typename t> static Vec2<t> get_cartesian(const Vec3<t> &v) {
    if (v.z == 0)
      return Vec2<t>(v.x, v.y);
    return Vec2<t>(v.x / v.z, v.y / v.z);
  }

  template <typename t> static Vec3<t> get_cartesian(const Vec4<t> &v) {
    if (v.w == 0)
      return Vec3<t>(v.x, v.y, v.z);
    return Vec3<t>(v.x / v.w, v.y / v.w, v.z / v.w);
  }

  template <typename t> static Vec3<t> to_Vec3(const Matrix &m) {
    assert(m.nrows() >= 3 && m.ncols() == 1);
    return Vec3<t>(m[0][0], m[1][0], m[2][0]);
  }

  template <typename t> static Vec4<t> to_Vec4(const Matrix &m) {
    assert(m.nrows() == 4 && m.ncols() == 1);
    return Vec4<t>(m[0][0], m[1][0], m[2][0], m[3][0]);
  }

  template <typename t> static Matrix to_Matrix3(const Vec3<t> &v) {
    Matrix m(3, 1);
    m[0][0] = v.x;
    m[1][0] = v.y;
    m[2][0] = v.z;
    return m;
  }

  template <typename t>
  static Matrix to_Matrix4(const Vec3<t> &v, float w = 1.0f) {
    Matrix m(4, 1);
    m[0][0] = v.x;
    m[1][0] = v.y;
    m[2][0] = v.z;
    m[3][0] = w;
    return m;
  }

  template <typename t> static Matrix to_Matrix4(const Vec4<t> &v) {
    Matrix m(4, 1);
    m[0][0] = v.x;
    m[1][0] = v.y;
    m[2][0] = v.z;
    m[3][0] = v.w;
    return m;
  }

  static float max_elevation_angle(float *zbuffer, Vec2f p, Vec2f dir,
                                   float reach, int width, int height) {
    float maxangle = 0;
    for (float t = 0.; t < reach; t += .1) {
      Vec2f cur_pos = p + dir * t;
      // check if the point is inside the screen
      if (cur_pos.x >= width || cur_pos.y >= height || cur_pos.x < 0 ||
          cur_pos.y < 0)
        return maxangle;
      float distance =
          (p - cur_pos).norm(); // magnitude distance to the initial point
      if (distance < 1.0f)
        continue; // check if the distance is at least 1 pixel

      // calclate elevation angle
      int cur_z_idx = int(cur_pos.x) + int(cur_pos.y) * width;
      int p_z_idx = int(p.x) + int(p.y) * width;
      float elevation = zbuffer[cur_z_idx] - zbuffer[p_z_idx];

      maxangle = std::max(maxangle, atanf(elevation / distance));
    }
    return maxangle;
  }
};
#endif

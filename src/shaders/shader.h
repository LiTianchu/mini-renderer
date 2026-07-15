#ifndef __SHADER_H__
#define __SHADER_H__

#include "../geometry.h"
#include "../he_model.h"
#include "../math.h"
#include "../tgaimage.h"
#include <algorithm>
#include <vector>

struct Shader_Global_Payload {
  Vec3f main_light_dir;
  float main_light_intensity;
  Vec3f camera_pos;
  HEModel *model;
  Matrix transform_matrix;
  Matrix view_proj_mat;
  Matrix proj_correction_mat;
  Matrix model_mat;
  Matrix view_mat;
  Matrix projection_mat;
  Matrix viewport_mat;
  Matrix screen_shadow_mat;
  TGAImage *shadow_buffer;
  float zDepth;
};

struct V2F {
  Vec3f pos;
  Vec3f norm;
  Vec3f face_norm;
  Vec2f tex_coord;
  Vec3i base_color;
  V2F() {}
  V2F(Vec3f pos_, Vec3f norm_, Vec2f tex_coord_, Vec3i color_)
      : pos(pos_), norm(norm_), tex_coord(tex_coord_), base_color(color_) {}
};

class Shader {
public:
  Shader_Global_Payload global_payload;
  Shader() {}

  Shader(Shader_Global_Payload &payload) { global_payload = payload; }

  virtual ~Shader() {}

  virtual V2F vertex_shader(const Vertex &vertex_input) = 0;
  virtual Vec3i fragment_shader(const V2F &processed_vertex) = 0;
};
#endif

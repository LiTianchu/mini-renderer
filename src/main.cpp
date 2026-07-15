#include "engine.h"
#include "he_model.h"
#include "model.h"
#include "shaders/depth_shader.cpp"
#include "shaders/diffuse_map_shader.cpp"
#include "shaders/empty_shader.cpp"
#include "shaders/flat_shader.cpp"
#include "shaders/gouraud_shader.cpp"
#include "shaders/normal_map_shader.cpp"
#include "shaders/shader.h"
#include "shaders/uv_shader.cpp"
#include "tgaimage.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdio.h>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);
Vec3f camera_pos = Vec3f(1, 1, 3);
Vec3f light_dir = Vec3f(1, 1, 1);
// Vec3f light_pos = Vec3f(7, 2, 0);
float main_light_intensity = 2.0f;
Vec3f center_pos = Vec3f(0, 0, 0);
Vec3f up_dir = Vec3f(0, 1, 0);
float far = 1;
float near = 0;
int img_w = 600;
int img_h = 600;

enum Mode { WIREFRAME, FLAT, SMOOTH, TEXTURE, UV };

int main(int argc, char **argv) {
  TGAImage image(img_w, img_h, TGAImage::RGB);

  float *zbuffer = new float[image.get_width() * image.get_height()];

  if (argc >= 2) {
    std::string mode = argv[1];
    std::string model_key = "head";
    if (argc >= 3) {
      model_key = argv[2];
    }

    // Optional runtime tuning (does not require rebuild):
    //   --intensity <float>   main light intensity
    //   --yaw <degrees>       rotate model around vertical/Y axis
    float runtime_light_intensity = main_light_intensity;
    float runtime_yaw_deg = 0.0f;
    for (int i = 3; i < argc; i++) {
      const std::string arg = argv[i];
      if (arg == "--intensity" && i + 1 < argc) {
        runtime_light_intensity = std::stof(argv[++i]);
      } else if (arg == "--yaw" && i + 1 < argc) {
        runtime_yaw_deg = std::stof(argv[++i]);
      }
    }

    std::string obj_path = "obj/african_head/african_head.obj";
    std::string diffuse_path = "obj/african_head/african_head_diffuse.tga";
    std::string normal_path = "obj/african_head/african_head_nm.tga";

    if (model_key == "head" || model_key == "african_head") {
      // defaults already set
    } else if (model_key == "diablo" || model_key == "diablo3" ||
               model_key == "diablo3_pose") {
      obj_path = "obj/diablo3_pose/diablo3_pose.obj";
      diffuse_path = "obj/diablo3_pose/diablo3_pose_diffuse.tga";
      normal_path = "obj/diablo3_pose/diablo3_pose_nm.tga";
    }

    HEModel he_model_loaded = HEModel(obj_path.c_str());
    TGAImage diffuse_texture = TGAImage();
    TGAImage normal_map = TGAImage();

    diffuse_texture.read_tga_file(diffuse_path.c_str());
    normal_map.read_tga_file(normal_path.c_str());
    he_model_loaded.set_diffuse_texture(&diffuse_texture);
    he_model_loaded.set_normal_map_texture(&normal_map);

    std::cout << "Rendering mode='" << mode << "' model='" << model_key << "'"
              << " intensity=" << runtime_light_intensity
              << " yawDeg=" << runtime_yaw_deg << std::endl;

    Engine engine = Engine(light_dir.normalize(), runtime_light_intensity,
                           camera_pos, zbuffer);

    Shader_Global_Payload shader_payload = Shader_Global_Payload();
    shader_payload.main_light_dir = light_dir.normalize();
    shader_payload.main_light_intensity = runtime_light_intensity;
    shader_payload.camera_pos = camera_pos;
    shader_payload.model = &he_model_loaded;
    shader_payload.zDepth = far - near;

    // transformation matrices
    const float yaw_rad = runtime_yaw_deg * (float)M_PI / 180.0f;
    Matrix rotation_matrix = Matrix::rotation(0, yaw_rad, 0);
    Matrix translation_matrix = Matrix::translation(0, 0, 0);
    Matrix scaling_matrix = Matrix::scaling(1, 1, 1);
    Matrix composite_linear_transform_mat =
        translation_matrix * rotation_matrix * scaling_matrix;
    Matrix view_matrix = Matrix::model_view(camera_pos, center_pos, up_dir);
    Matrix projection_matrix = Matrix::persp_projection(camera_pos);
    Matrix viewport_matrix = Matrix::viewport(0, 0, image.get_width(),
                                              image.get_height(), near, far);

    // store transform matrices for shader
    Matrix transform_matrix = viewport_matrix * projection_matrix *
                              view_matrix * composite_linear_transform_mat;
    shader_payload.transform_matrix = transform_matrix;
    shader_payload.view_proj_mat = projection_matrix * view_matrix;
    shader_payload.proj_correction_mat =
        shader_payload.view_proj_mat.inverse_transpose().first;
    shader_payload.model_mat = composite_linear_transform_mat;
    shader_payload.view_mat = view_matrix;
    shader_payload.projection_mat = projection_matrix;
    shader_payload.viewport_mat = viewport_matrix;

    // he_model_loaded.qem_simplify(200);
    // HEModel he_model_loaded = HEModel("obj/diablo3_pose/diablo3_pose.obj");
    if (mode == "wireframe") {
      engine.render_model_wireframe(he_model_loaded, &image);
    } else if (mode == "flat") {
      engine.render_shaded_model(he_model_loaded,
                                 new Flat_Shader(shader_payload), &image);
    } else if (mode == "smooth") {
      engine.render_shaded_model(he_model_loaded,
                                 new Gouraud_Shader(shader_payload), &image);
    } else if (mode == "texture") {
      // texture.read_tga_file("obj/diablo3_pose/diablo3_pose_diffuse.tga");
      engine.render_shaded_model(
          he_model_loaded, new Diffuse_Map_Shader(shader_payload), &image);
    }
    // else if (std::string(argv[1]) == "shaded-wireframe")
    // {
    //     engine.render_shaded_model(he_model_loaded, new
    //     Flat_Shader(shader_payload), &image);
    //     engine.render_model_wireframe(he_model_loaded, &image);
    // }
    else if (mode == "uv") {
      engine.render_shaded_model(he_model_loaded, new UV_Shader(shader_payload),
                                 &image);
    } else if (mode == "normal") {
      engine.render_shaded_model(he_model_loaded,
                                 new Normal_Map_Shader(shader_payload), &image);
    } else if (mode == "depth") {
      engine.render_shaded_model(he_model_loaded,
                                 new Depth_Shader(shader_payload), &image);
    } else if (mode == "shadowmap") {
      // shadow mapping

      engine.render_shaded_model(he_model_loaded,
                                 new Normal_Map_Shader(shader_payload), &image);
      engine.reset_zbuffer(img_w, img_h);
      // shadow mapping first pass
      Shader_Global_Payload shadow_payload = Shader_Global_Payload();
      Matrix shadow_view = Matrix::model_view(
          light_dir, center_pos,
          up_dir); // take the depth buffer from the light position
      // Matrix shadow_proj = Matrix::persp_projection(light_dir);
      Matrix shadow_viewport = Matrix::viewport(0, 0, img_w, img_h, near, far);
      Matrix shadow_transform = shadow_viewport * shadow_view;
      shadow_payload.transform_matrix = shadow_transform;
      shadow_payload.zDepth = far - near;
      // render shadow buffer
      TGAImage shadow_buffer =
          TGAImage(image.get_width(), image.get_height(), TGAImage::RGB);
      engine.render_shaded_model(
          he_model_loaded, new Depth_Shader(shadow_payload), &shadow_buffer);
      engine.reset_zbuffer(img_w, img_h);

      // shadow mapping second pass
      Matrix screen_shadow_mat =
          shadow_transform * shader_payload.transform_matrix.inverse().first;
      shader_payload.screen_shadow_mat = screen_shadow_mat;
      shader_payload.shadow_buffer = &shadow_buffer;
      engine.render_shaded_model(he_model_loaded,
                                 new Normal_Map_Shader(shader_payload), &image);
      // image = shadow_buffer;
    } else if (mode == "complete") {
      // complete = normal mapping + shadow mapping + SSAO (post-pass)
      // 1) build shadow buffer (depth from light view)
      Shader_Global_Payload shadow_payload = Shader_Global_Payload();
      Matrix shadow_view = Matrix::model_view(light_dir, center_pos, up_dir);
      Matrix shadow_viewport = Matrix::viewport(0, 0, img_w, img_h, near, far);
      Matrix shadow_transform = shadow_viewport * shadow_view;
      shadow_payload.transform_matrix = shadow_transform;
      shadow_payload.zDepth = far - near;

      TGAImage shadow_buffer =
          TGAImage(image.get_width(), image.get_height(), TGAImage::RGB);
      engine.render_shaded_model(
          he_model_loaded, new Depth_Shader(shadow_payload), &shadow_buffer);
      engine.reset_zbuffer(img_w, img_h);

      // 2) render lit model with shadow mapping enabled
      Matrix screen_shadow_mat =
          shadow_transform * shader_payload.transform_matrix.inverse().first;
      shader_payload.screen_shadow_mat = screen_shadow_mat;
      shader_payload.shadow_buffer = &shadow_buffer;
      engine.render_shaded_model(he_model_loaded,
                                 new Normal_Map_Shader(shader_payload), &image);

      // 3) SSAO post-pass: compute AO from current zbuffer and multiply the
      // shaded image
      float *current_zbuffer = engine.get_engine_data().z_buffer;
      const int max_count = img_h * img_w;
      int count = 0;
      for (int y = 0; y < img_h; y++) {
        if (y % 50 == 0) {
          std::cout << "Complete SSAO: row " << y << "/" << img_h << std::endl;
        }
        for (int x = 0; x < img_w; x++) {
          count++;
          if (current_zbuffer[x + y * img_w] <
              std::numeric_limits<float>::epsilon())
            continue;

          float total = 0;
          for (float a = 0; a < M_PI * 2 - 1e-4; a += M_PI / 4) {
            total += M_PI / 2 - Math::max_elevation_angle(current_zbuffer,
                                                          Vec2f(x, y),
                                                          Vec2f(cos(a), sin(a)),
                                                          1000.f, img_w, img_h);
          }
          total /= (M_PI / 2) * 8;
          total = pow(total, 100.f);
          total = std::clamp(total, 0.f, 1.f);

          TGAColor c = image.get(x, y);
          int r = (int)std::round((float)c.r * total);
          int g = (int)std::round((float)c.g * total);
          int b = (int)std::round((float)c.b * total);
          r = std::clamp(r, 0, 255);
          g = std::clamp(g, 0, 255);
          b = std::clamp(b, 0, 255);
          image.set(x, y,
                    TGAColor((unsigned char)r, (unsigned char)g,
                             (unsigned char)b, 255));
        }
      }
      std::cout << "Complete SSAO: " << count << "/" << max_count << std::endl;
    } else if (mode == "ssao") {
      // screen space ambient occlusion
      engine.render_shaded_model(he_model_loaded,
                                 new Empty_Shader(shader_payload), &image);
      // keep the z buffer

      float *current_zbuffer = engine.get_engine_data().z_buffer;

      int count = 0;
      int max_count = img_h * img_w;
      // generate ssao
      for (int y = 0; y < img_h; y++) {
        if (y % 50 == 0) {
          std::cout << "SSAO: row " << y << "/" << img_h << std::endl;
        }
        for (int x = 0; x < img_w; x++) {
          count++;
          if (current_zbuffer[x + y * img_w] <
              std::numeric_limits<float>::epsilon())
            continue;
          float total = 0;
          // sample 8 directions, calculate the elevation angle to the first hit
          // point in each direction, and average them to get the occlusion
          // factor
          for (float a = 0; a < M_PI * 2 - 1e-4; a += M_PI / 4) {
            total += M_PI / 2 - Math::max_elevation_angle(current_zbuffer,
                                                          Vec2f(x, y),
                                                          Vec2f(cos(a), sin(a)),
                                                          1000.f, img_w, img_h);
          }
          total /= (M_PI / 2) * 8;
          total = pow(total, 100.f);
          total = std::clamp(total, 0.f, 1.f);
          image.set(x, y, TGAColor(total * 255, total * 255, total * 255, 255));
        }
      }
      std::cout << "SSAO: " << count << "/" << max_count << std::endl;
    } else {
      std::cout << "Unknown mode: '" << mode << "'" << std::endl;
      std::cout << "Valid modes: wireframe, flat, smooth, texture, uv, normal, "
                   "depth, shadowmap, ssao, complete"
                << std::endl;
      std::cout
          << "Optional model key: head|african_head (default), diablo3_pose"
          << std::endl;
    }
  } else {
    std::cout << "Usage: ./minirenderer <mode> [model]" << std::endl;
    std::cout << "  mode: "
                 "wireframe|flat|smooth|texture|uv|normal|depth|shadowmap|ssao|"
                 "complete"
              << std::endl;
    std::cout << "  model (optional): head|african_head (default), diablo3_pose"
              << std::endl;
  }
  std::cout << "Writing rendered output..." << std::endl;

  image.flip_vertically();
  image.write_tga_file("output.tga");
  std::cout << "Done!" << std::endl;
  return 0;
}

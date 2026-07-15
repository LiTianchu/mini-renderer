#include "engine.h"
#include "math.h"
#include <iostream>

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);

void Engine::render_shaded_model(HEModel model, Shader *shader,
                                 TGAImage *frame_buffer) {
  // TGAImage frame_buffer = TGAImage(img_w, img_h, TGAImage::RGB);

  // for each face in the model
  for (std::set<Face *>::iterator face_itr = model.faces_begin();
       face_itr != model.faces_end(); ++face_itr) {
    HEdge *h_edge = (*face_itr)->h; // optain the first half edge of the face
    // initialize the arrays to store vertex data
    std::vector<Vec3f> screen_coords;
    std::vector<V2F> processed_vertices;
    std::vector<Vertex> tri;
    int j = 0;

    do {
      Vertex *v = h_edge->v;
      tri.push_back(*v);

      V2F processed_v = shader->vertex_shader(*v);

      processed_vertices.push_back(processed_v);
      h_edge = h_edge->next;
      j++;
    } while (h_edge != (*face_itr)->h);

    Vec3f face_normal = Math::get_face_normal(tri).normalize();
    for (int i = 0; i < processed_vertices.size(); i++) {
      processed_vertices[i].face_norm = face_normal;
    }

    rasterize_triangle(processed_vertices, shader, frame_buffer);
  }
  // return frame_buffer;
}

void Engine::render_model_wireframe(HEModel model, TGAImage *frame_buffer) {
  // TGAImage frame_buffer = TGAImage(img_h, img_w, TGAImage::RGB);
  bool faces_visited[model.num_of_faces()] = {false};
  Engine::wireframe_dfs(**model.faces_begin(), faces_visited, frame_buffer);
  // return frame_buffer;
}

void Engine::wireframe_dfs(const Face &f, bool (&faces_visited)[],
                           TGAImage *frame_buffer) {
  if (faces_visited[f.index]) {
    return;
  }
  // draw the face
  HEdge *h_edge = f.h;
  std::vector<Face> neighbor_faces;
  do {
    HEdge *prev_e = h_edge->prev;
    HEdge *next_e = h_edge->next;
    Vertex *vertex_start = prev_e->v;
    Vertex *vertex_end = h_edge->v;
    Vec3 pos_start = vertex_start->pos;
    Vec3 pos_end = vertex_end->pos;

    HEdge *pair = h_edge->pair;
    if (pair != NULL) {
      Face neighbor_face = *pair->f;
      neighbor_faces.push_back(neighbor_face);
    }

    // map the world coordinates to image coordinates
    int x0 = (pos_start.x + 1.0) / 2.0 * frame_buffer->get_width();
    int y0 = (pos_start.y + 1.0) / 2.0 * frame_buffer->get_height();
    int x1 = (pos_end.x + 1.0) / 2.0 * frame_buffer->get_width();
    int y1 = (pos_end.y + 1.0) / 2.0 * frame_buffer->get_height();

    draw_line(x0, y0, x1, y1, blue, frame_buffer);
    h_edge = h_edge->next;
  } while (h_edge != f.h);

  faces_visited[f.index] = true;

  for (Face &n : neighbor_faces) {
    wireframe_dfs(n, faces_visited, frame_buffer);
  }
}

void Engine::rasterize_triangle(std::vector<V2F> vert_data, Shader *shader,
                                TGAImage *frame_buffer) {
  //  extract the vertex data
  Vec3f pts[] = {vert_data[0].pos, vert_data[1].pos, vert_data[2].pos};
  float z_indices[] = {vert_data[0].pos.z, vert_data[1].pos.z,
                       vert_data[2].pos.z};
  Vec3f norms[] = {vert_data[0].norm, vert_data[1].norm, vert_data[2].norm};
  Vec2f tex_coords[] = {vert_data[0].tex_coord, vert_data[1].tex_coord,
                        vert_data[2].tex_coord};

  // obtain the bounding box cooridnates
  int x_min = std::min(pts[0].x, std::min(pts[1].x, pts[2].x));
  int x_max = std::max(pts[0].x, std::max(pts[1].x, pts[2].x));
  int y_min = std::min(pts[0].y, std::min(pts[1].y, pts[2].y));
  int y_max = std::max(pts[0].y, std::max(pts[1].y, pts[2].y));

  //  clamp the coordinates to prevent exceeding the boundary
  x_min = std::max(0, x_min);
  x_max = std::min(frame_buffer->get_width() - 1, x_max);
  y_min = std::max(0, y_min);
  y_max = std::min(frame_buffer->get_height() - 1, y_max);

  //  foreach pixel in bounding box, put pixel if it is inside triangle
  for (int x = x_min; x <= x_max; x++) {
    for (int y = y_min; y <= y_max; y++) {
      V2F interpolated_v2f;
      Vec3f b_coord = Math::get_barycentric(
          pts, Vec2i(x, y)); // calculate the barycentric coordinates

      if (b_coord.x < 0 || b_coord.y < 0 ||
          b_coord.z < 0) // if any of the barycentric coordinates is negative,
                         // it is outside of triangle
        continue;        // skip if it is outside of triangle

      // calculate the z value
      float interpolated_z = b_coord.x * z_indices[0] +
                             b_coord.y * z_indices[1] +
                             b_coord.z * z_indices[2];

      int zbuffer_index = x + y * frame_buffer->get_width();
      if (interpolated_z >
          z_buffer[zbuffer_index]) // if pass z test, start shading the pixel
      {
        // interpolation stage
        z_buffer[zbuffer_index] = interpolated_z; // update z buffer
        interpolated_v2f.pos = Vec3f(x, y, interpolated_z);

        Vec3 interpolated_norm = {
            b_coord.x * norms[0].x + b_coord.y * norms[1].x +
                b_coord.z * norms[2].x,
            b_coord.x * norms[0].y + b_coord.y * norms[1].y +
                b_coord.z * norms[2].y,
            b_coord.x * norms[0].z + b_coord.y * norms[1].z +
                b_coord.z * norms[2].z};
        interpolated_v2f.norm = interpolated_norm.normalize();

        float interpolated_u = b_coord.x * tex_coords[0].u +
                               b_coord.y * tex_coords[1].u +
                               b_coord.z * tex_coords[2].u;
        float interpolated_v = b_coord.x * tex_coords[0].v +
                               b_coord.y * tex_coords[1].v +
                               b_coord.z * tex_coords[2].v;

        interpolated_v2f.tex_coord = Vec2f(interpolated_u, interpolated_v);
        interpolated_v2f.base_color = TGAColor(255, 255, 255, 255).to_vec3();
        interpolated_v2f.face_norm = vert_data[0].face_norm.normalize();

        // obtain shaded color using shader
        Vec3i color_vector = shader->fragment_shader(interpolated_v2f);
        TGAColor shade_color = TGAColor(color_vector);
        frame_buffer->set(x, y, shade_color);
      }
    }
  }
}

void Engine::draw_line(int x0, int y0, int x1, int y1, TGAColor color,
                       TGAImage *frame_buffer) {
  bool steep = false;
  /*if y is greator than x, the line drawn will have gaps,
  so need to swap the x and y coordinates before drawing*/
  if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
    std::swap(x0, y0);
    std::swap(x1, y1);
    steep = true;
  }

  /*//if x0 greater than x1,
  swap them so that drawing always goes left to right*/
  if (x0 > x1) {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }

  int dx = x1 - x0;
  int dy = y1 - y0;

  // error2 is the distance between the line and the pixel, multiplied by 2
  int derror2 = std::abs(dy) * 2;
  int error2 = 0;

  int y = y0;

  for (int x = x0; x < x1; x++) {
    if (steep) { // the drawing is transposed after swapping, so transpose it
                 // back
      frame_buffer->set(y, x, color);
    } else {
      frame_buffer->set(x, y, color);
    }
    error2 += derror2;
    if (error2 > dx) // check if the error is greater than the distance between
                     // the line and the pixel
    {
      y += (y1 > y0 ? 1
                    : -1); // if y1 above y0, increase by 1, else decrese by 1
      error2 -= dx * 2;
    }
  }
}

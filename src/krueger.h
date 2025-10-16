#ifndef KRUEGER_H
#define KRUEGER_H

typedef struct {
  char *chars;
  u32 num_glyph_x;
  u32 num_glyph_y;
  u32 glyph_width;
  u32 glyph_height;
  Image image;
} Font;

typedef struct {
  Vector3 *vertex_buf;
  u32 *vertex_index_buf;
} Mesh;

struct Krueger_State {
  Arena arena;
  
  Image font_image;
  Font font;

  Mesh mesh;
  f32 mesh_rot_angle;
  f32 mesh_rot_speed;

  Vector3 cam_p;
  Vector3 cam_up;
  Vector3 cam_dir;

  Vector3 cam_vel;
  f32 cam_speed;

  f32 cam_rot_vel;
  f32 cam_rot_speed;
  f32 cam_yaw;
};

#endif // KRUEGER_H

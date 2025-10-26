#ifndef KRUEGER_H
#define KRUEGER_H

typedef struct {
  char *chars;
  u32 num_char_x;
  u32 num_char_y;
  u32 glyph_w;
  u32 glyph_h;
  Image image;
} Font;

typedef struct {
  u32 vertex_count;
  Vector3 *vertices;

  u32 vertex_index_count;
  u32 *vertex_indices;
} Mesh;

typedef struct {
  Arena main_arena;
  Arena temp_arena;
  
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
} Krueger_State;

#endif // KRUEGER_H

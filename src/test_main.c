#define KRUEGER_PLATFORM_GRAPHICS 1

#include "krueger_base.h"
#include "krueger_platform.h"

#include "krueger_base.c"
#include "krueger_platform.c"

#if 0
typedef struct Node Node;
struct Node {
  u32 data;
  Node *next;
  Node *prev;
};

internal void
dll_test(Arena *arena) {
  log_info("[dll test]");
  Temp temp = temp_begin(arena);

  Node *first = 0;
  Node *last = 0;

  Node *rmv_node = 0;
  
  for (u32 i = 0; i < 4; ++i) {
    Node *node = push_array(arena, Node, 1);
    node->data = i;
    if (i == 2) rmv_node = node;
    dll_push_back(first, last, node);
  }

  dll_remove(first, last, rmv_node);

  for (u32 i = 0; i < 4; ++i) {
    Node *node = push_array(arena, Node, 1);
    node->data = i;
    if (i == 2) rmv_node = node;
    dll_push_front(first, last, node);
  }
  
  dll_remove(first, last, rmv_node);

  log_info("forward:");
  for (Node *node = first; node != 0; node = node->next) {
    log_info("%d", node->data);
  }

  log_info("backward:");
  for (Node *node = last; node != 0; node = node->prev) {
    log_info("%d", node->data);
  }

  temp_end(temp);
}

internal void
sll_queue_test(Arena *arena) {
  log_info("[sll queue test]");
  Temp temp = temp_begin(arena);

  Node *first = 0;
  Node *last = 0;

  for (u32 i = 0; i < 4; ++i) {
    Node *node = push_array(arena, Node, 1);
    node->data = i;
    sll_queue_push(first, last, node);
  }
  
  for (Node *node = first; node != 0; node = node->next) {
    log_info("%d", node->data);
    sll_queue_pop(first, last);
  }

  temp_end(temp);
}

internal void
sll_stack_test(Arena *arena) {
  log_info("[sll stack test]");
  Temp temp = temp_begin(arena);
  
  Node *first = 0;
  
  for (u32 i = 0; i < 4; ++i) {
    Node *node = push_array(arena, Node, 1);
    node->data = i;
    sll_stack_push(first, node);
  }

  while (first) {
    log_info("%d", first->data);
    sll_stack_pop(first);
  }

  temp_end(temp);
}

int
main(void) {
  Arena arena = arena_alloc(MB(64));
  dll_test(&arena);
  sll_queue_test(&arena);
  sll_stack_test(&arena);
  return(0);
}

#else

int
main(void) {
  platform_core_init();
  platform_graphics_init();

  u32 window_w = 200;
  u32 window_h = 150;

  u32 buffer_w = window_w;
  u32 buffer_h = window_h;
  
  Platform_Handle windows[3];
  for (u32 i = 0; i < array_count(windows); ++i) {
    windows[i] = platform_window_open(str8_lit("KRUEGER"), (i + 1)*window_w, (i + 1)*window_h);
    platform_window_show(windows[i]);
  }

  uxx buffer_size = buffer_w*buffer_h*sizeof(u32);
  u32 *buffer = platform_reserve(buffer_size);
  platform_commit(buffer, buffer_size);

  for (u32 i = 0; i < buffer_w*buffer_h; ++i) {
    buffer[i] = 0xFFFF00FF;
  }

  Platform_Graphics_Info graphics_info = platform_get_graphics_info();
  log_info("refresh rate: %.2f", graphics_info.refresh_rate);

  Arena event_arena = arena_alloc(MB(64));

  for (b32 quit = false; !quit;) {
    Temp temp = temp_begin(&event_arena);
    Platform_Event_List event_list = platform_get_event_list(temp.arena);
    for (Platform_Event *event = event_list.first; event != 0; event = event->next) {
      switch (event->type) {
        case PLATFORM_EVENT_WINDOW_CLOSE: {
          for (u32 i = 0; i < array_count(windows); ++i) {
            Platform_Handle *window = windows + i;
            if (platform_handle_match(event->window, *window)) {
              if (i == 0) quit = true;
              platform_window_close(*window);
              *window = PLATFORM_HANDLE_NULL;
            }
          }
        } break;
        case PLATFORM_EVENT_KEY_PRESS: {
          switch (event->keycode) {
            case KEY_F11: {
              for (u32 i = 0; i < array_count(windows); ++i) {
                if (platform_handle_match(windows[i], event->window)) {
                  platform_window_toggle_fullscreen(windows[i]);
                }
              }
            } break;
            case KEY_Q: {
              for (u32 i = 0; i < array_count(windows); ++i) {
                if (platform_handle_match(windows[i], event->window)) {
                  if (i == 0) quit = true;
                  platform_window_close(windows[i]);
                }
              }
              break;
            }
          }
        } break;
      }
    }
    temp_end(temp);
    for (u32 i = 0; i < array_count(windows); ++i) {
      platform_window_display_buffer(windows[i], buffer, buffer_w, buffer_h);
    }
  }

  platform_core_shutdown();
  return(0);
}
#endif


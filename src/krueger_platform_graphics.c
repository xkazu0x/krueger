#ifndef KRUEGER_PLATFORM_GRAPHICS_C
#define KRUEGER_PLATFORM_GRAPHICS_C

internal Platform_Event *
platform_event_list_push(Arena *arena, Platform_Event_List *list, Platform_Event_Type type) {
  Platform_Event *event = push_struct(arena, Platform_Event);
  queue_push(list->first, list->last, event);
  event->type = type;
  return(event);
}

#endif // KRUEGER_PLATFORM_GRAPHICS_C

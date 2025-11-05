#ifndef KRUEGER_RANDOM_C
#define KRUEGER_RANDOM_C

internal Random_Series
random_seed(u32 value) {
  Random_Series result = {0};
  result.index = value % array_count(random_number_table);
  return(result);
}

internal u32
random_next(Random_Series *series) {
  u32 result = random_number_table[series->index++];
  if (series->index >= array_count(random_number_table)) {
    series->index = 0;
  }
  return(result);
}

internal u32
random_choice(Random_Series *series, u32 choice_count) {
  u32 result = random_next(series) % choice_count;
  return(result);
}

internal f32
random_unilateral(Random_Series *series) {
  f32 divisor = 1.0f/(f32)MAX_RANDOM_NUMBER;
  f32 result = divisor*(f32)random_next(series);
  return(result);
}

internal f32
random_bilateral(Random_Series *series) {
  f32 result = 2.0f*random_unilateral(series) - 1.0f;
  return(result);
}

internal f32
random_range(Random_Series *series, f32 min, f32 max) {
  f32 result = lerp_f32(min, max, random_unilateral(series));
  return(result);
}

#endif // KRUEGER_RANDOM_C

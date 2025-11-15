#ifndef KRUEGER_BASE_CORE_C
#define KRUEGER_BASE_CORE_C

/////////////
// NOTE: Time

internal Dense_Time
dense_time_from_date_time(Date_Time time) {
  Dense_Time result = 0;
  result += time.year;
  result *= 12;
  result += time.month;
  result *= 31;
  result += time.day;
  result *= 24;
  result += time.hour;
  result *= 60;
  result += time.min;
  result *= 61;
  result += time.sec;
  result *= 1000;
  result += time.msec;
  return(result);
}

internal Date_Time
date_time_from_dense_time(Dense_Time time) {
  Date_Time result = {0};
  result.msec  = time % 1000;
  time /= 1000;
  result.sec   = time % 61;
  time /= 61;
  result.min   = time % 60;
  time /= 60;
  result.hour  = time % 24;
  time /= 24;
  result.day   = time % 31;
  time /= 31;
  result.month = time % 12;
  time /= 12;
  result.year  = cast(u16) time;
  return(result);
}

#endif // KRUEGER_BASE_CORE_C

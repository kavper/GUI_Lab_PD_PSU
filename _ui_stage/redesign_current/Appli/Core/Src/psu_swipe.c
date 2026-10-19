#include "psu_swipe.h"

int psu_swipe_back(int armed, int start_x, int start_y, int end_x, int end_y,
                   int horizontal, int velocity, int blocked)
{
  int dx;
  int dy;
  if (!armed || blocked || !horizontal || velocity <= 0)
    return 0;
  if (start_x < 0 || start_x >= PSU_SWIPE_EDGE_X)
    return 0;
  dx = end_x - start_x;
  dy = end_y - start_y;
  if (dy < 0)
    dy = -dy;
  if (dx < PSU_SWIPE_MIN_DX || dy > PSU_SWIPE_MAX_DY)
    return 0;
  (void)start_y;
  return 1;
}

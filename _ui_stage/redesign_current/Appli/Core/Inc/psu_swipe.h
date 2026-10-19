#ifndef PSU_SWIPE_H
#define PSU_SWIPE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Rightward swipe that starts in the left margin.
   Callers set blocked when the press began on a slider, editor, keypad,
   or list. A blocked press never navigates. */
#define PSU_SWIPE_EDGE_X 16
#define PSU_SWIPE_MIN_DX 72
#define PSU_SWIPE_MAX_DY 40

int psu_swipe_back(int armed, int start_x, int start_y, int end_x, int end_y,
                   int horizontal, int velocity, int blocked);

#ifdef __cplusplus
}
#endif

#endif

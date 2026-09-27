#ifndef PSU_EDIT_H
#define PSU_EDIT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PSU_EDIT_CAP 12u

typedef struct
{
  char text[PSU_EDIT_CAP];
  uint8_t length;
  uint8_t replace_on_next;
} PsuEditor;

void psu_editor_clear(PsuEditor *ed);
void psu_editor_load_milli(PsuEditor *ed, uint32_t milli, int decimals);
void psu_editor_key(PsuEditor *ed, char key);
void psu_editor_backspace(PsuEditor *ed);
/* Returns 0 on empty input. milli is rounded half-up and clamped by caller. */
int psu_editor_parse_milli(const PsuEditor *ed, uint32_t *milli_out);

#ifdef __cplusplus
}
#endif

#endif

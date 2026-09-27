#include "psu_edit.h"
#include <stdio.h>
#include <string.h>

void psu_editor_clear(PsuEditor *ed)
{
  if (ed == 0)
    return;
  ed->text[0] = '\0';
  ed->length = 0U;
  ed->replace_on_next = 0U;
}

void psu_editor_load_milli(PsuEditor *ed, uint32_t milli, int decimals)
{
  if (ed == 0)
    return;
  if (decimals == 2)
  {
    (void)snprintf(ed->text, sizeof(ed->text), "%u.%02u",
                   (unsigned)(milli / 1000U), (unsigned)((milli % 1000U) / 10U));
  }
  else
  {
    (void)snprintf(ed->text, sizeof(ed->text), "%u.%03u",
                   (unsigned)(milli / 1000U), (unsigned)(milli % 1000U));
  }
  ed->length = (uint8_t)strlen(ed->text);
  ed->replace_on_next = 1U;
}

void psu_editor_key(PsuEditor *ed, char key)
{
  if (ed == 0)
    return;
  if (ed->replace_on_next)
  {
    ed->length = 0U;
    ed->text[0] = '\0';
    ed->replace_on_next = 0U;
  }
  if (ed->length + 1U >= PSU_EDIT_CAP)
    return;
  if (key == '.' && strchr(ed->text, '.') != 0)
    return;
  if (key == '.' && ed->length == 0U)
    ed->text[ed->length++] = '0';
  if (!((key >= '0' && key <= '9') || key == '.'))
    return;
  ed->text[ed->length++] = key;
  ed->text[ed->length] = '\0';
}

void psu_editor_backspace(PsuEditor *ed)
{
  if (ed == 0)
    return;
  if (ed->replace_on_next)
  {
    psu_editor_clear(ed);
    return;
  }
  if (ed->length > 0U)
  {
    ed->text[--ed->length] = '\0';
  }
}

int psu_editor_parse_milli(const PsuEditor *ed, uint32_t *milli_out)
{
  const char *s;
  uint32_t whole = 0U;
  uint32_t frac = 0U;
  uint32_t digits = 0U;
  int saw_dot = 0;
  if (ed == 0 || milli_out == 0 || ed->length == 0U)
    return 0;
  s = ed->text;
  while (*s)
  {
    if (*s == '.')
    {
      if (saw_dot)
        return 0;
      saw_dot = 1;
    }
    else if (*s >= '0' && *s <= '9')
    {
      if (!saw_dot)
      {
        if (whole > 100000U)
          return 0;
        whole = whole * 10U + (uint32_t)(*s - '0');
      }
      else if (digits < 3U)
      {
        frac = frac * 10U + (uint32_t)(*s - '0');
        ++digits;
      }
    }
    else
      return 0;
    ++s;
  }
  while (digits < 3U)
  {
    frac *= 10U;
    ++digits;
  }
  *milli_out = whole * 1000U + frac;
  return 1;
}

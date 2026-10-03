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
  uint64_t whole = 0, result;
  uint32_t fraction = 0;
  unsigned i, decimals = 0;
  int dot = 0, digit_seen = 0, round_up = 0;
  if (!ed || !milli_out || !ed->length || ed->length >= PSU_EDIT_CAP)
    return 0;
  for (i = 0; i < ed->length; ++i)
  {
    const char c = ed->text[i];
    if (c == '.')
    {
      if (dot) return 0;
      dot = 1;
    }
    else if (c >= '0' && c <= '9')
    {
      digit_seen = 1;
      if (!dot) whole = whole * 10U + (unsigned)(c - '0');
      else
      {
        if (decimals < 3) fraction = fraction * 10U + (unsigned)(c - '0');
        else if (decimals == 3) round_up = c >= '5';
        ++decimals;
      }
    }
    else return 0;
  }
  if (!digit_seen) return 0;
  while (decimals < 3) { fraction *= 10U; ++decimals; }
  result = whole * 1000U + fraction + round_up;
  *milli_out = result > UINT32_MAX ? UINT32_MAX : (uint32_t)result;
  return 1;
}

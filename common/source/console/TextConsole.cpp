#include "TextConsole.h"

#include "KeyboardManager.h"
#include "Scheduler.h"
#include "Terminal.h"
#include "kprintf.h"
#include "kstring.h"

#include "ArchCommon.h"

#include "assert.h"
#include "debug.h"

TextConsole::TextConsole(uint32 num_terminals) :
    Console(num_terminals, "TxTConsoleThread")
{
  uint32 i, j = 10, log = 1, k = 0, l = 0;

  while (num_terminals / j)
  {
    j *= 10;
    log++;
  }

  char term_name[log + 5];
  term_name[0] = 't';
  term_name[1] = 'e';
  term_name[2] = 'r';
  term_name[3] = 'm';
  term_name[log + 4] = 0;

  for (i = 0; i < num_terminals; ++i)
  {
    uint32 cterm = i;
    for (l = 4, k = j / 10; k > 0; k /= 10, l++)
    {
      term_name[l] = cterm / k + '0';
      cterm -= ((cterm / k) * k);
    }

    debug(CONSOLE, "Creating Terminal\n");
    Terminal *term = new Terminal(term_name, this, consoleGetNumColumns(), consoleGetNumRows());
    debug(CONSOLE, "Created Terminal at [%p, %p)\n", term, (char*)term + sizeof(*term));
    terminals_.push_back(term);
  }

  active_terminal_ = 0;
}

uint32 TextConsole::consoleGetNumRows() const
{
  return 25;
}

uint32 TextConsole::consoleGetNumColumns() const
{
  return 80;
}

void TextConsole::consoleClearScreen()
{
  char *fb = (char*) ArchCommon::getFBPtr();
  uint32 i;
  for (i = 0; i < consoleGetNumRows() * consoleGetNumColumns() * 2; ++i)
  {
    fb[i] = 0;
  }
}

uint32 TextConsole::consoleSetCharacter(const uint32& row,
                                        const uint32& column,
                                        const uint8& character,
                                        const uint8& state)
{
  char *fb = (char*) ArchCommon::getFBPtr();
  uint32_t console_columns = consoleGetNumColumns();
  uint32_t console_rows = consoleGetNumRows();
  assert(column < console_columns);
  assert(row < console_rows);
  fb[(column + row * console_columns) * 2] = character;
  fb[(column + row * console_columns) * 2 + 1] = state;

  return 0;
}


#define STAT_ROWS (2)

void TextConsole::consoleScrollUp(const uint8& state)
{
  char* fb = (char*) ArchCommon::getFBPtr();
  memmove((void*) (fb + (consoleGetNumColumns() * 2 * STAT_ROWS)),
          (void*) (fb + (consoleGetNumColumns() * 2 * (STAT_ROWS + 1))),
          (consoleGetNumRows() - 1 + STAT_ROWS) * consoleGetNumColumns() * 2);
  for(size_t i = 0; i < consoleGetNumColumns(); i++)
  {
    fb[(i + (consoleGetNumRows() - 1) * consoleGetNumColumns()) * 2] = ' ';
    fb[(i + (consoleGetNumRows() - 1) * consoleGetNumColumns()) * 2 + 1] = state;
  }
}

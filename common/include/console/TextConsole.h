/**
 * @file TextConsole.h
 */

#ifndef _TEXTCONSOLE_H_
#define _TEXTCONSOLE_H_

#include "Console.h"

/**
 * @class TextConsole the text console implementation
 */

class TextConsole : public Console
{
  public:

    /**
     * Constructor creates a TextConsole Thread with the given number of terminals.
     * @param num_terminals the number of terminals to create
     * @return TextConsole instance
     */
    TextConsole ( uint32 num_terminals );

  private:

    /**
     * Clears the console screen.
     * @pre console should be locked for drawing
     */
    virtual void consoleClearScreen();

    /**
     * Sets the given character to the given position on the console.
     * @pre console should be locked for drawing
     * @param row the row number
     * @param column the column number
     * @param character the character to set
     * @param state not implemented - should change the output color
     * @return 0
     */
    virtual uint32 consoleSetCharacter ( uint32 const &row, uint32 const&column, uint8 const &character, uint8 const &state );

    /**
     * Returns the console's number of rows.
     * @return the number of rows
     */
    virtual uint32 consoleGetNumRows() const;

    /**
     * Returns the console's number of Columns.
     * @return the number of columns
     */
    virtual uint32 consoleGetNumColumns() const;

    /**
     * Scrolls up the console.
     * @pre console should be locked for drawing
     */
    virtual void consoleScrollUp(uint8 const &state);
};

#endif

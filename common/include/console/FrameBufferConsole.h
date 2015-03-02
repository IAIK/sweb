/**
 * @file FrameBufferConsole.h
 */

#ifndef FRAMEBUFFERCONSOLE_H__
#define FRAMEBUFFERCONSOLE_H__

#include "Console.h"

/**
 * @class FrameBufferConsole the framebuffer console implementation
 */
class FrameBufferConsole : public Console
{
  public:

    /**
     * Constructor creates a FramebufferConsole Thread with the given number of terminals.
     * @param num_terminals the number of terminals to create
     * @return FramebufferConsole instance
     */
    FrameBufferConsole(uint32 num_terminals);

    /**
     * Sets the given character to the given position on the console.
     * @param row the row number
     * @param column the column number
     * @param character the character to set
     * @param state not implemented - should change the output color
     * @return 0
     */
    virtual uint32 consoleSetCharacter(uint32 const &row, uint32 const&column, uint8 const &character,
                                       uint8 const &state);

  private:

    /**
     * Clears the console screen.
     */
    virtual void consoleClearScreen();

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
     */
    virtual void consoleScrollUp(uint8 const &state);

    /**
     * Sets the Pixel at the given position in the given color.
     * @param x the horizontal position
     * @param y the vertical position
     * @param r the red color component
     * @param g the green color component
     * @param b the blue color component
     */
    void setPixel(uint32 x, uint32 y, uint8 r, uint8 g, uint8 b);

    uint16 convertConsoleColor(CONSOLECOLOR color);
    void colorsFromState(uint8 const &state, CONSOLECOLOR &fg, CONSOLECOLOR &bg);

    uint32 x_res_;
    uint32 y_res_;
    uint32 bits_per_pixel_;
    uint32 bytes_per_pixel_;
};

#endif

#pragma once

#include "Console.h"

class TextConsole : public Console
{
public:
    TextConsole(uint32 num_terminals);
    ~TextConsole() override = default;

private:
    /**
     * Clears the console screen.
     * @pre console should be locked for drawing
     */
    void consoleClearScreen() override;

    /**
     * Sets the given character to the given position on the console.
     * @pre console should be locked for drawing
     * @param row the row number
     * @param column the column number
     * @param character the character to set
     * @param state not implemented - should change the output color
     * @return 0
     */
    uint32 consoleSetCharacter(const uint32& row,
                               const uint32& column,
                               const uint8& character,
                               const uint8& state) override;

    /**
     * Returns the console's number of rows.
     * @return the number of rows
     */
    [[nodiscard]] uint32 consoleGetNumRows() const override;

    /**
     * Returns the console's number of Columns.
     * @return the number of columns
     */
    [[nodiscard]] uint32 consoleGetNumColumns() const override;

    /**
     * Scrolls up the console.
     * @pre console should be locked for drawing
     */
    void consoleScrollUp(const uint8& state) override;
};

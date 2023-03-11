#pragma once

#include "Console.h"
#include "VgaColors.h"
#include "chardev.h"

#include "types.h"

class Terminal : public CharacterDevice
{

    friend class Console;

  public:

    enum LAYOUTS
    {
      EN = 0, DE = 1
    };

    static const uint32 TERMINAL_BUFFER_SIZE = 256;

    /**
     * Constructor creates the Terminal Character Device.
     * @param name name of the terminal
     * @param console the console the terminal is created for
     * @param num_columns the number of column
     * @param num_rows the number of rows
     * @return the Terminal instance
     */
    Terminal(char *name, Console *console, uint32 num_columns, uint32 num_rows);

    /**
     * Writes the character to the terminal.
     * @param character the character to write
     */
    void write(char character);

    /**
     * Writes a string to the terminal.
     * @param string the string to write
     */
    void writeString(const char* string);

    /**
     * Writes a buffer with the given length to the terminal.
     * @param buffer the buffer to write
     * @param len the buffer's length
     */
    void writeBuffer(const char* buffer, size_t len);

    /**
     * Writes Data starting at the offset from the buffer with the given length to the terminal.
     * @param offset from where to start
     * @param size the buffer's size
     * @param buffer the buffer to write
     * @return the size
     */
    int32 writeData(uint32 offset, uint32 size, const char*buffer) override;

    void setForegroundColor(CONSOLECOLOR const &color);
    void setBackgroundColor(CONSOLECOLOR const &color);

    /**
     * Reads one character.from the input
     * @return the character read
     */
    char read();

    /**
     * Reads the given number of characters from the input or until the end of line.
     * @param line the buffer to write
     * @param size the number of characters to read
     * @return the number of chracters read
     */
    uint32 readLine(char *line, uint32 size);

    /**
     * Reads the given number of characters from the input or until the end of line.
     * and don't remove control characters from the string
     * @param line the buffer to write
     * @param size the number of characters to read
     * @return the number of chracters read
     */
    uint32 readLineRaw(char *line, uint32 size);

    /**
     * Reads the given number of characters from the input or until the end of line
     * or until there is no more input
     * @param line the buffer to write
     * @param size the number of characters to read
     * @return the number of chracters read
     */
    uint32 readLineNoBlock(char *line, uint32 size);

    void clearBuffer();
    void putInBuffer(uint32 key);

    void initTerminalColors(CONSOLECOLOR fg, CONSOLECOLOR bg);

    void backspace();

    /**
     * Remaps the key if shift or capslock is active
     * @param key the key to remap
     * @return the remaped key
     */
    uint32 remap(uint32 key);

    void setLayout(Terminal::LAYOUTS layout);

  protected:

    void setAsActiveTerminal();
    void unSetAsActiveTerminal();

  private:

    /**
     * Writes a character to the terminal.
     * Used internal by write methods.
     * @param character the character to write
     */
    void writeInternal(char character);

    [[nodiscard]] uint32 getNumRows() const;
    [[nodiscard]] uint32 getNumColumns() const;

    uint32 setCharacter(uint32 row, uint32 column, uint8 character);
    void scrollUp();

    static bool isLetter(uint32 key);
    static bool isNumber(uint32 key);

    void clearScreen();
    void fullRedraw();

    Console *console_;
    uint32 num_columns_;
    uint32 num_rows_;
    uint32 len_;
    uint8 *characters_;
    uint8 *character_states_;

    uint32 current_column_;
    uint8 current_state_;

    uint8 active_;

    Mutex mutex_;

    LAYOUTS layout_;

};

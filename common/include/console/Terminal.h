
#ifndef TERMINAL_H__
#define TERMINAL_H__

#include "types.h"
#include "Console.h"
#include "Thread.h"
#include "FiFo.h"

#include "chardev.h"

/**
 * @class Terminal terminal used by a console
 */
class Terminal : public CharacterDevice
{

    friend class Console;

  public:

    /**
     * @enum LAYOUTS the keyboard layouts
     */
    enum LAYOUTS
    {
        EN = 0,
        DE = 1
    };

    static uint32 const TERMINAL_BUFFER_SIZE = 256;

    /**
     * Constructor creates the Terminal Character Device.
     * @param name name of the terminal
     * @param console the console the terminal is created for
     * @param num_columns the number of column
     * @param num_rows the number of rows
     * @return the Terminal instance
     */
    Terminal ( char *name, Console *console, uint32 num_columns, uint32 num_rows );

    /**
     * Writes the character to the terminal.
     * @param character the character to write
     */
    void write ( char character );

    /**
     * Writes a string to the terminal.
     * @param string the string to write
     */
    void writeString ( char const *string );

    /**
     * Writes a buffer with the given length to the terminal.
     * @param buffer the buffer to write
     * @param len the buffer's length
     */
    void writeBuffer ( char const *buffer, size_t len );

    /**
     * Writes Data starting at the offset from the buffer with the given length to the terminal.
     * @param offset from where to start
     * @param size the buffer's size
     * @param buffer the buffer to write
     * @return the size
     */
    virtual int32 writeData ( int32 offset, int32 size, const char*buffer );

    /**
     * Sets the foregroundcolor of the terminal.
     * @param color the color to set
     */
    void setForegroundColor ( Console::FOREGROUNDCOLORS const &color );

    /**
     * Sets the backgroundcolor of the terminal.
     * @param color the color to set
     */
    void setBackgroundColor ( Console::BACKGROUNDCOLORS const &color );

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
    uint32 readLine ( char *line, uint32 size );

    /**
     * Reads the given number of characters from the input or until the end of line
     * or until there is no more input
     * @param line the buffer to write
     * @param size the number of characters to read
     * @return the number of chracters read
     */
    uint32 readLineNoBlock ( char *line, uint32 size );

    /**
     * Deletes all input in the buffer
     */
    void clearBuffer();

    /**
     * Puts a key in the input buffer
     * @param key the key
     */
    void putInBuffer ( uint32 key );

    /**
     * Handles backspace.
     */
    void backspace();

    /**
     * Remaps the key if shift or capslock is active
     * TODO: implement lookup tables for various keyboard layouts
     * @param key the key to remap
     * @return the remaped key
     */
    uint32 remap ( uint32 key );

    /**
     * Sets the terminal layout.
     * @param layout the layout
     */
    void setLayout ( Terminal::LAYOUTS layout );

    /**
     * Returns if lock is free.
     * @return true if lock is free
     */
    bool isLockFree()
    {
      return mutex_.isFree();
    }

  protected:

    /**
     * Sets the terminal active.
     */
    void setAsActiveTerminal();

    /**
     * Sets the terminal inactive.
     */
    void unSetAsActiveTerminal();

  private:

    /**
     * Writes a character to the terminal.
     * Used internal by write methods.
     * @param character the character to write
     */
    void writeInternal ( char character );

    /**
     * not implemented
     * @param key not implemented
     */
    void handleKey ( uint32 key );

    /**
     * Clears the terminal screen.
     */
    void clearScreen();

    /**
     * Redraws the terminal.
     */
    void fullRedraw();

    /**
     * Returns the number of the terminal's rows.
     * @return the number of rows
     */
    uint32 getNumRows() const;


    /**
     * Returns the number of the terminal's columns.
     * @return the number of columns
     */
    uint32 getNumColumns() const;

    /**
     * Sets the given character in the terminal.
     * @param row the row number
     * @param column the column number
     * @param character the character to set
     * @return 0
     */
    uint32 setCharacter ( uint32 row,uint32 column, uint8 character );

    /**
     * not implemented
     */
    void processInBuffer() {};

    /**
     * not implemented
     */
    void processOutBuffer() {};

    /**
     * Scrolls up the terminal.
     */
    void scrollUp();

    /**
     * Checks if the given key is a lower case letter.
     * @param key the key to check
     * @return true if key is letter
     */
    bool isLetter ( uint32 key );

    /**
     * Checks if the given key is a number.
     * @param key the key to check
     * @return true if key is a number
     */
    bool isNumber ( uint32 key );

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


#endif

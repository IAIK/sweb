#pragma once

#include "types.h"
#include <ulist.h>
#include "Mutex.h"

#include "Thread.h"

class Terminal;

class Console : public Thread
{
    friend class Terminal;
    friend class ConsoleManager;

  public:
    Console(uint32 num_terminals, const char *name);

    /**
     * Writes input from the keyboard to the active terminal
     */
    virtual void Run();

    /**
     * Checks if the given key is displayable.
     * @param key the key to check
     * @return true if displayable
     */
    bool isDisplayable(uint32 key);

    virtual ~Console()
    {
    }

    uint32 getNumTerminals() const;
    Terminal* getActiveTerminal();
    Terminal* getTerminal(uint32 term);

    ustl::list<Terminal*>::iterator terminalsBegin();
    ustl::list<Terminal*>::iterator terminalsEnd();

    /**
     * Sets the terminal with the given number active.
     * Dangerous: will most likely produce a deadlock.
     * @param term the number of the terminal to set active
     */
    void setActiveTerminal(uint32 term);

    void lockConsoleForDrawing();
    void unLockConsoleForDrawing();

  protected:

    /**
     * Handles special non displayable keys:
     * F-keys for switching active terminals
     * displaying threads list
     * backspace
     * @param key the key to handle
     */
    void handleKey(uint32 key);

    virtual void consoleClearScreen() =0;
    virtual uint32 consoleSetCharacter(uint32 const &row, uint32 const&column, uint8 const &character,
                                       uint8 const &state) =0;
    virtual uint32 consoleGetNumRows() const=0;
    virtual uint32 consoleGetNumColumns() const=0;
    virtual void consoleScrollUp(uint8 const &state) =0;

    ustl::list<Terminal* > terminals_;
    Mutex console_lock_;
    Mutex set_active_lock_;
    uint8 locked_for_drawing_;

    uint32 active_terminal_;

};

extern Console* main_console;

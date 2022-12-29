#pragma once

#include "Mutex.h"
#include "types.h"
#include "EASTL/vector.h"

#include "Thread.h"

class Terminal;

class Console : public Thread
{
    friend class Terminal;

public:
    Console(uint32 num_terminals, const char* name);
    ~Console() override = default;

    /**
     * Writes input from the keyboard to the active terminal
     */
    void Run() override;

    /**
     * Checks if the given key is displayable.
     * @param key the key to check
     * @return true if displayable
     */
    [[nodiscard]] bool isDisplayable(uint32 key) const;

    [[nodiscard]] uint32 getNumTerminals() const;
    [[nodiscard]] Terminal* getActiveTerminal() const;
    [[nodiscard]] Terminal* getTerminal(uint32 term) const;

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

    virtual void consoleClearScreen() = 0;
    virtual uint32 consoleSetCharacter(const uint32& row,
                                       const uint32& column,
                                       const uint8& character,
                                       const uint8& state) = 0;

    [[nodiscard]] virtual uint32 consoleGetNumRows() const = 0;
    [[nodiscard]] virtual uint32 consoleGetNumColumns() const = 0;

    virtual void consoleScrollUp(const uint8& state) = 0;

    eastl::vector<Terminal*> terminals_;
    Mutex console_lock_;
    Mutex set_active_lock_;
    uint8 locked_for_drawing_;

    uint32 active_terminal_;
};

extern Console* main_console;

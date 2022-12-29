#pragma once

#include "Console.h"

class FrameBufferConsole : public Console
{
public:
    FrameBufferConsole(uint32 num_terminals);
    ~FrameBufferConsole() override = default;

    uint32 consoleSetCharacter(const uint32& row,
                               const uint32& column,
                               const uint8& character,
                               const uint8& state) override;

private:
    void consoleClearScreen() override;
    [[nodiscard]] uint32 consoleGetNumRows() const override;
    [[nodiscard]] uint32 consoleGetNumColumns() const override;
    void consoleScrollUp(const uint8& state) override;

    void setPixel(uint32 x, uint32 y, uint8 r, uint8 g, uint8 b);

    static uint16 convertConsoleColor(CONSOLECOLOR color);
    static void colorsFromState(const uint8& state, CONSOLECOLOR& fg, CONSOLECOLOR& bg);

    uint32 x_res_;
    uint32 y_res_;
    uint32 bits_per_pixel_;
    uint32 bytes_per_pixel_;
};

/**
 * @file XenConsole.h
 *
 */

#ifndef _XENCONSOLE_H_
#define _XENCONSOLE_H_

#include "console/Console.h"

class XenConsole : public Console
{
public:

  /**
   * Constructor
   * @param num_terminals number of terminals, that the console should have
   *
   */
  XenConsole(uint32 num_terminals);

  /**
   * run methode of the XenConsole
   *
   */
  virtual void Run();

  /**
   * handles the given key
   *
   */
  void handleKey( uint32 );

  /**
   *
   *
   */
  uint32 remap( uint32 ); // this should be moved to terminal

  /**
   * true, if displayable
   *
   */
  bool isDisplayable( uint32 );

  /**
   * true, if letter
   *
   */
  bool isLetter( uint32 );

  /**
   * true, if number
   *
   */
  bool isNumber( uint32 );


private:

  /**
   * clears the screen of the console
   *
   */
  virtual void consoleClearScreen();

  /**
   * prints a character in the Console
   * @param row vertical position in the console
   * @param column horizontal position in the console
   * @param character the character to print
   * @param state
   *
   */
  virtual uint32 consoleSetCharacter(uint32 const &row, uint32 const&column, uint8 const &character, uint8 const &state);

  /**
   * @return gives the number of rows of the console
   *
   */
  virtual uint32 consoleGetNumRows() const;

  /**
   * @return gives the number of columns of the console
   *
   */
  virtual uint32 consoleGetNumColumns() const;

  /**
   *
   *
   */
  virtual void consoleScrollUp();

  /**
   * sets the color of the foreground
   *
   */
  virtual void consoleSetForegroundColor(FOREGROUNDCOLORS const &color);

  /**
   * sets the color of the background
   *
   */
  virtual void consoleSetBackgroundColor(BACKGROUNDCOLORS const &color);

};

#endif

/**
 * @file FrameBufferConsole.cpp
 */

#include "FrameBufferConsole.h"
#include "ArchCommon.h"
#include "Terminal.h"
#include "image.h"
#include "arch_keyboard_manager.h"
#include "kprintf.h"

FrameBufferConsole::FrameBufferConsole ( uint32 num_terminals ) :Console ( num_terminals )
{
  x_res_ = ArchCommon::getVESAConsoleWidth();
  y_res_ = ArchCommon::getVESAConsoleHeight();
  bits_per_pixel_ = ArchCommon::getVESAConsoleBitsPerPixel();
  bytes_per_pixel_ = bits_per_pixel_ / 8;

  uint32 i, j = 10, log = 1, k = 0, l = 0;

  while ( num_terminals / j )
  {
    j *= 10;
    log++;
  }

  char *term_name = new char[ log + 5 ];
  term_name[ 0 ] = 't';
  term_name[ 1 ] = 'e';
  term_name[ 2 ] = 'r';
  term_name[ 3 ] = 'm';
  term_name[ log + 4 ] = 0;

  for ( i=0;i<num_terminals;++i )
  {
    uint32 cterm = i;
    for ( l = 4, k = j/10; k > 0 ; k /= 10, l++ )
    {
      term_name[ l ] = cterm/k + '0';
      cterm -= ( ( cterm/k ) * k );
    }

    Terminal *term = new Terminal ( term_name, this,consoleGetNumColumns(),consoleGetNumRows() );
    terminals_.pushBack ( term );
  }

  delete term_name;

  active_terminal_ = 0;
  name_ = "VESAConsoleThrd";
  consoleSetForegroundColor ( FG_BLACK );
  consoleSetBackgroundColor ( BG_WHITE );
  consoleClearScreen();
}

void FrameBufferConsole::consoleClearScreen()
{
  uint8 *lfb = ( uint8* ) ArchCommon::getVESAConsoleLFBPtr();
  uint32 i,k;

  ArchCommon::bzero ( ( pointer ) lfb,x_res_*y_res_*bytes_per_pixel_ );

  uint32 off = ( x_res_ - logo.width ) / 2;

  for ( i=0;i<logo.height;++i )
  {
    for ( k=0;k<logo.width;++k )
    {
      setPixel ( off+k, ( y_res_-65+i ),logo.pixel_data[ ( i*logo.width+k ) *3],logo.pixel_data[ ( i*logo.width+k ) *3+1],logo.pixel_data[ ( i*logo.width+k ) *3+2] );
    }
  }
}

uint32 FrameBufferConsole::consoleGetNumRows() const
{
  return ( y_res_-65 ) / 16;
}

uint32 FrameBufferConsole::consoleGetNumColumns() const
{
  return x_res_ / 8;
}

extern uint8 fontdata_sun8x16[];

void FrameBufferConsole::setPixel ( uint32 x,uint32 y,uint8 r,uint8 g,uint8 b )
{
  uint16 *lfb = ( uint16* ) ArchCommon::getVESAConsoleLFBPtr();
  uint32 offset = ( x + y*x_res_ );
  uint16 color= ( b>>3 );
  color|= ( g>>2 ) <<5;
  color|= ( r>>3 ) <<11;

  lfb[offset] = color;
  /*
  lfb[offset + 0] = b;
  lfb[offset + 1] = g;
  lfb[offset + 2] = r;
  lfb[offset + 3] = 0;
  */
}

uint32 FrameBufferConsole::consoleSetCharacter ( uint32 const &row, uint32 const&column, uint8 const &character, uint8 const __attribute__((unused)) &state )
{
  uint32 i,k;
  uint32 character_index = character * 16;

  uint16 *lfb = ( uint16* ) ArchCommon::getVESAConsoleLFBPtr();

  uint32 top_left_pixel = column*8 + row*16*x_res_;

  for ( i=0;i<16;++i )
  {

    for ( k=0;k<8;++k )
    {
      // find out the bit we want to draw
      uint8 temp = fontdata_sun8x16[character_index+i];
      temp &= 1<< ( 7-k );
      if ( temp )
      {
        lfb[top_left_pixel + k + i*x_res_] = current_foreground_color_;
      }
      else
      {
        lfb[top_left_pixel + k + i*x_res_] = current_background_color_;
      }
    }

  }

  return 0;
}


void FrameBufferConsole::consoleScrollUp()
{
  pointer fb = ArchCommon::getVESAConsoleLFBPtr();
  ArchCommon::memcpy ( fb, fb+ ( consoleGetNumColumns() *bytes_per_pixel_*8*16 ),
                       ( consoleGetNumRows()-1 ) *consoleGetNumColumns() *bytes_per_pixel_*8*16 );
  ArchCommon::bzero ( fb+ ( ( consoleGetNumRows()-1 ) *consoleGetNumColumns() *bytes_per_pixel_*8*16 ),consoleGetNumColumns() *bytes_per_pixel_*8*16 );

}

void FrameBufferConsole::consoleSetForegroundColor ( FOREGROUNDCOLORS const &color )
{
  uint8 r,g,b;
  r = 0;
  g = 255;
  b = 0;

  current_foreground_color_ = ( r<<16 ) + ( g<<8 ) + ( b );

  if ( color )
    return;

}
void FrameBufferConsole::consoleSetBackgroundColor ( BACKGROUNDCOLORS const &color )
{
  uint8 r,g,b;
  r = 0;
  g = 0;
  b = 0;
  current_background_color_ = ( r<<16 ) + ( g<<8 ) + ( b );

  if ( color )
    return;
}

void FrameBufferConsole::Run ( void )
{
  KeyboardManager * km = KeyboardManager::getInstance();
  uint32 key;
  do
  {
    while ( km->getKeyFromKbd ( key ) )
      if ( isDisplayable ( key ) )
      {
        key = terminals_[active_terminal_]->remap ( key );
        terminals_[active_terminal_]->write ( key );
        terminals_[active_terminal_]->putInBuffer ( key );
      }
      else
        handleKey ( key );
    Scheduler::instance()->yield();
  }
  while ( 1 ); // until the end of time

}
void FrameBufferConsole::handleKey ( uint32 key )
{
  KeyboardManager * km = KeyboardManager::getInstance();

  uint32 terminal_selected = ( key - KeyboardManager::KEY_F1 );

  if ( terminal_selected < getNumTerminals() && km->isShift() )
  {
    setActiveTerminal ( terminal_selected );
    return;
  }

  if ( terminal_selected == 11 )
    Scheduler::instance()->printThreadList();

  if ( key == '\b' )
    terminals_[active_terminal_]->backspace();

  return;
}
bool FrameBufferConsole::isDisplayable ( uint32 key )
{
  return ( ( ( key & 127 ) >= ' ' ) || ( key == '\n' ) || ( key == '\b' ) );
}

#include "Thread.h"

/// Various testing threads 

class TestTerminalThread : public Thread
{
  public:

  Console * main_console;
  uint32 on_terminal;
  
  TestTerminalThread(char *name, Console *console, uint32 terminal)
  {
    name_=name;
    main_console = console;
    on_terminal = terminal;
  };

  virtual void Run()
  {
    char *name = new char[ 50 ];
    
    do 
    {
      Terminal *t = main_console->getTerminal( on_terminal );
      t->clearBuffer();
      t->writeString( "\n\n Greetings ! \n Could you tell me your name ? \n" );
      if( t->readLine( name, 50 ) > 48 )
        t->writeString( "Wow ! \n Your name is very long I was not prepared and I can not remember more than 49 characters !! \n" );
      
      t->writeString("Hello, ");
      t->writeString( name );
      t->writeString(" pleased to meet you !! \n\n");
    }
    while(1);
  };

};


class SerialThread : public Thread
{
  public:

  SerialThread(char *name)
  {
    name_=name;
  };

  virtual void Run()
  {
    SerialManager *sm = SerialManager::getInstance();
    uint32 num_ports = sm->get_num_ports();
    uint32 i = 0, j = 0;
    for( i=0; i < num_ports; i++ )
    {
      SerialPort *sp = sm->serial_ports[i];
      kprintfd( "SerialThread::Run: Port number : %d, Port name : %s \n", i ,
      sp->friendly_name );

      // read from serial port and write to console
      uint8 gotch = 0;
      uint32 num_read = 0;

      do
      {
        sp->read( &gotch, 1, num_read );
        if( num_read )
          kprintf( "%c", gotch );
      }
      while( 1 );
      // until forever*/
    }

    kprintf("SerialThread::Run: Done with serial ports\n");
    for(;;) Scheduler::instance()->yield();
  };

};

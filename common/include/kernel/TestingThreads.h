#include "Thread.h"
#include "fs/VirtualFileSystem.h"
#include "fs/fs_global.h"

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
    uint32 numread=0;
    name[49]=0;
    
    do 
    {
      Terminal *t = main_console->getTerminal( on_terminal );
      t->clearBuffer();
      t->writeString( "\n\n Greetings ! \n Could you tell me your name ? \n" );
      numread = t->readLine( name, 49 );
      name[numread]=0;
      if( numread > 48 )
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
    kprintf("Serial Device creation\n");
    SerialManager::getInstance()->do_detection( 1 );
    kprintf("Serial Device done\n");

    SerialManager *sm = SerialManager::getInstance();
    uint32 num_ports = sm->get_num_ports();
    uint32 i = 0;
    
    for( i=0; i < num_ports; i++ )
    {
      SerialPort *sp = sm->serial_ports[i];
      kprintfd( "SerialThread::Run: Port number : %d, Port name : %s \n", i ,
      sp->device_name );
      kprintfd( "SerialThread::Run: I/O Port : %X, Port irq : %d \n", 
      sp->get_info().base_port, sp->get_info().irq_num );
    }
    
    kprintfd("SerialThread::Run:opening /dev/sp5\n");
    int32 test05_fd = vfs_syscall.open("/dev/sp5", 2);
    kprintfd("SerialThread::Run: open returned %d\n", test05_fd);
    
    kprintfd("SerialThread::Run:writting hallo world!\n");
    vfs_syscall.write(test05_fd, "hallo world\n", 12);
    
    kprintfd("SerialThread::Run:Closing /dev/sp5 !\n");
    vfs_syscall.close( test05_fd );
    
    
    kprintfd("SerialThread::Run:opening /dev/sp1\n");
    int32 test01_fd = vfs_syscall.open("/dev/sp1", 2);
    kprintfd("SerialThread::Run: open returned %d\n", test01_fd);
    
    kprintfd("SerialThread::Run:writting hallo world!\n");
    int32 wres = vfs_syscall.write(test01_fd, "hallo world\n", 12);
    kprintfd("SerialThread::Run:write returned %d!\n", wres);
    
    kprintfd("SerialThread::Run:Closing /dev/sp1 !\n");
    vfs_syscall.close( test01_fd );

    for( i=0; i < num_ports; i++ )
    {
      SerialPort *sp = sm->serial_ports[i];
      // read from serial port and write to console
      char gotch = 0;
      uint32 num_read = 0;
      
      char *msg = "MESSAGE";
      sp->writeData( 0, 7, msg );
      
      do
      {
        num_read = sp->readData( 0, 1, &gotch );
        if( num_read )
          kprintfd( "%c", gotch );
      }
      while( 1 );
      // until forever*/
    }

    kprintf("SerialThread::Run: Done with serial ports\n");
    currentThread->kill();
  };

};

class BDThread : public Thread
{
  public:

  BDThread()
  {
    name_="BlockDevices";
  };

  virtual void Run()
  {

    kprintf("Block Device creation\n");
    BDManager::getInstance()->doDeviceDetection( );
    kprintf("Block Device done\n");  
      
    kprintfd("BDThread::Run: Now getting info on blockdevices ...\n");
    uint32 numdev = BDManager::getInstance()->getNumberOfDevices();
    uint32 dev_cnt = 0;
    
    for( ;dev_cnt < numdev; dev_cnt++ )
      kprintfd("BDThread::Run: device %s size: %u \n",  
      BDManager::getInstance()->getDeviceByNumber(dev_cnt)->getName(), 
      BDManager::getInstance()->getDeviceByNumber(dev_cnt)->getNumBlocks()*BDManager::getInstance()->getDeviceByNumber(dev_cnt)->getBlockSize() );
      
    kprintfd("BDThread::Run: Done with blockdevices\n");
    
    
   //Actually we know the virtual device number we want to use: "2" (swap)
   //What is the block size on this device?
   //Well, let's have a look. 
   BDRequest * bd_bs = new BDRequest(0, BDRequest::BD_GET_BLK_SIZE);
   BDManager::getInstance()->getDeviceByNumber(2)->addRequest ( bd_bs );
   //this is a blocking request. It just references data already in memory.
   if (bd_bs->getStatus() != BDRequest::BD_DONE)
   {
     // something went wrong... EXIT!
   }
   uint32 block_size = bd_bs->getResult();
   
   //We have to check how many blocks ae available on this device:
   BDRequest * bd_bc = new BDRequest(0, BDRequest::BD_GET_NUM_BLOCKS);
   BDManager::getInstance()->getDeviceByNumber(2)->addRequest ( bd_bc );
   //this is a blocking request. It just references data already in memory.
   if (bd_bs->getStatus() != BDRequest::BD_DONE)
   {
     // something went wrong... EXIT!
   }
   uint32 block_count = bd_bc->getResult();
      
   //We assume that we want to read two blocks (Number 234 and 235)     
   uint32 blocks2read = 2;
   uint32 offset = 233;
   if (offset + blocks2read > block_count)
   {
     //do this little check.
   } 
   //allocate some buffer or point somewhere in memory.
   char *my_buffer = (char *) kmalloc( blocks2read*block_size*sizeof(uint8) );
   //build the command
   BDRequest * bd = new BDRequest(2, BDRequest::BD_READ, offset, blocks2read, my_buffer);
   //and send it.
   BDManager::getInstance()->getDeviceByNumber(2)->addRequest ( bd );
   uint32 jiffies = 0;
   //actually we don't know if this request is blocking or not. Just to be
   //on the safe side, check if the output is valid by now.   
   while( bd->getStatus() == BDRequest::BD_QUEUED && jiffies++ < 50000 );
          
   if( bd->getStatus() != BDRequest::BD_DONE )
   {
      //We should definitely should have a closer look at the status by now.
      //It may happen, that the request is still in queue or had an error.
   }
   //By now the reqested data should be in my buffer.
   kfree( my_buffer );
   //Print the buffer...
   for(uint32 i = 0; i < blocks2read*block_size; i++ )
      kprintfd( "%2X%c", *(my_buffer+i), i%8 ? ' ' : '\n' );  
   delete bd;
   delete bd_bs;
   delete bd_bc;



    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    kprintfd("BDThread::Run:opening /dev/idea\n");
    int32 testide_fd = vfs_syscall.open("/dev/idea", 2);
    kprintfd("BDThread::Run: open returned %d\n", testide_fd);
   
    char buffer[1100] = { 0 };
    kprintfd("BDThread::Run:reading /dev/idea\n");
    int32 res = vfs_syscall.read(testide_fd, buffer , 1100);
    kprintfd("BDThread::Run: read returned %d\n", res);
    
    kprintfd("BDThread::Run: open read ------------- \n");
    
    //uint32 i;
    //for( i = 0; i < 1100; i++ )
    //  kprintfd( "%2X%c", buffer[i], i%8 ? ' ' : '\n' );
    
    kprintfd("BDThread::Run: ----------------------- \n");
    
    kprintfd("BDThread::Run:Closing /dev/idea !\n");
    vfs_syscall.close( testide_fd );
    currentThread->kill();
  };

};

class DeviceFSMountingThread : public Thread
{
  public:

  DeviceFSMountingThread()
  {
    name_="DeviceFSMountingThread";
  };

  virtual void Run()
  {
    kprintfd("\n> open /term1\n");
    int32 test01_fd = vfs_syscall.open("/dev/term1", 2);
    kprintfd("\n> open returned %d\n", test01_fd);
    
    kprintfd("\n> open /dev/term2\n");
    int32 test02_fd = vfs_syscall.open("/dev/term2", 2);
    kprintfd("\n> open returned %d\n", test02_fd);
    
    kprintfd("\n> write hallo world!\n");
    int32 wres = vfs_syscall.write(test01_fd, "hallo world!", 12);
    kprintfd("\n> write returned %d!\n", wres);
    wres = vfs_syscall.write(test02_fd, "hallo world!", 12);
    kprintfd("\n> write returned %d!\n", wres);
    wres = vfs_syscall.write(test02_fd, "enter 10 chars!", 15);
    kprintfd("\n> write returned %d!\n", wres);
    
    char buffer[10];
    vfs_syscall.read(test02_fd, buffer , 10);
    
    vfs_syscall.write(test02_fd, buffer, 10);
    vfs_syscall.write(test02_fd, buffer, 15); // this is a BIG NO NO !
  }
};


/**
 * @file TestingThreads.h
 * Various testing threads
 */

#include "Thread.h"
#include "fs/VirtualFileSystem.h"
#include "fs/fs_global.h"
#include "Scheduler.h"

/**
 * @class TestTerminalThread
 * thread for terminal testing
 */
class TestTerminalThread : public Thread
{
  public:

    Console * main_console;
    uint32 on_terminal;

    /**
       * Constructor
       * @param name the name
       * @param console the terminal's console
       * @param terminal the terminal to test
       * @return TestTerminalThread instance
       */
    TestTerminalThread ( char *name, Console *console, uint32 terminal )
    {
      name_=name;
      main_console = console;
      on_terminal = terminal;
    };

    /**
     * Runs the terminal test.
     */
    virtual void Run()
    {
      char *name = new char[ 50 ];
      uint32 numread=0;
      name[49]=0;

      do
      {
        Terminal *t = main_console->getTerminal ( on_terminal );
        t->clearBuffer();
        t->writeString ( "\n\n Greetings ! \n Could you tell me your name ? \n" );
        numread = t->readLine ( name, 49 );
        name[numread]=0;
        if ( numread > 48 )
          t->writeString ( "Wow ! \n Your name is very long I was not prepared and I can not remember more than 49 characters !! \n" );

        t->writeString ( "Hello, " );
        t->writeString ( name );
        t->writeString ( " pleased to meet you !! \n\n" );
      }
      while ( 1 );
    };

};


/**
 * @class SerialThread
 * thread for serial device testing
 */
class SerialThread : public Thread
{
  public:

    /**
     * Constructor
     * @param name the name
     * @return SerialThread instance
     */
    SerialThread ( char *name )
    {
      name_=name;
    };

    /**
     * Runs the serial device test.
     */
    virtual void Run()
    {
      kprintf ( "Serial Device creation\n" );
      SerialManager::getInstance()->do_detection ( 1 );
      kprintf ( "Serial Device done\n" );


      SerialManager *sm = SerialManager::getInstance();
      uint32 num_ports = sm->get_num_ports();
      uint32 i = 0;

      for ( i=0; i < num_ports; i++ )
      {
        SerialPort *sp = sm->serial_ports[i];
        kprintfd ( "SerialThread::Run: Port number : %d, Port name : %s \n", i ,
                   sp->device_name );
        kprintfd ( "SerialThread::Run: I/O Port : %X, Port irq : %d \n",
                   sp->get_info().base_port, sp->get_info().irq_num );
      }

      kprintfd ( "SerialThread::Run:opening /dev/sp5\n" );
      int32 test05_fd = vfs_syscall.open ( "/dev/sp5", 2 );
      kprintfd ( "SerialThread::Run: open returned %d\n", test05_fd );

      kprintfd ( "SerialThread::Run:writting hallo world!\n" );
      vfs_syscall.write ( test05_fd, "hallo world\n", 12 );

      kprintfd ( "SerialThread::Run:Closing /dev/sp5 !\n" );
      vfs_syscall.close ( test05_fd );


      kprintfd ( "SerialThread::Run:opening /dev/sp1\n" );
      int32 test01_fd = vfs_syscall.open ( "/dev/sp1", 2 );
      kprintfd ( "SerialThread::Run: open returned %d\n", test01_fd );

      kprintfd ( "SerialThread::Run:writting hallo world!\n" );
      int32 wres = vfs_syscall.write ( test01_fd, "hallo world\n", 12 );
      kprintfd ( "SerialThread::Run:write returned %d!\n", wres );

      kprintfd ( "SerialThread::Run:Closing /dev/sp1 !\n" );
      vfs_syscall.close ( test01_fd );

      for ( i=0; i < num_ports; i++ )
      {
        SerialPort *sp = sm->serial_ports[i];
        // read from serial port and write to console
        char gotch = 0;
        uint32 num_read = 0;

        char *msg = "MESSAGE";
        sp->writeData ( 0, 7, msg );

        do
        {
          num_read = sp->readData ( 0, 1, &gotch );
          if ( num_read )
            kprintfd ( "%c", gotch );
        }
        while ( 1 );
        // until forever*/
      }

      kprintf ( "SerialThread::Run: Done with serial ports\n" );
      //currentThread->kill();
      while ( 1 );
    };

};

/**
 * @class BDThread
 * thread for block device testing
 */
class BDThread : public Thread
{
  public:

    /**
     * Consturctor
     * @return BDThread instance
     */
    BDThread()
    {
      name_="BlockDevices";
    };

    /**
     * Runs the block device test.
     */
    virtual void Run()
    {

      kprintfd ( "BDThread1::Run: Now getting info on blockdevices ...\n" );
      uint32 numdev = BDManager::getInstance()->getNumberOfDevices();
      uint32 dev_cnt = 0;

      for ( ;dev_cnt < numdev; dev_cnt++ )
        kprintfd ( "BDThread1::Run: device %s size: %u \n",
                   BDManager::getInstance()->getDeviceByNumber ( dev_cnt )->getName(),
                   BDManager::getInstance()->getDeviceByNumber ( dev_cnt )->getNumBlocks() *BDManager::getInstance()->getDeviceByNumber ( dev_cnt )->getBlockSize() );

      kprintfd ( "BDThread1::Run: Done with %d blockdevices\n",numdev );

      kprintfd ( "BDThread1::Run: Adding read request \n" );
      char message[4096];

      uint32 i;
      for ( i = 0; i < 4096; i++ )
        message[i] = '0';

      BDRequest *bdr = new BDRequest ( 3, BDRequest::BD_READ, 0, 8, message );
      BDManager::getInstance()->addRequest ( bdr );
      Scheduler::instance()->yield();
      kprintfd ( "BDThread1::Run: Read request %d \n", bdr->getStatus() );
      kprintf ( " FINISHED BDThread" );

      //currentThread->kill();
      while ( 1 );
    };

};

/**
 * @class BDThread2
 * second thread for block deevice testing.
 */
class BDThread2 : public Thread
{
  public:

    /**
     * Consturctor
     * @return BDThread2 instance
     */
    BDThread2()
    {
      name_="BlockDevices2";
    };

    /**
     * Runs the second block device test.
     */
    virtual void Run()
    {
      kprintfd ( "BDThread::Run: Now getting info on blockdevices ...\n" );
      uint32 numdev = BDManager::getInstance()->getNumberOfDevices();
      uint32 dev_cnt = 0;

      for ( ;dev_cnt < numdev; dev_cnt++ )
        kprintfd ( "BDThread::Run: device %s size: %u \n",
                   BDManager::getInstance()->getDeviceByNumber ( dev_cnt )->getName(),
                   BDManager::getInstance()->getDeviceByNumber ( dev_cnt )->getNumBlocks() *BDManager::getInstance()->getDeviceByNumber ( dev_cnt )->getBlockSize() );

      kprintfd ( "BDThread::Run: Done with blockdevices\n" );

      kprintfd ( "BDThread::Run: Adding write request \n" );
      char message[4096];

      uint32 i;
      for ( i = 0; i < 4096; i++ )
        message[i] = 'x';

      BDRequest *bdr = new BDRequest ( 3, BDRequest::BD_WRITE, 233, 8, message );
      BDManager::getInstance()->addRequest ( bdr );
      //Scheduler::instance()->yield();
      kprintfd ( "BDThread::Run: Write request %d \n", bdr->getStatus() );

      //Actually we know the virtual device number we want to use: "2" (swap)
      //What is the block size on this device?
      //Well, let's have a look.
      BDRequest * bd_bs = new BDRequest ( 2, BDRequest::BD_GET_BLK_SIZE );
      BDManager::getInstance()->getDeviceByNumber ( 2 )->addRequest ( bd_bs );
      //this is a blocking request. It just references data already in memory.
      if ( bd_bs->getStatus() != BDRequest::BD_DONE )
      {
        // something went wrong... EXIT!
      }
      uint32 block_size = bd_bs->getResult();

      //We have to check how many blocks are available on this device:
      BDRequest * bd_bc = new BDRequest ( 2, BDRequest::BD_GET_NUM_BLOCKS );
      BDManager::getInstance()->getDeviceByNumber ( 2 )->addRequest ( bd_bc );
      //this is a blocking request. It just references data already in memory.
      if ( bd_bs->getStatus() != BDRequest::BD_DONE )
      {
        // something went wrong... EXIT!
      }
      uint32 block_count = bd_bc->getResult();

      //We assume that we want to read two blocks (Number 234 and 235)
      uint32 blocks2read = 2;
      uint32 offset = 233;
      if ( offset + blocks2read > block_count )
      {
        //do this little check.
      }
      //allocate some buffer or point somewhere in memory.
      char *my_buffer = ( char * ) kmalloc ( blocks2read*block_size*sizeof ( uint8 ) );
      //build the command
      BDRequest * bd = new BDRequest ( 2, BDRequest::BD_READ, offset, blocks2read, my_buffer );
      //and send it.
      BDManager::getInstance()->getDeviceByNumber ( 2 )->addRequest ( bd );
      uint32 jiffies = 0;
      //actually we don't know if this request is blocking or not. Just to be
      //on the safe side, check if the output is valid by now.
      while ( bd->getStatus() == BDRequest::BD_QUEUED && jiffies++ < 50000 );

      if ( bd->getStatus() != BDRequest::BD_DONE )
      {
        //We should definitely should have a closer look at the status by now.
        //It may happen, that the request is still in queue or had an error.
      }
      //By now the reqested data should be in my buffer.

      //Print the buffer...
      for ( uint32 i = 0; i < blocks2read*block_size; i++ )
        kprintfd ( "%2X%c", * ( my_buffer+i ), i%8 ? ' ' : '\n' );

      kfree ( my_buffer );

      delete bd;
      delete bd_bs;
      delete bd_bc;
      kprintf ( " FINISHED BDThread2" );

//currentThread->kill();
      while ( 1 );
    };

};

/**
 * @class DeviceFSMountingThread
 * thread for device file system mounting testing
 */
class DeviceFSMountingThread : public Thread
{
  public:

    /**
     * Constructor
     * @return DeviceFSMountingThread instance
     */
    DeviceFSMountingThread()
    {
      name_="DeviceFSMountingThread";
    };

    /**
     * Runs the device file system mounting test.
     */
    virtual void Run()
    {
      kprintfd ( "\n> open /term1\n" );
      int32 test01_fd = vfs_syscall.open ( "/dev/term1", 2 );
      kprintfd ( "\n> open returned %d\n", test01_fd );

      kprintfd ( "\n> open /dev/term2\n" );
      int32 test02_fd = vfs_syscall.open ( "/dev/term2", 2 );
      kprintfd ( "\n> open returned %d\n", test02_fd );

      kprintfd ( "\n> write hallo world!\n" );
      int32 wres = vfs_syscall.write ( test01_fd, "hallo world!", 12 );
      kprintfd ( "\n> write returned %d!\n", wres );
      wres = vfs_syscall.write ( test02_fd, "hallo world!", 12 );
      kprintfd ( "\n> write returned %d!\n", wres );
      wres = vfs_syscall.write ( test02_fd, "enter 10 chars!", 15 );
      kprintfd ( "\n> write returned %d!\n", wres );

      char buffer[10];
      vfs_syscall.read ( test02_fd, buffer , 10 );

      vfs_syscall.write ( test02_fd, buffer, 10 );
      vfs_syscall.write ( test02_fd, buffer, 15 ); // this is a BIG NO NO !
      while ( 1 );
    }
};

/**
 * @class MinixTestingThread
 * thread for minix file system testing
 */
class MinixTestingThread : public Thread
{
  public:
    /**
     * Constructor
     * @param root_fs_info the FileSystemInfo
     */
    MinixTestingThread ( FileSystemInfo* root_fs_info ) : Thread ( root_fs_info )
    {
      name_="MinixTestingThread";
    }

    /**
     * Runs the minix file system tests
     */
    virtual void Run()
    {

//       kprintfd("\n> list /\n");
//       vfs_syscall.readdir("/");
//
      kprintfd ( "\n> calling mkdir: /minix\n" );
      int32 mkdir_ret = vfs_syscall.mkdir ( "/minix", 2 );
      kprintfd ( "\n> mkdir returned <%d>\n",mkdir_ret );

      kprintfd ( "\n> calling mount: idec, /minix, minixfs\n" );
      int32 mount_ret = vfs_syscall.mount ( "idec","/minix", "minixfs",0 );
      kprintfd ( "\n> mount returned <%d>\n",mount_ret );

      kprintfd ( "\n> chdir tests\n" );
      int32 chdir_ret = vfs_syscall.chdir ( "/minix/tests" );
      kprintfd ( "\n> chdir returned: <%d>\n",chdir_ret );

      MinixUserThread *user_thread = new MinixUserThread ( "/minix/tests/stdin-test.sweb", new FileSystemInfo ( *this->getFSInfo() ) );
      Scheduler::instance()->addNewThread ( user_thread );


      Scheduler::instance()->yield();


//       kprintfd("\n> calling umount: /minix\n");
//       int32 umount_ret = vfs_syscall.umount("/minix",0);
//       kprintfd("\n> umount returned <%d>\n",umount_ret);


//
//
//       kprintfd("\n> list ./\n");
//       vfs_syscall.readdir("./");
//
//       kprintfd("\n> chdir /minix\n");
//       int32 chdir_ret = vfs_syscall.chdir("/minix");
//       kprintfd("\n> chdir returned: <%d>\n",chdir_ret);
//
//
//       kprintfd("\n> list ./\n");
//       vfs_syscall.readdir("./");
//
//
//       kprintfd("\n> chdir ./dir\n");
//       chdir_ret = vfs_syscall.chdir("./dir");
//       kprintfd("\n> chdir returned: <%d>\n",chdir_ret);
//
//
//       kprintfd("\n> list ./\n");
//       vfs_syscall.readdir("./");


//       kprintfd("\n> list /minix/dir\n");
//       vfs_syscall.readdir("/minix/dir");
//       kprintfd("\n> list /dev\n");
//       vfs_syscall.readdir("/dev");
//       kprintfd("\n> open /minix/vbf\n");
//       int32 fd = vfs_syscall.open("/minix/vbf", 2);
//       kprintfd("\n> open returned fd: <%d>\n",fd);
//       kprintfd("\n> flush fd: <%d>\n",fd);
//       int32 flush_ret = vfs_syscall.flush(fd);
//       kprintfd("\n> flush returned: <%d>\n", flush_ret);
//
//       char *buffer = new char[5001];
//       buffer[5000] = '\0';
//
//       for(uint32 i = 0; i<5000; i++)
//         buffer[i] = 1;
//
//       int32 read_ret = vfs_syscall.read(fd, buffer, 5000);
//       kprintfd("\n> read returned: <%d>\n",read_ret);
//       for(uint32 i = 0; i<500000; i++)
//         kprintfd("%x",buffer[i]);

//       kprintfd("\n> open /minix/test1.txt\n");
//       int32 t_fd = vfs_syscall.open("/minix/test1.txt", 2);
//       kprintfd("\n> open returned fd: <%d>\n",t_fd);
//
//
//       int32 write_ret = vfs_syscall.write(t_fd, buffer, 5000);
//       kprintfd("\n> write returned: <%d>\n",write_ret);
//
//
//       char *r_buffer = new char[5001];
//       r_buffer[5000] = '\0';
//
//
//       for(uint32 i = 0; i<5000; i++)
//         r_buffer[i] = 1;
//
//       read_ret = vfs_syscall.read(t_fd, r_buffer, 5000);
//       kprintfd("\n> read returned: <%d>\n",read_ret);
//
//        for(uint32 i = 0; i<5000; i++)
//          kprintfd("%x",r_buffer[i]);


//        kprintfd("\n> calling umount: /minix\n");
//        int32 umount_ret = vfs_syscall.umount("/minix",0);
//        kprintfd("\n> umount returned <%d>\n",umount_ret);


//        kprintfd("\n> list /\n");
//        vfs_syscall.readdir("/");
//
//        kprintfd("\n> list /minix\n");
//        vfs_syscall.readdir("/minix");

//       char *t_buffer = new char[6];
//       t_buffer[5] = '\0';

//       int32 t_read_ret = vfs_syscall.read(t_fd, t_buffer, 5);
//       kprintfd("\n> read returned: <%d>\n",t_read_ret);
//       kprintfd("\n> read: <%s>\n",t_buffer);

//       kprintfd("\n> closing file <%d>\n",t_fd);
//       int32 close_ret = vfs_syscall.close(t_fd);
//       kprintfd("\n> close returned: <%d>\n",close_ret);


//       kprintfd("\n> rm /minix/test1.txt\n");
//       int32 rm_ret = vfs_syscall.rm("/minix/test1.txt");
//       kprintfd("\n> rm returned : <%d>\n",rm_ret);

//       kprintfd("\n> calling mkdir: /minix/folder\n");
//       int32 mkdir_ret = vfs_syscall.mkdir("/minix/folder", 2);
//       kprintfd("\n> mkdir returned <%d>\n",mkdir_ret);


//       kprintfd("\n> list /minix\n");
//       vfs_syscall.readdir("/minix");
//       int32 t_write_ret = vfs_syscall.write(t_fd, "hello test1!", 12);
//       kprintfd("\n> write returned: <%d>\n",t_write_ret);

//       kprintfd("\n> calling rmdir: /minix/folder\n");
//       int32 rmdir_ret = vfs_syscall.rmdir("/minix/folder");
//       kprintfd("\n> rmdir returned <%d>\n",rmdir_ret);



//       kprintfd("\n> list /minix\n");
//       vfs_syscall.readdir("/minix");

      //
    }
};

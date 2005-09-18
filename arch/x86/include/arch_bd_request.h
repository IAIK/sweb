/********************************************************************
*
*    $Id: arch_bd_request.h,v 1.2 2005/09/18 20:46:52 nelles Exp $
*    $Log: arch_bd_request.h,v $
********************************************************************/

#ifndef _BD_REQUEST_H_
#define _BD_REQUEST_H_

#include "types.h"
#include "Thread.h"

// How to use:

// Create the BDRequest object with the proper parameters,
// pass the instance of that object to the pleaseProcessRequest 
// method of the BDManager and go to sleep.
// The calling thread will be awaken when the command is 
// processed. No timeouts are implemented so if there is some 
// communication error between the BDManager and the drivers,
// the thread will be sleeping for a looooong looong time.
// Possible solution is to implement timeout in sleep in 
// scheduler.
// The second option is to make a busy wait and check the
// getStatus() method.
// Look at the BD_CMD enum for the list of possible commands.

extern Thread * currentThread;

class BDRequest
{
  public:
  
    typedef enum BD_CMD_ 
    {
      BD_READ            = 0x00,
      BD_WRITE           = 0x10,
      BD_GET_NUM_DEVICES = 0x20,
      BD_GET_BLK_SIZE    = 0x21,
      BD_GET_NUM_BLOCKS  = 0x22,
      BD_REINIT          = 0x31,
      BD_DEINIT          = 0x32
    } BD_CMD;

    typedef enum BD_RESULT_ 
    {
      BD_QUEUED          = 0x00,
      BD_DONE            = 0x20,
      BD_ERROR           = 0x40,
    } BD_RESULT;      
  
    BDRequest( uint32 dev_id, BD_CMD cmd, uint32 start_block = 0, uint32 num_block = 0, void * buffer = 0 ) 
    {
      num_block_=num_block;
      start_block_=start_block;
      dev_id_=dev_id;
      cmd_=cmd;
      result_= 0;
      status_ = BD_QUEUED;
      buffer_ = buffer;
      
      requesting_thread_ = currentThread;
      blocks_done_ = 0;
    };
   
    uint32 getDevID()            { return dev_id_; };
    BD_CMD getCmd()              { return cmd_; };
    uint32 getStartBlock()       { return start_block_; };
    uint32 getNumBlocks()        { return num_block_; };
    uint32 getResult()           { return result_; };
    BD_RESULT getStatus()        { return status_; };
    uint32 getNumBlock()         { return num_block_; };
    uint32 getBlocksDone()       { return blocks_done_; };
    void * getBuffer()           { return buffer_; };
    
    Thread * getThread()         { return requesting_thread_; };
    
    void setStartBlock( uint32 start_blk )  { start_block_=start_blk; };
    void setResult( uint32 result )         { result_=result; };
    void setStatus( BD_RESULT status )      { status_=status; };
    void setBlocksDone( uint32 bdone )      { blocks_done_=bdone; };
    
  private:
    BDRequest();

    uint32 dev_id_;
    BD_CMD cmd_;
    uint32 num_block_;
    uint32 start_block_;
    
    uint32 result_;
    BD_RESULT status_;
    
    uint32 blocks_done_;
    
    void * buffer_;
    Thread * requesting_thread_;

};

#endif

#pragma once

#include "types.h"
#include "Scheduler.h"
#include "uatomic.h"
#include "NonBlockingQueue.h"

class Thread;

class BDRequest
{
  protected:
    friend class BDVirtualDevice;
    friend class RamDiskDriver;
    friend class ATADriver;
    friend class MMCDriver;
    friend class BDManager;

    enum class BD_CMD
    {
      BD_READ            = 0x00,
      BD_WRITE           = 0x10,
      BD_GET_NUM_DEVICES = 0x20,
      BD_GET_BLK_SIZE    = 0x21,
      BD_GET_NUM_BLOCKS  = 0x22,
      BD_REINIT          = 0x31,
      BD_DEINIT          = 0x32,
      BD_SEND_RAW_CMD    = 0x40
    };

    /**
     * Enumeration containing the possible status values. I admit this is very bad
     * thing because there is a member variable named result, that has NOTHING to do
     * with these values.
     *
     */
    enum class BD_RESULT
    {
      BD_QUEUED          = 0x00,
      BD_DONE            = 0x20,
      BD_ERROR           = 0x40,
    };

    /**
     *
     * The constructor.
     * @param dev_id the id of the device
     * @param cmd command to send to the device \sa BD_CMD enum
     * @param start_block the starting block *optional* used only with read or write
     * commands
     * @param num_block the number of blocks to read or write
     * @param buffer the buffer, must be large enough to store the data, no sanity
     * checks performed, possible pagefault here
     *
     */
    BDRequest( uint32 dev_id, BD_CMD cmd, uint32 start_block = 0, uint32 num_block = 0, void * buffer = 0 ) :
        next_node_(0),
        dev_id_(dev_id),
        cmd_(cmd),
        num_block_(num_block),
        start_block_(start_block),
        result_(0),
        status_(BD_RESULT::BD_QUEUED),
        blocks_done_(0),
        buffer_(buffer),
        requesting_thread_(currentThread)
    {
    };

    uint32 getDevID()           { return dev_id_; };
    BD_CMD getCmd()             { return cmd_; };
    uint32 getStartBlock()      { return start_block_; };
    uint32 getNumBlocks()       { return num_block_; };
    uint32 getResult()          { return result_; };
    BD_RESULT getStatus()       { return status_; };
    uint32 getBlocksDone()      { return blocks_done_; };
    void *getBuffer()           { return buffer_; };
    Thread *getThread()         { return requesting_thread_; };

    void setStartBlock( uint32 start_blk ) { start_block_  = start_blk; };
    void setResult( uint32 result )        { result_       = result;    };
    void setStatus( BD_RESULT status )     { status_       = status;    };
    void setBlocksDone( uint32 bdone )     { blocks_done_  = bdone;     };
    void setNumBlocks(uint32 num_block)    { num_block_    = num_block; };


    friend class NonBlockingQueue<BDRequest>;
    ustl::atomic<BDRequest*> next_node_;

  private:
    BDRequest();

    uint32 dev_id_;
    BD_CMD cmd_;
    uint32 num_block_;
    uint32 start_block_;
    uint32 result_;
    BD_RESULT status_;
    uint32 blocks_done_;
    void *buffer_;
    Thread *requesting_thread_;
};

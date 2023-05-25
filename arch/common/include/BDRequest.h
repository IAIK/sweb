#pragma once

#include "NonBlockingQueue.h"
#include "Scheduler.h"

#include "types.h"

#include "EASTL/atomic.h"

class Thread;

class BDRequest
{
  protected:
    friend class BDVirtualDevice;
    friend class RamDiskDriver;
    friend class ATADrive;
    friend class MMCDrive;
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
    BDRequest(uint32 dev_id,
              BD_CMD cmd,
              uint32 start_block = 0,
              uint32 num_block = 0,
              void* buffer = nullptr) :
        request_id(++request_counter),
        dev_id_(dev_id),
        cmd_(cmd),
        num_block_(num_block),
        start_block_(start_block),
        buffer_(buffer),
        requesting_thread_(currentThread)
    {
    }

    uint32 getDevID() const      { return dev_id_; }
    BD_CMD getCmd() const        { return cmd_; }
    uint32 getStartBlock() const { return start_block_; }
    uint32 getNumBlocks() const  { return num_block_; }
    uint32 getResult() const     { return result_; }
    BD_RESULT getStatus() const  { return status_; }
    uint32 getBlocksDone() const { return blocks_done_; }
    void *getBuffer() const      { return buffer_; }
    Thread *getThread() const    { return requesting_thread_; }

    void setStartBlock( uint32 start_blk ) { start_block_  = start_blk; }
    void setResult( uint32 result )        { result_       = result;    }
    void setStatus( BD_RESULT status )     { status_       = status;    }
    void setBlocksDone( uint32 bdone )     { blocks_done_  = bdone;     }
    void setNumBlocks(uint32 num_block)    { num_block_    = num_block; }


    friend class NonBlockingQueue<BDRequest>;
    eastl::atomic<BDRequest*> next_node_ = nullptr;

    inline static eastl::atomic<size_t> request_counter = 0;
    size_t request_id;

  private:
    BDRequest();

    uint32 dev_id_;
    BD_CMD cmd_;
    uint32 num_block_;
    uint32 start_block_;
    uint32 result_ = 0;
    BD_RESULT status_ = BD_RESULT::BD_QUEUED;
    uint32 blocks_done_ = 0;
    void* buffer_;
    Thread *requesting_thread_;
};

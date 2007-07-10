// Projectname: SWEB
// Simple operating system for educational purposes
//
// Copyright (C) 2005  Nebojsa Simic 
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

/********************************************************************
*
*    $Id: arch_bd_request.h,v 1.4 2005/11/20 21:18:08 nelles Exp $
*    $Log: arch_bd_request.h,v $
*    Revision 1.3  2005/09/20 21:14:31  nelles
*
*
*    Some comments added
*
*     ----------------------------------------------------------------------
*
*    Revision 1.2  2005/09/18 20:46:52  nelles
*
*     Committing in .
*
*     Modified Files:
*     	arch/x86/include/arch_bd_ata_driver.h
*     	arch/x86/include/arch_bd_ide_driver.h
*     	arch/x86/include/arch_bd_manager.h
*     	arch/x86/include/arch_bd_request.h
*     	arch/x86/include/arch_bd_virtual_device.h
*     	arch/x86/source/arch_bd_ata_driver.cpp
*     	arch/x86/source/arch_bd_ide_driver.cpp
*     	arch/x86/source/arch_bd_manager.cpp
*     	arch/x86/source/arch_bd_virtual_device.cpp
*     ----------------------------------------------------------------------
*
********************************************************************/

#ifndef _BD_REQUEST_H_
#define _BD_REQUEST_H_

#include "types.h"
#include "Thread.h"


// \class BDRequest
// \brief Class containing command and parameters to pass to the BDManager.
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
  
    /// Enumeration containing the possible commands
    typedef enum BD_CMD_ 
    {
      BD_READ            = 0x00,
      BD_WRITE           = 0x10,
      BD_GET_NUM_DEVICES = 0x20,
      BD_GET_BLK_SIZE    = 0x21,
      BD_GET_NUM_BLOCKS  = 0x22,
      BD_REINIT          = 0x31,
      BD_DEINIT          = 0x32,
      BD_SEND_RAW_CMD    = 0x40
    } BD_CMD;

    /// Enumeration containing the possible status values. I admit this is very bad thing because there is a member variable named result, that has NOTHING to do with these values.
    typedef enum BD_RESULT_ 
    {
      BD_QUEUED          = 0x00,
      BD_DONE            = 0x20,
      BD_ERROR           = 0x40,
    } BD_RESULT;      
  
    /// The constructor.
    /// \param dev_id the id of the device
    /// \param cmd command to send to the device \sa BD_CMD enum
    /// \param start_block the starting block *optional* used only with read or write commands
    /// \param num_block the number of blocks to read or write
    /// \param buffer the buffer, must be large enough to store the data, no sanity checks performed, possible pagefault here
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
	  next_request_ = 0;
    };
   
    /// returns the id of the device
    uint32 getDevID()            { return dev_id_; };
    /// returns the command
    BD_CMD getCmd()              { return cmd_; };
    /// returns the starting block
    uint32 getStartBlock()       { return start_block_; };
    /// returns the number of blocks
    uint32 getNumBlocks()        { return num_block_; };
    /// returns the result of the operation 
    uint32 getResult()           { return result_; };
    /// returns the status of the operation \sa BD_RESULT
    BD_RESULT getStatus()        { return status_; };
    /// returns the number of the blocks
    uint32 getNumBlock()         { return num_block_; };
    /// returns the number of the blocks already read/written *do not use unless you know EXACTLY what you are doing* - heavy sync issues can come up.
    uint32 getBlocksDone()       { return blocks_done_; };
    /// returns the buffer
    void * getBuffer()           { return buffer_; };
    
    /// returns the thread that created this request object
    Thread * getThread()         { return requesting_thread_; };
	
	BDRequest * getNextRequest(  )		{ return next_request_; };
    
    /// sets the start block of this request
    void setStartBlock( uint32 start_blk )  { start_block_=start_blk; };
    /// sets the start block of this request
    void setResult( uint32 result )         { result_=result; };
    /// sets the status of this request
    void setStatus( BD_RESULT status )      { status_=status; };
    /// sets the the number of the blocks already read/written \sa getBlocksDone
    void setBlocksDone( uint32 bdone )      { blocks_done_=bdone; };
    /// sets the the next request in the linked list
    void setNextRequest( BDRequest *next )  { next_request_=next; };
        
    void setNumBlocks(uint32 num_block)      { num_block_ = num_block; };
    
  private:
    BDRequest();  ///< default constructor, must never be called

    uint32 dev_id_;  ///< id of the device to which this request is made
    BD_CMD cmd_;    ///< command to be sent to the device
    uint32 num_block_;  ///< number of blocks affected by the command
    uint32 start_block_; ///< block to start from
    
    uint32 result_;    ///< result of the operation, zB. the number of blocks in a device 
    BD_RESULT status_; ///< status of the operation \sa BD_RESULT
    
    uint32 blocks_done_; ///< how many blocks have been processed by interrupt handler.
    
    void * buffer_;     ///< Name says it all.
    Thread * requesting_thread_; ///< Thread that created the object

    BDRequest *next_request_;  ///< next_request in the linked list
};

#endif

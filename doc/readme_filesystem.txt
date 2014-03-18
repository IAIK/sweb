********************************************************************************
						Readme FileSystem implement
********************************************************************************

----------------------------------------
HOW TOs
----------------------------------------
 How To mount a vmdk image under linux to the virtual file-system?
 Solution: using loop-backs
 	losetup /dev/loop[n] SWEB-flat.vmdk
	losetup -o [offset] /dev/loop[n+1] /dev/loop[n]
	mount /dev/loop[n+1] /mnt/sweb-image
	[offset] specifies the byte-offset from the image's start
	where the partition desired to be mounted begins
	
	mount -t fs dev dir
	eg: sudo mount -t minix /dev/loop6 /mnt

How to create a raw virtual disk image?
	1. create a new raw image using qemu-img
	2. parted: new disk label
	3. creating a new partition
	4. creating a new file-system on the given partition
	5. repeat steps 3+4 as much as needed
	
	qemu-img create -f raw image-name.img size[K|M|G|T]
	parted image-name.img mklabel label-type
	parted image-name.img mkpart part-type [fs-type] start end
	parted image-name.img mkfs minor fs-type

----------------------------------------
1. Requirements
----------------------------------------
+ Basic FS-Operations (open, read, write, ...)
+ an open generic design
	compact design that allows to integrate many
	new features and (exotic) file-systems
+ Caching strategies
+ FileSystem-auto detection
+ Journaling / FS consitency
+ Hooks for
	+ Time-System calls (creat, mod, access-times)
	+ User rights / ACLs
	+ Compression
	+ Encryption
	+ Backups
	+ Defragementation

----------------------------------------
2. FileSystem Syscalls
----------------------------------------

  a. General
  	mount()
  	unmount()
  	
  b. File related
  	creat()
  	open()
  	read()
  	write()
  	lseek()
  	close()
  	
  	stat()
  	fstat()
  	pipe()
  	fcntl()
  	
  c. Directory
  	mdkir()
  	rmdir()
  	link()
  	unlink()
  	
  	chdir() / fchdir()
  	getcwd() / getwd() / get_current_dir_name()
  	
  	opendir()
  	readdir() - getting Directory infos
  	rewinddir()
  	closedir()

----------------------------------------
2. Bugs
----------------------------------------

----------------------------------------
3. TODOs
----------------------------------------

	* ATA-Driver has serious PROBLEMS !!!
	* mem-leak: !! deleting path-strings at function return !!
	* sweb-img-util - format is very dirty implemented (if - else if - else if ... )
	* recursivly unmounting old mounted fs
	* Data-Race Testcases
	* FAT (16/32/VFAT) Implementation
	* Devices as Files
	* Performance measurements
	* optimizing the Cache-Size and finding the best working
	  strategries
	- some hooks
		time, compression, crypto
	- Unicode and Multi-byte char support...

----------------------------------------
4. SWEB changes (outside of the FS)
----------------------------------------
	Changes affecting SWEB - other than the old file-system.
	* all classes outside of the FileSystem that were derived from Inode
	  last this base-class (e.g. chardev, BDVirtualDevice)
	* BDVirtualDevice new member partition_type_ and getter/setter for
	  this member
	* - 


----------------------------------------
5. KU-taks
----------------------------------------
	* Cache-Strategy / PRA
	* Reader/Writer Lock
	* Timestamps
	* new Filesystem

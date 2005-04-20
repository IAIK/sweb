/*
 * mformat.c
 */

#define DONT_NEED_WAIT

#include "sysincludes.h"
#include "msdos.h"
#include "mtools.h"
#include "mainloop.h"
#include "fsP.h"
#include "file.h"
#include "plain_io.h"
#include "floppyd_io.h"
#include "nameclash.h"
#include "buffer.h"
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef USE_XDF
#include "xdf_io.h"
#endif
#include "partition.h"

#ifndef abs
#define abs(x) ((x)>0?(x):-(x))
#endif

#ifdef OS_linux
#include "linux/hdreg.h"

#define _LINUX_STRING_H_
#define kdev_t int
#include "linux/fs.h"
#undef _LINUX_STRING_H_

#endif


extern int errno;

static int init_geometry_boot(struct bootsector *boot, struct device *dev,
			       int sectors0, int rate_0, int rate_any,
			       unsigned long *tot_sectors, int keepBoot)
{
	int i;
	int nb_renum;
	int sector2;
	int size2;
	int j;
	int sum;

	set_word(boot->nsect, dev->sectors);
	set_word(boot->nheads, dev->heads);

	*tot_sectors = dev->heads * dev->sectors * dev->tracks - DWORD(nhs);

	if (*tot_sectors < 0x10000){
		set_word(boot->psect, *tot_sectors);
		set_dword(boot->bigsect, 0);
	} else {
		set_word(boot->psect, 0);
		set_dword(boot->bigsect, *tot_sectors);
	}

	if (dev->use_2m & 0x7f){
		int bootOffset;
		strncpy(boot->banner, "2M-STV04", 8);
		boot->ext.old.res_2m = 0;
		boot->ext.old.fmt_2mf = 6;
		if ( dev->sectors % ( ((1 << dev->ssize) + 3) >> 2 ))
			boot->ext.old.wt = 1;
		else
			boot->ext.old.wt = 0;
		boot->ext.old.rate_0= rate_0;
		boot->ext.old.rate_any= rate_any;
		if (boot->ext.old.rate_any== 2 )
			boot->ext.old.rate_any= 1;
		i=76;

		/* Infp0 */
		set_word(boot->ext.old.Infp0, i);
		boot->jump[i++] = sectors0;
		boot->jump[i++] = 108;
		for(j=1; j<= sectors0; j++)
			boot->jump[i++] = j;

		set_word(boot->ext.old.InfpX, i);
		
		boot->jump[i++] = 64;
		boot->jump[i++] = 3;
		nb_renum = i++;
		sector2 = dev->sectors;
		size2 = dev->ssize;
		j=1;
		while( sector2 ){
			while ( sector2 < (1 << size2) >> 2 )
				size2--;
			boot->jump[i++] = 128 + j;
			boot->jump[i++] = j++;
			boot->jump[i++] = size2;
			sector2 -= (1 << size2) >> 2;
		}
		boot->jump[nb_renum] = ( i - nb_renum - 1 ) / 3;

		set_word(boot->ext.old.InfTm, i);

		sector2 = dev->sectors;
		size2= dev->ssize;
		while(sector2){
			while ( sector2 < 1 << ( size2 - 2) )
				size2--;
			boot->jump[i++] = size2;
			sector2 -= 1 << (size2 - 2 );
		}
		
		set_word(boot->ext.old.BootP,i);
		bootOffset = i;

		/* checksum */		
		for (sum=0, j=64; j<i; j++) 
			sum += boot->jump[j];/* checksum */
		boot->ext.old.CheckSum=-sum;
		return bootOffset;
	} else {
		if(!keepBoot) {
			boot->jump[0] = 0xeb;
			boot->jump[1] = 0;
			boot->jump[2] = 0x90;
			strncpy(boot->banner, "MTOOL399", 8);
			/* It looks like some versions of DOS are
			 * rather picky about this, and assume default
			 * parameters without this, ignoring any
			 * indication about cluster size et al. */
		}
		return 0;
	}
}


static int comp_fat_bits(Fs_t *Fs, int estimate, 
			 unsigned long tot_sectors, int fat32)
{
	int needed_fat_bits;

	needed_fat_bits = 12;

#define MAX_DISK_SIZE(bits,clusters) \
	TOTAL_DISK_SIZE((bits), Fs->sector_size, (clusters), \
			Fs->num_fat, MAX_BYTES_PER_CLUSTER/Fs->sector_size)

	if(tot_sectors > MAX_DISK_SIZE(12, FAT12-1))
		needed_fat_bits = 16;
	if(fat32 || tot_sectors > MAX_DISK_SIZE(16, FAT16-1))
		needed_fat_bits = 32;

#undef MAX_DISK_SIZE

	if(abs(estimate) && abs(estimate) < needed_fat_bits) {
		if(fat32) {
			fprintf(stderr,
				"Contradiction between FAT size on command line and FAT size in conf file\n");
			exit(1);
		}
		fprintf(stderr,
			"Device too big for a %d bit FAT\n",
			estimate);
		exit(1);
	}

	if(!estimate) {
		unsigned int min_fat16_size;

		if(needed_fat_bits > 12)
			return needed_fat_bits;
		min_fat16_size = DISK_SIZE(16, Fs->sector_size, FAT12,
					   Fs->num_fat, 1);
		if(tot_sectors < min_fat16_size)
			return 12;
		else if(tot_sectors >= 2* min_fat16_size)
			return 16; /* heuristics */
	}

	return estimate;
}


/*
 * According to Microsoft "Hardware White Paper", "Microsoft
 * Extensible Formware Initiative", "FAT32 File System Specification",
 * Version 1.03, December 6, 2000:
 * If (CountofClusters < 4085) { 
 *  // Volume is FAT12  
 * } else if (CountofClusters < 65525) { 
 *  // Volume is FAT16
 * } else { 
 *  //Volume is FAT32
 * }
 *
 * This document can be found at the following URL
 * http://www.microsoft.com/hwdev/download/hardware/fatgen103.pdf
 * The relevant passus is on page 15.
 *
 * Actually, experimentations with Windows NT 4 show that the
 * cutoff is 4087 rather than 4085... This is Microsoft after all.
 * Not sure what the other Microsoft OS'es do though...
 */
static void calc_fat_bits2(Fs_t *Fs, unsigned long tot_sectors, int fat_bits,
			   int may_change_cluster_size, 
			   int may_change_root_size)
{
	unsigned long rem_sect;

	/*
	 * the "remaining sectors" after directory and boot
	 * hasve been accounted for.
	 */
	rem_sect = tot_sectors - Fs->dir_len - Fs->fat_start;
	switch(abs(fat_bits)) {
		case 0:

#define MY_DISK_SIZE(bits,clusters) \
			DISK_SIZE( (bits), Fs->sector_size, (clusters), \
				   Fs->num_fat, Fs->cluster_size)

			if(rem_sect >= MY_DISK_SIZE(16, FAT12+2))
				/* big enough for FAT16 
				 * We take a margin of 2, because NT4 
				 * misbehaves, and starts considering a disk
				 * as FAT16 only if it is larger than 4086
				 * sectors, rather than 4084 as it should
				 */
				set_fat16(Fs);
			else if(rem_sect <= MY_DISK_SIZE(12, FAT12-1))
				 /* small enough for FAT12 */
				 set_fat12(Fs);
			else {
				/* "between two chairs",
				 * augment cluster size, and
				 * settle it */
				if(may_change_cluster_size && 
				   Fs->cluster_size * Fs->sector_size * 2 
				   <= MAX_BYTES_PER_CLUSTER)
					Fs->cluster_size <<= 1;
				else if(may_change_root_size) {
					Fs->dir_len += 
						rem_sect - MY_DISK_SIZE(12, FAT12-1);
				}
				set_fat12(Fs);
			}
			break;
#undef MY_DISK_SIZE

		case 12:
			set_fat12(Fs);
			break;
		case 16:
			set_fat16(Fs);
			break;
		case 32:
			set_fat32(Fs);
			break;
	}
}

static __inline__ void format_root(Fs_t *Fs, char *label, struct bootsector *boot)
{
	Stream_t *RootDir;
	char *buf;
	int i;
	struct ClashHandling_t ch;
	int dirlen;

	init_clash_handling(&ch);
	ch.name_converter = label_name;
	ch.ignore_entry = -2;

	buf = safe_malloc(Fs->sector_size);
	RootDir = OpenRoot((Stream_t *)Fs);
	if(!RootDir){
		fprintf(stderr,"Could not open root directory\n");
		exit(1);
	}

	memset(buf, '\0', Fs->sector_size);

	if(Fs->fat_bits == 32) {
		/* on a FAT32 system, we only write one sector,
		 * as the directory can be extended at will...*/
		dirlen = Fs->cluster_size;
		fatAllocate(Fs, Fs->rootCluster, Fs->end_fat);
	} else
		dirlen = Fs->dir_len; 
	for (i = 0; i < dirlen; i++)
		WRITES(RootDir, buf, sectorsToBytes((Stream_t*)Fs, i),  
			   Fs->sector_size);

	ch.ignore_entry = 1;
	if(label[0])
		mwrite_one(RootDir,label, 0, labelit, NULL,&ch);

	FREE(&RootDir);
	if(Fs->fat_bits == 32)
		set_word(boot->dirents, 0);
	else
		set_word(boot->dirents, Fs->dir_len * (Fs->sector_size / 32));
	free(buf);
}


static void xdf_calc_fat_size(Fs_t *Fs, unsigned long tot_sectors, 
			      int fat_bits)
{
	unsigned int rem_sect;

	rem_sect = tot_sectors - Fs->dir_len - Fs->fat_start - 2 * Fs->fat_len;

	if(Fs->fat_len) {
		/* an XDF disk, we know the fat_size and have to find
		 * out the rest. We start with a cluster size of 1 and
		 * keep doubling until everything fits into the
		 * FAT. This will occur eventually, as our FAT has a
		 * minimal size of 1 */
		for(Fs->cluster_size = 1; 1 ; Fs->cluster_size <<= 1) {
			Fs->num_clus = rem_sect / Fs->cluster_size;
			if(abs(fat_bits) == 16 || Fs->num_clus >= FAT12)
				set_fat16(Fs);
			else
				set_fat12(Fs);
			if (Fs->fat_len >= NEEDED_FAT_SIZE(Fs))
				return;
		}
	}
	fprintf(stderr,"Internal error while calculating Xdf fat size\n");
	exit(1);
}


static void calc_fat_size(Fs_t *Fs, unsigned long tot_sectors)
{
	unsigned long rem_sect;
	unsigned long real_rem_sect;
	unsigned long numerator;
	unsigned long denominator;
	int fat_nybbles;
	unsigned int slack;
	int printGrowMsg=1; /* Should we print "growing FAT" messages ?*/
	
#if DEBUG
	fprintf(stderr, "Fat start=%d\n", Fs->fat_start);
	fprintf(stderr, "tot_sectors=%lu\n", tot_sectors);
	fprintf(stderr, "dir_len=%d\n", Fs->dir_len);
#endif
	real_rem_sect = rem_sect = tot_sectors - Fs->dir_len - Fs->fat_start;

	/* Cheat a little bit to address the _really_ common case of
	   odd number of remaining sectors while both nfat and cluster size
	   are even... */
	if(rem_sect         %2 == 1 &&
	   Fs->num_fat      %2 == 0 &&
	   Fs->cluster_size %2 == 0)
		rem_sect--;

#if DEBUG
	fprintf(stderr, "Rem sect=%lu\n", rem_sect);
#endif

	if(Fs->fat_bits == 0) {
		fprintf(stderr, "Weird, fat bits = 0\n");
		exit(1);		
	}


	/* See fat_size_calculation.tex or
	   (http://www.mtools.linux.lu/fat_size_calculation.pdf) for an
	   explantation about why the stuff below works...
	*/

	fat_nybbles = Fs->fat_bits / 4;
	numerator   = rem_sect+2*Fs->cluster_size;
	denominator = 
	  Fs->cluster_size * Fs->sector_size * 2 +
	  Fs->num_fat * fat_nybbles;

	if(fat_nybbles == 3)
		numerator *= fat_nybbles;
	else
		/* Avoid numerical overflows, divide the denominator
		 * rather than multiplying the numerator */
		denominator = denominator / fat_nybbles;

#if DEBUG
	fprintf(stderr, "Numerator=%lu denominator=%lu\n",
		numerator, denominator);
#endif

	Fs->fat_len = (numerator-1)/denominator+1;
	Fs->num_clus = (rem_sect-(Fs->fat_len*Fs->num_fat))/Fs->cluster_size;

	/* Apply upper bounds for FAT bits */
	if(Fs->fat_bits == 16 && Fs->num_clus >= FAT16)
		Fs->num_clus = FAT16-1;
	if(Fs->fat_bits == 12 && Fs->num_clus >= FAT12)
		Fs->num_clus = FAT12-1;
	
	/* A safety, if above math is correct, this should not be happen...*/
	if(Fs->num_clus > (Fs->fat_len * Fs->sector_size * 2 / 
			   fat_nybbles - 2)) {
		fprintf(stderr, 
			"Fat size miscalculation, shrinking num_clus from %d ",
			Fs->num_clus);
		Fs->num_clus = (Fs->fat_len * Fs->sector_size * 2 / 
				fat_nybbles - 2);
		fprintf(stderr, " to %d\n", Fs->num_clus);
	}
#if DEBUG
	fprintf(stderr, "Num_clus=%d fat_len=%d nybbles=%d\n",
		Fs->num_clus, Fs->fat_len, fat_nybbles);
#endif

	if ( Fs->num_clus < FAT16 && Fs->fat_bits > 16 ){
		fprintf(stderr,"Too few clusters for this fat size."
			" Please choose a 16-bit fat in your /etc/mtools.conf"
			" or .mtoolsrc file\n");
		exit(1);
	}

	/* As the number of clusters is specified nowhere in the boot sector,
	 * it will be calculated by removing everything else from total number
	 * of sectors. This means that if we reduced the number of clusters
	 * above, we will have to grow the FAT in order to take up any excess
	 * sectors... */
#ifdef HAVE_ASSERT_H
	assert(rem_sect >= Fs->num_clus * Fs->cluster_size + 
	       Fs->fat_len * Fs->num_fat);
#endif
	slack = rem_sect - 
		Fs->num_clus * Fs->cluster_size - 
		Fs->fat_len * Fs->num_fat;
	if(slack >= Fs->cluster_size) {
		/* This can happen under two circumstances:
		   1. We had to reduce num_clus because we reached maximum
		   number of cluster for FAT12 or FAT16
		*/
		if(printGrowMsg) {
			fprintf(stderr, "Slack=%d\n", slack);
			fprintf(stderr, "Growing fat size from %d", 
				Fs->fat_len);
		}
		Fs->fat_len += 
			(slack - Fs->cluster_size) / Fs->num_fat + 1;
		if(printGrowMsg) {
			fprintf(stderr,
				" to %d in order to take up excess cluster area\n",
				Fs->fat_len);
		}
		Fs->num_clus = (rem_sect-(Fs->fat_len*Fs->num_fat))/
			Fs->cluster_size;

	}

#ifdef HAVE_ASSERT_H
	/* Fat must be big enough for all clusters */
	assert( ((Fs->num_clus+2) * fat_nybbles) <= 
		(Fs->fat_len*Fs->sector_size*2));

	/* num_clus must be big enough to cover rest of disk, or else further
	 * users of the filesystem will assume a bigger num_clus, which might
	 * be too big for fat_len */
	assert(Fs->num_clus ==
	       (real_rem_sect - Fs->num_fat * Fs->fat_len) / Fs->cluster_size);
#endif
}


static unsigned char bootprog[]=
{0xfa, 0x31, 0xc0, 0x8e, 0xd8, 0x8e, 0xc0, 0xfc, 0xb9, 0x00, 0x01,
 0xbe, 0x00, 0x7c, 0xbf, 0x00, 0x80, 0xf3, 0xa5, 0xea, 0x00, 0x00,
 0x00, 0x08, 0xb8, 0x01, 0x02, 0xbb, 0x00, 0x7c, 0xba, 0x80, 0x00,
 0xb9, 0x01, 0x00, 0xcd, 0x13, 0x72, 0x05, 0xea, 0x00, 0x7c, 0x00,
 0x00, 0xcd, 0x19};

static __inline__ void inst_boot_prg(struct bootsector *boot, int offset)
{
	memcpy((char *) boot->jump + offset, 
	       (char *) bootprog, sizeof(bootprog) /sizeof(bootprog[0]));
	if(offset - 2 < 0x80) {
	  /* short jump */
	  boot->jump[0] = 0xeb;
	  boot->jump[1] = offset -2;
	  boot->jump[2] = 0x90;
	} else {
	  /* long jump, if offset is too large */
	  boot->jump[0] = 0xe9;
	  boot->jump[1] = offset -3;
	  boot->jump[2] = 0x00;
	}
	set_word(boot->jump + offset + 20, offset + 24);
}

static void calc_cluster_size(struct Fs_t *Fs, unsigned long tot_sectors,
			      int fat_bits)
			      
{
	unsigned int max_clusters; /* maximal possible number of sectors for
				   * this FAT entry length (12/16/32) */
	unsigned int max_fat_size; /* maximal size of the FAT for this FAT
				    * entry length (12/16/32) */
	unsigned int rem_sect; /* remaining sectors after we accounted for
				* the root directory and boot sector(s) */

	switch(abs(fat_bits)) {
		case 12:			
			max_clusters = FAT12-1;
			max_fat_size = Fs->num_fat * 
				FAT_SIZE(12, Fs->sector_size, max_clusters);
			break;
		case 16:
		case 0: /* still hesititating between 12 and 16 */
			max_clusters = FAT16-1;
			max_fat_size = Fs->num_fat * 
				FAT_SIZE(16, Fs->sector_size, max_clusters);
			break;
		case 32:		  
			Fs->cluster_size = 8;
			/* According to
			 * http://support.microsoft.com/support/kb/articles/q154/9/97.asp
			 * Micro$oft does not support FAT32 with less than 4K
			 */
			return;
		default:
			fprintf(stderr,"Bad fat size\n");
			exit(1);
	}

	rem_sect = tot_sectors - Fs->dir_len - Fs->fat_start;

	/* double the cluster size until we can fill up the disk with
	 * the maximal number of sectors of this size */
	while(Fs->cluster_size * max_clusters  + max_fat_size < rem_sect) {
		if(Fs->cluster_size > 64) {
			/* bigger than 64. Should fit */
			fprintf(stderr,
				"Internal error while calculating cluster size\n");
			exit(1);
		}
		Fs->cluster_size <<= 1;
	}
}


struct OldDos_t old_dos[]={
{   40,  9,  1, 4, 1, 2, 0xfc },
{   40,  9,  2, 7, 2, 2, 0xfd },
{   40,  8,  1, 4, 1, 1, 0xfe },
{   40,  8,  2, 7, 2, 1, 0xff },
{   80,  9,  2, 7, 2, 3, 0xf9 },
{   80, 15,  2,14, 1, 7, 0xf9 },
{   80, 18,  2,14, 1, 9, 0xf0 },
{   80, 36,  2,15, 2, 9, 0xf0 },
{    1,  8,  1, 1, 1, 1, 0xf0 },
};

static int old_dos_size_to_geom(size_t size, int *cyls, int *heads, int *sects)
{
	unsigned int i;
	size = size * 2;
	for(i=0; i < sizeof(old_dos) / sizeof(old_dos[0]); i++){
		if (old_dos[i].sectors * 
		    old_dos[i].tracks * 
		    old_dos[i].heads == size) {
			*cyls = old_dos[i].tracks;
			*heads = old_dos[i].heads;
			*sects = old_dos[i].sectors;
			return 0;
		}
	}
	return 1;
}


static void calc_fs_parameters(struct device *dev, unsigned long tot_sectors,
			       struct Fs_t *Fs, struct bootsector *boot)
{
	unsigned int i;

	for(i=0; i < sizeof(old_dos) / sizeof(old_dos[0]); i++){
		if (dev->sectors == old_dos[i].sectors &&
		    dev->tracks == old_dos[i].tracks &&
		    dev->heads == old_dos[i].heads &&
		    (dev->fat_bits == 0 || abs(dev->fat_bits) == 12) &&
		    (Fs->dir_len == 0 || Fs->dir_len == old_dos[i].dir_len) &&
		    (Fs->cluster_size == 0 || 
		     Fs->cluster_size == old_dos[i].cluster_size)) {
			boot->descr = old_dos[i].media;
			Fs->cluster_size = old_dos[i].cluster_size;
			Fs->dir_len = old_dos[i].dir_len;
			Fs->fat_len = old_dos[i].fat_len;
			Fs->fat_bits = 12;
			break;
		}
	}
	if (i == sizeof(old_dos) / sizeof(old_dos[0]) ){
		int may_change_cluster_size = (Fs->cluster_size == 0);
		int may_change_root_size = (Fs->dir_len == 0);

		/* a non-standard format */
		if(DWORD(nhs))
			boot->descr = 0xf8;
		  else
			boot->descr = 0xf0;


		if(!Fs->cluster_size) {
			if (dev->heads == 1)
				Fs->cluster_size = 1;
			else {
				Fs->cluster_size = (tot_sectors > 2000 ) ? 1:2;
				if (dev->use_2m & 0x7f)
					Fs->cluster_size = 1;
			}
		}
		
		if(!Fs->dir_len) {
			if (dev->heads == 1)
				Fs->dir_len = 4;
			else
				Fs->dir_len = (tot_sectors > 2000) ? 32 : 7;
		}			

		calc_cluster_size(Fs, tot_sectors, dev->fat_bits);
		if(Fs->fat_len)
			xdf_calc_fat_size(Fs, tot_sectors, dev->fat_bits);
		else {
			calc_fat_bits2(Fs, tot_sectors, dev->fat_bits,
				       may_change_cluster_size,
				       may_change_root_size);
			calc_fat_size(Fs, tot_sectors);
		}
	}

	set_word(boot->fatlen, Fs->fat_len);
}



static void calc_fs_parameters_32(unsigned long tot_sectors,
				  struct Fs_t *Fs, struct bootsector *boot)
{
	if(DWORD(nhs))
		boot->descr = 0xf8;
	else
		boot->descr = 0xf0;
	if(!Fs->cluster_size)
		/* According to
		 * http://www.microsoft.com/kb/articles/q154/9/97.htm,
		 * Micro$oft does not support FAT32 with less than 4K
		 */
		Fs->cluster_size = 8;
	
	Fs->dir_len = 0;
	Fs->num_clus = tot_sectors / Fs->cluster_size;
	set_fat32(Fs);
	calc_fat_size(Fs, tot_sectors);
	set_word(boot->fatlen, 0);
	set_dword(boot->ext.fat32.bigFat, Fs->fat_len);
}




static void usage(void)
{
	fprintf(stderr, 
		"Mtools version %s, dated %s\n", mversion, mdate);
	fprintf(stderr, 
		"Usage: %s [-V] [-t tracks] [-h heads] [-n sectors] "
		"[-v label] [-1] [-4] [-8] [-f size] "
		"[-N serialnumber] "
		"[-k] [-B bootsector] [-r root_dir_len] [-L fat_len] "
		"[-F] [-I fsVersion] [-C] [-c cluster_size] "
		"[-H hidden_sectors] "
#ifdef USE_XDF
		"[-X] "
#endif
		"[-S hardsectorsize] [-M softsectorsize] [-3] "
		"[-2 track0sectors] [-0 rate0] [-A rateany] [-a]"
		"device\n", progname);
	exit(1);
}

void mformat(int argc, char **argv, int dummy)
{
	int r; /* generic return value */
	Fs_t Fs;
	int hs, hs_set;
	int arguse_2m = 0;
	int sectors0=18; /* number of sectors on track 0 */
	int create = 0;
	int rate_0, rate_any;
	int mangled;
	int argssize=0; /* sector size */
	int msize=0;
	int fat32 = 0;
	struct label_blk_t *labelBlock;
	int bootOffset;

#ifdef USE_XDF
	unsigned int i;
	int format_xdf = 0;
	struct xdf_info info;
#endif
	struct bootsector *boot;
	char *bootSector=0;
	int c;
	int keepBoot = 0;
	struct device used_dev;
	int argtracks, argheads, argsectors;
	unsigned long tot_sectors;
	int blocksize;

	char drive, name[EXPAND_BUF];

	char label[VBUFSIZE], buf[MAX_SECTOR], shortlabel[13];
	struct device *dev;
	char errmsg[200];

	unsigned long serial;
 	int serial_set;
	int fsVersion;
	int mediaDesc=-1;

	mt_off_t maxSize;

	int Atari = 0; /* should we add an Atari-style serial number ? */
 
	hs = hs_set = 0;
	argtracks = 0;
	argheads = 0;
	argsectors = 0;
	arguse_2m = 0;
	argssize = 0x2;
	label[0] = '\0';
	serial_set = 0;
	serial = 0;
	fsVersion = 0;
	
	Fs.cluster_size = 0;
	Fs.refs = 1;
	Fs.dir_len = 0;
	if(getenv("MTOOLS_DIR_LEN")) {
	  Fs.dir_len = atoi(getenv("MTOOLS_DIR_LEN"));
	  if(Fs.dir_len <= 0)
	    Fs.dir_len=0;
	}
	Fs.fat_len = 0;
	Fs.num_fat = 2;
	if(getenv("MTOOLS_NFATS")) {
	  Fs.num_fat = atoi(getenv("MTOOLS_NFATS"));
	  if(Fs.num_fat <= 0)
	    Fs.num_fat=2;
	}
	Fs.Class = &FsClass;	
	rate_0 = mtools_rate_0;
	rate_any = mtools_rate_any;

	/* get command line options */
	while ((c = getopt(argc,argv,
			   "i:148f:t:n:v:qub"
			   "kB:r:L:IFCc:Xh:s:l:N:H:M:S:2:30:Aad:m:"))!= EOF) {
		switch (c) {
			case 'i':
				set_cmd_line_image(optarg, 0);
				break;

			/* standard DOS flags */
			case '1':
				argheads = 1;
				break;
			case '4':
				argsectors = 9;
				argtracks = 40;
				break;
			case '8':
				argsectors = 8;
				argtracks = 40;
				break;
			case 'f':
				r=old_dos_size_to_geom(atoi(optarg),
						       &argtracks, &argheads,
						       &argsectors);
				if(r) {
					fprintf(stderr, 
						"Bad size %s\n", optarg);
					exit(1);
				}
				break;
			case 't':
				argtracks = atoi(optarg);
				break;

			case 'n': /*non-standard*/
			case 's':
				argsectors = atoi(optarg);
				break;

			case 'l': /* non-standard */
			case 'v':
				strncpy(label, optarg, VBUFSIZE-1);
				label[VBUFSIZE-1] = '\0';
				break;

			/* flags supported by Dos but not mtools */
			case 'q':
			case 'u':
			case 'b':
			/*case 's': leave this for compatibility */
				fprintf(stderr, 
					"Flag %c not supported by mtools\n",c);
				exit(1);
				


			/* flags added by mtools */
			case 'F':
				fat32 = 1;
				break;


			case 'S':
				argssize = atoi(optarg) | 0x80;
				if(argssize < 0x81)
					usage();
				break;

#ifdef USE_XDF
			case 'X':
				format_xdf = 1;
				break;
#endif

			case '2':
				arguse_2m = 0xff;
				sectors0 = atoi(optarg);
				break;
			case '3':
				arguse_2m = 0x80;
				break;

			case '0': /* rate on track 0 */
				rate_0 = atoi(optarg);
				break;
			case 'A': /* rate on other tracks */
				rate_any = atoi(optarg);
				break;

			case 'M':
				msize = atoi(optarg);
				if(msize != 512 &&
				   msize != 1024 &&
				   msize != 2048 &&
				   msize != 4096) {
				  fprintf(stderr, "Only sector sizes of 512, 1024, 2048 or 4096 bytes are allowed\n");
				  usage();
				}
				break;

			case 'N':
 				serial = strtoul(optarg,0,16);
 				serial_set = 1;
 				break;
			case 'a': /* Atari style serial number */
				Atari = 1;
				break;

			case 'C':
				create = O_CREAT | O_TRUNC;
				break;

			case 'H':
				hs = atoi(optarg);
				hs_set = 1;
				break;

			case 'I':
				fsVersion = strtoul(optarg,0,0);
				break;

			case 'c':
				Fs.cluster_size = atoi(optarg);
				break;

			case 'r': 
				Fs.dir_len = strtoul(optarg,0,0);
				break;
			case 'L':
				Fs.fat_len = strtoul(optarg,0,0);
				break;


			case 'B':
				bootSector = optarg;
				break;
			case 'k':
				keepBoot = 1;
				break;
			case 'h':
				argheads = atoi(optarg);
				break;
			case 'd':
				Fs.num_fat = atoi(optarg);
				break;
			case 'm':
				mediaDesc = strtoul(optarg,0,0);
				break;
			default:
				usage();
		}
	}

	if (argc - optind != 1 ||
	    !argv[optind][0] || argv[optind][1] != ':')
		usage();

#ifdef USE_XDF
	if(create && format_xdf) {
		fprintf(stderr,"Create and XDF can't be used together\n");
		exit(1);
	}
#endif
	
	drive = toupper(argv[argc -1][0]);

	/* check out a drive whose letter and parameters match */	
	sprintf(errmsg, "Drive '%c:' not supported", drive);	
	Fs.Direct = NULL;
	blocksize = 0;
	for(dev=devices;dev->drive;dev++) {
		FREE(&(Fs.Direct));
		/* drive letter */
		if (dev->drive != drive)
			continue;
		used_dev = *dev;

		SET_INT(used_dev.tracks, argtracks);
		SET_INT(used_dev.heads, argheads);
		SET_INT(used_dev.sectors, argsectors);
		SET_INT(used_dev.use_2m, arguse_2m);
		SET_INT(used_dev.ssize, argssize);
		if(hs_set)
			used_dev.hidden = hs;
		
		expand(dev->name, name);
#ifdef USING_NEW_VOLD
		strcpy(name, getVoldName(dev, name));
#endif

#ifdef USE_XDF
		if(!format_xdf) {
#endif
			Fs.Direct = 0;
#ifdef USE_FLOPPYD
			Fs.Direct = FloppydOpen(&used_dev, dev, name, O_RDWR | create,
									errmsg, 0, 1);
			if(Fs.Direct) {
				maxSize = max_off_t_31;
			}
#endif
			if(!Fs.Direct) {			
				Fs.Direct = SimpleFileOpen(&used_dev, dev, name,
										   O_RDWR | create,
										   errmsg, 0, 1, &maxSize);
			}
#ifdef USE_XDF
		} else {
			used_dev.misc_flags |= USE_XDF_FLAG;
			Fs.Direct = XdfOpen(&used_dev, name, O_RDWR,
					    errmsg, &info);
			if(Fs.Direct && !Fs.fat_len)
				Fs.fat_len = info.FatSize;
			if(Fs.Direct && !Fs.dir_len)
				Fs.dir_len = info.RootDirSize;
		}
#endif

		if (!Fs.Direct)
			continue;

#ifdef OS_linux
		if ((!used_dev.tracks || !used_dev.heads || !used_dev.sectors) &&
			(!IS_SCSI(dev))) {
			int fd= get_fd(Fs.Direct);
			struct MT_STAT buf;

			if (MT_FSTAT(fd, &buf) < 0) {
				sprintf(errmsg, "Could not stat file (%s)", strerror(errno));
				continue;						
			}

			if (S_ISBLK(buf.st_mode)) {
				struct hd_geometry geom;
				long size;
				int sect_per_track;

				if (ioctl(fd, HDIO_GETGEO, &geom) < 0) {
					sprintf(errmsg, "Could not get geometry of device (%s)",
							strerror(errno));
					continue;
				}

				if (ioctl(fd, BLKGETSIZE, &size) < 0) {
					sprintf(errmsg, "Could not get size of device (%s)",
							strerror(errno));
					continue;
				}

				sect_per_track = geom.heads * geom.sectors;
				used_dev.heads = geom.heads;
				used_dev.sectors = geom.sectors;
				used_dev.hidden = geom.start % sect_per_track;
				used_dev.tracks = (size + used_dev.hidden) / sect_per_track;
			}
		}
#endif

		/* no way to find out geometry */
		if (!used_dev.tracks || !used_dev.heads || !used_dev.sectors){
			sprintf(errmsg, 
				"Unknown geometry "
				"(You must tell the complete geometry "
				"of the disk, \neither in /etc/mtools.conf or "
				"on the command line) ");
			continue;
		}

#if 0
		/* set parameters, if needed */
		if(SET_GEOM(Fs.Direct, &used_dev, 0xf0, boot)){
			sprintf(errmsg,"Can't set disk parameters: %s", 
				strerror(errno));
			continue;
		}
#endif
		Fs.sector_size = 512;
		if( !(used_dev.use_2m & 0x7f)) {
			Fs.sector_size = 128 << (used_dev.ssize & 0x7f);
		}

		SET_INT(Fs.sector_size, msize);
		{
		    unsigned int i;
		    for(i = 0; i < 31; i++) {
			if (Fs.sector_size == (unsigned int) (1 << i)) {
			    Fs.sectorShift = i;
			    break;
			}
		    }
		    Fs.sectorMask = Fs.sector_size - 1;
		}

		if(!used_dev.blocksize || used_dev.blocksize < Fs.sector_size)
			blocksize = Fs.sector_size;
		else
			blocksize = used_dev.blocksize;
		
		if(blocksize > MAX_SECTOR)
			blocksize = MAX_SECTOR;

		/* do a "test" read */
		if (!create &&
		    READS(Fs.Direct, (char *) buf, 0, Fs.sector_size) != 
		    (signed int) Fs.sector_size) {
			sprintf(errmsg, 
				"Error reading from '%s', wrong parameters?",
				name);
			continue;
		}
		break;
	}


	/* print error msg if needed */	
	if ( dev->drive == 0 ){
		FREE(&Fs.Direct);
		fprintf(stderr,"%s: %s\n", argv[0],errmsg);
		exit(1);
	}

	/* the boot sector */
	boot = (struct bootsector *) buf;
	if(bootSector) {
		int fd;

		fd = open(bootSector, O_RDONLY | O_LARGEFILE);
		if(fd < 0) {
			perror("open boot sector");
			exit(1);
		}
		read(fd, buf, blocksize);
		keepBoot = 1;
	}
	if(!keepBoot && !(used_dev.use_2m & 0x7f)) {
		memset((char *)boot, '\0', Fs.sector_size);
		if(Fs.sector_size == 512 && !used_dev.partition) {
			/* install fake partition table pointing to itself */
			struct partition *partTable=(struct partition *)
				(((char*) boot) + 0x1ae);
			setBeginEnd(&partTable[1], 0,
						used_dev.heads * used_dev.sectors * used_dev.tracks,
						used_dev.heads, used_dev.sectors, 1, 0);
		}
	}
	set_dword(boot->nhs, used_dev.hidden);

	Fs.Next = buf_init(Fs.Direct,
			   blocksize * used_dev.heads * used_dev.sectors,
			   blocksize * used_dev.heads * used_dev.sectors,
			   blocksize);
	Fs.Buffer = 0;

	boot->nfat = Fs.num_fat;
	if(!keepBoot)
		set_word(boot->jump + 510, 0xaa55);
	
	/* get the parameters */
	tot_sectors = used_dev.tracks * used_dev.heads * used_dev.sectors - 
		DWORD(nhs);

	set_word(boot->nsect, used_dev.sectors);
	set_word(boot->nheads, used_dev.heads);

	used_dev.fat_bits = comp_fat_bits(&Fs,used_dev.fat_bits, tot_sectors, fat32);

	if(used_dev.fat_bits == 32) {
		Fs.primaryFat = 0;
		Fs.writeAllFats = 1;
		Fs.fat_start = 32;
		calc_fs_parameters_32(tot_sectors, &Fs, boot);

		Fs.clus_start = Fs.num_fat * Fs.fat_len + Fs.fat_start;

		/* extension flags: mirror fats, and use #0 as primary */
		set_word(boot->ext.fat32.extFlags,0);

		/* fs version.  What should go here? */
		set_word(boot->ext.fat32.fsVersion,fsVersion);

		/* root directory */
		set_dword(boot->ext.fat32.rootCluster, Fs.rootCluster = 2);

		/* info sector */
		set_word(boot->ext.fat32.infoSector, Fs.infoSectorLoc = 1);
		Fs.infoSectorLoc = 1;

		/* no backup boot sector */
		set_word(boot->ext.fat32.backupBoot, 6);
		
		labelBlock = & boot->ext.fat32.labelBlock;
	} else {
		Fs.infoSectorLoc = 0;
		Fs.fat_start = 1;
		calc_fs_parameters(&used_dev, tot_sectors, &Fs, boot);
		Fs.dir_start = Fs.num_fat * Fs.fat_len + Fs.fat_start;
		Fs.clus_start = Fs.dir_start + Fs.dir_len;
		labelBlock = & boot->ext.old.labelBlock;

	}
	
	if (!keepBoot)
		/* only zero out physdrive if we don't have a template
		 * bootsector */
		labelBlock->physdrive = 0x00;
	labelBlock->reserved = 0;
	labelBlock->dos4 = 0x29;

	if (!serial_set || Atari)
		srandom((long)time (0));
	if (!serial_set)
		serial=random();
	set_dword(labelBlock->serial, serial);	
	if(!label[0])
		strncpy(shortlabel, "NO NAME    ",11);
	else
		label_name(label, 0, &mangled, shortlabel);
	strncpy(labelBlock->label, shortlabel, 11);
	sprintf(labelBlock->fat_type, "FAT%2.2d  ", Fs.fat_bits);
	labelBlock->fat_type[7] = ' ';

	set_word(boot->secsiz, Fs.sector_size);
	boot->clsiz = (unsigned char) Fs.cluster_size;
	set_word(boot->nrsvsect, Fs.fat_start);

	bootOffset = init_geometry_boot(boot, &used_dev, sectors0, 
					rate_0, rate_any,
					&tot_sectors, keepBoot);
	if(!bootOffset) {
		bootOffset = ((char *) labelBlock) - ((char *) boot) +
			sizeof(struct label_blk_t);
	}
	if(Atari) {
		boot->banner[4] = 0;
		boot->banner[5] = random();
		boot->banner[6] = random();
		boot->banner[7] = random();
	}		

	if (create) {
		WRITES(Fs.Direct, (char *) buf,
		       sectorsToBytes((Stream_t*)&Fs, tot_sectors-1),
		       Fs.sector_size);
	}

	if(!keepBoot)
		inst_boot_prg(boot, bootOffset);
	/* Mimic 3.8 behavior, else 2m disk do not work (???)
	 * luferbu@fluidsignal.com (Luis Bustamante), Fri, 14 Jun 2002
	 */
	if(used_dev.use_2m & 0x7f) {
	  boot->jump[0] = 0xeb;
	  boot->jump[1] = 0x80;
	  boot->jump[2] = 0x90;
	}
	if(used_dev.use_2m & 0x7f)
		Fs.num_fat = 1;
	if(mediaDesc != -1)
		boot->descr=mediaDesc;
	Fs.lastFatSectorNr = 0;
	Fs.lastFatSectorData = 0;
	zero_fat(&Fs, boot->descr);
	Fs.freeSpace = Fs.num_clus;
	Fs.last = 2;

#ifdef USE_XDF
	if(format_xdf)
		for(i=0; 
		    i < (info.BadSectors+Fs.cluster_size-1)/Fs.cluster_size; 
		    i++)
			fatEncode(&Fs, i+2, 0xfff7);
#endif

	format_root(&Fs, label, boot);
	WRITES((Stream_t *)&Fs, (char *) boot, (mt_off_t) 0, Fs.sector_size);
	if(Fs.fat_bits == 32 && WORD(ext.fat32.backupBoot) != MAX16) {
		WRITES((Stream_t *)&Fs, (char *) boot, 
		       sectorsToBytes((Stream_t*)&Fs, WORD(ext.fat32.backupBoot)),
		       Fs.sector_size);
	}
	FLUSH((Stream_t *)&Fs); /* flushes Fs. 
				 * This triggers the writing of the FAT */
	FREE(&Fs.Next);
	Fs.Class->freeFunc((Stream_t *)&Fs);
#ifdef USE_XDF
	if(format_xdf && isatty(0) && !getenv("MTOOLS_USE_XDF"))
		fprintf(stderr,
			"Note:\n"
			"Remember to set the \"MTOOLS_USE_XDF\" environmental\n"
			"variable before accessing this disk\n\n"
			"Bourne shell syntax (sh, ash, bash, ksh, zsh etc):\n"
			" export MTOOLS_USE_XDF=1\n\n"
			"C shell syntax (csh and tcsh):\n"
			" setenv MTOOLS_USE_XDF 1\n" );	
#endif
	exit(0);
}

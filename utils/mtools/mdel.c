/*
 * mdel.c
 * Delete an MSDOS file
 *
 */

#include "sysincludes.h"
#include "msdos.h"
#include "mtools.h"
#include "stream.h"
#include "mainloop.h"
#include "fs.h"
#include "file.h"

typedef struct Arg_t {
	int deltype;
	int verbose;
} Arg_t;

/**
 * Wiped the given entry
 */
void wipeEntry(direntry_t *entry)
{
	direntry_t longNameEntry;
	int i;
	initializeDirentry(&longNameEntry, entry->Dir);
	for(i=entry->beginSlot; i< entry->endSlot; i++) {
	    int error;
	    longNameEntry.entry=i;
	    dir_read(&longNameEntry, &error);
	    if(error)
		break;
	    longNameEntry.dir.name[0] = (char) DELMARK;
	    dir_write(&longNameEntry);
	}
	entry->dir.name[0] = (char) DELMARK;
	dir_write(entry);
}

static int del_entry(direntry_t *entry, MainParam_t *mp)
{
	Arg_t *arg=(Arg_t *) mp->arg;

	if(got_signal)
		return ERROR_ONE;

	if(entry->entry == -3) {
		fprintf(stderr, "Cannot remove root directory\n");
		return ERROR_ONE;
	}

	if (arg->verbose) {
		fprintf(stderr,"Removing ");
		fprintPwd(stderr, entry,0);
		fputc('\n', stderr);
	}

	if ((entry->dir.attr & (ATTR_READONLY | ATTR_SYSTEM)) &&
	    (ask_confirmation("%s: \"%s\" is read only, erase anyway (y/n) ? ",
			      progname, entry->name)))
		return ERROR_ONE;
	if (fatFreeWithDirentry(entry)) 
		return ERROR_ONE;

	wipeEntry(entry);
	return GOT_ONE;
}

static int del_file(direntry_t *entry, MainParam_t *mp)
{
	char shortname[13];
	direntry_t subEntry;
	Stream_t *SubDir;
	Arg_t *arg = (Arg_t *) mp->arg;
	MainParam_t sonmp;
	int ret;
	int r;

	sonmp = *mp;
	sonmp.arg = mp->arg;

	r = 0;
	if (IS_DIR(entry)){
		/* a directory */
		SubDir = OpenFileByDirentry(entry);
		initializeDirentry(&subEntry, SubDir);
		ret = 0;
		while((r=vfat_lookup(&subEntry, "*", 1,
				     ACCEPT_DIR | ACCEPT_PLAIN,
				     shortname, NULL)) == 0 ){
			if(shortname[0] != DELMARK &&
			   shortname[0] &&
			   shortname[0] != '.' ){
				if(arg->deltype != 2){
					fprintf(stderr,
						"Directory ");
					fprintPwd(stderr, entry,0);
					fprintf(stderr," non empty\n");
					ret = ERROR_ONE;
					break;
				}
				if(got_signal) {
					ret = ERROR_ONE;
					break;
				}
				ret = del_file(&subEntry, &sonmp);
				if( ret & ERROR_ONE)
					break;
				ret = 0;
			}
		}
		FREE(&SubDir);
		if (r == -2)
			return ERROR_ONE;
		if(ret)
			return ret;
	}
	return del_entry(entry, mp);
}


static void usage(void)
{
	fprintf(stderr, 
		"Mtools version %s, dated %s\n", mversion, mdate);
	fprintf(stderr, 
		"Usage: %s [-v] msdosfile [msdosfiles...]", progname);
	exit(1);
}

void mdel(int argc, char **argv, int deltype)
{
	Arg_t arg;
	MainParam_t mp;
	int c,i;

	arg.verbose = 0;
	while ((c = getopt(argc, argv, "i:v")) != EOF) {
		switch (c) {
			case 'i':
				set_cmd_line_image(optarg, 0);
				break;
			case 'v':
				arg.verbose = 1;
				break;
			default:
				usage();
		}
	}

	if(argc == optind)
		usage();

	init_mp(&mp);
	mp.callback = del_file;
	mp.arg = (void *) &arg;
	mp.openflags = O_RDWR;
	arg.deltype = deltype;
	switch(deltype){
	case 0:
		mp.lookupflags = ACCEPT_PLAIN; /* mdel */
		break;
	case 1:
		mp.lookupflags = ACCEPT_DIR; /* mrd */
		break;
	case 2:
		mp.lookupflags = ACCEPT_DIR | ACCEPT_PLAIN; /* mdeltree */
		break;
	}
	mp.lookupflags |= NO_DOTS;
	for(i=optind;i<argc;i++) {
		int b,l;
		if(argv[i][0] && argv[i][1] == ':')
			b = 2;
		else
			b = 0;
		l = strlen(argv[i]+b);
		if(l > 1 && argv[i][b+l-1] == '/')
			argv[i][b+l-1] = '\0';
	}
		
	exit(main_loop(&mp, argv + optind, argc - optind));
}

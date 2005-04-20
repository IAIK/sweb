/*
 * mdu.c:
 * Display the space occupied by an MSDOS directory
 */

#include "sysincludes.h"
#include "msdos.h"
#include "vfat.h"
#include "mtools.h"
#include "file.h"
#include "mainloop.h"
#include "fs.h"
#include "codepage.h"


typedef struct Arg_t {
	int all;
	int inDir;
	int summary;
	struct Arg_t *parent;
	char *target;
	char *path;
	unsigned int blocks;
	MainParam_t mp;
} Arg_t;

static void usage(void)
{
		fprintf(stderr, "Mtools version %s, dated %s\n",
			mversion, mdate);
		fprintf(stderr, "Usage: %s: msdosdirectory\n",
			progname);
		exit(1);
}

static int file_mdu(direntry_t *entry, MainParam_t *mp)
{
	unsigned int blocks;
	Arg_t * arg = (Arg_t *) (mp->arg);

	blocks = countBlocks(entry->Dir,getStart(entry->Dir, &entry->dir));
	if(arg->all || !arg->inDir) {
		fprintPwd(stdout, entry,0);
		printf(" %d\n", blocks);
	}
	arg->blocks += blocks;
	return GOT_ONE;
}


static int dir_mdu(direntry_t *entry, MainParam_t *mp)
{
	Arg_t *parentArg = (Arg_t *) (mp->arg);
	Arg_t arg;
	int ret;
	
	arg = *parentArg;
	arg.mp.arg = (void *) &arg;
	arg.parent = parentArg;
	arg.inDir = 1;

	/* account for the space occupied by the directory itself */
	if(!isRootDir(entry->Dir)) {
		arg.blocks = countBlocks(entry->Dir,
					 getStart(entry->Dir, &entry->dir));
	} else {
		arg.blocks = 0;
	}

	/* recursion */
	ret = mp->loop(mp->File, &arg.mp, "*");
	if(!arg.summary || !parentArg->inDir) {
		fprintPwd(stdout, entry,0);
		printf(" %d\n", arg.blocks);
	}
	arg.parent->blocks += arg.blocks;
	return ret;
}

void mdu(int argc, char **argv, int type)
{
	Arg_t arg;
	int c;

	arg.all = 0;
	arg.inDir = 0;
	arg.summary = 0;
	while ((c = getopt(argc, argv, "i:as")) != EOF) {
		switch (c) {
			case 'i':
				set_cmd_line_image(optarg, 0);
				break;
			case 'a':
				arg.all = 1;
				break;
			case 's':
				arg.summary = 1;
				break;
			case '?':
				usage();
		}
	}

	if (optind >= argc)
		usage();

	if(arg.summary && arg.all) {
		fprintf(stderr,"-a and -s options are mutually exclusive\n");
		usage();
	}

	init_mp(&arg.mp);
	arg.mp.callback = file_mdu;
	arg.mp.openflags = O_RDONLY;
	arg.mp.dirCallback = dir_mdu;

	arg.mp.arg = (void *) &arg;
	arg.mp.lookupflags = ACCEPT_PLAIN | ACCEPT_DIR | DO_OPEN_DIRS | NO_DOTS;
	exit(main_loop(&arg.mp, argv + optind, argc - optind));
}

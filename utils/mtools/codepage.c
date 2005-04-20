#include "sysincludes.h"
#include "mtools.h"
#include "codepage.h"


Codepage_t *Codepage=0;
char *mstoupper=0;


#undef WORD
#define WORD(x,y) (file[(x)+2*(y)] + (file[(x)+1+2*(y)] << 8))

#define COUNTRY(x) WORD(25+(x)*14, 1)
#define CODEPAGE(x) WORD(25+(x)*14, 2)
#define DATA(x) WORD(25+(x)*14, 5)
#define CTYINFO(x) WORD(DATA(x), 3)
#define CTYINFOCP(x) WORD(CTYINFO(x), 6)
#define UCASE(x) WORD(DATA(x), 11)
#define UCASEBYTE file[ucase+10+j*8+k]

#define NBVARS(x) WORD(DATA(x), 0)
#define VARID(x,y) WORD(DATA(x), y*4+2)
#define VARVAL(x,y) WORD(DATA(x), y*4+3)

static void bad_country_file(void)
{
	fprintf(stderr,"Corrupted country.sys file\n");
	exit(1);
}

static void not_found(int country_found, int country, int codepage)
{
	if(!country_found)
		fprintf(stderr,"Country code %03d not supported\n",
			country);
	else	
		fprintf(stderr,"Country/codepage combo %03d/%d not supported\n",
			country, codepage);
	exit(1);
}


static short get_variable(unsigned char *file, int i, int id)
{
	int j;

	for(j=0; j < NBVARS(i); j++)
		if(VARID(i,j) == id)
			return VARVAL(i,j);
	return 0;
}


static void set_toupper_from_builtin(int country, int *codepage)
{
	country_t *p;
	int country_found = 0;

	if(mstoupper)
		return;
	for(p = countries; p->country; p++) {
		if(p->country == country) {
			country_found = 1;
			if(!*codepage)
				*codepage = p->default_codepage;
			if (p->codepage == *codepage) {
				mstoupper = (char *) toucase[p->to_upper];
				return;
			}
		}
	}
	not_found(country_found, country, *codepage);
}


static void load_toupper(int country, int *codepage, char *filename)
{
	int fd, filesize;
	unsigned char *file;
	unsigned short ucase=0, records;
	int i;
	int country_found = 0;
	struct MT_STAT buf;

	if(!filename) {
		set_toupper_from_builtin(country, codepage);
		return;
	}

	fd = open(filename, O_RDONLY | O_LARGEFILE);
	if(fd < 0) {
		perror("open country.sys");
		exit(1);
	}

	MT_FSTAT(fd, &buf);
	file = (unsigned char *) malloc(buf.st_size);
	if(!file) {
		printOom();
		exit(1);
	}

	/* load country.sys */
	filesize=read(fd, (char *) file, 65536);
	if(filesize < 0) {
		perror("Read country.sys\n");
		exit(1);
	}
	close(fd);
	
	if(strcmp((char *)file, "\377COUNTRY"))
		bad_country_file();

	records = WORD(23,0);	

	/* second pass: locate translation table */
	for(i=0; i<records; i++) {
		if(country == COUNTRY(i)) {
			country_found = 1;
			if(!*codepage)
				*codepage = get_variable(file, i, 1);
			if(mstoupper)
				continue;
			if(*codepage == CODEPAGE(i)) {
				ucase = get_variable(file, i, 4);
				if(!ucase) {
					fprintf(stderr,
						"No translation table for this"
						"country and code page\n");
					exit(1);
				}
				if(strncmp((char*)file+ucase+1, "UCASE", 5) &&
				   strncmp((char*)file+ucase+1, "FUCASE", 6))
					bad_country_file();
				mstoupper = (char *) toucase[0];
				memcpy(mstoupper, file + ucase + 10, 128);
				free(file);
				return;
			}
		}
	}
	not_found(country_found, country, *codepage);
	free(file);
}

static void set_codepage(int nr)
{
	if(Codepage)
		return;
	for(Codepage = codepages; Codepage->nr; Codepage++)
		if(Codepage->nr == nr)
			return;
	fprintf(stderr,"Unknown code page %d\n", nr);
	exit(1);

}


static void syntax(void)
{
	fprintf(stderr,"Syntax error in COUNTRY environmental variable\n");
	fprintf(stderr,"Usage: export COUNTRY=countrycode[,[codepage][,filename]]\n");
	exit(1);
}

void init_codepage(void)
{
    char *country, *file;
    int country_prefix;
    int codepage;

    file = 0;
    country=country_string;
    if(!country) {
	    codepage = 850;
	    country_prefix = 41; /* Switzerland */
    } else {
	    file = 0;
	    codepage = 0;
	    country_prefix = strtoul(country, &country, 10);
	    if(!country_prefix)
		    syntax();
	    if(*country==',') {
		    country++;
		    codepage = strtoul(country, &country,10);
		    if(*country==',') {
			    file = country+1;
		    } else if (*country)
			    syntax();
	    } else if(*country)
		    syntax();
    }    

    load_toupper(country_prefix, &codepage, file);
    set_codepage(codepage);
}

unsigned char to_dos(unsigned char c)
{
	int oc;

	if(c < 0x80)
		return c;
	for(oc = 0 ; oc < 128; oc++) {
		if(c == Codepage->tounix[oc])
			return oc | 0x80;
	}
	return '_';
}


void to_unix(char *a, int n)
{
	for( ; *a && n > 0; n--, a++) {
		/* special case, 0xE5 */
		if(*a == 0x05)
			*a = DELMARK;
		if(*a & 0x80)
			*a = (char) Codepage->tounix[(*a) & 0x7f];
	}
}

/**
 * Same thing as to_unix, except that it is meant for file contents
 * rather than filename and thus doesn't do anything for delete marks
 */
char contents_to_unix(char a) {
    if(a & 0x80)
	return (char) Codepage->tounix[a & 0x7f];
    else
	return a;
}

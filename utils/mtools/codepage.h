typedef struct Codepage_l {
	int nr;   
	unsigned char tounix[128];
} Codepage_t;


typedef struct country_l {
	int country;
	int codepage;
	int default_codepage;
	int to_upper;
} country_t;


void init_codepage(void);
unsigned char to_dos(unsigned char c);
void to_unix(char *a, int n);
char contents_to_unix(char a);

extern Codepage_t *Codepage;
extern char *mstoupper;
extern country_t countries[];
extern unsigned char toucase[][128];
extern Codepage_t codepages[];
extern char *country_string;

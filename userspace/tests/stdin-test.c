#include "unistd.h"
#include "stdio.h"

int main(int argc, char *argv[]) 
{
	char* str1="Hi, ich bin ein Userspace Programm\n";
	char* str2="Sag mir deinen Namen bitte:\n";
	char* str3="Jetzt dein Alter bitte: \n";
	char* str4="Danke und Sayonara !\n";

	char name[200];
	name[199]=0;
	unsigned int age=0;
	
	printf("%s",str1);
	printf("%s",str2);
	gets(name,199);
	//scanf("%s\n",name);
	printf("%s",str3);
	scanf("%d\n",&age);
	printf("Doumo, Anatano namae %s desu. Antawa %d sai desu\n%s",name,age,str4);
	return age;
}

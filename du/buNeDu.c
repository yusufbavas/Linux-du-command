#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>


int postOrderApply (char *path, int pathfun (char *path1));

int sizepathfun (char *path);

char* pathname(char *path);

int flag;

// Arguman sayisina gore parametre kontrolu yapilir. Eger yanlis bir kullanim var ise usage ekrana basilir.
int main(int argc, char *argv[])
{
	int result;

	if(argc == 2)
	{
		flag = 1;
		result = postOrderApply(argv[1],sizepathfun);
	}
	else if(argc == 3 && (strcmp("-z",argv[1]) == 0 || strcmp("-z",argv[1]) == 0))
	{
		flag = 0;
		result = postOrderApply(argv[2],sizepathfun);
	}
	else
	{
		fprintf(stderr, "Usage: ./buNeDu [-z] <pathfun>\n");
		return 1;	
	}

	printf("%d 		%s\n",result == -1 ? 0 : result , flag ? argv[1] : argv[2]);

	return 0;
}
/*
	Dosyalarin boyutlari en icerden baslanarak hesaplanir ve ekrana basilir.
*/
int postOrderApply(char *path, int pathfun(char *path1))
{
	struct dirent *direntptr;
	struct stat statbuf;
	int result = 0;
	DIR *dirptr;

	dirptr = opendir(path);

	while (dirptr != NULL &&  errno != EACCES && (direntptr = readdir(dirptr)) != NULL)
	{
		// . ve . directoryleri hesaplamaya katilmaz.
		if(strcmp(direntptr->d_name,".") == 0 || strcmp(direntptr->d_name,"..") == 0)
			continue;
		
		char *str = (char*) malloc(strlen(path) + strlen(direntptr->d_name) + 2);
		strcpy(str,path);
		if( str[strlen(str)-1] != '/')
			strcat(str,"/");
		strcat(str,direntptr->d_name);

		lstat(str,&statbuf);
		if(S_ISDIR(statbuf.st_mode))
		{
			// dosya bir directory ise fonksiyon o directory icin tekrar cagirilir.
			int temp = postOrderApply(str,pathfun);
			// Dondurulen size degeri komutun calistirilma parametresine gore sonuc size'ina eklenir ya da eklenmez.
			if (temp != -1)
			{
				printf("%d 		%s\n",temp,str);
				if(!flag)
				result += temp;
			}	
		}
		// Eger dosya bir directory degil ise boyutu hesaplanmak icin pathfun cagirilir.
		// Eger pathfun -1 yani error return etmis ise sonuca eklenmez.
		else if(pathfun(str) != -1)
			result += pathfun(str);
		free(str);
	}
	// Eger dosya acmada bir sorun olmus ise bilgilendirme mesaji ekrana basilir.
	if (errno == EACCES) 
	{
		printf("Cannot read folder 	%s\n",path);
		errno = 0;
		result = -1;
	}

	closedir(dirptr);
	
	return result;
}
/*
	verilen pathdeki file bir regular file ise size i return edilir.
	degil ise special file yazilip -1 return edilir.
*/
int sizepathfun(char *path)
{
		struct stat statbuf;
		lstat(path,&statbuf);
		if (!S_ISREG(statbuf.st_mode))
		{
			printf("Special file 	%s\n",pathname(path));
			return -1;
		}
		return statbuf.st_size / 1024;
}

/* 
	regular file olmayan seylerde ekrana sadece dosyanin adi yazilacagi icin
	path stringi parse edilir.
*/
char* pathname(char * path)
{	
	char* result;
	if((result = strstr(path,"/")) != 0)
		return pathname(&result[1]);
	return path;
	
}
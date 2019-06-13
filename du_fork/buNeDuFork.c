#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/file.h>

void postOrderApply (char *path, int pathfun (char *path1));

int sizepathfun (char *path);

char* pathname(char *path);

int flag;

// Arguman sayisina gore parametre kontrolu yapilir. Eger yanlis bir kullanim var ise usage ekrana basilir.
int main(int argc, char *argv[]){

	int pid;
	int status;
	if(argc == 2){

		printf("PID 	SIZE 	PATH\n");
		flag = 1;
		switch(pid = fork()){
			case -1:
				printf("Fork Failed!\n");
				exit(0);
			case 0:
				postOrderApply(argv[1],sizepathfun);	
				break;
		}
	}
	else if(argc == 3 && (strcmp("-z",argv[1]) == 0 || strcmp("-z",argv[1]) == 0)){

		printf("PID 	SIZE 	PATH\n");
		flag = 0;
		switch(pid = fork()){
			case -1:
				printf("Fork Failed!\n");
				exit(0);
			case 0:
				postOrderApply(argv[2],sizepathfun);	
				break;
		}
	}
	else{

		fprintf(stderr, "Usage: ./buNeDuFork [-z] <rootpath>\n");
		return 1;	
	}

	waitpid(pid,&status,0);

	int fp;
	if ((fp = open("151044018sizes.txt", O_RDONLY,0777)) == -1){

		printf("File can not opended!\n");
		exit(0);
	}	
	char str[255] = "";

	if(read(fp,str,255) == -1){
		printf("Read error!\n");
		exit(0);
	}
	close(fp);
	int res = 0;

	// olusturulan toplam process sayisi hesaplanir.
	for (char *temp = str; (temp = strchr(temp,'\n')) != NULL; res++,temp++);
	
	printf("%d child processes created. Main process is %d.\n",res,getpid());

	return 0;
}

/*
	Dosyalarin boyutlari hesaplanir. Her dosya icin ayrin bir process acilir.
	Processler arasi haberlesme icin bir txt dosyasi kullanilir.
*/
void postOrderApply(char *path, int pathfun(char *path1)){

	struct dirent *direntptr;
	struct stat statbuf;
	int result = 0;
	DIR *dirptr;
	int pid;
	int status;
	dirptr = opendir(path);
	int pathRes;


	int fp_write = open("151044018sizes.txt", O_WRONLY | O_CREAT | O_TRUNC,0777);
	int fp_read = open("151044018sizes.txt", O_RDONLY,0777);

	if (fp_read == -1 || fp_write == -1){

		printf("File can not opened!\n");
		exit(0);
	}

	while (dirptr != NULL &&  errno != EACCES && (direntptr = readdir(dirptr)) != NULL){

		// . ve . directoryleri hesaplamaya katilmaz.
		if(strcmp(direntptr->d_name,".") == 0 || strcmp(direntptr->d_name,"..") == 0)
			continue;
		
		char *str = (char*) malloc(strlen(path) + strlen(direntptr->d_name) + 2);
		strcpy(str,path);
		if( str[strlen(str)-1] != '/')
			strcat(str,"/");
		strcat(str,direntptr->d_name);

		lstat(str,&statbuf);
		// Eger bir directory bulunmus ise o directory icin yeni bir process acilir.
		if(S_ISDIR(statbuf.st_mode)){

			switch(pid = fork()){
				case -1:
					printf("Fork Failed!\n");
					exit(0);
				case 0:
					result = 0;
					strcpy(path,str);
					closedir(dirptr);
					dirptr = opendir(path);
					break;
				default:
				// Chield process hesaplamasini bitirip dosyaya yazdıktan sonra parent process 
				// dosyadan okuyarak size i alır.
					waitpid(pid,&status,0);		
					if (!flag){
						char temp[255] = "";
						int res;
						if(read(fp_read,temp,255) == -1){
							printf("Read error!\n");
							exit(0);
						}
						int junk;
						sscanf(temp,"%d %d",&junk,&res);
						if (res != -1)
							result += res; 	
					}	
			}

		}
		// Eger dosya bir directory degil ise boyutu hesaplanmak icin pathfun cagirilir.
		// Eger pathfun -1 yani error return etmis ise sonuca eklenmez.
		else if((pathRes = pathfun(str)) != -1)
			result += pathRes;
		
		free(str);
	}

	// Eger dosya acmada bir sorun olmus ise bilgilendirme mesaji ekrana basilir.
	if (errno == EACCES || errno == ENOENT) {

		printf("%d 		Cannot read folder %s\n",getpid(),path);
		errno = 0;
		result = -1;
	}

	char str[255];
	// Sonuc dosyaya ve ekrana basilir.
	sprintf(str,"%d 	%d 	%s\n",getpid(),result,path);
	if (result != -1)
		printf("%s",str);

	flock(fp_write,LOCK_EX);
	if(write(fp_write,str,strlen(str)) == -1){

		printf("Write error!\n");
		exit(0);
	}
	flock(fp_write,LOCK_UN);

	closedir(dirptr);
	close(fp_write);
	close(fp_read);

	exit(1);
}

/*
	verilen pathdeki file bir regular file ise size i return edilir.
	degil ise special file yazilip -1 return edilir.
*/
int sizepathfun(char *path){

		struct stat statbuf;
		lstat(path,&statbuf);
		if (!S_ISREG(statbuf.st_mode)){

			printf("%d 		Special file %s\n",getpid(),pathname(path));
			return -1;
		}

		return statbuf.st_size / 1024;
}

/* 
	regular file olmayan seylerde ekrana sadece dosyanin adi yazilacagi icin
	path stringi parse edilir.
*/
char* pathname(char * path){

	char* result;
	if((result = strstr(path,"/")) != 0)
		return pathname(&result[1]);
	return path;
	
}
#define _GNU_SOURCE
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

#define PERM (S_IRUSR | S_IWUSR)

#define BUFFER_SIZE 255
#define FIFO_SIZE 1048476

void postOrderApply (char *path, int pathfun (char *path1));

int sizepathfun (char *path);

char* pathname(char *path);

int flag;
char *fifo = "/tmp/fifo";

// Arguman sayisina gore parametre kontrolu yapilir. Eger yanlis bir kullanim var ise usage ekrana basilir.
int main(int argc, char *argv[]){

	int pid;
	int status;
	unlink(fifo);
	errno = 0;

	if(mkfifo(fifo,PERM) == -1){
		printf("Fifo Failed! -- %s \n",strerror(errno));
		exit(0);
	}

	if(argc == 2){

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

	// fifo okuma modunda acilir ve cheild processlerin bitmesi beklenir.
	int fp;
	if ((fp = open(fifo, O_RDONLY)) == -1){

		printf("File can not opended!\n");
		exit(0);
	}	
	
	waitpid(pid,&status,0);

	
	int count,res=0;
	// fifo sonuna kadar okunur ve ekrana basilir.
	printf("PID 	SIZE 	PATH\n");
	do{
		
		char str[BUFFER_SIZE] = "";
		count = read(fp,str,BUFFER_SIZE);
		str[count] = '\0';
		if (count == -1)
			exit(0);
		printf("%s",str);
		
		for (char *temp = str; (temp = strchr(temp,'\n')) != NULL;temp++,res++);
		for (char *temp = str; (temp = strstr(temp,"-1")) != NULL;temp++,res--);

	}while(count == BUFFER_SIZE);

	close(fp);
	
	printf("%d child processes created. Main process is %d.\n",res,getpid());
	unlink(fifo);
	return 0;
}

/*
	Dosyalarin boyutlari hesaplanir. Her dosya icin ayri bir process acilir.
	-z opsiyonu icin chield parent haberlesmesi pipe uzerinden,
	tum sonuclarin main processe verilebilmesi icin ise fifo kullanilir.
*/
void postOrderApply(char *path, int pathfun(char *path1)){

	struct dirent *direntptr;
	struct stat statbuf;
	DIR *dirptr;

	int result = 0,pathRes,pid = 0, pipe_count = 0,write_pipe = -1,specialFile = 0;

	int pipefd[2];
	int read_pipe[BUFFER_SIZE];

	char *special;

	int fp_write = open(fifo, O_WRONLY);
	fcntl(fp_write, F_SETPIPE_SZ,FIFO_SIZE);

	dirptr = opendir(path);

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
		// Eger bir directory bulunmus ise o directory icin yeni bir process 
		// ve haberlesme icin bir pipe acilir.
		if(S_ISDIR(statbuf.st_mode)){

			if(pipe(pipefd) == -1){
				printf("Pipe Failed!\n");
				exit(0);
			}
	
			switch(pid = fork()){
				case -1:
					printf("Fork Failed!\n");
					exit(0);
					// chield process icin pipe in okuma descriptor u kapatilir.
				case 0:
					result = 0;
					strcpy(path,str);
					closedir(dirptr);
					dirptr = opendir(path);
					pipe_count = 0;
					specialFile = 0;
					write_pipe = dup(pipefd[1]);

					close(pipefd[0]);
					close(pipefd[1]);
					break;
				//Parent process icin pipe in yazma descriptoru kapatilir.
				// bir dosya altinda birden fazla dosya olabilecegi icin read descriptorleri bir array de toplanir 
				default:
					read_pipe[pipe_count] = dup(pipefd[0]);
					pipe_count++;
					close(pipefd[0]);
					close(pipefd[1]);

			}

		}
		// Eger dosya bir directory degil ise boyutu hesaplanmak icin pathfun cagirilir.
		// Eger pathfun -1 yani error return etmis ise sonuca eklenmez.
		else{
			pathRes = pathfun(str);
			if (pathRes == -1){
				specialFile = 1;
				special = (char *) malloc(strlen(pathname(str)) +1);
				strcpy(special,pathname(str));
			}

			else
				result += pathRes;		
		}

		free(str);
	}

	// Eger dosya acmada bir sorun olmus ise bilgilendirme mesaji ekrana basilir.
	if (errno == EACCES || errno == ENOENT) {
		printf("%d 		Cannot read folder %s\n",getpid(),path);
		errno = 0;
		result = -1;
	}
	char buffer[BUFFER_SIZE] = "";

	// parent process chieldlarin pipe a attiklari sizelari okur ve onlari kendi boyutuna ekler.
	// Sonrasinda sonucu fifo ya yazar.
	for (int i = 0; i < pipe_count; ++i){
		int temp=0;
		int pid;
		char buf[BUFFER_SIZE] = "";

		read(read_pipe[i],buffer,BUFFER_SIZE);
		close(read_pipe[i]);
		sscanf(buffer,"%d %s %d",&temp,buf,&pid);

		sprintf(buffer,"%d 	%d 	%s\n",pid,temp,buf);

		if(write(fp_write,buffer,strlen(buffer)) == -1){

			printf("Write error!\n");
			exit(0);
		}

		if (temp != -1 && !flag)
			result += temp;
	}
	// Eger acik bir pipe var ise sonucu oraya yazar.
	if (specialFile == 1){
		sprintf(buffer,"%d 	-1 	Special file %s\n",getpid(),special);
		free(special);
		if(write(fp_write,buffer,strlen(buffer)) == -1){
			printf("Write error!\n");
				exit(0);
		}
	
	}
	if (write_pipe == -1){

		sprintf(buffer,"%d 	%d 	%s\n",getpid(),result,path);

		if(write(fp_write,buffer,strlen(buffer)) == -1){

			printf("Write error!\n");
			exit(0);
		}
		
	}
	// acik bir pipe yok ise sonucu fifo ya yazar.
	else{

		strcpy(buffer,"");
		sprintf(buffer,"% d %s %d ",result,path,getpid());
		if(write(write_pipe,buffer,strlen(buffer)) == -1){
			
			printf("Write error!\n");
			exit(0);
		}
		close(write_pipe);			
	}

	closedir(dirptr);
	close(fp_write);

	exit(1);
}

/*
	verilen pathdeki file bir regular file ise size i return edilir.
	degil ise special file yazilip -1 return edilir.
*/
int sizepathfun(char *path){

		struct stat statbuf;
		lstat(path,&statbuf);
		if (!S_ISREG(statbuf.st_mode))
			return -1;
		
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
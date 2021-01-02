#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>

#define BUF_SIZE 1024
#define TOKEN_SIZE 100
#define SECOND_TO_MICRO 1000000

void ssu_runtime(struct timeval *begin_t, struct timeval *end_t); //실행시간 측정
void move_dst(char original_dst[PATH_MAX]); //입력받은 dst의 파일들을 저장할 디렉토리 만들기
int copy_src(char original_dst[PATH_MAX]); //입력받은 src의 파일 중 동기화할 파일 저장
void file_name(void); //입력받은 src가 파일이라면 파일이름 구하기
void remove_dst(char original_dst[PATH_MAX]); //임시로 옮겨놓은 dst파일들 dst로 이동
void print_log(char src[BUF_SIZE], char dst[BUF_SIZE]); //로그 출력
void delete_dst(void); //동기화가 취소된 경우 이미 동기화된 파일들 모두 삭제
void *rsync(void *arg); //쓰레드를 생성하여 동기화하는 함수
static void ssu_signal_handler(int signo); //SIGINT 시그널 핸들러 등록

char cwd[PATH_MAX]={0,}; //현재의 작업디렉토리 경로
char absolute_path_dst[PATH_MAX]={0,}; //dst의 절대경로
char absolute_path_src[PATH_MAX]={0,}; //src의 절대경로
char src_file[BUF_SIZE][PATH_MAX]={0,}; //src의 동기화할 파일들 이름
char file_src[PATH_MAX]={0,}; //src에 디렉토리가 아니라 파일이 입력되었을 때 저장
struct timeval begin_t; //실행하기 전 시간
int cancel_num=0;

int main(int argc, char*argv[])
{
	struct stat src_statbuf;
	struct stat dst_statbuf;
	char original_dst[PATH_MAX]={0,};
	int status;
	struct timeval end_t; //실행 후 시간
	gettimeofday(&begin_t, NULL); //실행 전 시간
	
	if(getcwd(cwd, PATH_MAX)==NULL){ //현재 작업경로 구하기
		fprintf(stderr, "get cwd error\n");
		exit(1);}

	if(argc<3){ //인자 갯수 확인
		fprintf(stderr, "usage : ssu_rsync <src> <dst>\n");
		exit(1);
	}

	else if(argc==4){ //옵션도 입력한경우
		}

	else if(argc==3){ //옵션없이 입력한 경우

		pthread_t tid;

		if(signal(SIGINT, ssu_signal_handler) == SIG_ERR){ //SIGINT 시그널 핸들러 등록
			fprintf(stderr, "cannot handler SIGINT\n");
			exit(1);
		}
	
		/*src의 파일 존재 확인*/
		if(access(argv[1], F_OK)==-1){ //입력받은 src가 존재하는지 확인
			fprintf(stderr, "usage : 입력하신 src는 존재하지 않습니다.\n");
			exit(1);}
		if(realpath(argv[1], absolute_path_src)==NULL){ //dst의 절대경로 구하기
			fprintf(stderr, "src의 절대경로를 구할 수 없습니다.\n");
			exit(1);
		}

		/*dst의 파일 존재, 접근권한 확인*/
		if(access(argv[2], F_OK)==-1){ //입력받은 dst가 존재하는지 확인
			fprintf(stderr, "usage : 입력하신 dst는 존재하지 않습니다.\n");
			exit(1);}
		if(access(argv[2], R_OK|W_OK|X_OK)==-1){ //입력받은 dst의 접근권한 확인
			fprintf(stderr, "usage : 입력하신 dst의 접근권한이 없습니다.\n");
			exit(1);}
		if(realpath(argv[2], absolute_path_dst)==NULL){ //dst의 절대경로 구하기
			fprintf(stderr, "dst의 절대경로를 구할 수 없습니다.\n");
			exit(1);}
		
		/*dst 디렉토리인지 판별*/
		stat(absolute_path_dst, &dst_statbuf);
		if(!(S_ISDIR(dst_statbuf.st_mode))){ //입력받은 dst 디렉토리 파일인지 확인
			fprintf(stderr, "입력하신 dst는 디렉토리 파일이 아닙니다.\n");
			exit(1);}

		move_dst(original_dst); //dst에 원래 있던 파일들을 저장할 디렉토리 만들기

		/*src 파일 디렉토리인지 판별*/
		stat(absolute_path_src, &src_statbuf);
		if(S_ISDIR(src_statbuf.st_mode)){ //디렉토리라면
			if(access(argv[1], R_OK|X_OK)==-1){ //입력받은 src의 접근권한 확인
				fprintf(stderr, "usage : 입력하신 src의 접근권한이 없습니다.\n");
				remove_dst(original_dst); //임시저장했던 dst의 파일들 복구하기
				exit(1);}

			if(copy_src(original_dst)!=0){ //src의 파일 중 동기화할 파일 이름 저장
				remove_dst(original_dst);
				exit(1);}
		}
		else if(S_ISREG(src_statbuf.st_mode)){ //파일이라면
			file_name(); //파일이름 구하기
		}
		else{ //디렉토리, 파일이 아니면 에러
			fprintf(stderr, "<src> : 파일 및 디렉토리를 입력해주세요.\n");
			remove_dst(original_dst); //임시저장했던 dst의 파일들 복구하기
			exit(1);}

		if(pthread_create(&tid, NULL, rsync, NULL)!=0){ //쓰레드 생성하여 동기화 실행
			fprintf(stderr, "pthread_create error\n");
			exit(1);}
		
		pthread_join(tid, (void*)&status); //pthread 종료 기다림
		
		if(cancel_num==0) //동기화가 정상 종료된 경우 로그 출력
			print_log(argv[1], argv[2]); //동기화가 완료되었다면 log찍기
		
		else if(cancel_num==-1){ //SIGINT시그널로 동기화가 취소된 경우
			delete_dst(); //이미 동기화된 파일들 삭제
		}

		remove_dst(original_dst); //임시저장했던 dst의 파일들 복구하기
	}
	gettimeofday(&end_t, NULL); //실행 후 시간
	ssu_runtime(&begin_t, &end_t); //실행시간 구하기
	exit(0);
}

void ssu_runtime(struct timeval *begin_t, struct timeval *end_t) //실행시간 구하기
{
	end_t->tv_sec -= begin_t->tv_sec; //실행이 끝났을 때의 초에서 실행 시작 전 초를 빼서 실행시간을 구한다.

	if(end_t->tv_usec < begin_t->tv_usec){ //실행 전의 usec가 더 크다면
		end_t->tv_sec--; //end의 초를 하나 감소하여
		end_t->tv_usec += SECOND_TO_MICRO; //단위를 바꿔 usec에 더한다.
	}

	end_t->tv_usec -= begin_t->tv_usec; //실행 후에서 전을 뺀 시간을 usec 단위로 구한다.
	printf("Runtime: %ld:%06ld(sec:usec)\n", end_t->tv_sec, end_t->tv_usec);
}

void move_dst(char original_dst[PATH_MAX]) //동기화를 하기 전, dst의 모든 파일 옮기기
{
	char tmp[TOKEN_SIZE][TOKEN_SIZE]={0,}; //dst의 파일이름을 저장할 버퍼
	struct dirent *dirp;
	DIR *dp;
	int file=0;

	sprintf(original_dst, "%s/original_dst", cwd); //원래의 dst의 파일,디렉토리들을 저장할 디렉토리
	
	mkdir(original_dst, 0777); //저장해둘 디렉토리 만들기

	chdir(absolute_path_dst); //dst 디렉토리로 이동

	if((dp=opendir(absolute_path_dst))==NULL){
		fprintf(stderr, "dst opendir error\n");
		exit(1);
	}

	while((dirp=readdir(dp))!=NULL){ //dst의 파일 이름 저장
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) //'.'과 '..'는 현재 폴더와 이전 폴더를 가리키므로 제외
			continue;

		sprintf(tmp[file], "%s", dirp->d_name); //tmp에 그 파일의 이름을 씀
		file++;
	}

	closedir(dp); //오픈한 디렉토리 닫기

	for(int i=0; i<file; i++){ //dst의 각각의 파일들 임시 저장할 디렉토리로 이동
		char dst_file_path[PATH_MAX]={0,};
		char copy_file_path[PATH_MAX]={0,};
		sprintf(dst_file_path, "%s/%s", absolute_path_dst, tmp[i]); //원래의 파일이름
		sprintf(copy_file_path, "%s/%s", original_dst, tmp[i]); //이동한 파일이름
		
		if(rename(dst_file_path, copy_file_path)!=0)
			fprintf(stderr, "rename error\n");
	}

	chdir(cwd); //원래의 작업디렉토리로 이동
}

void remove_dst(char original_dst[PATH_MAX]) //동기화가 완료되고 옮겼던 dst의 모든 파일 다시 dst로 이동
{
	char tmp[TOKEN_SIZE][TOKEN_SIZE]={0,}; //dst의 파일이름을 저장할 버퍼
	struct dirent *dirp;
	DIR *dp;
	int file=0;

	chdir(original_dst); //저장해둔 디렉토리로 이동

	if((dp=opendir(original_dst))==NULL){
		fprintf(stderr, "dst opendir error\n");
		exit(1);
	}

	while((dirp=readdir(dp))!=NULL){ //dst의 파일 이름 저장
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) //'.'과 '..'는 현재 폴더와 이전 폴더를 가리키므로 제외
			continue;

		sprintf(tmp[file], "%s", dirp->d_name); //tmp에 그 파일의 이름을 씀
		file++;
	}

	closedir(dp); //오픈한 디렉토리 닫기

	for(int i=0; i<file; i++){ //임시 저장했던 dst의 파일들 다시 dst 디렉토리로 이동
		char dst_file_path[PATH_MAX]={0,};
		char copy_file_path[PATH_MAX]={0,};
		sprintf(copy_file_path, "%s/%s", original_dst, tmp[i]); //현재의 파일이름
		sprintf(dst_file_path, "%s/%s", absolute_path_dst, tmp[i]); //이동한 파일이름

		if(access(dst_file_path, F_OK)==0){ //동기화로 인해 같은 이름의 파일이 존재하는 경우
			if(remove(copy_file_path)==-1) //그 파일은 삭제
				fprintf(stderr, "dst file remove failed\n");
			continue;
		}

		if(rename(copy_file_path, dst_file_path)!=0) //원래의 dst파일들 이동
			fprintf(stderr, "rename error\n");
	}

	if(remove(original_dst)==-1) //이동이 다 끝나고 비어있는 임시저장 디렉토리 삭제
		fprintf(stderr, "buf dir remove failed\n");

	chdir(cwd); //원래의 작업디렉토리로 이동
}

int copy_src(char original_dst[PATH_MAX]){ //입력받은 src의 파일 중 동기화할 파일 저장
	struct dirent *dirp;	
	DIR *dp;
	int src_num=0;
	struct stat src_statbuf;
	struct stat dst_statbuf;
	char tmp[BUF_SIZE][PATH_MAX]={0,};

	chdir(absolute_path_src); //src 디렉토리로 이동
	
	if((dp=opendir(absolute_path_src))==NULL){
		fprintf(stderr, "src opendir error\n");
		exit(1);
	}

	while((dirp=readdir(dp))!=NULL){ //src의 동기화할 파일 이름 저장
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) //'.'과 '..'는 현재 폴더와 이전 폴더를 가리키므로 제외
			continue;

		stat(dirp->d_name, &src_statbuf);
		if(S_ISDIR(src_statbuf.st_mode)) //디렉토리라면 제외
			continue;

		char origin_file[PATH_MAX]={0,};
		sprintf(origin_file, "%s/%s", original_dst, dirp->d_name);
		if(access(origin_file, F_OK)==0){ //dst에 이미 같은 이름의 파일이 존재한다면
			lstat(origin_file, &dst_statbuf);
			char dst_mtime[BUF_SIZE]={0,};
			char src_mtime[BUF_SIZE]={0,};
			sprintf(dst_mtime, "%ld", dst_statbuf.st_mtime);
			sprintf(src_mtime, "%ld", src_statbuf.st_mtime);
			if(strcmp(dst_mtime, src_mtime)==0&&(dst_statbuf.st_size==src_statbuf.st_size)) //수정시간과 파일의 크기가 같다면
				continue; //해당 파일은 동기화 안함
		}

		sprintf(src_file[src_num], "%s", dirp->d_name); //인자로 받은 배열에 각각의 파일의 이름 저장
		src_num++;
	}

	closedir(dp); //오픈한 디렉토리 닫기
	chdir(cwd); //원래의 작업디렉토리로 이동
	return 0;
}

void print_log(char src[BUF_SIZE], char dst[BUF_SIZE]) //동기화가 완료된 경우 로그 출력
{
	time_t t = time(NULL); //현재시간 구하기
	char run_time[BUF_SIZE] = {0,}; //실행시간 저장
	FILE *fp;
	struct stat statbuf;
	
	if((fp = fopen("ssu_rsync_log", "a"))==NULL) //ssu_rsync_log 파일 오픈
		fprintf(stderr, "log open error\n");

	strncpy(run_time, ctime(&t), strlen(ctime(&t))-1);
	fprintf(fp, "[%s] ssu_rsync %s %s\n", run_time, src, dst);
	
	if(strlen(file_src)>0){ //src가 일반파일인 경우
		stat(absolute_path_src, &statbuf);
		fprintf(fp, "       %s %ldbytes\n", file_src, statbuf.st_size);
	}

	else { //src가 디렉토리인 경우
		chdir(absolute_path_src); //src 디렉토리로 이동

		for(int i=0; strlen(src_file[i])>0; i++){ //로그에 동기화한 파일 쓰기
			stat(src_file[i], &statbuf);
			fprintf(fp, "       %s  %ldbytes\n", src_file[i], statbuf.st_size);
		}

		chdir(cwd); //원래의 작업디렉토리로 이동
	}

	fclose(fp); //오픈한 로그 파일 닫기
}

void file_name(void) //src가 일반파일인 경우 파일이름 구하기
{
	char src_name[BUF_SIZE]={0,};
	sprintf(src_name, "%s", absolute_path_src); //입력받은 src의 절대경로 저장
	char *ptr = src_name; 
	
	while((strchr(ptr,'/'))!=NULL) //경로를 제외한 파일 이름 구하기
		ptr++;
	strcpy(file_src, ptr); //파일 이름 저장
}
		

void delete_dst() //동기화가 취소되었으므로 dst에 이미 동기화되었던 파일 모두 삭제
{
	struct dirent *dirp;
	DIR *dp;

	chdir(absolute_path_dst); //dst 디렉토리로 이동

	if((dp=opendir(absolute_path_dst))==NULL){
		fprintf(stderr, "dst opendir error\n");
		exit(1);
	}

	while((dirp=readdir(dp))!=NULL){ //dst의 파일 이름 저장
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) //'.'과 '..'는 현재 폴더와 이전 폴더를 가리키므로 제외
			continue;

		char tmp[BUF_SIZE]={0,}; //dst의 파일이름을 저장할 버퍼
		sprintf(tmp, "%s/%s", absolute_path_dst, dirp->d_name); //tmp에 그 파일의 절대경로 저장
		if(remove(tmp)!=0) //이미 dst에 동기화된 파일 삭제
			fprintf(stderr, "rsync dst_file remove error\n");
	}

	closedir(dp); //오픈한 디렉토리 닫기
	chdir(cwd);//원래의 작업디렉토리로 이동
}

static void ssu_signal_handler(int signo) //SIGINT 발생 시 동기화 취소
{
	cancel_num = -1;
}
	
void *rsync(void *arg) //동기화
{	
	struct stat statbuf;
	mode_t cancel_mode;

	if(strlen(file_src)>0){ //일반파일을 동기화하는 경우
		stat(absolute_path_src, &statbuf);
		cancel_mode = statbuf.st_mode; //원래의 접근권한 저장
		chmod(absolute_path_src,0000); //동기화 중 파일을 오픈하지 못하도록 접근권한 변경
		char dst_file[PATH_MAX]={0,};
		sprintf(dst_file, "%s/%s", absolute_path_dst, file_src); //dst_file에 동기화 이름 저장
		
		if(link(absolute_path_src, dst_file)!=0) //dst로 동기화
			fprintf(stderr, "link error\n");

		chmod(absolute_path_src, cancel_mode); //원래의 접근권한으로 되돌림
		
		if(cancel_num==-1) //동기화가 취소되는 경우
			pthread_exit(PTHREAD_CANCELED);
	}

	else {//디렉토리를 동기화하는 경우
		for(int i=0; strlen(src_file[i])>0; i++){
				char dst_file[PATH_MAX]={0,};
				char rsync_file[PATH_MAX]={0,};
				sprintf(dst_file, "%s/%s", absolute_path_dst, src_file[i]);
				sprintf(rsync_file, "%s/%s", absolute_path_src, src_file[i]);
				stat(rsync_file, &statbuf);
				cancel_mode = statbuf.st_mode; //원래의 접근권한 저장
				
				chmod(rsync_file, 0000); //동기화하는 동안 파일의 접근권한 바꾸기

				if(link(rsync_file, dst_file)!=0)
					fprintf(stderr, "link error\n");

				chmod(rsync_file, cancel_mode); //원래의 접근권한으로 되돌림
				
				if(cancel_num==-1) //동기화가 취소되는경우
					pthread_exit(PTHREAD_CANCELED);	
			}
	}
	return NULL;
}

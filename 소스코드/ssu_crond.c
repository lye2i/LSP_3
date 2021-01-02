#include <stdio.h> //printf, fprintf
#include <stdlib.h> //exit
#include <unistd.h> //sleep, getpid, fork, setsid, dup, chdir
#include <fcntl.h> //open
#include <signal.h> //signal
#include <syslog.h> //openlog, syslog, closelog
#include <sys/stat.h> //umask, open
#include <sys/types.h> //umask, open
#include <string.h>
#include <dirent.h>
#include <time.h>

#define BUF_SIZE 1024
#define TOKEN_SIZE 100
#define TIME_SIZE 5
int ssu_daemon_init(void);

int min(char token[TOKEN_SIZE], struct tm *ptime); //분 비교
int hour(char token[TOKEN_SIZE], struct tm *ptime); //시 비교
int day(char token[TOKEN_SIZE], struct tm *ptime); //일 비교
int month(char token[TOKEN_SIZE], struct tm *ptine); //월 비교
int week(char token[TOKEN_SIZE], struct tm *ptime); //요일 비교
int comma(char token[TOKEN_SIZE], char buf[TIME_SIZE], int start); //','기호일 때 확인
int range(char token[TOKEN_SIZE], char buf[TIME_SIZE]); //'-'기호일 때 확인
int slash(char token[TOKEN_SIZE], char buf[TIME_SIZE], int start); //'/'기호확인

int first = 0;
int main(void)
{

	if(ssu_daemon_init()<0) { //디몬 코딩 수행 준비 에러시
		fprintf(stderr, "ssu_daemon_init failed\n"); //오류메시지 출력
		exit(1); //에러시 종료
	}

	exit(0); //종료
}

int min(char token[TOKEN_SIZE], struct tm *ptime) //분 비교
{
	char min_buf[TIME_SIZE] = {0,};
	sprintf(min_buf, "%d", ptime->tm_min);
	
	if(strlen(token)==1){ //한자릿수 일때
		if(token[0]=='*') //*인 경우
			return 1;

		else if(strpbrk(token, "0123456789")!=NULL){ //숫자인 경우
			if(strcmp(token, min_buf)==0) //현재 분과 같다면
				return 1;
			else //다르다면
				return 0;}
	}

	else if(strlen(token)==2){ //두자릿 수 일 때는 숫자만 올 수 있으므로
		if(strcmp(token, min_buf)==0)
			return 1;
		else
			return 0;
	}

	else if(strchr(token, ',')!=NULL){ //','기호가 있는 경우
		if(comma(token, min_buf, 0))
			return 1;
		else 
			return 0;
	}

	else if(strchr(token, '/')!=NULL){ //'/'기호가 있는 경우
		if(slash(token, min_buf, 0))
			return 1;
		else
			return 0;
	}

	else if(strchr(token, '-')!=NULL){ //'-'기호가 있는 경우
		if(range(token, min_buf))
			return 1;
		else
			return 0;
	}
}

int hour(char token[TOKEN_SIZE], struct tm *ptime) //시 비교
{
	char hour_buf[TIME_SIZE] = {0,};
	sprintf(hour_buf, "%d", ptime->tm_hour);

	if(strlen(token)==1){ //한자릿수 일때
		if(token[0]=='*') //*인 경우
			return 1;

		else if(strpbrk(token, "0123456789")!=NULL){ //숫자인 경우
			if(strcmp(token, hour_buf)==0) //현재 분과 같다면
				return 1;
			else //다르다면
				return 0;}
	}

	else if(strlen(token)==2){ //두자릿 수 일 때는 숫자만 올 수 있으므로
		if(strcmp(token, hour_buf)==0)
			return 1;
		else
			return 0;
	}

	else if(strchr(token, ',')!=NULL){ //','기호가 있는 경우
		if(comma(token, hour_buf, 0))
			return 1;
		else
			return 0;
	}

	else if(strchr(token, '/')!=NULL){ //'/'기호가 있는 경우
		if(slash(token, hour_buf, 0))
			return 1;
		else
			return 0;
	}

	else if(strchr(token, '-')!=NULL){ //'-'기호가 있는 경우
		if(range(token, hour_buf))
			return 1;
		else
			return 0;
	}
}

int day(char token[TOKEN_SIZE], struct tm *ptime) //일 비교
{
	char day_buf[TIME_SIZE] = {0,};
	sprintf(day_buf, "%d", ptime->tm_mday);

	if(strlen(token)==1){ //한자릿수 일때
		if(token[0]=='*') //*인 경우
			return 1;

		else if(strpbrk(token, "123456789")!=NULL){ //숫자인 경우
			if(strcmp(token, day_buf)==0) //현재 분과 같다면
				return 1;
			else //다르다면
				return 0;}
	}

	else if(strlen(token)==2){ //두자릿 수 일 때는 숫자만 올 수 있으므로
		if(strcmp(token, day_buf)==0)
			return 1;
		else
			return 0;
	}

	else if(strchr(token, ',')!=NULL){ //','기호가 있는 경우
		if(comma(token, day_buf, 1))
			return 1;
		else
			return 0;
	}

	else if(strchr(token, '/')!=NULL){ //'/'기호가 있는 경우
		if(slash(token, day_buf, 1))
			return 1;
		else
			return 0;
	}

	else if(strchr(token, '-')!=NULL){ //'-'기호가 있는 경우
		if(range(token, day_buf))
			return 1;
		else
			return 0;
	}
}

int month(char token[TOKEN_SIZE], struct tm *ptime) //월 비교
{
	char month_buf[TIME_SIZE] = {0,};
	sprintf(month_buf,"%d", ptime->tm_mon+1);
	
	if(strlen(token)==1){ //한자릿수 일때
		if(token[0]=='*') //*인 경우
			return 1;

		else if(strpbrk(token, "123456789")!=NULL){ //숫자인 경우
			if(strcmp(token, month_buf)==0) //현재 분과 같다면
				return 1;
			else //다르다면
				return 0;}
	}

	else if(strlen(token)==2){ //두자릿 수 일 때는 숫자만 올 수 있으므로
		if(strcmp(token, month_buf)==0)
			return 1;
		else
			return 0;
	}

	else if(strchr(token, ',')!=NULL){ //','기호가 있는 경우
		if(comma(token, month_buf, 1))
			return 1;
		else
			return 0;
	}

	else if(strchr(token, '/')!=NULL){ //'/'기호가 있는 경우
		if(slash(token, month_buf, 1))
			return 1;
		else
			return 0;
	}

	else if(strchr(token, '-')!=NULL){ //'-'기호가 있는 경우
		if(range(token, month_buf))
			return 1;
		else
			return 0;
	}
}

int week(char token[TOKEN_SIZE], struct tm *ptime) //요일 비교
{
	char week_buf[TIME_SIZE] = {0,};
	sprintf(week_buf, "%d",ptime->tm_wday);
	
	if(strlen(token)==1){ //한자릿수 일때
		if(token[0]=='*') //*인 경우
			return 1;

		else if(strpbrk(token, "0123456")!=NULL){ //숫자인 경우
			if(strcmp(token, week_buf)==0) //현재 분과 같다면
				return 1;
			else //다르다면
				return 0;}
	}

	else if(strchr(token, ',')!=NULL){ //','기호가 있는 경우
		if(comma(token, week_buf, 0))
			return 1;
		else
			return 0;
	}

	else if(strchr(token, '/')!=NULL){ //'/'기호가 있는 경우
		if(slash(token, week_buf, 0))
			return 1;
		else
			return 0;
	}

	else if(strchr(token, '-')!=NULL){ //'-'기호가 있는 경우
		if(range(token, week_buf))
			return 1;
		else
			return 0;
	}
}

int comma(char token[TOKEN_SIZE], char buf[TIME_SIZE], int start) //','기호일 때 확인
{
	char comma_buf[TOKEN_SIZE]={0,}; //인자로 받은 문자열 저장 버퍼
	char comma_token[TOKEN_SIZE][TOKEN_SIZE]={0,}; //comma를 기준으로 자른 토큰 저장
	sprintf(comma_buf, "%s", token); //인자로 받은 문자열 저장
	int comma_num=0;

	char *ptr = strtok(comma_buf, ","); //comma를 기준으로 자르기
	
	while(ptr!=NULL) //','을 기준으로 문자열을 잘라 입력된 다른 값들 저장
	{
		sprintf(comma_token[comma_num], "%s", ptr);
		comma_num++;
		ptr = strtok(NULL, ",");
	}

	for(int j=0; j<comma_num; j++) //각각의 토큰들 시간 비교
	{
		if(strlen(comma_token[j])>2&&strchr(comma_token[j],'/')!=NULL){ //'/'를 포함한 값이라면
			if(slash(comma_token[j], buf, start)) //'/'규칙 확인
				return 1;
			continue;
		}

		else if(strlen(comma_token[j])>2&&strchr(comma_token[j],'-')!=NULL){ //'-'를 포함한 값이라면
			if(range(comma_token[j], buf)) //'-'규칙 확인
				return 1;
			continue;
		}

		else if(strlen(comma_token[j])<3){ //숫자만 입력된 경우
			if(strcmp(comma_token[j],buf)==0) //현재의 해당 시간과 일치한다면
				return 1;
		}
	}

	return 0;
}

int range(char token[TOKEN_SIZE], char buf[TIME_SIZE]) //'-' 기호 확인
{
	char range_token[TOKEN_SIZE][TOKEN_SIZE] = {0,}; //'-'를 기준으로 자른 토큰 저장
	char range_buf[TOKEN_SIZE] = {0,}; //인자로 받은 문자열 저장 버퍼
	sprintf(range_buf, "%s", token); //인자로 받은 문자열 저장
	int range_num=0;

	char *ptr = strtok(range_buf, "-"); //'-'를 기준으로 자르기
	while(ptr!=NULL) //'-'기준으로 문자열을 잘라 입력된 다른 값들 저장
	{
		sprintf(range_token[range_num], "%s", ptr);
		range_num++;
		ptr = strtok(NULL, "-");
	}

	int num1 = atoi(range_token[0]); //앞의 정수
	int num2 = atoi(range_token[1]); //뒤의 정수
	int buf_t = atoi(buf); //현재시간

	if(buf_t>=num1&&buf_t<=num2) //각각의 정수가 현재 시간 범위에 만족한다면
		return 1;

	return 0; //아니라면
}

int slash(char token[TOKEN_SIZE], char buf[TIME_SIZE], int start) //'/'기호 확인
{
	char slash_token[TOKEN_SIZE][TOKEN_SIZE] = {0,}; //'/'를 기준으로 자른 토큰 저장
	char slash_buf[TOKEN_SIZE] = {0,}; //인자로 받은 문자열 저장 버퍼
	sprintf(slash_buf, "%s", token); //인자로 받은 문자열 저장
	int slash_num = 0;
	char *ptr = strtok(slash_buf, "/"); //'/'를 기준으로 자르기
	int buf_t = atoi(buf); //현재시간 저장
	
	while(ptr!=NULL) //'/'를 기준으로 문자열을 잘라 입력된 다른 값들 저장
	{
		sprintf(slash_token[slash_num], "%s", ptr);
		slash_num++;
		ptr = strtok(NULL, "/");
	}

	int num = atoi(slash_token[1]); //구간 설정 숫자 저장

	if(strcmp(slash_token[0], "*")==0){ //"*"라면
		if(buf_t%num==start) //현재 시간이 해당 주기에 들어있다면
			return 1;
		else
			return 0;
	}

	else if(strlen(slash_token[0])>2){ //범위가 들어있다면
		char range_token[2][TIME_SIZE]={0,}; //숫자 토큰
		char *r_ptr = strtok(slash_token[0], "-"); //범위의 숫자 구하기
		int i=0;
		while(r_ptr!=NULL){//범위의 숫자 구하기
			sprintf(range_token[i], "%s", r_ptr);
			i++;
			r_ptr=strtok(NULL,"-");}

		int num1 = atoi(range_token[0]); //앞의 숫자
		int num2 = atoi(range_token[1]); //뒤의 숫자
		
		while(num1<=num2){
			if(num1==buf_t) //현재시간과 해당 숫자가 일치한다면
				return 1;
			
			num1 = num1 + num;} //아니라면 지정숫자만큼 더하기

		return 0; //해당 범위에 속하지 않았다면
	}
}

int ssu_daemon_init(void) //디몬코딩수행 준비
{
	pid_t pid;
	pid_t sys_pid;
	int fd, maxfd;
	char *log = "ssu_crontab_log";

	if((pid = fork()) < 0) {//자식 프로세스 생성
		fprintf(stderr, "fork error\n");
		exit(1);}

	else if(pid != 0) //부모프로세스라면
		exit(0); //종료

	setsid(); //자식프로세스를 새로운 세션리더로 만든다.

	/*티미널 입출력 시그널 무시*/
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	maxfd = getdtablesize(); //허용된 파일디스크립터 갯수 구하기

	/*허용된 파일디스크립터 닫기*/
	for(fd = 0; fd<maxfd; fd++)
		close(fd);

	umask(0); //파일모드 생성 마스크 해제

	dup(0);
	dup(0);

	sys_pid = fork();

	while(1){
		time_t t;
		time(&t);
		struct tm *ptime = localtime(&t); //현재 시간 구하기
		char *log = "ssu_crontab_log";
		char *crontab_file = "ssu_crontab_file";
		FILE *log_fp;
		FILE *file_fp;
		char file[TOKEN_SIZE][TOKEN_SIZE]={0,}; //crontab파일 읽기
		char run_time[BUF_SIZE]={0,}; //실행시간
		char command[BUF_SIZE] = {0,};
		int command_num=0;

		if((file_fp = fopen(crontab_file, "a+"))==NULL){ //crontab_file 오픈
			fprintf(stderr, "fopen error for %s\n", crontab_file);
			exit(1);
		}

		while(fgets(file[command_num], BUF_SIZE, file_fp)!=NULL) //파일의 내용을 줄단위로 읽음
			command_num++;

		fclose(file_fp); //crontab_file 닫기

		if((first==0&&ptime->tm_sec==0)||first!=0){
			if((log_fp = fopen(log, "a+"))==NULL){ //log파일 오픈
				fprintf(stderr, "fopen error for %s\n", log);
				exit(1);
			}

			strncpy(run_time, ctime(&t), strlen(ctime(&t))-1);

			for(int i=0; i<command_num; i++){ //각각의 명령어 실행 비교
				int token_num=0;
				char buf[BUF_SIZE]={0,}; //명령어를 저장할 버퍼
				char token[TOKEN_SIZE][TOKEN_SIZE]={0,}; //토큰단위로 저장할 버퍼
				char command[BUF_SIZE] = {0,}; //실행할 명령어
				strcpy(buf, file[i]); //버퍼에 저장
				char *ptr = strtok(buf, " "); //공백을 기준으로 입력받은 명령어 문자열 자르기
				while(ptr!=NULL){
					strncpy(token[token_num], ptr, strlen(ptr));
					token_num++;
					if(token_num==5){ //명령어 구하기
						ptr=strtok(NULL, "\n");
						sprintf(token[token_num], "%s", ptr);
						break;}
					ptr = strtok(NULL, " "); //다음 문자열 잘라서 포인터 반환
				}
	
				if(min(token[0], ptime) && hour(token[1], ptime) && day(token[2], ptime) && month(token[3], ptime) && week(token[4], ptime)){ //실행주기와 일치한다면
					if(sys_pid==0) //자식프로세스는
						system(token[5]); //명령어 실행
					else if(sys_pid>0) //부모프로세스는
						fprintf(log_fp, "[%s] run %s", run_time, file[i]); //로그 출력
				}
			}

			fclose(log_fp); //오픈한 log파일 닫기
			first++;
			sleep(60); //60초 대기
		}
	}
}

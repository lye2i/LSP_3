#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define BUF_SIZE 1024
#define TOKEN_SIZE 100
#define SECOND_TO_MICRO 1000000

void ssu_runtime(struct timeval *begin_t, struct timeval *end_t); //실행시간 측정
int check_command(char command_token[BUF_SIZE][BUF_SIZE]); //입력받은 명령어 확인
void add_command(char command_line[BUF_SIZE], char command_token[BUF_SIZE][BUF_SIZE]); //add 명령어 실행
int comma(int i, char token[TOKEN_SIZE]); //실행주기 ','확인
int range(int i, char token[TOKEN_SIZE]); //실행주기 '-'확인
int slash(int i, char token[TOKEN_SIZE]); //실행주기 '/'확인
void num_range(int i, int *start, int *end); //실행주기의 범위 구하기
void remove_command(char crontab_buf[BUF_SIZE][BUF_SIZE]); //remove명령어 실행
void log_print(char command_line[BUF_SIZE]); //로그파일에 기록
_Bool _isdigit(char token[BUF_SIZE]); //문자열이 숫자로만 이루어졌는지 확인

int remove_num = -1;

int main(void)
{
	struct timeval begin_t, end_t; //실행시작하기 전, 후 시간
	gettimeofday(&begin_t, NULL); //실행 전 시간

	while(1)
	{
		FILE *crontab_fp;
		char crontab_buf[BUF_SIZE][BUF_SIZE]={0,};
		int num=0;

		if((crontab_fp = fopen("ssu_crontab_file", "a+"))==NULL){ //crontab파일 오픈
			fprintf(stderr, "ssu_crontab_file open error\n");
			exit(1);}

		while(fgets(crontab_buf[num], BUF_SIZE, crontab_fp)!=NULL) //crontab파일의 줄단위로 읽어서 buf에 저장
			num++;

		fclose(crontab_fp); //오픈한 crontab파일 닫기

		for(int i=0; i<num; i++) //crontab파일에 저장된 모든 명령어 출력
			printf("%d. %s", i, crontab_buf[i]);

		printf("\n");

		char command_line[BUF_SIZE] = {0,}; //command_line
		char command_buf[BUF_SIZE] = {0,}; //토큰으로 쪼개기 위해 입력받은 문자열 복사할 buf
		char command_token[BUF_SIZE][BUF_SIZE] = {0,}; //입력받은 명령어를 토큰단위로 쪼개기
		
		int token_num=0;
		printf("20180787> "); //프롬프트 출력
		fgets(command_line, sizeof(command_line), stdin); //명령어 받기

		if(command_line[0]=='\n') //명령어를 입력받지 않았다면 다시 프롬프트 출력
			continue;
		strncpy(command_buf, command_line, strlen(command_line)-1); //토큰으로 쪼개기 위해 입력받은 개행문자는 빼고 버퍼에 저장

		char *ptr = strtok(command_buf, " "); //공백을 기준으로 입력받은 명령어 문자열 자르기
		while(ptr!=NULL){
			strncpy(command_token[token_num],ptr,strlen(ptr)); //공백문자를 기준으로 입력받은 명령어 문자열 자르기
			token_num++;
			ptr = strtok(NULL," "); //다음 문자열을 잘라서 포인터 반환
		}

		int command_num = check_command(command_token); //입력받은 명령어 확인

		if(command_num==1) //add명령어를 입력받았다면
			add_command(command_line, command_token);
		
		else if(command_num==2){ //remove명령어를 입력받았다면

			if(strlen(command_token[1])<1) //번호를 입력받지 않았다면
				fprintf(stderr, "번호를 입력해주세요.\n");
			else if(_isdigit(command_token[1])&&atoi(command_token[1])<num&&atoi(command_token[1])>=0){ //정확한 번호를 입력받았는지 확인
				remove_num = atoi(command_token[1]); //입력받은 번호 저장
				remove_command(crontab_buf); //remove명령어 실행
				remove_num = -1; //remove명령어 실행 후 초기화
			}
			else //잘못된 번호를 입력받은 경우
				fprintf(stderr, "잘못된 번호입니다.\n");
		}

		else if(command_num==3){ //exit명령어를 입력받았다면
			gettimeofday(&end_t, NULL); //실행 후 시간
			ssu_runtime(&begin_t, &end_t); //실행시간 구하기
			exit(0);
		}
	}

	return 0;
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

int check_command(char command_token[BUF_SIZE][BUF_SIZE]) //입력받은 명령어 확인
{
	if(strcmp(command_token[0],"add")==0) //add명령어를 입력받았다면 리턴1
		return 1;
	
	else if(strcmp(command_token[0], "remove")==0) //remove명령어를 입력받았다면 리턴2
		return 2;

	else if(strcmp(command_token[0], "exit")==0) //exit명령어를 입력받았다면 리턴3
		return 3;

	else{ //지정된 명령어 이외의 다른 명령어를 입력하였다면 에러메시지 출력 후 리턴 0
		fprintf(stderr, "usage : add(명령어 추가), remove(명령어 삭제), exit(프로그램 종료)\n");
		return 0;}
}

void add_command(char command_line[BUF_SIZE],char command_token[BUF_SIZE][BUF_SIZE]) //add명령어 실행
{
	FILE *crontab_fp; 
	char *command_ptr = command_line; //저장할 명령어를 가리티는 포인터
	int i=1;

	if((crontab_fp = fopen("ssu_crontab_file", "a+"))==NULL) //crontab파일 오픈
		fprintf(stderr, "crontab_file open error\n");

	for(i=1; strlen(command_token[i])>0&&i<6; i++){ //입력받은 실행주기 확인

		char *ptr = command_token[i];
		char *sptr;
		int check=0;
		while((sptr=strpbrk(ptr, "0123456789,/-*"))!=NULL){ //사용가능한 문자로만 이루어졌는지 확인
			if(*ptr!=*sptr) //시작포인터가 일치해야 모든 자리가 유효한 문자
				break;
			ptr++; //하나씩 이동하면서 확인
			check++;
		}

		if(check!=strlen(command_token[i])) //유효하지 않은 문자가 포함된다면 실행주기 에러
			break;

		if(strlen(command_token[i])==1){ //각 토큰의 자릿수가 한 자리 라면
			if(i==1 || i==2){ //분,시 일때
				if(strpbrk(command_token[i],"0123456789*")==NULL) //숫자, *이 아닌 다른 문자가 있다면 에러
					break;
			}
			else if(i==3 || i==4){ //일, 월 일때
				if(strpbrk(command_token[i],"123456789*")==NULL) //숫자, *이 아닌 다른 문자가 있다면 에러
					break;
			}
			else{ //요일 일때
				if(strpbrk(command_token[i], "0123456*")==NULL) //숫자, *이 아닌 다른 문자가 있다면 에러
					break;
			}
		}

		else if(_isdigit(command_token[i])){ //각 토큰이 숫자로만 이루어져있다면 지정된 범위를 벗어나면 에러
			int num = atoi(command_token[i]); //해당 숫자 저장
			int start = 0;
			int end = 0;
			num_range(i, &start, &end); //실행주기의 범위 구하기

			if(num<start || num>end) //범위에 벗어난다면
				break;
		}

		else if(strchr(command_token[i], ',')!=NULL){ //',' 기호가 들어있다면
			if(comma(i, command_token[i])==0) //','기호 규칙 확인
				break;
		}

		else if(strchr(command_token[i],'/')!=NULL){ //'/'기호가 들어있다면
			if(slash(i, command_token[i])==0) //'/'기호 규칙 확인
				break;
		}
		
		else if(strchr(command_token[i],'-')!=NULL){ //'-'기호만 들어있다면
			if(range(i,command_token[i])==0) //'-'기호 규칙 확인
				break;
		}

		else
			break;
	}

	if(i==6){ //토큰을 모두 비교했을 때 문제가 없고 실행주기의 목록과 토큰 갯수가 일치한다면 add명령어 실행
		fprintf(crontab_fp, "%s", command_ptr+4); //주기적으로 실행할 명령어 저장 
		log_print(command_line); //저장된 명령어 로그파일에 기록
	}
	
	else
		fprintf(stderr, "잘못된 실행주기 입니다.\n");

	fclose(crontab_fp); //오픈한 crontab파일 닫기
}

int comma(int i, char token[TOKEN_SIZE]) //','기호 확인
{
	char comma_token[TOKEN_SIZE][TOKEN_SIZE] = {0,}; //comma를 기준으로 자른 토큰 저장
	char comma_buf[BUF_SIZE] = {0,}; //인자로 받은 문자열 저장 버퍼
	sprintf(comma_buf, "%s", token); //인자로 받은 문자열 저장
	int comma_num=0;
	int start = 0;
	int end = 0;

	num_range(i, &start, &end); //실행주기의 범위 구하기

	char *ptr = strtok(comma_buf, ","); //comma를 기준으로 자르기
	while(ptr!=NULL) //','을 기준으로 문자열을 잘라 입력된 다른 값들 저장
	{
		sprintf(comma_token[comma_num], "%s", ptr);
		comma_num++;
		ptr = strtok(NULL, ",");
	}

	for(int j=0; j<comma_num; j++)
	{
		if(strcmp(comma_token[j], "*")==0) //'*'를 입력받았다면 에러
			return 0;
		
		else if(strlen(comma_token[j])>2&&strchr(comma_token[j],'/')!=NULL){ //'/'를 포함한 값이라면
			if(slash(i, comma_token[j])==0) //'/'규칙 확인
				return 0;
			continue;
		}

		else if(strlen(comma_token[j])>2&&strchr(comma_token[j],'-')!=NULL){ //'-'를 포함한 값이라면
			if(range(i, comma_token[j])==0) //'-'규칙 확인
				return 0;
			continue;
		}

		int integer = atoi(comma_token[j]); //정수 저장
		
		if(!(_isdigit(comma_token[j])&&integer>=start&&integer<=end)) //정수가 아니거나 지정범위에 속하지 않는다면
			return 0; //해당 범위 사용 못함
		
		for(int k=j+1; k<comma_num; k++) //중복된 값이 입력되었는지 확인
		{
			if(strcmp(comma_token[j],comma_token[k])==0)
				return 0;
		}
	}

	return 1;
}

int range(int i, char token[TOKEN_SIZE]) //'-' 기호 확인
{
	char range_token[TOKEN_SIZE][TOKEN_SIZE] = {0,}; //'-'를 기준으로 자른 토큰 저장
	char range_buf[BUF_SIZE] = {0,}; //인자로 받은 문자열 저장 버퍼
	sprintf(range_buf, "%s", token); //인자로 받은 문자열 저장
	int range_num=0;
	int start = 0;
	int end = 0;
	
	num_range(i, &start, &end); //실행주기의 범위 구하기

	char *ptr = strtok(range_buf, "-"); //'-'를 기준으로 자르기
	while(ptr!=NULL) //'-'기준으로 문자열을 잘라 입력된 다른 값들 저장
	{
		sprintf(range_token[range_num], "%s", ptr);
		range_num++;
		ptr = strtok(NULL, "-");
	}

	if(range_num!=2) //'-'를 기준으로 자른 토큰은 두개여야함
		return 0;

	if(_isdigit(range_token[0]) && _isdigit(range_token[1])){ //'-'를 기준으로 앞 뒤가 정수인지 확인
		int num1 = atoi(range_token[0]); //앞의 정수
		int num2 = atoi(range_token[1]); //뒤의 정수

		if(num1>=start && num1<num2 && num2<=end) //각각의 정수가 범위에 만족한다면
			return 1;
	}

	return 0;
}

int slash(int i, char token[TOKEN_SIZE]) //'/'기호 확인
{
	char slash_token[TOKEN_SIZE][TOKEN_SIZE] = {0,}; //'/'를 기준으로 자른 토큰 저장
	char slash_buf[BUF_SIZE] = {0,}; //인자로 받은 문자열 저장 버퍼
	sprintf(slash_buf, "%s", token); //인자로 받은 문자열 저장
	int slash_num = 0;
	int start = 0;
	int end = 0;

	num_range(i, &start, &end); //실행주기의 범위 구하기

	char *ptr = strtok(slash_buf, "/"); //'/'를 기준으로 자르기
	while(ptr!=NULL) //'/'를 기준으로 문자열을 잘라 입력된 다른 값들 저장
	{
		sprintf(slash_token[slash_num], "%s", ptr);
		slash_num++;
		ptr = strtok(NULL, "/");
	}

	if(slash_num!=2) //'/'를 기준으로 자른 토큰은 두개여야함
		return 0;

	if(strlen(slash_token[0])==1){
		if(strcmp(slash_token[0],"*")!=0) //범위가 한문자일 때 '*'가 아니라면
			return 0;

		if(_isdigit(slash_token[1])){ //뒤의 숫자 확인
			int n = atoi(slash_token[1]);
			if(n>=start&&n<=end)
				return 1;
		}
	}

	else if(strchr(slash_token[0], '-')!=NULL){ //범위 확인
		if(range(i, slash_token[0])==0) //잘못된 범위라면
			return 0;
		
		if(_isdigit(slash_token[1])){ //숫자가 입력되었다면
			int n = atoi(slash_token[1]);
			char *num2 = strtok(slash_token[0], "-");
			char num1[TOKEN_SIZE]={0,};
			strcpy(num1, num2);
			num2 = strtok(NULL,"-");
			int num = atoi(num2)-atoi(num1); //범위의 폭 구하기

			if(n<=num) //뒤에 나온 숫자는 범위의 폭을 넘으면 안됨
				return 1;
		}
	}

	return 0;
}
		
void num_range(int i, int *start, int *end) //실행주기의 범위 구하기
{
	if(i==1){ //분
		*start = 0;
		*end = 59;}

	else if(i==2){ //시
		*start = 0;
		*end = 23;}

	else if(i==3){ //일
		*start = 1;
		*end = 31;}

	else if(i==4){ //월
		*start = 1;
		*end = 12;}

	else{ //요일
		*start = 0;
		*end = 6;}
}
	

void remove_command(char crontab_buf[BUF_SIZE][BUF_SIZE]) //remove명령어 실행
{
	FILE *crontab_fp;

	if((crontab_fp = fopen("ssu_crontab_file", "w"))==NULL) //crontab파일 쓰기 전용으로 오픈 기존의 데이터들 모두 초기화
		fprintf(stderr, "ssu_crontab_file open error\n");

	for(int i=0; strlen(crontab_buf[i])>0; i++){ //저장했던 데이터들 다시 쓰기
		if(i==remove_num) //삭제하려는 명령어라면 쓰지 않음
			continue;
		fprintf(crontab_fp, "%s", crontab_buf[i]);
	}

	fclose(crontab_fp); //오픈한 파일 닫기

	log_print(crontab_buf[remove_num]); //삭제된 명령어 로그파일에 기록
}

void log_print(char command_line[BUF_SIZE]) //ssu_crontab_log 파일에 기록
{
	FILE *log_fp; //ssu_crontab_log
	time_t t = time(NULL); //현재 시간 구하기
	char run_time[BUF_SIZE] = {0,}; //실행시간 저장

	if((log_fp = fopen("ssu_crontab_log", "a"))==NULL){ //ssu_crontab_log파일 오픈
		fprintf(stderr,"log open error\n");
		exit(1);
	}

	strncpy(run_time, ctime(&t), strlen(ctime(&t))-1);

	if(remove_num==-1) //add명령어 실행 시
		fprintf(log_fp, "[%s] %s", run_time, command_line); //저장된 시간과 해당 명령어를 로그파일에 기록
	else //remove명령어 실행 시
		fprintf(log_fp, "[%s] remove %s", run_time, command_line); //삭제된 시간과 해당 명령어>    를 로그파일에 기록

	fclose(log_fp); //오픈한 파일 닫기
}

_Bool _isdigit(char token[BUF_SIZE]) //문자열이 숫자로만 이루어졌는지 확인
{
	int i=atoi(token); //정수로 변환
	char str[BUF_SIZE]={0,}; //변환된 정수를 다시 문자열로 변환
	sprintf(str, "%d", i); //정수를 문자열로 변환

	if(i!=0) //0이 아닌 정수가 나온다면
		return strlen(token)==strlen(str); //그 문자열에 다른 문자가 포함되었는지를 판단
	
	else//0이라면
		return strcmp(token,str)==0; //그 문자열이 0인지 다른 문자가 포함되었는지 판단
}	

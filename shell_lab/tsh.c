/* 
 * tsh - A tiny shell program with job control
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */

struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline) //cmd처리 함수
{
    char *argv[MAXARGS];
    char buf[MAXLINE];
    int bg;
    pid_t pid;
    sigset_t mask;
    strcpy(buf, cmdline); //buf에 cmd copy
    bg = parseline(buf, argv); //parseint가 &->1 리턴, 1 : background 0:foreground
    if (argv[0] == NULL) //empty line
    {
        return; //아무것도 하지 않음
    } 
    if(builtin_cmd(argv) == 0) //builtin_cmd에 있으면 해당 함수 동작 후 1 리턴, builtin이 아니므로 fork후 프로그램 호출해야함
    {
        if(sigemptyset(&mask) != 0 ) //signal 초기화
			unix_error("sigemptyset error"); //0이 아니면 초기화 실패
		if(sigaddset(&mask,SIGCHLD) != 0 ) //block을 위해 sigchld addset
			unix_error("sigaddset error"); //0이 아니면 addset 실패
		if(sigaddset(&mask,SIGINT) != 0 ) //block을 위해 sigint addset
			unix_error("sigaddset error"); //0이 아니면 addset 실패
		if(sigaddset(&mask,SIGTSTP) != 0 ) //block을 위해 sigtstp addset
			unix_error("sigaddset error"); //0이 아니면 addset 실패

		if(sigprocmask(SIG_BLOCK, &mask, NULL) != 0) //sigchld,sigint,sigststp를 block
			unix_error("sigprocmask error"); //0이 아니면 block 실패

        if((pid = fork()) == 0) //check child(child인 경우)
        {
            setpgid(0,0); //child를 process group에 넣음
            if(sigprocmask(SIG_UNBLOCK, &mask, NULL) != 0 ) //sigchld,sigint,sigststp를 unblock
				unix_error("sigprocmask error"); //0이 아니면 unblock 실패
            if((execve(argv[0], argv, environ) < 0)) //execve로 호출 안대면 -1 리턴
            {
              printf("%s: Command not found\n", argv[0]);  //fail
              exit(0); //끝내기
            }
        }
        else if(pid<0) //포크 실패
        {
            unix_error("fork error"); //error 메세지
        }
        else //check child(parent인 경우)
        {
            if(bg == 0) //foreground
            {
                addjob(jobs, pid, FG, cmdline); //job에 추가
                if(sigprocmask(SIG_UNBLOCK, &mask, NULL) != 0 ) //sigchld,sigint,sigststp를 unblock
				    unix_error("sigprocmask error"); //0이 아니면 unblock 실패
                waitfg(pid); //foreground는 하나만 돌게 waitfg(waitpid쓰면 sigchld_handler랑 중복으로 쓰여서 방지)
            }
            else //background
            {
                addjob(jobs, pid, BG, cmdline); //job에 추가
                if(sigprocmask(SIG_UNBLOCK, &mask, NULL) != 0 ) //sigchld,sigint,sigststp를 unblock
				    unix_error("sigprocmask error"); //0이 아니면 unblock실패
                printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline); //jid, pid, 커맨드 출력
            }
        }
    }
    return;
}

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.  
 */
int parseline(const char *cmdline, char **argv) 
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
	buf++;
	delim = strchr(buf, '\'');
    }
    else {
	delim = strchr(buf, ' ');
    }

    while (delim) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* ignore spaces */
	       buf++;

	if (*buf == '\'') {
	    buf++;
	    delim = strchr(buf, '\'');
	}
	else {
	    delim = strchr(buf, ' ');
	}
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* ignore blank line */
	return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) {
	argv[--argc] = NULL;
    }
    return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv) //builtin이면 실행시키는 함수
{
    char *cmd = argv[0]; //argv[0]: 명령어의 첫 단어의 주소값을 문자열로 넘김
    if(!strcmp(cmd, "quit")){ //quit가 입력되면
        exit(0); //끝냄
        return 1; //혹시 몰라서 return 1
    }
    if(!strcmp(cmd, "jobs")){ //jobs가 입력되면
        listjobs(jobs); //jobs 목록 호출
        return 1; //eval에서 return값으로 builtin인지를 판단하므로 0이 아닌값 리턴
    }
    if(!strcmp(cmd, "bg") || !strcmp(cmd,"fg")){ //bg나 fg가 입력되면
        do_bgfg(argv); //do_bgfg 에서 처리
        return 1; //eval에서 return값으로 builtin인지를 판단하므로 0이 아닌값 리턴
    }
    return 0;     /* not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) //bg,fg일때 처리하는 함수
{
    char *cmd = argv[0]; //argv[0]에 첫 명령어 단어 저장되있음(fg,bg)
    if(!strcmp(cmd, "bg")){ //background
        if(argv[1] == 0){ //bg 뒤에 아무것도 저장되있지 않을경우 null -> 주솟값이 0
            printf("bg command requires PID or %%jobid argument\n"); //error
            return; //종료
        }
        if((argv[1][0] != '%') && (atoi(&argv[1][0]))==0){ //이후 단어가 %가 아니고, atoi는 문자열에 대해서 0 리턴 -> jid나 process 넘어 아닌경우
            printf("bg: argument must be a PID or %%jobid\n"); //error
            return; //종료
        }
        if((argv[1][0] != '%')){ //process가 입력된 경우
            char *sproc = &argv[1][0]; //sproc에 process 문자열로 저장
            int iproc = atoi(sproc); //atoi로 문자열을 int값으로 받아옴
            for(int j = 0 ; j < MAXJOBS ; j++){ //job을 돌면서
                if(jobs[j].pid == iproc && jobs[j].state == ST) //해당하는 process가 있는지 확인
                    return; //있으면 종료
            }
            printf("(%d): No such process\n",iproc); //없으면 해당하는 프로세스가 없다는 error
            return; //종료
        } //이후 jid 입력 되었을때 정상 작동하는 함수부
        char *sjobnum = &argv[1][1]; //jid string으로 받아옴
        int ijobnum = atoi(sjobnum); //int로 바꿈
        for(int i = 0; i<MAXJOBS; i++){ //job을 돌면서
            if(jobs[i].jid == ijobnum){ //해당하는 jid가 있을 경우
                if(jobs[i].state == ST){ //stop일 경우만 background로 갖고오므로
                    jobs[i].state = BG; //job의 state 변경
                    printf("[%d] (%d) %s",jobs[i].jid, jobs[i].pid, jobs[i].cmdline); //출력
                    kill(-jobs[i].pid,SIGCONT); //stop을 실행시키므로 continue 시그널을 보내되, -를 통해 해당 pid가 만든 child에게도 sig가 모두 전달되게 하여 mysplit에도 동작 확인
                    return;//종료
                }
            }
        }
        printf("%%%d: No such job\n",ijobnum); //해당하는 job이 없는 error
        return;
    }
    if(!strcmp(cmd, "fg")){ //foreground
        if(argv[1] == 0){ //fg 뒤에 아무것도 저장되있지 않을경우 null -> 주솟값이 0
            printf("fg command requires PID or %%jobid argument\n"); //error
            return;
        }
        if((argv[1][0] != '%') && (atoi(&argv[1][0]))==0){ //이후 단어가 %가 아니고, atoi는 문자열에 대해서 0 리턴 -> jid나 process 넘어 아닌경우
            printf("fg: argument must be a PID or %%jobid\n"); //error
            return;
        }
        if((argv[1][0] != '%')){ //process가 입력된 경우
            char *sproc = &argv[1][0]; //sproc에 process 문자열로 저장
            int iproc = atoi(sproc); //atoi로 문자열을 int값으로 받아옴
            for(int j = 0 ; j < MAXJOBS ; j++){ //job을 돌면서
                if(jobs[j].pid == iproc && (jobs[j].state == ST || jobs[j].state == BG )) //해당하는 process가 있는지 확인
                    return; //있으면 종료
            }
            printf("(%d): No such process\n",iproc); //없으면 해당하는 프로세스가 없다는 error
            return; //종료
        }//이후 jid 입력 되었을때 정상 작동하는 함수부
        char *sjobnum = &argv[1][1]; //jid string으로 받아옴
        int ijobnum = atoi(sjobnum); //int로 바꿈
        for(int i = 0; i<MAXJOBS; i++){ //job을 돌면서
            if(jobs[i].jid == ijobnum){ //해당하는 jid가 있을 경우
                if(jobs[i].state == ST){ //state가 stop이면
                    jobs[i].state = FG;  //foreground로 state 수정
                    kill(-jobs[i].pid,SIGCONT); //stop을 실행시키므로 continue 시그널을 보내되, -를 통해 해당 pid가 만든 child에게도 sig가 모두 전달되게 하여 mysplit에도 동작 확인
                }
                else if(jobs[i].state == BG){ //state가 background이면
                    jobs[i].state = FG; //foreground로 state 수정
                    kill(-jobs[i].pid,SIGCONT); //stop을 실행시키므로 continue 시그널을 보내되, -를 통해 해당 pid가 만든 child에게도 sig가 모두 전달되게 하여 mysplit에도 동작 확인
                }
                waitfg(jobs[i].pid); //foreground는 하나만 실행 가능하므로 waitfg
                return; //종료
            }
        }
        printf("%%%d: No such job\n",ijobnum); //job이 없을 경우의 error
        return; //종료
    }
    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid) 
{
    struct job_t *jobp = getjobpid(jobs, pid); //job 구조체 포인터를 pid를 통해 받아옴
    if(jobp == 0){ //해당 pid job이 없는 경우, null -> 주솟값 0 
        return; //종료
    }
    for(;(jobp -> state == FG);){ //pid의 state가 foreground면 wait 처럼 붙잡아야함
        sleep(1); //sleep으로 붙잡고 있음
    }
    return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig)  //chld로부터 온 시그널 처리용
{
    int status; //waitpid 저장용
    pid_t pid;
    struct job_t *jobp;
    for(;((pid = waitpid(-1, &status, WNOHANG|WUNTRACED))>0);){ //모든 자식 PROCESS가 정지하였거나, 종료하였다면 0 리턴 - > LOOP 끝남, OR 자식 중 한개의 PID 값 리턴 -> PID로 LOOP진행
        jobp = getjobpid(jobs, pid); //job_t 구조체 포인터를 pid로 받아옴
        if(WIFEXITED(status)){ //exit 시그널이 올 경우
            deletejob(jobs,pid); //job에서 삭제
        }
        else if(WIFSIGNALED(status)){ //INT signal 온 경우
            printf("Job [%d] (%d) terminated by signal %d\n",pid2jid(pid),pid,WTERMSIG(status)); //종료하게 한 signal 코드 출력
            deletejob(jobs,pid); //job에서 삭제
        }
        else if(WIFSTOPPED(status)){ //TSTP SIGNAL 온 경우
            printf("Job [%d] (%d) stopped by signal %d\n", pid2jid(pid), pid, WSTOPSIG(status)); //멈추게 한 SIGNAL 코드 출력
            jobp->state = ST; //JOBs의 해당 job_t의 state를 stop으로 바꿈
        }
    }
    return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) //INT SIGNAL 처리용
{
    pid_t pid = fgpid(jobs); //가진 구조체를 이용해 pid 획득
    kill(-pid,SIGINT); //해당하는 pid로 INT 시그널 보냄, 해당 프로그램의 내장 INT가 실행되며 멈춰짐, -를 통해 해당 pid가 만든 child에게도 sig가 모두 전달되게 하여 mysplit에도 동작 확인
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig)  //TSTP SIGNAL 처리용
{
    pid_t pid = fgpid(jobs); //가진 구조체를 이용해 pid 획득
    kill(-pid,SIGTSTP); //해당하는 pid로 TSTP 시그널 보냄, 해당 프로그램의 내장 TSTP 가 실행되며 멈춰짐, -를 통해 해당 pid가 만든 child에게도 sig가 모두 전달되게 하여 mysplit에도 동작 확인
    return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
    //printf("add %d %s\n",pid,cmdline);
    int i;
    
    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = nextjid++;
	    if (nextjid > MAXJOBS)
		nextjid = 1;
	    strcpy(jobs[i].cmdline, cmdline);
  	    if(verbose){
	        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
	}
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
    //printf("del %d\n",pid);
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == pid) {
	    clearjob(&jobs[i]);
	    nextjid = maxjid(jobs)+1;
	    return 1;
	}
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) 
{
    int i;
    
    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid != 0) {
	    printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG: 
		    printf("Running ");
		    break;
		case FG: 
		    printf("Foreground ");
		    break;
		case ST: 
		    printf("Stopped ");
		    break;
	    default:
		    printf("listjobs: Internal error: job[%d].state=%d ", 
			   i, jobs[i].state);
	    }
	    printf("%s", jobs[i].cmdline);
	}
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}


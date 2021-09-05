/* $begin shellmain */
#include "csapp.h"
#include "myshell.h"
#include<errno.h>
#include<pwd.h>
#define MAXJOBS 128

#define UDEF 0
#define FG 1
#define BG 2
#define ST 3

char predir[100];

struct job jobs[MAXJOBS];
int jid_top=1;
int run_jid=1, pre_jid=0;
pid_t shell_pid=0;
struct job fg_job;

//job functions
void sigchld_handler(int sig){
	int status;
	pid_t pid;
	struct job *job_t;
	while((pid=waitpid(-1, &status, WNOHANG|WUNTRACED))>0){
		if(WIFEXITED(status)){
			del_job(pid);
		}
	}
	return;
}
void clear_job(struct job *job_t){
	job_t->pid=0;
	job_t->jid=0;
	job_t->state=UDEF;
	job_t->cmd[0]=0;
}
void init_jobs(){
	for(int i=0;i<MAXJOBS;++i){
		clear_job(&jobs[i]);
	}
}
int max_jid(){
	int ret=0;
	for(int i=0;i<MAXJOBS;++i){
		if(jobs[i].jid>ret)	ret=jobs[i].jid;
	}
	return ret;
}
int add_job(pid_t pid, int state, char *cmd){
	if(pid<1)	return 0;
	for(int i=0;i<MAXJOBS;++i){
		if(jobs[i].pid==0){
			jobs[i].pid=pid;
			jobs[i].state=state;
			strcpy(jobs[i].cmd, cmd);
			jobs[i].jid=jid_top++;
			pre_jid=run_jid;
			run_jid=jobs[i].jid;
			if(jid_top>MAXJOBS)	jid_top=1;
			return run_jid;
		}
	}
	return 0;
}
int del_job(pid_t pid){
	if(pid<1) return 0;
	for(int i=0;i<MAXJOBS;++i){
		if(jobs[i].pid==pid){
			clear_job(&jobs[i]);
			jid_top=max_jid()+1;
			return 1;
		}
	}
	return 0;
}
pid_t fg_pid(){
	for(int i=0;i<MAXJOBS;++i){
		if(jobs[i].state==FG)	return jobs[i].pid;
	}
	return 0;
}
struct job *get_pid(pid_t pid){
	if(pid<1)	return NULL;
	for(int i=0;i<MAXJOBS;++i){
		if(jobs[i].pid==pid)	return &jobs[i];
	}
	return NULL;
}
struct job *get_jid(int jid){
	if(jid<1)	return NULL;
	for(int i=0;i<MAXJOBS;++i){
		if(jobs[i].jid==jid)	return &jobs[i];
	}
	return NULL;
}
int pid_to_jid(pid_t pid){
	if(pid<1)	return 0;
	for(int i=0;i<MAXJOBS;++i){
		if(jobs[i].pid==pid)	return jobs[i].jid;
	}
	return 0;
}
void print_jobs(){
	for(int i=0;i<MAXJOBS;++i){
		if(jobs[i].pid!=0){
			printf("[%d]", jobs[i].jid);
			if(jobs[i].jid==run_jid)	printf("+  ");
			else if(jobs[i].jid==pre_jid) printf("-  ");
			else	printf("   ");
			switch(jobs[i].state){
				case ST:
					printf("Stopped\t");
					break;
				case BG:
				case FG:
					printf("Running\t");
					break;
				default:
					break;
			}
			printf("%s", jobs[i].cmd);
		}
	}
}

int main() 
{
    char cmdline[MAXLINE]; /* Command line */
		sigset_t block;
		Signal(SIGCHLD, sigchld_handler);
		sigemptyset(&block);
		sigaddset(&block, SIGINT);
		sigaddset(&block, SIGQUIT);
		sigaddset(&block, SIGTSTP);
		sigprocmask(SIG_BLOCK, &block, NULL);

		shell_pid=getpid();

		init_jobs();

    while (1) {
	/* Read */
	printf("CSE4100-SP-P#4> ");                   
	cmdline[0]=0;
	if(fgets(cmdline, MAXLINE, stdin)==NULL){
		printf("input error\n");
		exit(0);
	}
	
	if (feof(stdin))
	    exit(0);

	if(strchr(cmdline, '|')!=NULL){
		pipef(cmdline);
	}
	else{
		if(strcmp(cmdline, "exit")==0)	printf("\n\n exit inputed\n\n");
		eval(cmdline);
	}
    } 
}
/* $end shellmain */
 
void pipef(char *cmdline)
{
	pid_t pid;
	int status;
	int fds[5][2], pipeidx[5], pipenum=0, bg;
	char *argv[3][MAXARGS];
	char buf[MAXLINE];
	sigset_t mask, chmask, prev;
	Sigemptyset(&mask);
	Sigaddset(&mask, SIGINT);
	Sigaddset(&mask, SIGQUIT);
	Sigaddset(&mask, SIGTSTP);
	Sigemptyset(&chmask);
	Sigaddset(&chmask, SIGCHLD);

	strcpy(buf, cmdline);
	for(int i=0;cmdline[i];++i){	//parsing
		if(cmdline[i]=='|'){
			pipeidx[pipenum++]=i;
			buf[i+1]=0;
		}
	}
	bg=parseline(buf, argv[0]);

	for(int i=0;i<pipenum;++i){
		bg=parseline(buf+(pipeidx[i]+2), argv[i+1]);
	}
	if(pipe(fds[0])<0){	//pipe
		fprintf(stderr, "error: %s\n", strerror(errno));
		exit(-1);
	}
	if(argv[0][0]==NULL){	//first command
		return;
	}
	else if(!builtin_command(argv[0])){
		if((pid=fork())<0){
			fprintf(stderr, "error: %s\n", strerror(errno));
			exit(-1);
		}
		else if(pid==0){
			sigprocmask(SIG_UNBLOCK, &mask, NULL);
			close(STDOUT_FILENO);
			if(dup2(fds[0][1], STDOUT_FILENO)<0){
				fprintf(stderr, "error: %s\n", strerror(errno));
				exit(-1);
			}
			close(fds[0][1]);
			if(execvp(argv[0][0], argv[0])<0){
				fprintf(stderr, "error: %s\n", strerror(errno));
				exit(-1);
			}
		}
		close(fds[0][1]);
		if(waitpid(pid, &status, 0)<0){
			fprintf(stderr, "error: %s\n", strerror(errno));
		}
	}
	for(int i=1;i<pipenum;++i){
		if(pipe(fds[i])<0){
			fprintf(stderr, "error: %s\n", strerror(errno));
			exit(-1);
		}
		if(argv[i][0]==NULL){
			close(fds[i][1]);
			return;
		}
		else if(!builtin_command(argv[0])){
			if((pid=fork())<0){
				fprintf(stderr, "error: %s\n", strerror(errno));
				exit(-1);
			}
			else if(pid==0){
				sigprocmask(SIG_UNBLOCK, &mask, NULL);
				close(STDIN_FILENO);
				close(STDOUT_FILENO);
				if(dup2(fds[i-1][0], STDIN_FILENO)<0){
					fprintf(stderr, "error: %s\n", strerror(errno));
					exit(-1);
				}
				if(dup2(fds[i][1], STDOUT_FILENO)<0){
					fprintf(stderr, "error: %s\n", strerror(errno));
					exit(-1);
				}
				close(fds[i-1][0]);
				close(fds[i][1]);
				if(execvp(argv[i][0], argv[i])<0){
					fprintf(stderr, "error: %s\n", strerror(errno));
					exit(-1);
				}
			}
			close(fds[i][1]);
			if(waitpid(pid, &status, 0)<0){
				fprintf(stderr, "error: %s\n", strerror(errno));
			}
		}
	}
	if(argv[pipenum][0]==NULL){
		fprintf(stderr, "syntax error\n");
		return;
	}
	if(!builtin_command(argv[pipenum])){	//last command
		Sigprocmask(SIG_BLOCK, &chmask, &prev);
		if((pid=fork())<0){
			fprintf(stderr, "error: %s\n", strerror(errno));
			exit(-1);
		}
		else if(pid==0){
			Sigprocmask(SIG_SETMASK, &prev, NULL);
			Sigprocmask(SIG_UNBLOCK, &mask, NULL);
			close(STDIN_FILENO);
			if(dup2(fds[pipenum-1][0], STDIN_FILENO)){
				fprintf(stderr, "error: %s\n", strerror(errno));
				exit(-1);
			}
			close(fds[pipenum-1][0]);
			close(fds[pipenum-1][1]);
			if(execvp(argv[pipenum][0], argv[pipenum])<0){
				fprintf(stderr, "error: %s\n", strerror(errno));
				exit(-1);
			}
		}
		Sigprocmask(SIG_SETMASK, &prev, NULL);
		if(!bg){
			while(waitpid(pid, &status, WNOHANG|WUNTRACED)==0){
			}
			if(WIFSIGNALED(status)){
				if(WTERMSIG(status)==SIGKILL){
					if(shell_pid==getpid()){
						exit(0);
					}
					else{
						kill(-getpid(), SIGKILL);
					}
				}
			}
			if(WIFSTOPPED(status)){
				int jid;
				jid=add_job(pid, ST, cmdline);
				printf("[%d]%c  stopped\t %s", jid,(jid==run_jid ? '+':jid==pre_jid ? '-':' '), cmdline);
			}
		}
		else{	//when background
			add_job(pid, BG, cmdline);
			printf("[%d] %d\n", pid_to_jid(pid), pid);
		}
	}
	return;
	
}
/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
		sigset_t mask, chmask, prev;
	Sigemptyset(&mask);
	Sigaddset(&mask, SIGINT);
	Sigaddset(&mask, SIGQUIT);
	Sigaddset(&mask, SIGTSTP);
	Sigemptyset(&chmask);
	Sigaddset(&chmask, SIGCHLD);
    
    strcpy(buf, cmdline);
    bg = parseline(buf, argv); 
    if (argv[0] == NULL)  return;   /* Ignore empty lines */
    if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run
		Sigprocmask(SIG_BLOCK, &chmask, &prev);
		pid=fork();
		if(pid<0){	//error
			fprintf(stderr, "error: %s\n", strerror(errno));
			exit(-1);
		}
		else if(pid==0){	//child
			Sigprocmask(SIG_SETMASK, &prev, NULL);
			Sigprocmask(SIG_UNBLOCK, &mask, NULL);
        	if (execvp(argv[0], argv) < 0) {	//ex) /bin/ls ls -al &
				fprintf(stderr, "error: %s\n", strerror(errno));
				exit(-1);
        	}
		}
				
			/* Parent waits for foreground job to terminate */
		Sigprocmask(SIG_SETMASK, &prev, NULL);
		if (!bg){ 
		    int status;
			while(waitpid(pid, &status, WNOHANG|WUNTRACED)==0){
			}
			if(WIFSIGNALED(status)){ //child process was killed by a signal
				if(WTERMSIG(status)==SIGKILL){
					if(shell_pid==getpid()){
						exit(0);
					}
					else{
						kill(-getpid(), SIGKILL);
					}
				}
			}
			if(WIFSTOPPED(status)){
				int jid;
				jid=add_job(pid, ST, cmdline);
			printf("[%d]%c  stopped\t %s", jid,(jid==run_jid ? '+':jid==pre_jid ? '-':' '), cmdline);
			}
		}
		else{//when there is backgrount process!
			add_job(pid, BG, cmdline);
			printf("[%d] %d\n", pid_to_jid(pid), pid);
		}
    }
    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
    if (!strcmp(argv[0], "quit")) /* quit command */
	exit(0);  
    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
	return 1;
	if(!strcmp(argv[0], "exit")){
		if(shell_pid==getpid())	exit(0);
		else	kill(-getpid(), SIGKILL);
	}
	if(!strcmp(argv[0], "cd")){
		char changedir[100];
		char *homedir;
		if(argv[1]==NULL || argv[1][0]=='~'){
			homedir=getenv("HOME");
			strcpy(changedir, homedir);
		}
		else if(argv[1][0]=='-') strcpy(changedir, predir);
		else strcpy(changedir, argv[1]);
		if(getcwd(predir, 100)==0){
			fprintf(stderr, "error: %s\n", strerror(errno));
			return -1;
		}
		int resval=chdir(changedir);
		if(resval<0){
			fprintf(stderr, "error: %s\n", strerror(errno));
			return -1;
		}
		return 1;
	}
	pid_t pid;
	int status, jid;
	struct job *job_t=NULL;
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);
	sigaddset(&mask, SIGTSTP);
	if(!strcmp(argv[0], "bg")){
		if(argv[1]==NULL)	return 1;
		if(argv[1][0]!='%')	return 1;
		sscanf(argv[1]+1, "%d", &jid);
		job_t=get_jid(jid);
		if(job_t==NULL){
			printf("[%s]: No such job\n", argv[1]);
			return 1;
		}
		job_t->state=BG;
		pre_jid=run_jid;
		run_jid=job_t->jid;

		pid=fork();
		if(pid<0){
			fprintf(stderr, "%s\n", strerror(errno));
			exit(-1);
		}
		else if(pid==0){
			bg_f(job_t);
			exit(0);
		}
		if(waitpid(pid, &status, 0)<0){
			fprintf(stderr, "%s\n", strerror(errno));
		}
		return 1;
	}
	if(!strcmp(argv[0], "fg")){
		if(argv[1]==NULL)	return 1;
		if(argv[1][0]!='%')	return 1;
		sscanf(argv[1]+1, "%d", &jid);
		job_t=get_jid(jid);
		if(job_t==NULL){
			printf("[%s]: No such job\n", argv[1]);
			return 1;
		}
		job_t->state=FG;
		pre_jid=run_jid;
		run_jid=job_t->jid;

		pid=fork();
		if(pid<0){
			fprintf(stderr, "%s\n", strerror(errno));
			exit(-1);
		}
		else if(pid==0){
			sigprocmask(SIG_UNBLOCK, &mask, NULL);
			fg_f(job_t);
			exit(0);
		}
		if(waitpid(pid, &status, 0)<0){
			fprintf(stderr, "%s\n", strerror(errno));
		}
		del_job(job_t->pid);
		return 1;
	}
	if(!strcmp(argv[0], "jobs")){
		pid=fork();
		if(pid<0){
			fprintf(stderr, "%s\n", strerror(errno));
			exit(-1);
		}
		else if(pid==0){
			print_jobs();
			exit(0);
		}
		if(waitpid(pid, &status, 0)<0){
			fprintf(stderr, "%s\n", strerror(errno));
			exit(0);
		}
		return 1;
	}
	if(!strcmp(argv[0], "kill")){
		if(argv[1][0]!='%'){
			printf("usage: kill %%[process]\n");
			return 1;
		}
		int jid;
		pid_t dpid;
		sscanf(argv[1]+1, "%d", &jid);
		struct job *job_t=get_jid(jid);
		if(job_t==NULL)	return 1;
		dpid=job_t->pid;
		del_job(dpid);
		sprintf(argv[1], "%d", dpid);
		return 0;
	}
    return 0;                     /* Not a builtin command */
}
/* $end eval */

void bg_f(struct job *job_t){
	kill(-job_t->pid, SIGCONT);
	printf("[%d] %s", job_t->jid, job_t->cmd);
}
void fg_f(struct job *job_t){
	int status;
	pid_t fg_pid, pid;
	fg_pid=job_t->pid;
	kill(-fg_pid, SIGCONT);
	printf("%s", job_t->cmd);
	do{
		pid=waitpid(-1, &status, WUNTRACED);
	}while(pid!=fg_pid);
}

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;              /* Background job? */

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* Ignore blank line */
	return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
	argv[--argc] = NULL;

    return bg;
}
/* $end parseline */



#define MAXARGS 128
//jobs
struct job{
	pid_t pid;
	int jid, state;
	char cmd[MAXARGS];
};
/* Function prototypes */
void pipef(char *cmdline);
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 

//signal & job functions
void sigchld_handler(int sig);
void clear_job(struct job *job_t);
void init_jobs();
int max_jid();
int ad_job(pid_t pid, int state, char *cmd);
int del_job(pid_t pid);
pid_t fg_pid();
struct job *get_pid(pid_t pid);
struct job *get_jid(int jid);
int pid_to_jid(pid_t pid);
void print_jobs();
void bg_f(struct job *job_t);
void fg_f(struct job *job_t);

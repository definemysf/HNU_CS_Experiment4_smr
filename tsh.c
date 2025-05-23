/* 
 * tsh - 一个带作业控制功能的微型 shell 程序
 * 
 * <在这里填写你的名字和学号>
 */
 //头文件与常量定义
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* 一些杂项常量 */
#define MAXLINE    1024   /* 命令行最大长度 */
#define MAXARGS     128   /* 命令行参数最大数量 */
#define MAXJOBS      16   /* 同时存在的最大作业数 */
#define MAXJID    1<<16   /* 最大作业ID */

/* 作业状态 */
#define UNDEF 0 /* 未定义 */
#define FG 1    /* 前台运行 */
#define BG 2    /* 后台运行 */
#define ST 3    /* 已停止 */

/* 
 * 作业状态：FG（前台），BG（后台），ST（停止）
 * 作业状态转换及触发动作：
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg 命令
 *     ST -> BG  : bg 命令
 *     BG -> FG  : fg 命令
 * 最多只能有一个作业处于 FG 状态。
 */
//全局变量与作业结构体
/* 全局变量 */
extern char **environ;      /* 在 libc 中定义的环境变量 */
char prompt[] = "tsh> ";    /* 命令行提示符（不要更改） */
int verbose = 0;            /* 如果为真，打印额外的输出信息 */
int nextjid = 1;            /* 下一个可分配的作业ID */
char sbuf[MAXLINE];         /* 用于 sprintf 消息的缓冲区 */

struct job_t {              /* 作业结构体 */
    pid_t pid;              /* 作业的进程ID */
    int jid;                /* 作业ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, 或 ST */
    char cmdline[MAXLINE];  /* 命令行内容 */
};
struct job_t jobs[MAXJOBS]; /* 作业列表 */

/* 函数声明 */

/* 下面是你需要实现的函数 */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* 下面是我们为你提供的辅助函数 */
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
 * main - shell 的主循环
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* 是否显示提示符（默认显示） */

    /* 将标准错误重定向到标准输出（这样驱动程序能获取所有输出） */
    dup2(1, 2);

    /* 解析命令行参数 */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* 打印帮助信息 */
            usage();
	    break;
        case 'v':             /* 打印额外的诊断信息 */
            verbose = 1;
	    break;
        case 'p':             /* 不打印提示符 */
            emit_prompt = 0;  /* 便于自动测试 */
	    break;
	default:
            usage();
	}
    }

    /* 安装信号处理函数 */

    /* 下面这些是你需要实现的 */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* 子进程终止或停止 */

    /* 这个用于优雅地终止 shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* 初始化作业列表 */
    initjobs(jobs);

    /* shell 的主循环 */
    while (1) {

	/* 读取命令行 */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* 文件结束（ctrl-d） */
	    fflush(stdout);
	    exit(0);
	}

	/* 解析并执行命令行 */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* 程序不会到达这里 */
}
//核心功能实现
/* 
 * eval - 解析并执行用户输入的命令行
 * 
 * 如果是内建命令（quit, jobs, bg, fg），立即执行。
 * 否则，fork 一个子进程并在子进程中运行命令。
 * 如果是前台作业，等待其结束后返回。
 * 注意：每个子进程必须有唯一的进程组ID，这样后台进程不会收到来自终端的 SIGINT/SIGTSTP。
*/
void eval(char *cmdline) 
{
    char *argv[MAXARGS];
    int bg;
    pid_t pid;
    sigset_t mask_all, mask_one, prev_one;

    bg = parseline(cmdline, argv);
    if (argv[0] == NULL) return;

    if (!builtin_cmd(argv)) {
        sigemptyset(&mask_one);
        sigaddset(&mask_one, SIGCHLD);
        sigfillset(&mask_all);

        sigprocmask(SIG_BLOCK, &mask_one, &prev_one); // 阻塞SIGCHLD
        if ((pid = fork()) == 0) { // 子进程
            sigprocmask(SIG_SETMASK, &prev_one, NULL);
            setpgid(0, 0); // 新进程组
            if (execve(argv[0], argv, environ) < 0) {
                printf("%s: Command not found\n", argv[0]);
                exit(0);
            }
        }
        // 父进程
        int state = bg ? BG : FG;
        addjob(jobs, pid, state, cmdline);
        sigprocmask(SIG_SETMASK, &prev_one, NULL);

        if (!bg) {
            waitfg(pid);
        } else {
            struct job_t *job = getjobpid(jobs, pid);
            printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
        }
    }
}
/*
eval函数说明：
- 解析命令行，判断是否为内建命令（如quit、jobs、bg、fg）。
- 如果不是内建命令，则fork子进程执行外部命令。
- 对于前台作业，父进程会等待其结束；后台作业则直接返回。
- 使用信号屏蔽防止竞争条件。
*/
//`eval` 是 shell 的核心，负责命令解析、内建命令处理、进程创建和作业控制。

/* 
 * parseline - 解析命令行并构建 argv 数组
 * 
 * 单引号括起来的内容视为一个参数。
 * 如果命令以 & 结尾，返回 true，表示后台作业；否则返回 false，表示前台作业。
 */
 //`parseline` 负责将命令行字符串拆分为参数数组，并判断是否为后台作业。
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
 * builtin_cmd - 如果是内建命令则立即执行
 */
int builtin_cmd(char **argv) 
{
    if (!strcmp(argv[0], "quit"))
        exit(0);
    if (!strcmp(argv[0], "jobs")) {
        listjobs(jobs);
        return 1;
    }
    if (!strcmp(argv[0], "bg") || !strcmp(argv[0], "fg")) {
        do_bgfg(argv);
        return 1;
    }
    if (!strcmp(argv[0], "&"))
        return 1;
    return 0;
}
/*
builtin_cmd函数说明：
- 判断并执行内建命令（quit、jobs、bg、fg），如果是则立即执行并返回1，否则返回0。
*/
//判断并执行内建命令（如 quit、jobs、bg、fg），否则返回 0。

/*------------------- bg/fg命令处理 -------------------*/
void do_bgfg(char **argv) 
{
    struct job_t *job = NULL;
    char *id = argv[1];
    int jid;
    pid_t pid;

    if (id == NULL) {
        printf("%s command requires PID or %%jobid argument\n", argv[0]);
        return;
    }
    if (id[0] == '%') {
        jid = atoi(&id[1]);
        job = getjobjid(jobs, jid);
        if (!job) {
            printf("%s: No such job\n", id);
            return;
        }
    } else if (isdigit(id[0])) {
        pid = atoi(id);
        job = getjobpid(jobs, pid);
        if (!job) {
            printf("(%d): No such process\n", pid);
            return;
        }
    } else {
        printf("%s: argument must be a PID or %%jobid\n", argv[0]);
        return;
    }

    kill(-job->pid, SIGCONT);

    if (!strcmp(argv[0], "bg")) {
        job->state = BG;
        printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
    } else {
        job->state = FG;
        waitfg(job->pid);
    }
}
/*
do_bgfg函数说明：
- 处理bg/fg命令，将指定作业放到后台或前台运行。
- 支持通过PID或%JID指定作业。
- bg命令恢复作业到后台并打印信息，fg命令恢复到前台并等待其结束。
*/

/*------------------- 前台作业等待 -------------------*/
void waitfg(pid_t pid)
{
    while (pid == fgpid(jobs)) {
        sleep(1);
    }
}
//等待前台作业结束。

/*****************
 * Signal handlers
 *****************/
//需要实现的信号处理函数
/* 
 * sigchld_handler - 内核在子进程终止或停止时发送 SIGCHLD
 * 该处理函数回收所有僵尸子进程，但不会等待其他正在运行的子进程。
 */
/*------------------- 信号处理函数 -------------------*/
void sigchld_handler(int sig) 
{
    int olderrno = errno;
    pid_t pid;
    int status;
    sigset_t mask_all, prev_all;
    sigfillset(&mask_all);

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
        struct job_t *job = getjobpid(jobs, pid);

        if (WIFEXITED(status)) {
            deletejob(jobs, pid);
        } else if (WIFSIGNALED(status)) {
            printf("Job [%d] (%d) terminated by signal %d\n", pid2jid(pid), pid, WTERMSIG(status));
            deletejob(jobs, pid);
        } else if (WIFSTOPPED(status)) {
            printf("Job [%d] (%d) stopped by signal %d\n", pid2jid(pid), pid, WSTOPSIG(status));
            if (job) job->state = ST;
        }
        sigprocmask(SIG_SETMASK, &prev_all, NULL);
    }
    errno = olderrno;
}
/*
sigchld_handler函数说明：
- 回收所有已终止或停止的子进程，避免僵尸进程。
- 如果子进程被信号终止或停止，打印提示信息并更新作业状态。
*/

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{
    pid_t pid = fgpid(jobs);
    if (pid != 0) {
        kill(-pid, SIGINT);
    }
}
/*
sigint_handler函数说明：
- 捕获ctrl-c信号，将SIGINT发送给前台进程组，实现终止前台作业。
*/

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
    pid_t pid = fgpid(jobs);
    if (pid != 0) {
        kill(-pid, SIGTSTP);
    }
}
/*
sigtstp_handler函数说明：
- 捕获ctrl-z信号，将SIGTSTP发送给前台进程组，实现暂停前台作业。
*/


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




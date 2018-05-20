#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <readline/readline.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/wait.h>

#define MAX_NAME_LEN 256
#define MAX_INPUT_LEN 1024

void vsh_info(); //获取用户名
char *get_cwdname(char *cwd); // 获取当前路径
char **get_command(char *); // 分割“ ”
char **get_pipe(char *); // 分割“|”
char *trim_space(char *); // 去掉前后空格
void run(char **, int , int ); // 执行指令函数
int cd(char *path); // 执行cd功能

int main() {
    char **command; 
    char *input;
    pid_t child_pid;
    int stat_loc;
    int pipe_cnt;

    while (1) {
        vsh_info();
        input = readline("shell > ");
        command = get_pipe(input);
        pipe_cnt = 0;
        while(command[pipe_cnt]) {
            // printf("command: %s\n", command[pipe_cnt]);
            pipe_cnt++;
        }
        // printf("%d\n",pipe_cnt);
        if(pipe_cnt == 1) {
            command = get_command(command[0]);
            if (!command[0]) {      // 处理输入为空
                free(input);
                free(command);
                continue;
            }
            // 如果输入为cd则不fork
            if (strcmp(command[0], "cd") == 0) {
                if (cd(command[1]) < 0) {
                    perror(command[1]);
                }
                continue;
            }
            //输入为exit
            if (strcmp(command[0], "exit") == 0) {
                printf("exit\n");
                exit(EXIT_SUCCESS);
            }
        }
        child_pid = fork();
        if (child_pid < 0) { // 异常
            perror("Fork failed");
            exit(1);
        }
        if (child_pid == 0) { // 子进程执行execvp
            if(pipe_cnt == 1){
                execvp(command[0], command);
            }
            else {
                run(command, 0, pipe_cnt - 1);
            }
        } else { // 父进程等待子进程结束
            waitpid(child_pid, &stat_loc, WUNTRACED);
        }

        free(input);
        free(command);
    }
    return 0;
}

/**
 * 将用户输入按空格分解
 */
char **get_command(char *input) {
    char **command = malloc(8 * sizeof(char *));
    if (command == NULL) {
        perror("malloc failed");
        exit(1);
    }
    char *separator = " ";
    char *parsed;
    int index = 0;
    /**
     * 在第一次调用时，strtok()必需给予参数s 字符串，往后的调用则将参数s 设置成NULL。
     * 每次调用成功则返回下一个分割后的字符串指针。
     * strtok()在参数input的字符串中发现到参数separator 
     * 的分割字符时则会将该字符改为\0 字符
     */
    parsed = strtok(input, separator);
    while (parsed != NULL) {
        command[index] = parsed;
        index++;
        parsed = strtok(NULL, separator);
    }
    command[index] = NULL;
    return command;
}
char **get_pipe(char *input) {
    char **command = malloc(8 * sizeof(char *));
    if (command == NULL) {
        perror("malloc failed");
        exit(1);
    }
    char *separator = "|";
    char *parsed;
    int index = 0;
    parsed = strtok(input, separator);
    while (parsed != NULL) {
        command[index] = trim_space(parsed);
        index++;
        parsed = strtok(NULL, separator);
    }
    command[index] = NULL;
    return command;
}
int cd(char *path) {
    return chdir(path);
}

char *trim_space(char *in) {
    int len = strlen(in);
    int i = 0, j = len - 1;
    // 去掉前面的空格
    while(in[i] == ' ' && in[i] != '\'') {
        i++;
    }
    // 去掉后面的空格
    while(in[j] == ' ' && in[j] != '\'') {
        in[j--] = 0;
    }
    return (in + i);
}

void run(char **command, int cur, int last) {
    if(cur > last) {
        exit(0);
    }
    char **args = get_command(command[cur]);
    // 如果只有一个指令
    if(cur == last) {
        execvp(args[0], args);
    }
    int fd[2], status;
    pid_t pid;
    pipe(fd);
    // printf("args[0]: %s\n", args[0]);
    // 子进程
    if( (pid = fork()) == 0 ) {
        dup2(fd[1], fileno(stdout));
        close(fd[0]);
        close(fd[1]);
        if( (status = execvp(args[0], args) < 0 )) {
            fprintf(stderr, "%s: command not found.\n",  args[0]);
            exit(-1);
        }
    }
    dup2(fd[0], fileno(stdin));
    close(fd[0]);
    close(fd[1]);
    run(command, cur + 1, last);
}
void vsh_info(){
	struct passwd *pwd = getpwuid(getuid());   
	char hostname[MAX_NAME_LEN] = {0};	   
	char cwd[MAX_NAME_LEN]; getcwd(cwd, MAX_NAME_LEN);
	char *cwdname = get_cwdname(cwd);
	gethostname(hostname, MAX_NAME_LEN);	  
	printf("[%s@%s ", pwd->pw_name, hostname, cwdname);
	if (strcmp(pwd->pw_name, cwdname)== 0)printf("%s(~)]", cwdname);
	else printf("%s]", cwdname);
	if (strcmp(pwd->pw_name, "root") == 0)printf("# ");
	else printf("# ");
}
char *get_cwdname(char *cwd){
	if (cwd == NULL) return NULL;
	if (strcmp("/", cwd) == 0) return cwd;
	char *tmp = &cwd[strlen(cwd)-1];
	while (*tmp != '/') tmp--;
	return ++tmp;
}
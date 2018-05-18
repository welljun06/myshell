#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <readline/readline.h>
#include <unistd.h>
#include <sys/wait.h>

char **get_input(char *);
int cd(char *path);
int main() {
    char **command;
    char *input;
    pid_t child_pid;
    int stat_loc;

    while (1) {
        input = readline("shell > ");
        command = get_input(input);

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
            /* Skip the fork */
            continue;
        }
        //输入为exit
        if (strcmp(command[0], "exit") == 0) {
            printf("exit\n");
            exit(EXIT_SUCCESS);
        }
        child_pid = fork();
        if (child_pid < 0) { // 异常
            perror("Fork failed");
            exit(1);
        }
        if (child_pid == 0) { // 子进程执行execvp
            if (execvp(command[0], command) < 0) {
                perror(command[0]);
                exit(1);
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
char **get_input(char *input) {
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
int cd(char *path) {
    return chdir(path);
}
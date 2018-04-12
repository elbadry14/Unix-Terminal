#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define RL_BUFSIZE 1024
#define TOK_BUFSIZE 64
#define TOK_DELIM " \t\r\n\a"
int pipe_flag = 0;
int input_flag = 0;
int outout_flag = 0;
int background_flag = 0;

char *read_line(void)
{
    char *line = NULL;
    ssize_t bufsize = 0;
    getline(&line, &bufsize, stdin);
    return line;
}

char **parse(char *line)
{
    int bufsize = TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    token = strtok(line, TOK_DELIM);

    while (token != NULL)
    {
        tokens[position] = token;
        position++;

        if (position >= bufsize)
        {
            bufsize += TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));

        }

        token = strtok(NULL, TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

int check_process (char **oldArgs)
{
    int index = 0;

    while (oldArgs[index] != NULL)
        index++;

    for (int i = 0; i < index; i++)
    {
        if (!strcmp(oldArgs[i], ">"))
        {
            outout_flag = 1;
        }

        else if (!strcmp(oldArgs[i], "<"))
        {
            input_flag = 1;
        }
        else if (!strcmp(oldArgs[i], "|"))
        {
            pipe_flag = 1;
            return 1;
        }
        else if (!strcmp(oldArgs[i], "&"))
        {
            background_flag = 1;
        }
    }
    return 0;
}
char **check_redirection(char **oldArgs)
{
    int redirects = 0;
    int index = 0;

    while (oldArgs[index] != NULL)
        index++;

    for (int i = 0; i < index; i++)
    {
        if (!strcmp(oldArgs[i], ">"))
        {
            freopen(oldArgs[i+1], "w", stdout);
            redirects++;
        }

        else if (!strcmp(oldArgs[i], "<"))
        {
            freopen(oldArgs[i+1], "r", stdin);
            redirects++;
        }
    }

    index -= 2*redirects;
    index++;

    char **newArgs = (char **) malloc(index * sizeof(char *));
    for (int i = 0 ; i < index; i++)
    {
        newArgs[i] = oldArgs[i];
    }

    newArgs[index-1] = NULL;
    return newArgs;
}

char **check_pipe(char **oldArgs)
{
    int count = 0;
    int index = 0;

    while (oldArgs[index] != NULL)
        index++;

    for (int i = 0; i < index; i++)
    {
        count += 1;
        if (!strcmp(oldArgs[i], "|"))
        {
            break;
        }
    }

    index = count;

    char **newArgs = (char **) malloc(index * sizeof(char *));
    for (int i = 0 ; i < index; i++)
    {
        newArgs[i] = oldArgs[i];
    }

    newArgs[index-1] = NULL;
    return newArgs;
}

char **get_pipe(char **oldArgs)
{
    int count = 0;
    int index = 0;

    while (oldArgs[index] != NULL)
        index++;

    for (int i = 0; i < index; i++)
    {
        count ++;
        if (!strcmp(oldArgs[i], "|"))
        {
            break;
        }
    }

    index = count;

    char **pipe_args = (char **) malloc(index * sizeof(char *));
    int j = 0;
    if (pipe_flag == 1)
    {
        while (oldArgs[index] != NULL)
        {
            pipe_args[j] = oldArgs[index];
            index++;
            j++;
        }
    }

    pipe_args[index-1] = NULL;
    return pipe_args;
}

void execute_pip (char **newArgs[] , char **pipe_args[] , int pipefd[])
{
    int pid;
    int i;

    pid = fork();

    if (pid == 0)
    {
        dup2(pipefd[1] , 1);
        close (pipefd[0]);
        execvp(newArgs[0] , newArgs);
    }
    else
    {
        dup2(pipefd[0] , 0);
        close (pipefd[1]);
        execvp(pipe_args[0] , pipe_args);
    }
}

void loop(void)
{
  char *line;
  char **oldArgs;

  while (1) {

    printf("Group13> ");
    line = read_line();
    oldArgs = parse(line);
    int z = check_process(oldArgs);

    int status;
    pid_t pid = fork();
    int pipefd [2];
    pipe(pipefd);

    if (oldArgs[0] == NULL || strcmp(oldArgs[0] , "exit") == 0 || strcmp(oldArgs[0] , "Ctrl-D") == 0)
    {
    exit(0);
    }

    if (pid < 0)
        perror("Fork");

    else if (pid == 0)
    {

        if (input_flag == 1 || outout_flag == 1)
        {
            char **newArgs = check_redirection(oldArgs);
            if (execvp(*newArgs, newArgs) == -1)
            {
                perror(*newArgs);
            }
            free(newArgs);
        }

        if (pipe_flag == 1)
        {
            char **newArgs = check_pipe(oldArgs);
            char **pipe_args = get_pipe(oldArgs);
            execute_pip(newArgs , pipe_args , pipefd);
            exit(0);
        }

        if (execvp(*oldArgs, oldArgs) == -1)
            perror(*oldArgs);
        exit(EXIT_FAILURE);
    }

    else
    {
        if (background_flag == 1){
            printf("[proc %d started] \n" , pid);
            continue ;
        }

        else{
             waitpid(pid, &status, 0);
        }

         if (!WIFEXITED(status))
            printf("ERROR : Child terminated abnormally!\n");
    }

    free(line);
    free(oldArgs);
  }
}

int main(int argc, char **argv)
{

  loop();

  return EXIT_SUCCESS;
}

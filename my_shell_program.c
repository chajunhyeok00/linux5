#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <setjmp.h>
#include <sys/types.h>
#include <errno.h>

#define BUFSIZE 256

sigjmp_buf jbuf;
pid_t pid;


void handler(int argc, char **argv);
//2번
void launch(int argc, char **argv);
//3번
void handler_SIGINT(int signo, pid_t pid);
void handler_SIGQUIT(int signo);
//4번
void redirection(int argc, char **argv);
void pipe_launch(int argc, char **argv);
//5번
void ls(int argc, char **argv);
void pwd();
void cd(int argc, char **argv);
void mkdir_(int argc, char **argv);
void rmdir_(int argc, char **argv);
void rmdir_rm(int argc, char **argv);
void ln(int argc, char **argv);
void cp(int argc, char **argv);
void mv(int argc, char **argv);
void cat(int argc, char **argv);

char *substring(int start, int end, char * str);
int getargs(char *cmd, char **argv);

char path[BUFSIZE];



int main()
{
   signal(SIGINT, handler_SIGINT);
   signal(SIGTSTP, handler_SIGQUIT);

   char buf[256];
   char * argv[50];
   int argc;
   pid_t pid;

   while (1)
       {
      char *argv[50] = {'\0'};

      getcwd(path, BUFSIZE);
           printf("\n[ My Shell Program ]\n\n");
            printf("Shell> ");
           gets(buf);

        if(!strcmp(buf, "exit")) {
         printf("GoodBye :(\n");
         exit(0);
   }

        else if(!strcmp(buf,"") || !strcmp(buf,"\t"))
               continue;

        argc = getargs(buf, argv);
        handler(argc, argv);
    }
}

int getargs(char *cmd, char **argv){
   int argc = 0;
   
   while(*cmd){
      if(*cmd == ' ' || *cmd == '\t')
         *cmd++ = '\0';
      else{
         argv[argc++] = cmd++;
         while(*cmd != '\0' && *cmd != ' '&&*cmd != '\t')
         cmd++;
      }
   }
   argv[argc] = NULL;
   return argc;
}

void handler_SIGINT(int signo, pid_t pid){
   if (kill(pid, SIGTERM) != 0){
      printf("Ctrl + C \n");
        printf("\n");
   }
}

void handler_SIGQUIT(int signo){
   printf("Ctrl + Z \n");
   printf("quit (core dumped) dump\n");
   
   exit(1);
}

void handler(int argc, char **argv) {

    int i = 0;
    int is_background = 0, is_redirection = 0, is_pipe = 0;
    for(i = 0; i < argc; i++) {
        if( (!strcmp(argv[i], ">")) || (!strcmp(argv[i], "<"))) {
            is_redirection = 1;
            break;
        }
        else if (!strcmp(argv[i], "|")) {
            is_pipe = 1;
            break;
        }
        else if (!strcmp(argv[i], "&")) {
            is_background = 1;
            break;
        }
    }

    if(is_background){
        launch(argc, argv);
        is_background = 0;
    }
    else if(is_redirection){
        redirection(argc, argv);
        is_redirection = 0;
    }
    else if(is_pipe){
        pipe_launch(argc, argv);
        is_pipe = 0;
    }
    else if(!strcmp(argv[0], "ls")){
        ls(argc, argv);
    }
    else if(!strcmp(argv[0], "cd")){
        cd(argc, argv);
    }
    else if (!strcmp(argv[0], "rmdir")){
        rmdir_(argc, argv);
    }
    else if (!strcmp(argv[0], "cp")){
        cp(argc, argv);
    }
    else if (!strcmp(argv[0], "mv")){
        mv(argc, argv);
    }
    else if (!strcmp(argv[0], "pwd")){
        pwd(argc, argv);
    }
    else if (!strcmp(argv[0], "rm")){
        rmdir_rm(argc, argv);
    }
    else if (!strcmp(argv[0], "mkdir")){
        mkdir_(argc, argv);
    }
    else if (!strcmp(argv[0], "ln")){
        ln(argc, argv);
    }
    else if (!strcmp(argv[0], "cat")){
        cat(argc, argv);
    }
    else {
        launch(argc,argv);
    }
}

void pwd(){
   char *buf = (char *)malloc(sizeof(char)*(BUFSIZE));
   
   if(getcwd(buf,BUFSIZE)==NULL){
      perror("(Error) Pwd");
      exit(EXIT_FAILURE);
   }
   else
      printf("%s \n",buf);
   
   free(buf);
}

void ls(int argc, char **argv){
   char temp[256];
   if(argc == 1){
      getcwd(temp, 256);
      printf("%s", temp);
      argv[1] = temp;
   }
   
   DIR *pdir;
   struct dirent *pde;
   int i = 0;
   if((pdir = opendir(argv[1])) < 0) {
      perror("(Error) Opendir: ");
   }
   printf("\n");
   while((pde = readdir(pdir)) != NULL) {
      printf("%-20s", pde->d_name);
      if(++i % 3 == 0)
         printf("\n");
      }
   printf("\n");
   closedir(pdir);
}

void cd(int argc, char **argv){
    char targetPath[BUFSIZE];
    
    if (argc == 1) {
        chdir("Home");
    }
    else {
        // 만약 상대 경로가 주어졌다면, 절대 경로로 변경
        if (argv[1][0] != '/') {
            getcwd(targetPath, BUFSIZE);
            strcat(targetPath, "/");
            strcat(targetPath, argv[1]);
        }
        else {
            strcpy(targetPath, argv[1]);
        }

        if (chdir(targetPath) == -1) {
            if (errno == ENOENT) {
                printf("cd: %s: No such file or directory\n", argv[1]);
            }
            else {
                perror("chdir");
            }
        }
    }

    // 디버깅을 위해 현재 경로 출력
    getcwd(path, BUFSIZE);
    printf("Current Directory: %s\n", path);
}


void mkdir_(int argc, char **argv){
   
   
   if(argc < 2)
      fprintf(stderr, "경로가 존재하지 않음 \n");
   else{
        char currentDir[BUFSIZE];
        getcwd(currentDir, BUFSIZE);
      if(mkdir(argv[1], 0777) < 0) {
         perror("(Error) mkdir");
         exit(EXIT_FAILURE);
      }
        chdir(currentDir);
   }
}

void rmdir_(int argc, char **argv){
    int i = 0;
    char temp[256];
    if(argc == 1){
        printf("rmdir: missing operand\n");
    }
    else{
        if (rmdir(argv[1]) == -1) {
            perror("rmdir");
        }
    }
}

void rmdir_rm(int argc, char ** argv) {

    if(argc < 2)
        fprintf(stderr, "Path is not exists\n");
    else {
        if(remove(argv[1]) < 0) {
            perror("[ERROR] RM/RMDIR");
            exit(EXIT_FAILURE);
        }
    }
}

void ln(int argc, char **argv){
   char cmd;
    char *src;  
    char *target;
    if (argc < 2) {
        fprintf(stderr, "Usage: ln [u,s] ...\n");
        fprintf(stderr, " ln src target\n");
        fprintf(stderr, " ln u[nlink] filename\n");
        fprintf(stderr, " ln s[ymlink] src target\n");
        exit(1);
    }
    
    if (!strcmp(argv[1],"-s")) {
        if (argc < 4) {
            fprintf(stderr, "ln l src target [link]\n");
            exit(1);
        }
        src = argv[2];
        target = argv[3]; 
        if (symlink(src, target) < 0) {
            perror("symlink");
            exit(1);
        }
    }
    else if (!strcmp(argv[1],"-u")) {
        src = argv[2];
        if (unlink(src) < 0) {
            perror("unlink");
            exit(1);
        }
    }
    else if (argc == 3) {
        src = argv[1];
        target = argv[2];
        if (link(src, target) < 0) {
            perror("link");
            exit(1);
        }
    }
    else {
        fprintf(stderr, "Unknown command...\n");
    }
}

void cp(int argc, char **argv){
   int src_fd; 
    int dst_fd;
    char buf[256];
    ssize_t rcnt;
    ssize_t tot_cnt = 0;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; 

    if (argc < 3){
        fprintf(stderr, "Usage: file_copy src_file dest_file\n");
        exit(1);
    }
    if ((src_fd = open(argv[1], O_RDONLY)) == -1){
        perror("[ERROR]SRC OPEN");
        exit(1);
    }

    if ((dst_fd = creat(argv[2], mode)) == -1){
        perror("[ERROR]DST OPEN");
        exit(1);
    }

    while ((rcnt = read(src_fd, buf, 256)) > 0){
        tot_cnt += write(dst_fd, buf, rcnt);
    }

    if (rcnt < 0){
        perror("[ERROR]READ");
        exit(1);
    }

    close(src_fd);
    close(dst_fd);
}
//이름 변경
void mv(int argc, char **argv){
struct stat buf;
    char *target;
    char *src_file_name_only;
    if (argc < 3)
    {
        fprintf(stderr, "Usage: file_rename src target\n");
        exit(1);
    }
   
    if (access(argv[1], F_OK) < 0)
    {
        fprintf(stderr, "%s not exists\n", argv[1]);
        exit(1);
    }
    else
    {
        char *slash = strrchr(argv[1], '/');
        src_file_name_only = argv[1];
        if (slash != NULL)
        { 
            src_file_name_only = slash + 1;
        }
    }
   
    target = (char *)calloc(strlen(argv[2]) + 1, sizeof(char));
    strcpy(target, argv[2]);
    if (access(argv[2], F_OK) == 0)
    {
        if (lstat(argv[2], &buf) < 0)
        {
            perror("lstat");
            exit(1);
        }
        else
        {
            if (S_ISDIR(buf.st_mode))
            {
                free(target);
                target = (char *)calloc(strlen(argv[1]) + strlen(argv[2]) + 2, sizeof(char));
                strcpy(target, argv[2]);
                strcat(target, "/");
                strcat(target, src_file_name_only);
            }
        }
    }
    printf("target = %s\n", target);
    if (rename(argv[1], target) < 0){
        perror("rename");
        exit(1);
    }
    free(target);
}

void cat(int argc, char **argv){
   FILE *file[argc-1];
    int loop;
    char buf;
    if (argc < 1) {
        fprintf(stderr, "Please Input Files \n");
        exit(1);
    }

    for(loop = 0; loop<argc-1; loop++) {
        file[loop] = fopen(argv[loop+1],"r");
        if(file[loop] == NULL) {
            printf("cat : %s : No such file or directory \n",argv[loop+1]);
        }
        else {
            while((buf = fgetc(file[loop])) != EOF) {
                printf("%c",buf);
            }
        
            if((fclose(file[loop])) != 0)
            {
                perror("CAT");
            }
        }
    }
    printf("\n");
}
void pipe_launch(int argc, char **argv) {
    int command_pos = 0;
    int count_pipe = 0;
    int i  = 0 ;
    int pd_idx = 0;
    int k = 0;
    int status;

    while(argv[command_pos] != NULL) {
        if(argv[command_pos][0] =='|')
            count_pipe ++;
        command_pos++;
    }

    int fd[count_pipe*2];
    for(i = 0; i < (count_pipe); i++){
        if(pipe(fd + i*2) < 0) {
            perror("couldn't pipe");
            exit(EXIT_FAILURE);
        }
    }

    command_pos = 0;
    for( i = 0 ; i<=count_pipe; i++) {
        int j =0;
        char **command = (char **)malloc(argc * sizeof(char *));

        if(!strcmp(argv[command_pos], "|"))
            command_pos++;

        if(i <count_pipe) {
        while(strcmp(argv[command_pos], "|")  ) {
            command[j] = (char *)malloc(100 * sizeof(char));
            strcpy(command[j],argv[command_pos]);
            command_pos++;
            j++;
            }
        }
        
         else {
            while(argv[command_pos] != NULL) {
                command[j] = (char *)malloc(100 * sizeof(char));
                strcpy(command[j],argv[command_pos]);
                command_pos++;
                j++;

            }
        }

        command[j] = NULL;

        pid = fork();
        if(pid == 0) {
            if(i<count_pipe) {
            if(dup2(fd[pd_idx + 1], 1) < 0){
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            }

            if(pd_idx != 0 ){
                if(dup2(fd[pd_idx-2], 0) < 0){
                    perror(" dup2");
                    exit(EXIT_FAILURE);

                }
            }
            for(k = 0; k < 2*count_pipe; k++){
                close(fd[k]);
            }

            if( execvp(command[0], command) < 0 ){
                    perror(command);
                    exit(EXIT_FAILURE);
            }
        }
        pd_idx+=2;

        k=0;
        while(command[k] != NULL) {
            free(command[k]);
            k++;
        }
        free(command);
    }

    for(k = 0; k < 2 * count_pipe; k++){
        close(fd[k]);
    }

    for(k = 0; k < count_pipe + 1; k++)
        wait(&status);
    
}


void launch(int argc, char **argv) {
    pid = 0;
    int i = 0;
    int is_background = 0;
    if(argc != 0 && !strcmp(argv[argc -1], "&") ){
        argv[argc-1] = NULL;
        is_background = 1;
    }
    pid = fork();
    if (pid == 0){
        if(is_background){
            printf("\n백그라운드 실행 PID: %ld\n", (long)getpid());
        }

        if(execvp(argv[0], argv) < 0) {
            perror("[ERROR] CREATE BACKGROUND: ");
        }
    }
    else {
        if(is_background == 0){
            wait(pid);
        }
    }
}

void redirection(int argc, char **argv) {
    pid;
    int i = 0;
    int fd;
    int split_index = 0, is_write = 0;

    int write_flags = O_WRONLY | O_CREAT | O_TRUNC;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    char *cmd[10] = {'\0'};


    for(i = 0; i < argc; i++){
        if(!strcmp(argv[i], ">")){
            split_index = i;
            is_write = 1;
        }
        else if(!strcmp(argv[i], "<")){
            split_index = i;
            is_write = 0;
        }
    }

    for(i = 0; i < split_index; i++){
        cmd[i] = argv[i];
    }

    pid = fork();
    if(pid == 0) {
  
        if (is_write){
            if ((fd = open(argv[split_index + 1], write_flags, mode)) == -1) {
                perror("[ERROR] OPEN: ");
                exit(1);
            }
            if (dup2(fd, 1) == -1) {
                perror("[ERROR] DUP2: ");
                exit(1);
            }
        }
 
        else{
            if ((fd = open(argv[split_index + 1], O_RDONLY)) == -1) {
                perror("[ERROR] OPEN: ");
                exit(1);
            }
            if (dup2(fd, 0) == -1) {
                perror("[ERROR] DUP2: ");
                exit(1);
            }
        }
    
        if (close(fd) == -1) {
            perror("[ERROR] CLOSE: ");
            exit(1);
        }
        execvp(cmd[0], cmd);
    }

    else if (pid > 0) {
        wait(pid);
    }
}

char *substring(int start, int end, char * str) {
    char *new = (char *)malloc(sizeof(char)*(end-start+2));
    strncpy(new,str+start,end-start+1);
    new[end-start+1] = 0;
    return new;
}

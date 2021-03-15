#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define UNDEFINED -1
#define DIM_BUFFER 64

typedef struct {
	char **cmd;
	pid_t pid;
	bool redirectout;
	bool redirectin;
	bool redirectoutToFile;
	bool redirectinToFile;
	int filein;
	int fileout;
	int pipeout[2];
	int pipein[2];
	int status;
} process_t;


process_t *process_init_command(char *cmd[]){
		process_t *process = malloc(sizeof(process_t));
		process->cmd = cmd;
		process->redirectout = false;
		process->redirectin = false;
		process->redirectoutToFile = false;
		process->redirectinToFile = false;
		process->status = UNDEFINED;
		return process;
}


int process_set_stdout_to_file(process_t *process, int file){
	process->redirectoutToFile = true;
	process->fileout = file;
	return 0;
}


int process_get_access_stdout(process_t *process){
	process->redirectout = true;
	return 0;
}


int process_start_execution(process_t *process){

	// criação dos pipes
	int pipein = pipe(process->pipein);
	int pipeout = pipe(process->pipeout);
	if(pipein < 0 || pipeout < 0){
		perror("Erro na criação do pipe");
		exit (-1);
	}

	// criação do processo filho
	process->pid = fork();
	if(process->pid < 0){
		perror("Erro na criação do processo");
		exit(-1);
	}

	// aplicação processo filho
	if (process->pid == 0){
		if (process->redirectout){
			close(process->pipeout[0]);
			dup2(process->pipeout[1], STDOUT_FILENO);
			close(process->pipeout[1]);
		}
		if(process->redirectoutToFile){
			dup2(process->fileout, STDOUT_FILENO);
		}
		if (process->redirectin){
			close(process->pipein[0]);
			dup2(process->pipein[1], STDIN_FILENO);
			close(process->pipein[1]);
		}
		if(process->redirectinToFile){
			dup2(process->filein, STDIN_FILENO);
		}
		execvp(process->cmd[0], process->cmd);
	}
	else {
		close(process->pipein[0]);
		close(process->pipeout[1]);
	}
	return 0;
}


int process_wait_for_termination(process_t * process, int *exitValue){
	int status;
	pid_t waitp;

	waitp = waitpid(process->pid, &status, 0); //WNOHANG = 0;
	if(waitp == -1){
		perror("Ocorreu um erro no waitpid");
		exit(-1);
	}

	// terminou com sucesso
	if(WIFEXITED(status) == true) {
		*exitValue = WEXITSTATUS(status);
		return 1;
	}
	// terminou abrutamente
	else if(WIFSIGNALED(status) == true){
		*exitValue = WTERMSIG(status);
		return 0;
	}
	// terminou com erro
	else{
		*exitValue = WSTOPSIG(status);
		return -1;
	}
}


void process_destroy(process_t *process){
	close (process->pipeout[0]);
	free(process);
}


int process_read_out(process_t *process, void * buf, size_t size){
	int rdsize;
	rdsize = read(process->pipeout[0], buf, size);
	if (rdsize == -1) {
		perror("Erro na leitura do ficheiro.");
		return -1;
	}
	return rdsize;
}


/* TESTES */

int test_get_access_to_out(){
	char *cmd[] = {"pdftotext", "-layout","SOt-05.01-Processos-API.pdf", "-", NULL};
    process_t *process = process_init_command(cmd);
    if (process == NULL) {
    	perror("process_init_command ");
    	exit(-1);
    }
    process_get_access_stdout(process);
    process_start_execution(process);

    int fdout = open("sot-05.01.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fdout < 0) {
    	perror("Open File");
    	exit(-2);
    }
    char buf[DIM_BUFFER];
    size_t  nBytes;
    int retval = 0;

    while ((nBytes = process_read_out(process, buf, sizeof(buf))) > 0 ) {
    	if (write(fdout, buf, nBytes) < 0) {
    		perror("write");
    		retval = -3;
    		break;
    	}
    }
    if (nBytes < 0) {
    	perror("read");
    	retval = -4;
    }
    int exitval;
    if ( process_wait_for_termination(process, &exitval) != 0 )
    	printf("Process finished with exit value of %d\n", exitval);

    close(fdout);
    process_destroy(process);

    return retval;
}


/* OPCIONAIS */

int process_set_stdin_to_file(process_t *process, int file){
	process->redirectinToFile = true;
	process->filein = file;
	return 0;
}

int process_get_access_stdin(process_t *process){
	process->redirectin = true;
	return 0;
}

int process_write_in(process_t *process, void *buf, size_t size){
	int wrsize;
	wrsize = write(process->pipein[1], buf, size);
	if ( wrsize == -1 ) {
		perror("Erro na escrita no pipe");
	    exit(-1);
	}
	return wrsize;
}

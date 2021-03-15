#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define DIM_BUFFER 64
#define UNDEFINED -1

typedef struct {
	char **cmd;
	bool redirectout;
	bool redirectin;
	int filein;
	int fileout;
	int pipein[2];
	int pipeout[2];
	int status;
} process_t;


process_t *process_t_init_command(char *cmd[]) {
	process_t *process = malloc(sizeof(process_t));
	process->cmd = cmd;
	process->redirectin = false;
	process->redirectout = false;
	process->filein = UNDEFINED;
	process->fileout = UNDEFINED;
	process->status = UNDEFINED;
	return process;
}


int process_set_stdout_to_file(process_t *process, int file) {
	process->redirectout = true;
	process->fileout = file;
	return 0;
}


int process_get_access_stdout(process_t *process) {
	process->redirectout = true;
	return 0;
}


int process_start_execution(process_t *process){

	// criação do pipe
	int pipein=pipe(process->pipein);
	int pipeout=pipe(process->pipeout);
	if (pipein < 0 || pipeout < 0){
		perror("Erro na criação do pipe");
		exit (-1);
	}

	// criação processo filho
	pid_t pid = fork();
	if (pid < 0) {
		perror("Erro na criação do processo");
		exit(-1);
	}

	// aplicação processo filho
	if (pid == 0){
		if (process->redirectout){
			close(process->pipeout[0]);
			if(process->fileout != UNDEFINED) {
				dup2(STDOUT_FILENO, process->fileout);
			}
			else {
				dup2(STDOUT_FILENO, process->pipeout[1]);
				close(process->pipeout[1]);
			}
		}
		if (process->redirectin){
			close(process->pipein[0]);
			if(process->filein != UNDEFINED) {
				dup2(STDIN_FILENO, process->filein);
			}
			else {
				dup2(STDIN_FILENO, process->pipein[1]);
				close(process->pipein[1]);
			}
		}
		execvp(process->cmd[0], process->cmd);
	}

	// aplicação processo pai
	else {
		close(process->pipein[0]);
		close(process->pipeout[1]);
	}
	return 0;
}


/*
		if (process->redirectout){
			close(process->pipeout[1]);
			if(process->fileout != -1) {
				dup2(process->fileout, process->pipein[0]);
				read(process->pipeout[0], buf, sizeof(buf));
			}
			else {
				read(process->pipeout[0], buf, sizeof(buf));
			}
		}
		if (process->redirectin){
			close(process->pipein[1]);
			if(process->filein != -1) {
				dup2(process->filein, process->pipein[0]);
				read(process->pipein[0], buf, sizeof(buf));
			}
			else {
				read(process->pipein[0], buf, sizeof(buf));
			}
		}*/
		//close(process->pipein[1]);
		//close(process->pipeout[0]);

int process_wait_for_termination(process_t *process, int *exitValue){
	int status;
	pid_t pid = fork();
	pid_t waitp;
	waitp = waitpid(pid, &status, 0); // WNOHANG ≃ 0
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
	else {
		*exitValue = WSTOPSIG(status);
		return -1;
	}
}


void process_destroy(process_t *process){
	close (process->pipeout[0]);
	free(process);
}


int process_read_out(process_t *process, void *buf, size_t size){
	int rdsize;
	rdsize = read(process->pipeout[0], buf, size);
		if ( rdsize == -1 ) {
			perror("Erro na leitura do ficheiro");
		    return -1;
		}
	return rdsize;
}



/* TESTES */

int test_get_access_to_out() {
	char *cmd[] = {"pdftotext", "-layout", " SOt-05.01-Processos API.pdf", "-", NULL};
	process_t *process = process_t_init_command(cmd);
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
	size_t nBytes;
	int retval = 0;
	while ( (nBytes = process_read_out(process, buf, sizeof(buf))) > 0 ) {
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
	if ( process_wait_for_termination(process, &exitval) != 0 ){
		printf("Process finished with exit value of %d\n", exitval);
 	}
 	close(fdout);
 	process_destroy(process);
 	return retval;
}

int main(){
	return test_get_access_to_out();
}


/* OPCIONAIS */

int process_set_stdin_to_file(process_t *process, int file) {
	process->redirectin = true;
	process->filein = file;
	return file;
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



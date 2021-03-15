/*
 * Process.c
 *
 *  Created on: 15/05/2019
 *      Author: aluno
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
	char **cmds;
	bool redirectout;
	bool redirectin;
	int filein;
	int fileout;
	int pipeout[2];
	int pipein[2];
	int status;
	pid_t pid;
} process_t;

#define NOT_DEFINED -1
#define DIM_BUFFER 64

//Função para criar uma nova variável do tipo process_t
 //e definir qual o programa a ser executado. A função devolve um apontador
 //para uma variável do tipo process_t onde serão guardadas
 //todas as informações relacionadas com o processo.
process_t *process_init_command(char *cmd[]){
		process_t *process = malloc(sizeof(process_t));
		process->cmds = cmd;
		process->redirectout = false;
		process->redirectin = false;
		process->fileout = NOT_DEFINED;
		process->filein = NOT_DEFINED;
		process->status = NOT_DEFINED;
		return process;
}

/*Redireciona o standard output para um ficheiro*/
int process_set_stdout_to_file(process_t *process, int file){
	//process->redirectout = true;
	process->fileout = file;
	return 0;
}


/*Redirecionar o standard output para o processo pai permitindo
 * a leitura dos dados escritos no standard
 * output pelo programa representado por process_t*/
int process_get_access_stdout(process_t *process){
	process->redirectout = true;
	return 0;
}

/*coloca em execução o programa representado pela variável process*/
int process_start_execution(process_t *process){
	int pipein = pipe(process->pipein);
	int pipeout = pipe(process->pipeout);
		//Se não for criado os pipes irá dar erro
		if (pipein < 0|| pipeout < 0){
			perror("Não houve criação do pipe");
			exit (-1);
		}


		// criaçao processo filho
		process->pid = fork();
		if (process->pid < 0){
			perror("Não houve criação do processo");
			exit(-1);
		}

		//Processo filho
		if (process->pid == 0){
			//queremos ler o standard output fechamos o  canal de escrita(pipeout[1] , duplica-se o processo
			// no filho e coloca-se os dados para leitura no pipeout[0], de seguida fecha-se
			if (process->redirectout){
				close(process->pipeout[0]);
				dup2(process->pipeout[1],STDOUT_FILENO);
				close(process->pipeout[1]);
			}

			if(process->fileout != NOT_DEFINED){
				dup2(process->fileout,STDOUT_FILENO);
			}
			//queremos ler o standard input fechamos o  canal de escrita(pipeout[0] , duplica-se o processo
			// no filho e coloca-se os dados para leitura no pipeout[1], de seguida fecha-se
			if (process->redirectin){
				close(process->pipein[1]);
				dup2(process->pipein[0],STDIN_FILENO);
				close(process->pipein[0]);
			}
			if(process->filein != NOT_DEFINED){
				dup2(process->filein,STDIN_FILENO);
			}

			execvp(process->cmds[0], process->cmds);
		}
			else {
				close(process->pipein[0]);
				close(process->pipeout[1]);
			}

		return 0;

}

/*espera que o programa representando por process termine a sua execução
 determinar se o programa finalizou a sua execução através de um exit devolvendo
 este valor através do argumento exitValue e a função retorna o valor 1.
 Termina abrutamente, sem ter feito exit, a função devolve o valor 0.
 Em caso de erro a função retorna o valor -1
 */
int process_wait_for_termination(process_t * process, int *exitValue){
	int status;
	pid_t w;

	w = waitpid(process->pid, &status, 0); //WNOHANG = 0;
	if(w == -1){
		perror("Ocorreu um erro no processo de waitpid");
		exit(-1);
	}

	//Terminou normalmente
	if(WIFEXITED(status) == true) {
		*exitValue = WEXITSTATUS(status);
		return 1;
	}
	//Saiu de forma abrupta
	else if(WIFSIGNALED(status) == true){
		*exitValue = WTERMSIG(status);
		return 0;
	}
	//Deu erro por completo
	else{
		*exitValue = WSTOPSIG(status);
		return -1;
	}
}


//eliminar todos os recursos associados à variável do tipo process_t
void process_destroy(process_t *process){
	close (process->pipeout[0]);
	free(process);
}

// ler a informação gerada, pelo programa, no standard output
int process_read_out(process_t *process, void * buf, size_t size){
	int readsize;
		readsize = read(process->pipeout[0], buf, size);
		        if ( readsize == -1 ) {
		           perror("Erro a ler o ficheiro.");
		           return -1;		        }
	return readsize;
}


int process_write_in(process_t *process,void *buf,size_t size){
	int writesize;
	writesize = write(process->pipein[1], buf, size);
	    if ( writesize == -1 ) {
	       perror("Erro a escrever no pipe");
	       exit(-1);
	    }
	    return writesize;
}

int process_get_acess_stdin(process_t *process){
	process->redirectin = true;
	return 0;
}
int process_set_stdin_to_file(process_t *process, int file){
	process->filein = file;
	return 0;
}

//TESTE//___________________________________________________________________________________

	int test_get_acess_to_out(){
	char *cmd[] = {"pdftotext", "-layout","SOt-05.01-Processos-API.pdf", "-", NULL};
     process_t *process = process_init_command(cmd);
     if (process == NULL) {
    	 perror("process_init_command ");
    	 exit(-1); }
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
     if ( process_wait_for_termination(process, &exitval) != 0 ) printf("Process finished with exit value of %d\n", exitval);

     close(fdout);
     process_destroy(process);

     return retval;
 }


int main(){
	int reval = test_get_acess_to_out();
	printf("%d\n", reval);

}


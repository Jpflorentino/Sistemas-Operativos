#ifndef _process_h
#define _process_h
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>


typedef struct {
	char **cmd;
	bool redirectout;
	bool redirectin;
	int filein;
	int fileout;
	int pipeout[2];
	int pipein[2];
	int status;
	pid_t pid;
} process_t;

process_t *process_init_command(char *cmd[]);
int process_set_stdout_to_file(process_t *process, int file);
int process_get_access_stdout(process_t *process);
int process_start_execution(process_t *process);
int process_wait_for_termination(process_t * process, int *exitValue);
void process_destroy(process_t *process);
int process_read_out(process_t *process, void * buf, size_t size);
int process_write_in(process_t *process,void *buf,size_t size);
int process_get_acess_stdin(process_t *process);
int process_set_stdin_to_file(process_t *process, int file);

#endif

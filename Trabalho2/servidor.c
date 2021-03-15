
#include "myinet.h"
#include "errorUtils.h"
#include "include.h"
#include "process_t.h"

typedef struct threadArgs{
    int socket, 
    convertedFiles,
    totalSizeConvFiles,
    avgSizeConvFiles;
} argsThread;

int readLine(int socket, char *buffer, int size){
    int i;
    for(i = 0; i < size; i++) {
        read(socket, &buffer[i], sizeof(char));
        if(buffer[i] == '\n') break;
    }
    buffer[i] = '\0';
    return i;
}

void *threadMenu(void *_args){
    argsThread *threadArgs = (argsThread *) _args;

    int nConnections = 1;

    threadArgs->convertedFiles = 0;
    threadArgs->totalSizeConvFiles = 0;

    while(1){
        if(nConnections == threadArgs->convertedFiles){
            printf("\n______________MENU____________\n");
            printf("Total de ficheiros convertidos: %d\n", threadArgs->convertedFiles);
            printf("Tamanho total dos ficheiros convertidos: %d\n", threadArgs->totalSizeConvFiles);
            threadArgs->avgSizeConvFiles = threadArgs->totalSizeConvFiles/threadArgs->convertedFiles;
            printf("Tamanho médio dos ficheiros convertidos: %d\n", threadArgs->avgSizeConvFiles);
            printf("\n À espera de uma ligação...\n");
            nConnections++;
        }
    }
}

void *threadAccept(void *_args){
    argsThread *threadArgs = (argsThread *) _args;

    char *filename = malloc(100);   // nome do ficheiro
    char *size = malloc(100);       // tamanho do ficheiro
    char *aux = malloc(100) ;       // \n de terminação do cabeçalho

    readLine(threadArgs->socket, filename, 100);
    readLine(threadArgs->socket, size, 100);
    readLine(threadArgs->socket, aux, 100);

    int filesize = atoi(size);  // dimensão do ficheiro

    printf("Nome do ficheiro: %s\n", filename);
    printf("Tamanho do ficheiro: %d\n", filesize);

    char path[DIM_BUFFER];
    sprintf(path, "./pdf_files/%s", filename);

    // abre o ficheiro PDF
    int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);  
    if(fd  < 0) {
        perror("Erro a abrir o ficheiro ");
        exit(-1);
    }

    // Ler do socket para um ficheiro
    char buffer[DIM_BUFFER+1];  // reserva um byte extra para o char de terminação
	int n_bytes;
    int fileContent = 0;
	while ( (n_bytes = read(threadArgs->socket, buffer, sizeof(buffer))) != 0 ) { //leitura caractere a caractere
		if ( n_bytes < 0 ) 
			FatalErrorSystem("Erro na leitura do socket");
		if (write(fd, buffer, n_bytes) <  0)
			FatalErrorSystem("Erro na escrita do ficheiro");
		fileContent += n_bytes;
		if(fileContent == filesize) { 
			break;
		}
    }
    close(fd);
    free(aux);
   

    char *cmd[] = {"pdftotext", "-layout", path, "-", NULL};
    process_t *process = process_init_command(cmd); // iniciar o processo

    // redirecionar o processo para o socket do cliente
    process_set_stdout_to_file(process, threadArgs->socket);
   
    // ficheiro para converter
    process_start_execution(process);

    threadArgs->convertedFiles++;   // incremento do nº de conversões
    threadArgs->totalSizeConvFiles += filesize; // soma do size do ficheiro ao total

    char *fileNameTxt = strtok(filename, ".");

    close(threadArgs->socket);    
    printf("O ficheiro %s.txt foi enviado para o cliente com sucesso.\n", fileNameTxt);
    return NULL;
}


int main(int argc, char * argv[]) {
    int                sockfd;
    int                newsockfd;
    struct sockaddr_in serv_addr;
    struct sockaddr_in client_addr;
    socklen_t          clientSize;
    //int                n_bytes;
    //char               buffer[DIM_BUFFER+1]; /* reserva um byte extra para o char de terminacao */
    
    unsigned int       serverPort = DEFAULT_PORT;
    
    printf("Programa de teste dos SOCKETS (server TCP) ...\n\n");
  
    if (argc == 2) {
        serverPort = atoi(argv[1]);
    }
    else if (argc != 1) {
        printf("Argumentos inválidos.\nUse: %s <port_number>\n", argv[0]);
        exit(EXIT_FAILURE);	 
    }
    
    if ( (serverPort < 1) || (serverPort > 65536) ) {
        printf("O porto deve estar entre 1 e 65536\n");
        exit(EXIT_FAILURE);	 
    }
        	
    printf("O servidor vai registar-se no porto %d\n", serverPort);
    
    /* Criar socket TCP */
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
        FatalErrorSystem("Erro ao pedir o descritor");

    /* Registar o endereço local de modo a que os clientes possam contactar com o servidor */
    memset( (char*)&serv_addr, 0, sizeof(serv_addr) );
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port        = htons(serverPort);

    if ( bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0 )
        FatalErrorSystem("Error ao efetuar o bind");

    /* Ativar socket com fila de espera de dimensão 5 */
    if (listen( sockfd, 5) < 0 ){
        FatalErrorSystem("Erro no listen");
    }
        
    argsThread *threadArgs = malloc(sizeof(argsThread));

    pthread_t menuThread;
    pthread_create(&menuThread, NULL, threadMenu, threadArgs);

    printf("À espera de uma ligação...\n");

    while (1) {
        
    clientSize = sizeof( client_addr );
    newsockfd = accept(sockfd, (struct sockaddr *)(&client_addr), &clientSize);
    if ( newsockfd < 0 )
        FatalErrorSystem("Erro ao efetuar o accept");
            
    printf("Ligação estabelecida\n");

    threadArgs->socket = newsockfd;

    pthread_t clientThread;
    pthread_create(&clientThread, NULL, threadAccept, threadArgs);  // inicializar a thread

    pthread_detach(clientThread); 
    }
  
    close(sockfd);
    return 0;
}
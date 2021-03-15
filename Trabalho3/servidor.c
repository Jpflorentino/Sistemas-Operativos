#ifdef STDC_ALLOC_LIB
#define STDC_WANT_LIB_EXT2 1
#else
#define _POSIX_C_SOURCE 200809L
#endif

#include "myinet.h"
#include "errorUtils.h"
#include "include.h"
#include "process_t.h"
#include "sharedBuffer.h"

#define NTHREADS 5

typedef struct menu{
    int convertedFiles;
    int totalSizeConvFiles;
    int avgSizeConvFiles;
} Menu;

typedef struct sharedBuffers{
    SharedBuffer *sBufferReceive;
    SharedBuffer *sBufferConvert; 
} SharedBuffers;

typedef struct threadArgs{
    int clientID;
    char *filename;
    int fileSize;
    int socket;
    Menu *menu;
    SharedBuffers *SharedBuffer;
} ArgsThread;

void sigintHandler(int sig_num) 
{ 
    /* Reset handler to catch SIGINT next time. 
       Refer http://en.cppreference.com/w/c/program/signal */
    signal(SIGINT, sigintHandler); 
    printf("\n Cannot be terminated using Ctrl+C \n"); 
    fflush(stdout); 
}

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
    ArgsThread *threadArgs = (ArgsThread *) _args;

    int nConnections = 1;

    threadArgs->menu->convertedFiles = 0;
    threadArgs->menu->totalSizeConvFiles = 0;

    while(1){
        if(nConnections == threadArgs->menu->convertedFiles){
            printf("\n______________MENU____________\n");
            printf("Total de ficheiros convertidos: %d \n", threadArgs->menu->convertedFiles);
            printf("Tamanho total dos ficheiros convertidos: %d \n", threadArgs->menu->totalSizeConvFiles);
            threadArgs->menu->avgSizeConvFiles = threadArgs->menu->totalSizeConvFiles/threadArgs->menu->convertedFiles;
            printf("Tamanho médio dos ficheiros convertidos: %d \n", threadArgs->menu->avgSizeConvFiles);
            printf("\n À espera de uma ligação... \n");
            nConnections++;
        }
    }
}

void threadAccept(int socket, char *file_name, Menu *menu){

    char *filename = malloc(100);   // nome do ficheiro
    char *size = malloc(100);       // tamanho do ficheiro
    char *aux = malloc(100) ;       // \n de terminação do cabeçalho

    readLine(socket, filename, 100);
    readLine(socket, size, 100);
    readLine(socket, aux, 100);

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
	while ( (n_bytes = read(socket, buffer, sizeof(buffer))) != 0 ) { //leitura caractere a caractere
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
    free(size);
    free(filename);

    menu->totalSizeConvFiles += filesize; // soma do size do ficheiro ao total
}

void *receiveFile(void *_sBuffer){
    SharedBuffers *sBuffer = (SharedBuffers *)_sBuffer;
    while(1){
        void *fileInfo = sharedBuffer_Get(sBuffer->sBufferReceive);
        ArgsThread *threadArgs = (ArgsThread *) fileInfo;
        threadAccept(threadArgs->socket, threadArgs->filename, threadArgs->menu); 
        sharedBuffer_Put(threadArgs->SharedBuffer->sBufferConvert, threadArgs);
    }
    return NULL; 
}

void *convertFile(void *_sBuffer){
    SharedBuffers *sBuffer = (SharedBuffers *)_sBuffer;
    while(1){
        void *args = sharedBuffer_Get(sBuffer->sBufferConvert);
        ArgsThread *_threadArgs = (ArgsThread *) args;

        char path[64];
        sprintf(path,"./ficheiros/%s", _threadArgs->filename);

        char *cmd[] = {"pdftotext", "-layout", path, "-", NULL};
        process_t *process = process_init_command(cmd); //iniciar o processo

        // redirecionar o processo para o socket do cliente
        process_set_stdout_to_file(process, _threadArgs->socket);
    
        // ficheiro para converter
        process_start_execution(process);
        
        _threadArgs->menu->convertedFiles++;   // incremento do nº de conversões

        char *filenameTxt = strtok(_threadArgs->filename, ".");

        close(_threadArgs->socket);  

        printf("O ficheiro %s.txt foi enviado para o cliente com sucesso.\n", filenameTxt);
    }

    return NULL;
}


int main(int argc, char * argv[]) {
    int                sockfd;
    int                newsockfd;
    struct sockaddr_in serv_addr;
    struct sockaddr_in client_addr;
    socklen_t          clientSize;
    pthread_t threadReceive[25];
    pthread_t threadConvert[25];
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
        	
    printf("O servidor vai registar-se no porto: %d\n", serverPort);
    
    /* Criar socket TCP */
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
        FatalErrorSystem("Erro ao pedir o descritor");

    /* Registar o endereço local de modo a que os clientes possam contactar com o servidor */
    memset( (char*)&serv_addr, 0, sizeof(serv_addr) );
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port        = htons(serverPort);

    /* Criar os SharedBuffers para receber e converter os ficheiros */
    SharedBuffer *sBufferRec = malloc(sizeof(*sBufferRec));
    SharedBuffer *sBufferConv = malloc(sizeof(*sBufferConv));

    sharedBuffer_init(sBufferRec,15);
    sharedBuffer_init(sBufferConv,15);

    if ( bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0 )
        FatalErrorSystem("Erro ao efetuar o bind");

    /* Ativar socket com fila de espera de dimensão 5 */
    if (listen( sockfd, NTHREADS) < 0 ){
        FatalErrorSystem("Erro no listen");
    }
        
    ArgsThread *threadArgs = malloc(sizeof(ArgsThread));

    pthread_t menuThread;
    int m = pthread_create(&menuThread, NULL, threadMenu, threadArgs);
    if (m != 0)
        FatalErrorSystem("Erro na criação do menu");
    
    /* Criar a estrutura SharedBuffers */
    SharedBuffers *sBuffer = malloc(sizeof(SharedBuffers));
    sBuffer->sBufferConvert = malloc(sizeof(SharedBuffers));
    sBuffer->sBufferReceive = malloc(sizeof(SharedBuffers));
    sBuffer->sBufferConvert = sBufferConv;
    sBuffer->sBufferReceive = sBufferRec;

    /* Criar as threads para receber o ficheiro */
    for (int IDThread = 0; IDThread < NTHREADS; IDThread++) {
		printf("O ID da thread que recebe o ficheiro é: %d \n", IDThread);
		pthread_create( &threadReceive[IDThread], NULL, receiveFile, sBuffer);
	}

    //Criar aas threads para converter o ficheiro
    for (int IDThread = 0; IDThread < NTHREADS; IDThread++) {
		printf("O ID da thread que converte o ficheiro é: %d \n", IDThread);
		pthread_create( &threadConvert[IDThread], NULL, convertFile, sBuffer);
	}

    printf("À espera de uma ligação...\n");

    signal(SIGINT, sigintHandler);

    while(1) { 
        clientSize = sizeof( client_addr );
        newsockfd = accept(sockfd, (struct sockaddr *)(&client_addr), &clientSize);
        if ( newsockfd < 0 )
            FatalErrorSystem("Erro ao efetuar o accept");
                
        printf("Ligação estabelecida\n");

        threadArgs->SharedBuffer = malloc(sizeof(SharedBuffers));
        threadArgs->SharedBuffer = sBuffer;
        threadArgs->filename = malloc(64);
        threadArgs->socket = newsockfd;
        threadArgs->clientID++;

        pthread_t clientThread;
        pthread_create(&clientThread, NULL, convertFile, threadArgs);  // inicializar a thread

        pthread_detach(clientThread); 
    }
    
    pthread_detach(menuThread);
    sharedBuffer_destroy(sBufferConv);
    sharedBuffer_destroy(sBufferRec);
    close(sockfd);
    return 0;
}
#include "myinet.h"
#include "errorUtils.h"
#include "include.h"



int main(int argc, char * argv[])  {
	
    int                sockfd;
    struct sockaddr_in serv_addr;
    struct hostent    *phe;
    in_addr_t          serverAddress;
    char              *serverName = DEFAULT_HOST;
    unsigned int       serverPort = DEFAULT_PORT;
  
    printf("Programa de teste dos SOCKETS (cliente TCP) ...\n\n");

    if (argc == 3) {
        serverName = argv[1];
        serverPort = atoi(argv[2]);
    }
    else if (argc != 1) {
        printf("Argumentos inválidos.\nUse: %s <host> <port_number>\n", argv[0]);
        exit(EXIT_FAILURE);	 
    } 

	/* Determinar o endereço IP do servidor */
    if ((phe = gethostbyname(serverName)) != NULL) 
        memcpy(&serverAddress, phe->h_addr_list[0], phe->h_length);
    else 
        if ( (serverAddress = inet_addr(serverName)) == -1)
           FatalErrorSystem("Impossível determinar endereço IP para a máquina \"%s\"",serverName);
        
    if ( (serverPort < 1) || (serverPort > 65536) ) {
        printf("O porto deve estar entre 1 e 65536\n");
        exit(EXIT_FAILURE);	 
    }
  
      
    /* Abrir um socket TCP (an Internet Stream socket) */
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
        FatalErrorSystem("Erro ao pedir o descritor");
        
  
    /* Preencher a estrutura serv_addr com o endereço do servidor que pretendemos contactar */
    memset((char*)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = serverAddress;
    serv_addr.sin_port        = htons(serverPort);
    
    printf("O cliente vai ligar-se ao servidor na máquina %s:%d\n", serverName, serverPort);
    printf("IP: %s\n", inet_ntoa(serv_addr.sin_addr));
    
        
    /* Ligar-se ao servidor */
    if ( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0 )
        FatalErrorSystem("Falha na ligação");
    
    printf("Ligação estabelecida...\n");

    
   
    char *filename;
    filename = malloc(30);
    printf("Nome do ficheiro: ");
    scanf("%s", filename);

    int fd = open(filename, O_RDONLY);

    if(fd < 0)
        perror("Erro na abertura do ficheiro");

    struct stat st;
    fstat(fd, &st);   // preenche a estrutura com as informações sobre o ficheiro especificado no file descriptor

    int size = st.st_size;  // tamanho do ficheiro
    char header[DIM_BUFFER];    // filename; size; /n

    sprintf(header, "%s\n%d\n\n", filename, size);

    write(sockfd, header, strlen(header));  // escrever o cabeçalho no socket

    // Ler o ficheiro e escrevê-lo no socket e enviá-lo para o servidor
    char buffer[DIM_BUFFER+1];    // reserva um byte extra para o char de terminação
    int n_bytes;
    int fileContent = 0;
	while ( (n_bytes = read(fd, buffer, sizeof(buffer))) != 0 ) {   // leitura caractere a caractere
		if ( n_bytes < 0 ) 
			FatalErrorSystem("Erro na leitura do ficheiro");
		if (write(sockfd, buffer, n_bytes) <  0)
			FatalErrorSystem("Erro na escrita no socket");
		fileContent+= n_bytes;
		if(fileContent == size)
			break;
    }
    close(fd);
    printf("Envio do ficheiro para o servidor\n");

    // retirar o .pdf
    char path[DIM_BUFFER];
    char *filenametxt = strtok(filename, ".");  // elimina o "." no filename

    sprintf(path, "./txt_files/%s.txt", filenametxt);

    // Criar um novo ficheiro na diretoria
    // Caso já exista um ficheiro com o mesmo nome, reescreve esse ficheiro com o novo
    int fdtxt = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);  
    if(fdtxt  < 0) {
        perror("Erro a abrir o ficheiro ");
        exit(-1);
    }

    // Ler o socket e escrever no ficheiro o que está nele, mandar para a pasta
    char buffertxt[DIM_BUFFER+1];   // reserva um byte extra para o char de terminação
	int n_bytestxt;
    int fileContenttxt = 0;
	while ( (n_bytestxt = read(sockfd, buffertxt, sizeof(buffertxt))) != 0 ) { //leitura caractere a caractere
		if ( n_bytestxt < 0 ) 
			FatalErrorSystem("Erro na leitura do Socket");
		if (write(fdtxt, buffertxt, n_bytestxt) <  0)
			FatalErrorSystem("Erro no Write do Ficheiro");
		fileContenttxt += n_bytestxt;
		if(fileContenttxt == size){
			break;
		}
    }
    close(fdtxt);

    printf("O ficheiro .txt foi criado e recebido com sucesso\n");
    free(filename);

    close(sockfd);
    return EXIT_SUCCESS;
}
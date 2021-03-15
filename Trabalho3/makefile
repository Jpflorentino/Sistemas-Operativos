CC = gcc
CFLAGS = -Wall -g -pthread -std=c99


BIN =  servidor cliente
      


all: $(BIN)
# Servidor TCP concorrente (baseado em múltiplos processos)
#
servidor: servidor.o errorUtils.o process_t.o sharedBuffer.o
	$(CC)  -pthread -o  $@ $^ 
	
servidor.o: servidor.c myinet.h errorUtils.h include.h process_t.h sharedBuffer.h
	$(CC) $(CFLAGS) -c -o $@ $< 
		
cliente: cliente.o errorUtils.o sharedBuffer.o
	$(CC) -pthread -o $@ $^ 

cliente.o: cliente.c myinet.h errorUtils.h include.h sharedBuffer.h
	$(CC) $(CFLAGS) -c -o $@ $<  

process_t.o: process_t.c process_t.h include.h
	$(CC) $(CFLAGS) -c -o $@ $<
	
# Tratamento de erros
#
errorUtils.o: errorUtils.c errorUtils.h
	$(CC) $(CFLAGS) -c -o $@ $<

sharedBuffer.o: sharedBuffer.c sharedBuffer.h
	$(CC) $(CFLAGS) -c -o $@ $<


clean:
	rm -f *.o $(BIN)


	
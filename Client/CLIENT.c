#include <stdlib.h> // getenv()
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h> // timeval
#include <sys/select.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h> // inet_aton()
#include <sys/un.h> // unix sockets
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h> // SCNu16
#include <string.h>
#include "errlib.h"
#include "sockwrap.h"
#include <time.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <dirent.h>
#include <sys/stat.h>

char bufferrc[1024];	//buffer ricezione dati file
char bufferReply[6];	//buffer ricezione intestazione +OK\r\n oppure -ERR\r\n
char bufferDim[4];	//buffer dimensione file da ricevere
char bufferCopy[4];
char bufferTime[4];	//buffer ultima modifica file


//metodo per aprire il file in scrittura
void writeFile (char name[], char values[], int size, char how) {	//name[]: nome del file da scrivere.   		  values[]: stringa da scrivere nel file.    
									//int size: fino a dove leggere values[]          char how: modalità di scrittura ('w': write, 'a': append)
	if (strlen(values) == 0) {	//non ho caratteri da scrivere
		return ;
	}

	FILE * fPtr;	//file su cui scrivere

        /* 
         * Open file in w (write) mode. 
         * "data/file1.txt" is complete path to create file
         */
	if (how == 'w')
      		fPtr = fopen(name, "w");

	else if (how == 'a')
		fPtr = fopen(name, "a");
	
	else {
		printf("Comando inatteso");
		return ;
	}


   	 /* fopen() return NULL if last operation was unsuccessful */
    	if(fPtr == NULL)
    	{
      	  	/* Errore di creazione/apertura file */
      	  	printf("Unable to create file.\n");
      	 	return ; 
   	}

    	/* Write data to file */
					//devo scrivere solo una porzione del buffer
	for (int i = 0; i < size; i++)
		fputc (values[i], fPtr);

    	/* Close file to save file data */
    	fclose(fPtr);


   	 /* Success message */
   	// printf("File created and saved successfully.\n\n");

}

//stampo carattere per carattere il buffer di caratteri passato come parametro
void printBuffer( char buffer[], int index) {
	
	for (int i = 0; i < index; i++)
		printf("%c", buffer[i]);
}




int main (int argc, char* argv[])
{
	//Stabiliamo connessione

	if (argc < 4) {
		printf ("\nNumero parametri errato!\n\n");
		exit(0);
	}

	char* destinazione = argv[1];
	uint16_t porta = (uint16_t)(atoi(argv[2]));
	porta = htons(porta);
	int s = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (s < 0)
	{
		printf ("\nSocket failed \n\n");
		exit(0);
	}

	char directory[MAXPATHLEN];	//directory di lavoro

	if(getcwd(directory, sizeof(directory)) == NULL) {
		printf ("\nDirectory non trovata\n\n");
		exit(0);
	}

	struct sockaddr_in saddr;
	struct in_addr ad;
	saddr.sin_family = AF_INET;
	saddr.sin_port = porta;
	int r = inet_aton(destinazione, &ad);

	if (r == -1) {
		printf ("Translation failed!\n");
		exit(0);
	}

	saddr.sin_addr = ad;
	saddr.sin_addr.s_addr = htonl(saddr.sin_addr.s_addr);
	int result = connect (s, (struct sockaddr*) &saddr, sizeof(saddr));

	if (result == -1) {
		printf ("Connection failed!\n");
		exit(0);
	}
	else {
		printf ("Connessione stabilita!\n");
	}

	//Fine connessione
	//Invio richiesta file
	
	char buffersn [150];

	struct timeval tval;
	fd_set cset;
	FD_ZERO(&cset); FD_SET(s, &cset);
	int t = 15; tval.tv_sec = t; tval.tv_usec = 0;

	int n;
	char path[MAXPATHLEN];

	for (int i = 3; i < argc; i++) {

		memset(buffersn, 0, sizeof(buffersn));	//inizializzo il buffer di richiesta
		memset(path, 0, sizeof(path));		//inizializzo il buffer del path del file da scrivere

		//costruisco il path per il file		
		strcpy(path, directory);
		strcat(path, "/");
		strcat(path, argv[i]);

		buffersn[0] = 'G';
		buffersn[1] = 'E';
		buffersn[2] = 'T';
		buffersn[3] = ' ';
		strcat(buffersn, argv[i]);

		strcat(buffersn, "\r\n");

		printf("Iterazione numero: %i.\n\nFile richiesto: %s.\n\nBuffer inviato: %s\n", i-2, path, buffersn);
	
		if ((n = select(s+1, NULL, &cset, NULL, &tval)) == -1) {
			printf("select() sulla richiesta file fallita\n\n");
			shutdown(s, SHUT_RDWR);
			close(s);
			exit(-1); 
		}

		else if (n == 0) {
			printf ("Impossibile inviare la richiesta. Procedo col prossimo file\n\n");
			shutdown(s, SHUT_RDWR);
			close(s); 
		}

		printf ("Invio richiesta al server...\n\n");
		write (s, buffersn, strlen(buffersn));

		int t = 15; tval.tv_sec = t; tval.tv_usec = 0;

		//non ho ricevuto alcuna risposta
		if ((n = select(s+1, &cset, NULL, NULL, &tval)) == -1) {
			printf("select() sulla risposta dal server fallita. Procedo con la prossima richiesta...\n\n");
			shutdown(s, SHUT_RDWR);
			close(s); 
		}

		//ho almeno un byte da leggere (+OK O -ERR)
		else if (n>0) {
			
			memset(bufferReply, 0, sizeof(bufferReply));		//inizializzo il buffer per l'intestazione server

			read(s, bufferReply, sizeof(bufferReply));

			uint32_t file_length;	//salvo la dimensione del file richiesto

			printf("Risposta ricevuta dal server!\n\n");

			//messaggio di errore. Chiudi la comunicazione
			if (strcmp(bufferReply, "-ERR\r\n") == 0) {
				printf ("File non presente sul server!\n\n");
				shutdown(s, SHUT_RDWR);
				close(s); 
				exit(0);
			}

			//ricevo  +OK e attendo per la dimensione file
			else if (strcmp(bufferReply, "+OK\r\n") == 0) {

				memset(bufferDim, 0 , sizeof(bufferDim));		//inizializzo il buffer per la dimensione file

				int t = 15; tval.tv_sec = t; tval.tv_usec = 0;

				if ((n = select(s+1, &cset, NULL, NULL, &tval)) == -1) {
					printf("select fallita per la dimensione file\n\n");
					shutdown(s, SHUT_RDWR);
					close(s); 
					break;
				}

				else if (n == 0) {
					printf("Nessuna risposta per la dimensione file\n\n");
					shutdown(s, SHUT_RDWR);
					close(s);
					break; 
				}

				read(s, bufferDim, 4);

				file_length = ntohl(*(uint32_t*)&bufferDim);

				printf ("La dimensione del file è: %i\n\n", file_length);
						
				int nBytes = 0;	//controllo numero bytes letti per capire qual e' l'ultimo pacchetto
				int end = 0;

				int bytesRicevuti = 0;
				int contoBytes = 0;

				int pacchettiTotali = (int) file_length / 1024;
				printf("Pacchetti da ricevere %d", pacchettiTotali);

				t = 15; tval.tv_sec = t; tval.tv_usec = 0;

				if ((n = select(s+1, &cset, NULL, NULL, &tval)) == -1) {
					printf("select fallita sui dati del file\n\n");
					remove(path);
					shutdown(s, SHUT_RDWR);
					close(s); 
					break;
				}

				else if (n == 0) {
					printf ("Timer scaduto per ricevere i dati\n\n");
					remove(path);
					shutdown(s, SHUT_RDWR);
					close(s); 
					break;
				}

				FILE *fp;
				fp = fopen(path, "ab");
				if(fp == NULL){
        			printf("Impossibile creare il file");    
            		exit(1);
    			}
					
				while((bytesRicevuti = read(s, bufferrc, sizeof(bufferrc))) > 0){
					printf("Pacchetti mancanti: %d\n", pacchettiTotali);
					pacchettiTotali -= 1;
					printf("Bytes ricevuti %d\n", bytesRicevuti);    
        			// recvBuff[n] = 0;
        			fwrite(bufferrc, 1,bytesRicevuti,fp);
        			// printf("%s \n", recvBuff);
					contoBytes += bytesRicevuti;
				}

				fclose(fp);
				

				/* //aspetto i dati dal server
				do {

					t = 15; tval.tv_sec = t; tval.tv_usec = 0;

					if ((n = select(s+1, &cset, NULL, NULL, &tval)) == -1) {
						printf("select fallita sui dati del file\n\n");
						remove(path);
						shutdown(s, SHUT_RDWR);
						close(s); 
						break;
					}

					else if (n == 0) {
						printf ("Timer scaduto per ricevere i dati\n\n");
						remove(path);
						shutdown(s, SHUT_RDWR);
						close(s); 
						break;
					}

					read(s, bufferrc, sizeof(bufferrc));
							
					//non e' l'ultimo pacchetto
					if ((file_length - nBytes) > 1024) {
							
						if (nBytes != 0) 				              //non il primo pacchetto: append
							writeFile(path, bufferrc, sizeof(bufferrc), 'a');
						else						              //primo pacchetto: write
							writeFile(path, bufferrc, sizeof(bufferrc), 'w');

						nBytes += 1024;	

						printf("Pacchetto numero %i. Contenuto: \n\n", nBytes);
						//printBuffer(bufferrc, strlen(bufferrc));
						printf("\n\n");			
					}
	
					//ultimo pacchetto
					else {
						int left = file_length - nBytes;		//numero bytes restanti

						if (nBytes != 0) 				              //non il primo pacchetto: append
							writeFile(path, bufferrc, left, 'a');
						else						              //primo pacchetto: write
							writeFile(path, bufferrc, left, 'w');

						end = 1;	//chiudo il ciclo do-while

						nBytes += left;

						printf("Pacchetto numero %i. Contenuto: \n\n", nBytes);
						//printBuffer(bufferrc, left);
						printf("\n\n");
					}
				
				}while(end == 0); */
			}
			
			//messaggio inatteso (ne' +OK\r\n e ne' -ERR\r\n)
			else {
				printf ("Messaggio non riconosciuto. Procedo col prossimo file...\n\n");
			}
		}

		//timer scaduto per +OK\r\n o -ERR\r\n
		else
			printf("Nessuna risposta dal server.\n\n");
	}

	char bufferEnd[6];

	//GET \r\n. Termine comunicazione
	bufferEnd[0] = 'G';
	bufferEnd[1] = 'E';
	bufferEnd[2] = 'T';
	bufferEnd[3] = ' ';
	strcat(bufferEnd, "\r\n");

	write (s, bufferEnd, sizeof(bufferEnd));

	shutdown(s, SHUT_RDWR);
	close(s); 
	return 0;
}

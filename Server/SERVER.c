#include <stdlib.h> // getenv()
#include <sys/types.h> //OK
#include <sys/socket.h> //OK
#include <sys/time.h> // timeval
#include <sys/select.h> //OK
#include <sys/wait.h>
#include <netinet/in.h> //OK
#include <arpa/inet.h> // inet_aton()
#include <sys/un.h> // unix sockets
#include <netdb.h>
#include <errno.h>
#include <unistd.h> //OK
#include <string.h> //OK
#include <stdio.h> //OK
#include <inttypes.h> // SCNu16
#include <string.h> //OK
#include "errlib.h"
#include "sockwrap.h"
#include <time.h>
#include <sys/dir.h> //OK
#include <sys/param.h> //OK
#include <dirent.h> //OK
#include <sys/stat.h> //OK


char files[200][30];	//matrice di caratteri contenente il nome dei file della directory corrente.
char buffersn[1024];	//buffer di invio dei dati dei file in caso di successo

//salva i nomi dei file nella matrice files[][] a partire dalla directory passata come parametro.
void savedir(char *dir, int depth)
{

    DIR *dp;
    int pos;

    struct dirent *entry;
    struct stat statbuf;

    if((dp = opendir(dir)) == NULL) {
        fprintf(stderr,"cannot open directory: %s\n", dir);
        return;
    }

    chdir(dir);
    while((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name,&statbuf);
        if(S_ISDIR(statbuf.st_mode)) {
            /* Found a directory, but ignore . and .. */
            if(strcmp(".",entry->d_name) == 0 ||
                strcmp("..",entry->d_name) == 0)
                continue;
            //printf("%*s%s/\n",depth,"",entry->d_name);
	    


	    
	    for (int i = 0; i < 200; i++) {
		if(files[i][0] == '0') {
			pos = i;
			break;
		}
	    }
	    strcat(files[pos], (const char*)&depth);
	    strcat(files[pos], "");
	    strcpy(files[pos], entry->d_name);
	    //strcpy(files[pos], "\n");




            /* Recurse at a new indent level */
            savedir(entry->d_name,depth+4);
        }
        else {
		//printf("%*s%s\n",depth,"",entry->d_name);
		for (int i = 0; i < 200; i++) {
		if(files[i][0] == '0') {
			pos = i;
			break;
		}
	    }
	    strcat(files[pos], (const char*)&depth);
	    strcat(files[pos], "");
	    strcpy(files[pos], entry->d_name);
	    //strcpy(files[pos], "\n");
	}

    }
    chdir("..");
    closedir(dp);
}





//stampo i file presenti nella directory.
void printFiles() {
	for (int i = 0; i < 200; i++) {
		if (files[i][0] == '0')
			break;
		for (int j = 0; j < 30; j++) {
			if (files[i][j] == '0')
				break;
			printf("%c", files[i][j]);

		}
		printf("%s", "\n");
	}
}


//stampo carattere per carattere il buffer di caratteri passato come parametro
void printBuffer( char buffer[], int index) {
	
	for (int i = 0; i < index; i++)
		printf("%c", buffer[i]);

}


//svuoto la lista dei file presenti nella directory.
void clearFiles() {
	    for (int i = 0; i < 200; i++)
		for (int j = 0; j < 30; j++)
			files[i][j] = '0';
}



//carico il buffer con i dati del file. Ritorna il numero di byte effettivamente caricati sul buffer.
int loadBuffer(int start, char *filename, int file_size) {	//start: indice di partenza (se ho già inviato qualcosa). *filename: nome del file da leggere(path completo). buffer[]: buffer di riempimento
										//file_size: dimensione file

	if (start > file_size)
		return 0;

   	FILE *fp;	//file su cui iterare
	int index = 0;	//indice inziale pari a 0. Serve per arrivare al carattere da leggere in posizione start
	int buf_index = 0;  //indice iniziale del buffer

	fp = fopen(filename, "r"); // read mode
 
   	if (fp == NULL) {
     		perror("Error while opening the file.\n");
     	 	return -1;
   	}
	
	while (index < start) {
		fgetc(fp);
		index++;	
	}

	char c;
	
   	while(index < file_size && buf_index < sizeof(buffersn)) {		//o arrivo alla fine del file, oppure ho riempito il buffer
		c = getc(fp);
      		buffersn[buf_index] = c;
		buf_index++;
		index++;	
	}

	/*if (index == file_size) 
		buffersn[buf_index] = '\0';*/

	fclose(fp);

	return buf_index;		//torno il numero di byte effettivamente letti
}

//Calcolo la dimensione (in Bytes) del file richiesto (path completo)
uint32_t fileSize(char *filename) {
	char ch;
   	FILE *fp;
	uint32_t nBytes = 0;
 
   	fp = fopen(filename, "r"); // read mode
 
   	if (fp == NULL) {
      		perror("Error while opening the file.\n");
     	 	return -1;
   	}
 
   	while((ch = fgetc(fp)) != EOF)
      		nBytes++;
 
   	fclose(fp);
   	return nBytes;
}



/*
//metodo per verificare l'esistenza di un file. Ritorna 0 se OK, -1 altrimenti.
int findFile(char *test) {
	for (int i = 0; i < 200; i++) {
		if (files[i][0] == '0')
			return -1;
		if (strlen(test) == strlen(files[i])) {
			for (int j = 0; j < strlen(test); j++)
				if (files[i][j] != 

		}
	}
	return -1;
}
*/





int main (int argc, char* argv[])
{

	int tuttiBytes = 0;
	int result; 		//risultato della listen
	int backlog = 5; 	//lunghezza max richieste pendenti
	uint32_t s1; 		//socket verso il client
	char bufferrc [150];	//buffer di ricezione delle richieste del client
	char substring[150];	//nome del file
	char path[MAXPATHLEN];  //path completo del file
	char OK[5];		//contiene l'intestazione "+OK\r\n"
	char directory[MAXPATHLEN];	//directory di lavoro

	//controllo numero parametri 
	if (argc != 2) {
		printf ("\nNumero parametri errato!\n\n");
		exit(0);
	}

	//numero di porta e controllo sulla validita' del valore
	int porta = (atoi(argv[1]));
	if (porta < 1023 || porta > 65535) {
		printf("\nPorta inesistente!\n\n");
		exit(0);
	}

        if(getcwd(directory, sizeof(directory)) == NULL) {
		printf ("\nDirectory non trovata\n\n");
		exit(0);
	}

	//svuoto e riempio la struttura per contenere i nomi dei file della directory
	clearFiles();
	savedir(directory, 0);
	printFiles();

	//passaggio al network byte order
	porta = htons((uint16_t)porta);

	//creazione socket e controllo
	int s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s < 0) {
		printf ("\nSocket failed \n\n");
		exit(0);
	}

	//creazione struct per la connessione
	struct sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = porta;
	saddr.sin_addr.s_addr = INADDR_ANY;
	
	int i = bind(s, (struct sockaddr *) &saddr, sizeof(saddr));
	if (i == -1) {
		printf("\nErrore della bind\n\n");
		exit(0);
	}

	result = listen(s, backlog);
	if (result == -1) {
		printf("\nErrore della listen\n\n");
		exit(0);
	}

	char buffererr [6];	//buffer di invio verso il client in caso di errore (file non trovato o messaggio malformato: -ERRCRLF)
	buffererr[0] = '-';
	buffererr[1] = 'E';
	buffererr[2] = 'R';
	buffererr[3] = 'R';
	strcat(buffererr, "\r\n");
	
	OK[0] = '+';		//stringa di intestazione completa
	OK[1] = 'O';	
	OK[2] = 'K';
	strcat(OK, "\r\n");

	//riempio preventivamente il buffer di invio nel caso standard

	for (;;) {

		/*strcpy(bufferrc, "");
		strcpy(buffersn, "");
		strcpy(path, ""); */

		printf("\nServer in attesa di connessione...\n\n");

		struct sockaddr_in caddr;
		socklen_t len = sizeof(caddr);
		s1 = accept(s, (struct sockaddr *) &caddr, &len);

		if (s1 == -1) {
			printf("Errore nella creazione del socket s1\n\n");
			exit(0);
		}		

		struct timeval tval;
		fd_set cset;
		FD_ZERO(&cset); FD_SET(s1, &cset);
		int t = 15; tval.tv_sec = t; tval.tv_usec = 0;
		
		for (;;){
			
			tuttiBytes = 0;
			int n;

			memset(bufferrc, 0, sizeof(bufferrc)); //svuoto buffer ricezione //TODO qui è 0 non '\0' perchè devo impostare tutto il buffer "bufferrc" con 0  non con il carattere di terminazione delle stringe ('\0')
			
			/*strcpy(bufferrc, "");
			strcpy(buffersn, "");
			strcpy(path, "");*/

			if ((n = select(s1+1, &cset, NULL, NULL, &tval)) == -1) {
				printf("select() fallita\n");
				shutdown(s1, SHUT_RDWR);
				close(s1); 
				break;
			}

			else if (n>0) {
				
				//strcpy(substring, "");

				printf("Messaggio ricevuto dal client!\n\n");

				read (s1, bufferrc, sizeof(bufferrc));

				if (bufferrc[0] == 'G' && bufferrc[1] == 'E' && bufferrc[2] == 'T' 
				&& bufferrc[3] == ' ' && bufferrc[strlen(bufferrc) -1] == '\n' 
				&& bufferrc[strlen(bufferrc) -2] == '\r') {

					if (strlen(bufferrc) == 6) {
						printf("Client servito. Attendo il prossimo...\n\n");
						break;
					}

					int c = 0;

					memset(substring, 0, sizeof(substring)); // svuoto sottostringa nome file

					//estrapolazione del nome del file
					while (c < (strlen(bufferrc) - 6)) {
      						substring[c] = bufferrc[c+4];
      						c++;
   					}				

					//verifico la presenza del file nella directory
					int found = -1;
					int try = 0;
					
					//controllo esistenza del file nella directory
					for (int i = 0; i < 200; i++) {
						try = 0;
						if (files[i][0] == '0') {
							found = -1;
							break;
						}
						if (strlen(substring) == strlen(files[i])) {
							for (int j = 0; j < strlen(substring); j++)
								if (files[i][j] != substring[j]) {
									try = -1;
									break; 
								}
							if (try == 0) {
								found = 0;
								break;
							}
						}
					}

					//il file e' presente nella directory
					if (found == 0) {			//controlla che ci sia il file nella directory

						printf ("file trovato\n\n");	
						memset(path, 0, sizeof(path));	//svuoto buffer path

						struct stat info;	//prelevo le info del file per l'ultima modifica
						
						strcat(path, directory);
						strcat(path, "/");
						strcat(path, substring);
						
						stat(path, &info);		//inserisco nella struct le statistiche del file corrente

						uint32_t length = info.st_size;
						printf("La dimensione del file richiesto e: %i\n\n", length);
						uint32_t send_length = htonl(length);		//trasformo in Network Byte Order

						//procedi con l'invio del file
						//int file_size = fileSize(path);	//dimensione del file in bytes		
												
						int nPacchetto = 0;		//numero di pacchetto inviato per il file corrente
						int ok = 0;			//indica se la lettura del file è andata a buon fine
						int nBytes;			//numero di bytes letti durante l'iterazione


						t = 15; tval.tv_sec = t; tval.tv_usec = 0;	//inizializzo il timer

						if (select (s1+1, NULL, &cset, NULL, 0) > 0) 
							write (s1, OK, sizeof(OK));

						t = 15; tval.tv_sec = t; tval.tv_usec = 0;
						if (select (s1+1, NULL, &cset, NULL, 0) > 0) 
							write (s1, &(send_length), 4);

						FILE *fp = fopen(path,	"rb");
        				if(fp==NULL){
             				printf("Errore durante l'apertura del file");    
        				}

						unsigned char fileBuffer[1024] = {0};
						int contoBytes = 0;

						while(contoBytes < length){
							int bytesLetti = fread(fileBuffer,1,1024,fp);
        					contoBytes += bytesLetti;
							write(s1, fileBuffer, bytesLetti);
							printf("Bytes trasferiti: %d di %d\n", contoBytes, bytesLetti);
            				/* if(bytesLetti > 0){
                				write(connfd, buff, nread);
            				} */


							if (ferror(fp)){
                    			printf("Error reading\n");
                				break;
            				}  
						}

						fclose(fp);
            			


	
						/* //fintanto che c'è almeno un byte da inviare, continuo a riempire il buffer e ad inviare	
						while (tuttiBytes < length) {

							nBytes = loadBuffer(nPacchetto*1024, path, length);
			
							printf ("Pacchetto numero: %i. Numero bytes letti: %i.\n\n", nPacchetto++, nBytes); 

							if (nBytes != 0 && nBytes != -1) {			//c'è almeno un carattere da leggere e non c'è stato errore
								printBuffer(buffersn, nBytes);
								printf("\n\n");
								tuttiBytes += nBytes;

								t = 15; tval.tv_sec = t; tval.tv_usec = 0;

								if ((n =select (s1+1, NULL, &cset, NULL, 0)) > 0) 
									write(s1, buffersn, strlen(buffersn));

								else if (n == 0) {
									printf ("Timer in scrittura del file scaduto!\n\n");
									ok = -1;
									break;
								}
			
								else {
									printf ("Problema generico\n\n");
									ok = -1;
									break;
								}	
							}

							else if (nBytes == -1) {		//errore nell'apertura del file
								ok = -1;
								break;
							}
							
							else {
								printf("Nessun byte letto!\n\n");
								break;
							}
							printf("DB --> VALORE DI TUTTIBYTES %d\n", tuttiBytes);
							printf("DB --> VALORE DI nBYTES %d\n", nBytes);
						} */
						printf("DB --> SONO USCITO DAL WHILE");
						//c'è stato un errore nell'apertura del file. Chiudi la comunicazione col client
						if (ok == -1) {
							printf("Chiusura della comunicazione col client...\n\n");
							shutdown(s1, SHUT_RDWR);
							close(s1); 
							break;
						}
						
						//l'invio è andato a buon fine
						else {
							printf("File inviato correttamente al client. Numero bytes: %i.\n\n", tuttiBytes);
							uint32_t timestamp = htonl(info.st_mtime);
							send (s1, (char*)&timestamp, 4, 0);
							shutdown(s1, SHUT_RDWR);
							close(s1);				
						}	
					}
					
					//il file non e' presente nella directory
					else {
						printf("file non  presente nella directory\n\n");
						//messaggio di errore			
	
						send (s1, buffererr, strlen(buffererr), 0);
	
						shutdown(s1, SHUT_RDWR);
						close(s1); 
						break;
					}
				}
	
				//messaggio inatteso
				else {
					printf("messaggio malformato\n\n");
					//messaggio di errore
					
					send (s1, buffererr, strlen(buffererr), 0);
	
					shutdown(s1, SHUT_RDWR);
					close(s1); 
					break;
				}	 
			}
		
			//timer scaduto
			else {
				printf("Timer scaduto\n\n");
				break;
			}	
		}
	}	
	return 0;
}	

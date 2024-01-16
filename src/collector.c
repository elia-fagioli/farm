/*
* Elia Fagioli
* Matricola 580115
* progetto SOL a.a. 2022/23
*/
#include "../includes/strutture.h"

//array che mantiene i dati ricevuti dai thread worker
socket_args *results = NULL;
//dimensione dell'array
int results_size = 0;

//unlink del socket file
void cleanup();
//funzione di stampa di results
void stampa_results();
//funzione di ordinamento passata a qsort prima della stampa.
int long_compare(const void *a, const void *b);
//maschera i segnali sul collector
void mask_signals();

int main(void){
    mask_signals();
    cleanup();
    atexit(cleanup);

    int server_fd, client_fd;

    //Crea connessione socket
    server_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Errore nel creare connessione socket Collector\n");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_un server_addr;
    // Imposta il server address
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_LOCAL;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    // Bind del fd sull'indirizzo
    if (bind(server_fd, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_un)) == -1) {
        perror("Errore bind socket Collector\n");
        exit(EXIT_FAILURE);
    }

    // Listen in attesa di connessione con masterWorker
    if (listen(server_fd, 1) == -1) {
        perror("Error listen socket Collector\n");
        exit(EXIT_FAILURE);
    }

    //Accetta una connessione, (BLOCCANTE)
    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd == -1) {
        perror("Errore accept collector\n");
        exit(EXIT_FAILURE);
    }
    //Connessione avvenuta
    int n;
    socket_args s_args;
    for(;;) {
    	//operazione bloccante in attesa di dati dalla socket.
    	n = read(client_fd, &s_args, sizeof(socket_args));

        if (n == -1) {
            fprintf(stderr, "Errore read del collector socket, errno: %d\n", errno);
            close(client_fd);
            break;
        } else if (n == 0) {
            //Ricevuta chiusura di connessione dal client
            close(client_fd);
            break;
        } else {
            //ricevuta richiesta di stampa SIGUSR1
            if (strcmp(s_args.file_path, "STAMPA RESULTS") == 0) {
                // Invio la risposta al client
                char ack[256] = "ack";
                write(client_fd, ack, strlen(ack));
                stampa_results();
            } else {
                //gestisco lettura e allocazione dinamica elementi in results.
                results = realloc(results, (results_size + 1) * sizeof(socket_args));
                results[results_size].result = s_args.result;
                strcpy(results[results_size].file_path, s_args.file_path);
                results_size++;

                // Invio la risposta al client
                const char* ack = "ack";
		write(client_fd, ack, strlen(ack));
            }
        }
    }
    stampa_results();
    free(results);
    close(server_fd);
    unlink(SOCKET_PATH);
    return 0;
}

void cleanup() {
    unlink(SOCKET_PATH);
}

void stampa_results(){
    int i;
    qsort(results, results_size, sizeof(socket_args), long_compare);
    for(i=0;i<results_size;i++){
        printf("%ld %s\n", results[i].result, results[i].file_path);
    }
}

int long_compare(const void *a, const void *b){
    socket_args *arg1 = (socket_args *)a;
    socket_args *arg2 = (socket_args *)b;
    if(arg1->result < arg2->result) return -1;
    else if(arg1->result > arg2->result) return 1;
    else return 0;
}

void mask_signals() {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGPIPE);
    sigaddset(&mask, SIGUSR1);

    if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0) {
        perror("Errore nel mascherare i segnali\n");
    }
}

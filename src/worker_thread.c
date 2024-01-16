/*
* Elia Fagioli
* Matricola 580115
* progetto SOL a.a. 2022/23
*/
#include"../includes/strutture.h"

//esegue una pop in mutua esclusione dalla coda concorrente
char* coda_pop(Queue *q);
//apre il path passatogli ed esegue il calcolo della funzione result sul contenuto del file
long calcolo_file(char *path);
//invia sulla connessione socket il pacchetto socket_args e attende acknowledgment di avvenuta lettura come risposta
int socket_send(socket_args *args);
//funzione per mascherare i segnali sui thread
void mask_signals();
//operazione di invio su socket locale al Collector
void invio_result(char *file);

//Funzione del thread worker
void *worker_thread(void *arg){
    mask_signals();
    //parsing
    Queue *q = (Queue *) arg;

    while (true) {
        pthread_mutex_lock(&q->mutex);
        if (q->empty && flag_fine_put) {
            pthread_mutex_unlock(&q->mutex);
            break;
        }
        //flag non settato quindi attendo inserimento e signal
        while (q->empty) {
            // La coda è vuota
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 1; // timeout di 1 secondo
            pthread_cond_timedwait(&q->cond_empty, &q->mutex, &ts);
            if (q->empty && flag_fine_put) {
                // La flag è stata settata, il worker termina
                pthread_mutex_unlock(&q->mutex);
                return NULL;
            }
        }
        //if (!q->empty)
        // Prende il file dalla coda
        char *file = coda_pop(q);
        /*
         * pthread_cond_signal invece di pthread_cond_broadcast per per svegliare solo un thread alla volta.
         * In questo modo, si riduce la possibilità di risvegli ingiusti e si migliora l'efficienza del sistema.
         * */
        pthread_cond_signal(&q->cond_full);
        pthread_mutex_unlock(&q->mutex);
	    invio_result(file);
    }
    //terminazione thread
    return NULL;
}

int socket_send(socket_args *args) {
    if ((write(server_fd, args, sizeof(socket_args))) == -1) {
        perror("Errore write del worker\n");
        close(server_fd);
        return -1;
    }
    char buf[256];
    //attesa di acknowledgment con read bloccante
    memset(buf, 0, sizeof(buf));
    // Wait for a response from the server
    if ((read(server_fd, buf, sizeof(buf) - 1)) == -1) {
        perror("Errore read del worker\n");
        close(server_fd);
        return -1;
    }
    return 0;
}

long calcolo_file(char *path){
    FILE *fp;
    // apertura il file in modalità "rb"
    fp = fopen(path, "rb");
    if (fp == NULL) {
        printf("Errore nell'apertura del file %s", path);
        //correggere il return
        return -1;
    }
    // posizionamento alla fine del file
    fseek(fp, 0L, SEEK_END);
    // ottenere posizione corrente (che è la dimensione del file)
    long size = ftell(fp);
    size = size / sizeof(long); //In Unix a 64 bit un long è 8 byte mentre su 32bit è 4 byte.
    long vals[size];

    fseek(fp, 0L, SEEK_SET);
    // lettura dei valori dal file e inserimento nell'array
    int count = fread(vals, sizeof(long), size, fp);
    fclose(fp);
    if(count != size){
        //non ho letto il numero corretto di elementi del file
        printf("Errore lettura long\n");
        exit(1);
    }
    //se la lettura è andata a buon fine eseguo il calcolo di result
    long result = 0, i;
    for(i=0;i<count;i++){
        result+=(i*vals[i]);
    }
    return result;
}

char* coda_pop(Queue *q){
    Node *n = q->head;
    char *msg = n->msg;
    q->head = n->next;
    if(q->head == NULL){//cioè se era l'ultimo elemento
        q->tail = NULL;
        q->empty = true;
    }
    q->queue_size--;
    free(n);
    return msg;
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

void invio_result(char *file){
    //apertura file e calcolo
    long result = calcolo_file(file);  
    
    //invio result al Collector tramite la socket
    socket_args *s_args = malloc(sizeof(socket_args));
    memset(s_args, 0, sizeof(socket_args));
    strcpy(s_args->file_path, file);
    s_args->result = result;

    pthread_mutex_lock(&mutex_socket);
    if (socket_send(s_args) == -1) {
        perror("Errore funzione socket_send worker\n");
        pthread_mutex_unlock(&mutex_socket);
        exit(EXIT_FAILURE);
    }
    pthread_mutex_unlock(&mutex_socket);
    free(s_args);
}

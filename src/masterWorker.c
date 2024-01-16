/*
* Elia Fagioli
* Matricola 580115
* progetto SOL a.a. 2022/23
*/
#include"../includes/strutture.h"
#include "../includes/worker_thread.h"

//definiti qui in caso di modifiche ai valori di default
#define N_THREAD 4
#define Q_LEN 8
#define DELAY 0

//flag per segnalare ai worker che la funzione di inserimento è terminata
bool flag_fine_put = false;
//flag che fa terminare l'inserimento di richieste nella coda attivato dal signal handler
bool sig_received = false;
//flag ricezione segnale SIGUSR1
bool sigusr1_received = false;

//utilizzata alla ricezione di SIGUSR1 per richiedere stampa temporanea al collector.
socket_args stampa_results = { "STAMPA RESULTS", 0 };

//fd della connessione socket verso il collector (extern in strutture.h)
int server_fd;
struct sockaddr_un server_addr;
//mutex per la socket
pthread_mutex_t mutex_socket = PTHREAD_MUTEX_INITIALIZER;

//Definizione funzioni
void parse_options(int argc, char *argv[], Options *options);
void signal_handler(int sig);
int set_connection();
void coda_init(Queue *q, int size);
void path_check_to_queue(Queue *q, char **file_list, int size, int delay);
void coda_put(Queue *q, char *msg);
void scan_directory_r(const char *path, Options *options);
void print_request();

int main(int argc, char *argv[]){
    //Gestione signal
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    // Registra i gestori dei segnali utilizzando sigaction
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGPIPE, &sa, NULL);
    //genero processo figlio collector
    pid_t pid;
    pid = fork();
    if(pid == 0){//processo figlio
        if (execl("./collector", "collector", NULL) < 0) {
            perror("Errore execl collector\n");
            exit(EXIT_FAILURE);
        }
    } else if (pid > 0){//processo padre
        /*
         * utilizza la funzione di sistema connect() sul lato client.
         * Questa funzione blocca il processo client finché la connessione non è stabilita con successo
         * o viene restituito un errore.
         * In questo modo, il client si mette in attesa che il server crei la socket e si metta in ascolto
         * */
        if(set_connection()==-1){
            perror("Errore nello stabilire connessione socket al Collector.\n");
            exit(EXIT_FAILURE);
        }

        //Struct per il parsing degli argomenti
        Options options = {
            .n_workers = N_THREAD, //numero di thread da creare
            .q_size = Q_LEN, //dimensione massima coda concorrente
            .delay = DELAY, //delay tra inserimento di due elementi
            .n_file = 0, //contatore file_name
            .file_dir = NULL, //eventuale directory name opzione -d
            .file_list = NULL //elenco dei file name
        };
        parse_options(argc, argv, &options);       
        
        //inizializzo coda concorrente
        Queue q;
        coda_init(&q, options.q_size);
        //una volta che so che il collector ha creato la socket ed è in ascolto creo i workers
        pthread_t workers_t[options.n_workers];
        for (int i = 0; i < options.n_workers; i++) {
            if (pthread_create(&workers_t[i], NULL, worker_thread, &q)) {
                fprintf(stderr, "Errore creazione thread worker %d\n", i);
                exit(EXIT_FAILURE);
            }
        }
        //inserisco vari file nella coda delle richieste
        path_check_to_queue(&q, options.file_list, options.n_file, options.delay);
        //segnalo sul flag che ho finito di inserire elementi nella coda
        flag_fine_put = true;

        //attendo terminazione dei thread worker
        for (int i = 0; i < options.n_workers; i++) {
            if (pthread_join(workers_t[i], NULL)) {
                fprintf(stderr, "Errore join worker thread: %d\n", i);
                exit(EXIT_FAILURE);
            }
        }
        
        //libero file_list
        for(int i=0; i<options.n_file;i++){
            free(options.file_list[i]);
        }
        free(options.file_list);
        
        //Invio chiusura connessione socket al collector
        //posso farlo perché con il meccanismo di invio e risposta tra worker e server sono sicuro che tutti nodi sono stati inviati
        close(server_fd);
        wait(NULL);
    }
    return 0;
}

void parse_options(int argc, char *argv[], Options *options){
    int c;
    //opzioni n q d t con argomento
    while ((c = getopt (argc, argv, "n:q:d:t:")) != -1){
        switch (c){
            case 'n':
                /*specifica il numero di thread Worker del processo MasterWorker (valore di default 4)*/
                if(optarg) {
                    options->n_workers = atoi(optarg);
                    if (options->n_workers == 0 && strcmp(optarg, "0") != 0) {
                        fprintf(stderr, "L'argomento dell'opzione -n deve essere un numero intero valido.\n");
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            case 'q':
                /*specifica la lunghezza della coda concorrente tra il thread Master ed i thread Worker (valore di default 8)*/
                if(optarg) {
                    options->q_size = atoi(optarg);
                    if (options->q_size == 0 && strcmp(optarg, "0") != 0) {
                        fprintf(stderr, "L'argomento dell'opzione -q deve essere un numero intero valido.\n");
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            case 'd':
                /*specifica una directory in cui sono contenuti file binari
                 * ed eventualmente altre directory contenenti file binari;
                 * i file binari dovranno essere utilizzati come file di input per il calcolo;*/
                if (optarg) {
                    options->file_dir = (char*) malloc(strlen(optarg) + 1);
                    if (options->file_dir == NULL) {
                        fprintf(stderr, "Errore nell'allocazione di memoria per l'opzione -d\n");
                        exit(EXIT_FAILURE);
                    }
                    strcpy(options->file_dir, optarg);
                }
                break;
            case 't':
                /*specifica un tempo in millisecondi che intercorre tra l’invio di due richieste successive ai thread Worker da parte del thread Master (valore di default 0) */
                if(optarg) {
                    options->delay = atoi(optarg);
                    if (options->delay == 0 && strcmp(optarg, "0") != 0) {
                        fprintf(stderr, "L'argomento dell'opzione -t deve essere un numero intero valido.\n");
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            default:
                fprintf(stderr, "Utilizzo: %s -n <nthread> -q <qlen> -d <directory-name> -t <delay>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    //se è stata specificata una directory la dovrò aggiungere alla lista di file
    for (int i = optind; i < argc; i++) {
        // Se supera i 255 caratteri salto al prossimo (Unix massimo 255 caratteri
        if (strlen(argv[i]) > 255) {
            fprintf(stderr, "Errore: il nome del file %d supera i 255 caratteri)\n", i-optind);
            continue;
        }
        //alloco spazio per ogni elemento
        options->file_list = realloc(options->file_list, (options->n_file+1)*sizeof(char *));
        options->file_list[i - optind] = (char *) malloc(strlen(argv[i]) + 1);
        if (options->file_list[i - optind] == NULL) {
            fprintf(stderr, "Errore nell'allocazione di memoria dell'%d-esimo file name\n", i-optind);
            exit(EXIT_FAILURE);
        }
        strcpy(options->file_list[i - optind], argv[i]);
        options->n_file++;
    }
    //inserisco elementi dalla directory
    if(options->file_dir!=NULL){
        scan_directory_r(options->file_dir, options);
    }
    free(options->file_dir);
}

void coda_init(Queue *q, int size){
    q->head = NULL;
    q->tail = NULL;
    q->empty = true;
    q->max_size = size;
    q->queue_size = 0;
    q->mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    q->cond_full = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    q->cond_empty = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
}

int set_connection(){
    server_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Errore socket MasterWorker\n");
        return -1;
    }

    // Azzero i campi del server_addr
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    // Famiglia per comunicazioni locali in Unix
    server_addr.sun_family = AF_LOCAL;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    //Meccanismo per effettuare connect in attesa che il collector faccia la listen
    bool connected = false;
    int retries = 0;
    struct timeval connect_timeout;
    connect_timeout.tv_sec = 1;
    connect_timeout.tv_usec = 0;
    // imposto timeout massimo di 1 secondo
    if (setsockopt(server_fd, SOL_SOCKET, SO_SNDTIMEO, &connect_timeout, sizeof(connect_timeout)) < 0) {
        fprintf(stderr, "Errore nella configurazione del timeout di connessione");
        return -1;
    }
    while (!connected && retries < MAX_RETRIES) {
        //ritento per massimo MAX_RETRIES volte a distanza di 1 secondo
        //in attesa che il processo collector faccia la listen
        if (connect(server_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_un)) < 0) {
            retries++;
            sleep(1);
        } else {
            connected = true;//serve?
            break;
        }
        
    }
    return 0;
}

void path_check_to_queue(Queue *q, char **file_list, int size, int delay){
    struct stat info;
    //funzione che scorre la file list e verifica se file o directory
    int i;
    for(i = 0; i<size; i++){
        //se ricevuto signal apposito esco dal ciclo e interrompo inserimento richieste
        if(sig_received) break;//a questo punto fine_put si attiva, la funzione si interrompe e i worker svuotano la coda
        if(sigusr1_received){
            //invio richiesta di stampa al collector
            print_request();
            i--;
            //passo alle iterazioni successive
            continue;
        }
        if (stat(file_list[i], &info) != 0) {
            fprintf(stderr,"Errore nella lettura delle informazioni sul file/directory %s\n", file_list[i]);
            exit(EXIT_FAILURE);
        }
        if (S_ISREG(info.st_mode)) {//Il percorso specificato corrisponde ad un file
            //se la coda è piena attendo che si liberino dei posti
            pthread_mutex_lock(&q->mutex);
            while(q->queue_size >= q->max_size){
                pthread_cond_wait(&q->cond_full, &q->mutex);
            }
            coda_put(q, file_list[i]);
            pthread_cond_signal(&q->cond_empty);
            pthread_mutex_unlock(&q->mutex);
            
            //dopo inserimento esegue sleep di delay con controllo periodico di ricezione segnale
            for(int i = 0; i<delay; i++){
            	//1000 perché usleep è in microsecondi
                usleep(1000);
                if(sig_received || sigusr1_received)break;
            }
        } else {
            fprintf(stderr, "%s non corrisponde né ad un file né ad una directory.\n", file_list[i]);
        }
    }
}

void coda_put(Queue *q, char *msg){
    Node *n = malloc(sizeof(Node));
    n->msg = msg;
    n->next = NULL;
    //se è il primo elemento
    if(q->head == NULL){
        q->head = n;
        q->tail = n;
        q->empty = false;
    } else {
        q->tail->next = n;
        q->tail = n;
    }
    q->queue_size++;
}

void scan_directory_r(const char *path, Options *options) {
    //apre path
    DIR *dir = opendir(path);
    if(dir == NULL) {
        fprintf(stderr, "Errore nell'apertura della directory %s\n", path);
        exit(EXIT_FAILURE);
    }
    //struttura per ottenere le voci all'interno di dir
    struct dirent *entry;
    //legge le voci fino alla fine
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char *full_path = malloc(strlen(path) + strlen(entry->d_name) + 2);
            sprintf(full_path, "%s/%s", path, entry->d_name);

            struct stat info;
            if (stat(full_path, &info) == 0) {
                if (S_ISREG(info.st_mode)) {
                    // Aggiungi il percorso del file alla lista, alloco spazio per ogni elemento
                    options->file_list = realloc(options->file_list, (options->n_file+1)*sizeof(char *));
                    options->file_list[options->n_file] = (char *) malloc(strlen(full_path) + 1);
                    if (options->file_list[options->n_file] == NULL) {
                        fprintf(stderr, "Errore nell'allocazione di memoria dell'%d-esimo file name\n", options->n_file);
                        exit(EXIT_FAILURE);
                    }
                    strcpy(options->file_list[options->n_file], full_path);
                    options->n_file++;
                } else if (S_ISDIR(info.st_mode)) {
                    // Esegui esplorazione ricorsiva della directory
                    scan_directory_r(full_path, options);
                }
            }
            free(full_path);
        }
    }
    closedir(dir);
}

void print_request(){
    //acquisisco mutex connessione socket ed invio richiesta di stampa
    pthread_mutex_lock(&mutex_socket);
    if ((write(server_fd, &stampa_results, sizeof(socket_args))) == -1) {
        perror("Errore write SIGUSR1\n");
        close(server_fd);
        return;
    }
    char ack[256];
    //attende acknowledgment con read (bloccante)
    memset(ack, 0, sizeof(ack));
    if ((read(server_fd, ack, sizeof(ack) - 1)) == -1) {
        perror("Errore read SIGUSR1\n");
        close(server_fd);
        return;
    }
    //resetto variabile segnale
    sigusr1_received = false;
    pthread_mutex_unlock(&mutex_socket);
    return;
}

void signal_handler(int sig) {
    switch(sig){
        case SIGHUP:
        case SIGINT:
        case SIGQUIT:
        case SIGTERM:
        case SIGPIPE:
            /*
             * il processo deve completare i task eventualmente presenti nella coda dei task da elaborare,
             * non leggendo più eventuali altri file in input,
             * e quindi terminare dopo aver atteso la terminazione del processo Collector ed effettuato la
             * cancellazione del socket file.
             * */
            sig_received = true;
            break;
        case SIGUSR1:
            /* gestione segnale SIGUSR1
             * Alla ricezione del segnale SIGUSR1 il processo MasterWorker notifica il processo Collector
             * di stampare i risultati ricevuti fino a quel momento (sempre in modo ordinato)*/
	        sigusr1_received = true;
            break;
    }
}



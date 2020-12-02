#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include "parser.h"

//para compilarlo usar: gcc -Wall -Wextra mshell.c libparser_64.a -static -o x
//para ejecutarlo basta con: ./x

/* XXXXXXXXXXXXXXXXXXXX Declaracion de variables XXXXXXXXXXXXXXXXXXXX */

char buffer[1024]; //buffer
tline * lineG; //tipo dato que representa los datos extraidos de la linea de comandos
int i,j,k; //variables de iteración
int ffgg;
int status;
char *dir;
char cwd[1024];
int fd; //variable file descriptor

pid_t pid; //pid (pid_t es como un int)
pid_t *pidArray; //array de pids
int **pipeArray; //array de pipes

int MAX = 100;
int aux;

int bgCount = 0;

char** bgList;
int* bgAcabados;
int* bgMostrado;

char str[1024];
char strAux[1024];

/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */

/* XXXXXXXXXXXXXXXXXXXX Declaracion de funciones XXXXXXXXXXXXXXXXXXXX */

void prtPrompt(){ //funcion que muestra por pantalla el prompt
    printf("msh> ");
}

char* returnMandatoArg(tline * lineAux) {
    strcpy(str,"");
    if (lineAux != NULL) {
        for (i = 0; i < lineAux->ncommands; i++) {
            for (j = 0; j < lineAux->commands[i].argc; j++) {
                strcpy(strAux,lineAux->commands[i].argv[j]);
                strcat(str,strAux);
                strcat(str," ");
            }
            if (i != lineAux->ncommands - 1) {
                strcat(str, "| ");
            }
        }
        if (lineAux->background) {
            strcat(str,"&");
        }
        strcat(str,"\n");
    }

    return str;
}

void manejador(){
    waitpid(pid,&status,__WNOTHREAD);
}

void mandatos(tline * line){

    if (line->ncommands == 0) {
        //cuando la linea de entrada esta vacia
        //simplemente avanza sin hacer nada y acaba poniendo el prompt de nuevo a la espera de un nuevo mandato
    }
    else if ((strcmp(line->commands[0].argv[0], "cd") == 0) && (line->ncommands == 1)) { //si la linea de comando tiene el mandato cd
        if ((line->commands[0].argc) > 2) { //comprobamos que el numero de parametros de cd sea correcto
            printf("cd: demasiados argumentos\n");
        }
        if (line->commands[0].argc == 1) { //cd sin arguementos que nos lleva directamente a home
            dir = getenv("HOME");
            if (dir == NULL) {
                fprintf(stderr, "No existe la variable $HOME\n");
            }
        } else {
            dir = line->commands[0].argv[1]; //cd con un argumento que nos lleva a ese argumento en concreto
        }

        if (chdir(dir) != 0) { //se comprobar si es un directorio y si no lo es sale un error
            fprintf(stderr, "cd: %s: No existe el archivo o el directorio \n", dir);
        } else {
            getcwd(cwd, sizeof(cwd));
            printf("%s\n", cwd); //escribe la ruta absoluta del directorio actual
        }
    }
    else if ((strcmp(line->commands[0].argv[0], "jobs") == 0) && (line->ncommands == 1)) { //si la linea de comando tiene el mandato jobs
        for(k=1; k<=bgCount; k++){
            if(bgList[k] != 0 && bgMostrado[k] == 0){
                if(bgAcabados[k] == 1){
                    printf("[%d] Done       ", k);
                    printf("%s", bgList[k]);
                    bgMostrado[k] = 1; // ya ha sido mostrado
                }
                else{
                    printf("[%d] Running    ", k);
                    printf("%s", bgList[k]);
                }
            }
        }
    }
    else if ((strcmp(line->commands[0].argv[0], "fg") == 0) && (line->ncommands == 1)) { //si la linea de comando tiene el mandato fg
        if(line->commands->argc == 1){ //sin numero, cogemos el +(el ultimo)
//            mandatos(bgList[bgCount]);
        }
        else { //numero del jobs
//            ffgg= atoi(line->commands->argv[1]); me da error el atoi
            if(ffgg == 0 && ffgg > bgCount){ //TODO si no es un numero
                printf("fg: %d: no existe ese trabajo", ffgg);
            }
            else{
//                mandatos(bgList[ffgg]);
            }
        }
    }
    else if (line->ncommands == 1) { //si es solo 1 mandato
        switch (pid = fork()) {  //se hace el fork dentro del switch y es el pid con el que se van eligiendo los case
            case -1: // Error fork (pid = -1)
                fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
                exit(-1);

            case 0:  // Proceso Hijo (pid = 0)
                signal(SIGQUIT, SIG_DFL); //captura las señales sigquit y sigint y actua de forma default
                signal(SIGINT, SIG_DFL);
                if (line->redirect_input != NULL) { // <
                    fd = open(line->redirect_input, O_RDONLY); //abrimos el fichero
                    if (fd != -1) {
                        dup2(fd, 0); //redirigimos la entrada
                        close(fd); //cerramos el fd
                    } else {
                        fprintf(stderr, "Fichero: Error al abrir el fichero\n");
                        exit(-1);
                    }
                }
                if (line->redirect_output != NULL) { // >
                    fd = creat(line->redirect_output, 0666); //creamos el fichero
                    if (fd !=
                        -1) {                                   // permisos del mode 0666: R y W para user group y other
                        dup2(fd, 1); //redirigimos la salida
                        close(fd); //cerramos el fd
                    } else {
                        fprintf(stderr, "Fichero: Error al crear el fichero\n");
                        exit(-1);
                    }
                }
                if (line->redirect_error != NULL) { // >&
                    fd = open(line->redirect_error, 0666); //creamos el fichero
                    if (fd !=
                        -1) {                                   // permisos del mode 0666: R y W para user group y other
                        dup2(fd, 2); //redirigimos el error
                        close(fd); //cerramos el fd
                    } else {
                        fprintf(stderr, "Fichero: Error al crear el fichero\n");
                        exit(-1);
                    }
                }

                //primer argumento es la ruta del comando, el segundo un array con los prametros
                execv(line->commands[0].filename, line->commands[0].argv);
                //si el comando anterior falla llegara aqui y saltará el comando no encontrado y luego el exit
                fprintf(stderr, "%s: No se encuentra el mandato\n", line->commands[0].argv[0]);
                exit(1);

            default: // Proceso Padre. (pid>0)
                wait(&status); // espera al hijo
                break; //sale del case

        }
    }
    else if (line->ncommands > 1) { //si mas de 1 mandato

        pipeArray = (int **) malloc(
                (line->ncommands - 1) * sizeof(int *)); //reservamos memoria para el array de pipes
        for (i = 0; i < line->ncommands - 1; i++) {
            pipeArray[i] = (int *) malloc(sizeof(int)); //necesitamos n-1 pipes
        }
        for (aux = 0; aux < line->ncommands - 1; aux++) {
            pipe(pipeArray[aux]); //convertimos en pipes
        }

        pidArray = malloc(line->ncommands * sizeof(pid_t)); //reservamos memoria para el array de pids

        for (i = 0;
             i < line->ncommands; i++) { //realizamos un bucle con el numero de comandos introducidos (>1)
            switch (pid = fork()) { //se hace el fork dentro del switch y es el pid con el que se van eligiendo los case
                //hacemos un fork por mandato (ncommands)
                case -1:  // Error fork (pid = -1)
                    fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
                    exit(1);

                case 0:   // Proceso Hijo (pid = 0)
                    signal(SIGQUIT, SIG_DFL); //captura las señales sigquit y sigint y actua de forma default
                    signal(SIGINT, SIG_DFL);
                    if (i == 0) { //primer mandato
                        for (j = 1;
                             j < line->ncommands - 1; j++) { //cierras todos los pipes que no vayas a utilizar
                            close(pipeArray[j][0]);
                            close(pipeArray[j][1]);
                        }
                        close(pipeArray[0][0]); //cerramos el extremo de lectura (del primer pipe) ya que vamos a escribir

                        if (line->redirect_input !=
                            NULL) { // < (el primer mandato solo podrá tener redirección de entrada)
                            fd = open(line->redirect_input, O_RDONLY); //abrimos el fichero
                            if (fd != -1) {
                                dup2(fd, 0); //redirigimos la entrada
                                close(fd); //cerramos el fd

                            } else {
                                fprintf(stderr, "Fichero: Error al abrir el fichero\n");
                                exit(-1);
                            }
                        }
                        dup2(pipeArray[0][1], 1);

                    } else if (i == line->ncommands - 1) { //  ultimo mandato

                        for (j = 0; j < line->ncommands -
                                        2; j++) { //cerramos los pipes menos el ultimo que es el que usaremos
                            close(pipeArray[j][0]);
                            close(pipeArray[j][1]);
                        }
                        close(pipeArray[line->ncommands - 2][1]); //cerramos el extremo de escritura

                        if (line->redirect_output !=
                            NULL) { // > (el ultimo mandato solo podrá tener redirección de salida y de error)
                            fd = creat(line->redirect_output, 0666); //creamos el fichero
                            if (fd !=
                                -1) {                                   // permisos del mode 0666: R y W para user group y other
                                dup2(fd, 1); //redirigimos la salida
                                close(fd); //cerramos el fd
                            } else {
                                fprintf(stderr, "Fichero: Error al crear el fichero\n");
                                exit(-1);
                            }
                        }
                        if (line->redirect_error != NULL) { // >& (el ultimo mandato solo podrá tener redirección de salida y de error)
                            fd = creat(line->redirect_error, 0666); //creamos el fichero
                            if (fd !=
                                -1) {                                   // permisos del mode 0666: R y W para user group y other
                                dup2(fd, 2); //redirigimos el error
                                close(fd); //cerramos el fd
                            } else {
                                fprintf(stderr, "Fichero: Error al crear el fichero\n");
                                exit(-1);
                            }
                        }
                        dup2(pipeArray[line->ncommands - 2][0], 0);

                    } else { //mandato intermedio (el resto son intermedios sin redirección de entrada)
                        close(pipeArray[i][0]); //cerramos el extremos de lectura
                        close(pipeArray[i - 1][1]); //cerramos el extremo de escritura

                        for (aux = 0; aux < i - 1; aux++) { //cerramos los pipes que no usaremos
                            close(pipeArray[aux][0]);
                            close(pipeArray[aux][1]);
                        }
                        for (aux = i + 1;
                             aux < line->ncommands - 1; aux++) { //cerramos los pipes que no usaremos
                            close(pipeArray[aux][0]);
                            close(pipeArray[aux][1]);
                        }

                        dup2(pipeArray[i - 1][0], 0);
                        dup2(pipeArray[i][1], 1);
                    }

                    //primer argumento es la ruta del comando, el segundo un array con los prametros
                    execv(line->commands[i].filename, line->commands[i].argv);
                    //si el comando anterior falla llegara aqui y saltará el comando no encontrado y luego el exit
                    fprintf(stderr, "%s: No se encuentra el mandato\n", line->commands[i].argv[0]);
                    exit(1);

                default:  // Proceso Padre. (pid>0)
                    pidArray[i] = pid; //guarda los pids en un array
                    break; //break para salir del case
            }
        }
        for (aux = 0; aux < line->ncommands - 1; aux++) {
            close(pipeArray[aux][0]);
            close(pipeArray[aux][1]);
        }

        for (aux = 0; aux < line->ncommands; aux++) {
            waitpid(pidArray[aux], NULL, 0); //waits
        }

        for (aux = 0; aux < line->ncommands - 1; aux++) //liberamos primero cada uno de los pipes
            free(pipeArray[aux]);
        free(pipeArray);    //y ahora liberamos el array de pipes
        //lo hemos hecho de esta forma porque creemos que por cada malloc hay que realizar un free

        free(pidArray); //liberamos el array de pids
    }
}

int main(void) { //funcion main donde se ejecutara tod0 el programa y se escribirá la mayor parte del codigo

    /* XXXXXXXXXXXXXXXXXXXXXX PROGRAMA PRINCIPAL XXXXXXXXXXXXXXXXXXXXXXXX */

    signal(SIGQUIT, SIG_IGN); //captura las señales sigquit y sigint y las ignora
    signal(SIGINT, SIG_IGN);

    prtPrompt(); //llama a la funcion que imprime el prompt

    bgList = calloc(MAX, sizeof(tline)); //inicializacion del array de mandatos
    bgAcabados = calloc(MAX, sizeof(int)); //inicializacion del array de indices de mandatos
    bgMostrado = calloc(MAX, sizeof(int)); //inicializacion del array de mandatos ya mostrados

    while (fgets(buffer, 1024, stdin)){ //bucle del programa

        signal(SIGCHLD, manejador);

//      bgAcabados[bgCount] = 1; //ya ha acabado el proceso

        for(k=1; k<=bgCount; k++){ //muestra los mandatos ya acabados
            if(bgAcabados[k] == 1 && bgList[k] != 0 && bgMostrado[k] == 0) { //
                printf("[%d] Done       ", k);
                printf("%s", bgList[k]);
                bgMostrado[k] = 1; // ya ha sido mostrado
            }
        }

        lineG = tokenize(buffer); //tokenize del buffer (funcion proporcinada por el enunciado)

        if(lineG->background==1){ //cuando se ejecuta en background

            switch (pid = fork()) {
                case -1: // Error fork (pid = -1)
                    fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
                    exit(-1);

                case 0: // Proceso Hijo (pid = 0) -> ejecuta los mandatos en background
                    mandatos(lineG); //llama a la funcion que realiza los mandatos
                    exit(0);

                default: // Proceso Padre. (pid > 0) -> ejecuta el prompt
                    bgCount++;
                    printf("[%d] %d\n",bgCount, pid); //imprime el pid
                    bgList[bgCount] = strdup(returnMandatoArg(lineG));
                    break;
            }
        }
        else{ //cuando se ejecuta en foreground
            mandatos(lineG); //llama a la funcion que realiza los mandatos
            signal(SIGQUIT, SIG_IGN); //captura las señales sigquit y sigint y las ignora
            signal(SIGINT, SIG_IGN);
        }

        prtPrompt(); //llama a la funcion que imprime el prompt

    }

    /* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */

    return 0;
}

/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
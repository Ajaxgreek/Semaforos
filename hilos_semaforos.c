/*
 * Dos hilos (pthread) que cuentan del 1 al 10000.
 * Cuando un hilo llega a 4000 entra a una seccion critica y no debe ser
 * interrumpido por el otro hasta llegar a 6000. La exclusion mutua se
 * implementa con un semaforo POSIX (inicializado en 1).
 *
 * Se usa un semaforo con nombre (sem_open) para que el programa funcione
 * tanto en Linux como en macOS (macOS no soporta sem_init).
 *
 * Compilar:  gcc -Wall -o hilos_semaforos hilos_semaforos.c -lpthread
 * Ejecutar:  ./hilos_semaforos
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#define LIMITE      10000
#define INICIO_SC   4000    /* Inicio de la seccion critica */
#define FIN_SC      6000    /* Fin de la seccion critica    */
#define NOMBRE_SEM  "/sem_seccion_critica"

static sem_t *sem;          /* Semaforo binario que protege la seccion critica */

/* Pequena pausa para que los hilos se intercalen fuera de la seccion critica */
static void pausa(void)
{
    struct timespec ts = {0, 100000};   /* 100 microsegundos */
    nanosleep(&ts, NULL);
}

static void *contar(void *arg)
{
    int id = *(int *)arg;

    for (int i = 1; i <= LIMITE; i++) {

        if (i == INICIO_SC) {
            printf("Hilo %d: llego a %d, solicitando la seccion critica...\n",
                   id, INICIO_SC);
            sem_wait(sem);                       /* ENTRADA a la seccion critica */
            printf("Hilo %d: ENTRA a la seccion critica\n", id);
        }

        printf("Hilo %d: %d\n", id, i);

        if (i == FIN_SC) {
            printf("Hilo %d: SALE de la seccion critica\n", id);
            sem_post(sem);                       /* SALIDA de la seccion critica */
        }

        /* Fuera de la seccion critica el hilo puede ser interrumpido;
         * dentro (4000..5999) no se hace pausa. */
        if (i < INICIO_SC || i >= FIN_SC)
            pausa();
    }

    printf("Hilo %d: termino su cuenta.\n", id);
    return NULL;
}

int main(void)
{
    pthread_t h1, h2;
    int id1 = 1, id2 = 2;

    /* Eliminar un semaforo previo con el mismo nombre (si quedo de otra corrida) */
    sem_unlink(NOMBRE_SEM);
    sem = sem_open(NOMBRE_SEM, O_CREAT | O_EXCL, 0644, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    printf("=== Dos hilos contando de 1 a %d ===\n", LIMITE);
    printf("Seccion critica: de %d a %d (protegida con semaforo)\n\n",
           INICIO_SC, FIN_SC);

    if (pthread_create(&h1, NULL, contar, &id1) != 0 ||
        pthread_create(&h2, NULL, contar, &id2) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    pthread_join(h1, NULL);
    pthread_join(h2, NULL);

    sem_close(sem);
    sem_unlink(NOMBRE_SEM);

    printf("\n=== Fin del programa ===\n");
    return 0;
}

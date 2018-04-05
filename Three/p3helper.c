/*
 * Tom Paulus
 *
 * p3helper.c
 * Program 3 assignment
 * CS570 - Dr. Carroll
 * SDSU
 * Spring 2018
 *
 * This is the ONLY file you are allowed to change. (In fact, the other
 * files should be symbolic links to
 *   ~cs570/Three/p3main.c
 *   ~cs570/Three/p3robot.c
 *   ~cs570/Three/p3.h
 *   ~cs570/Three/makefile
 *   ~cs570/Three/CHK.h
 * The file ~cs570/Three/createlinks is a script that will create these for you.
 */

#include "p3.h"
#include "CHK.h"
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>

#define TRUE 1
#define FALSE 0

#define COUNTFILE_NAME "countfile"

//#define DEBUG

/* You may put declarations/definitions here.
 * In particular, you will probably want access to information
 * about the job (for details see the assignment and the documentation
 * in p3robot.c):
 */
extern int nrRobots;
extern int seed;
extern int width;
extern int quota;

int fd;
sem_t *pmutx; /* semaphore guarding access to shared data */
char semaphoreMutx[SEMNAMESIZE];

void finish(void);

/* General documentation for the following functions is in p3.h
 * Here you supply the code, and internal documentation:
 */
void initStudentStuff(void) {
    int count = 0;

    sprintf(semaphoreMutx, "/%s%ldmutx", COURSEID, (long) getuid());

    if (SEM_FAILED == (pmutx = sem_open(semaphoreMutx, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0))) {
        // Semaphore Already Exists
#ifdef DEBUG
        printf("\n%d: Count File already exists. Opening necessary resources.\n", getpid());
#endif

        pmutx = sem_open(semaphoreMutx, O_RDWR);
        CHK(sem_wait(pmutx));
        CHK(fd = open(COUNTFILE_NAME, O_RDWR));
        CHK(sem_post(pmutx));
    } else {
        // First Run
#ifdef DEBUG
        printf("\n\n%d: Count file does not exist yet. Creating count & semaphore file\n", getpid());
#endif

        CHK(fd = open(COUNTFILE_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR));
        CHK(lseek(fd, 0, SEEK_SET));
        assert(sizeof(count) == write(fd, &count, sizeof(count)));
        CHK(sem_post(pmutx));
    }

#ifdef DEBUG
    printf("%d: Init Completed!\n", getpid());
#endif
}

void placeWidget(int n) {
    int count;
    int last = FALSE;

    CHK(sem_wait(pmutx)); /* request access to critical section */

    /* begin critical section -- read count, increment count, write count */
    CHK(lseek(fd, 0, SEEK_SET));
    assert(sizeof(count) == read(fd, &count, sizeof(count)));

    // Place Widget
    printeger(n);

    if (count % width == width - 1) {
        // End of Line
        printf("N\n");
    }
    if (count == nrRobots * quota - 1) {
        // Last Widget
        last = TRUE;
#ifdef DEBUG
        printf("\n%d: Placed last Widget - Setting Flag\n", getpid());
#endif
    }

    count++;
    CHK(lseek(fd, 0, SEEK_SET));
    assert(sizeof(count) == write(fd, &count, sizeof(count)));
    fflush(stdout);
    /* end critical section */

    CHK(sem_post(pmutx)); /* release critical section */

#ifdef DEBUG
    printf("%d: Released Critical\n", getpid());
#endif

    if (last) finish();
    fflush(stdout);
}

/* If you feel the need to create any additional functions, please
 * write them below here, with appropriate documentation:
 */

/*
 * Clean up after the last widget has been placed. This includes closing and removing the count file, and closing the
 * Semaphore.
 */
void finish() {
#ifdef DEBUG
    printf("Cleaning Up...");
#endif

    printf("F\n");
    CHK(close(fd));
    CHK(unlink(COUNTFILE_NAME));
    CHK(sem_close(pmutx));
    CHK(sem_unlink(semaphoreMutx));

#ifdef DEBUG
    printf("    All Done.");
#endif
}
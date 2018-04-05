/* semex.c  (Adapted from an example created by Professor Vernor Vinge)
 * Program purpose: Illustrates multiple processes using POSIX named semaphores
     to guard access to a shared file that represents an integer counter.
 * Input parameters: command line parameter is the total number of
     processes participating in the count (including the parent).
     If this number is too large, the OS will refuse to create all
     the processes and the program will fail (probably with some
     kind of "resource unavailable" message from the Operating System).
 * Output parameters: at the end of the run, the parent process prints out
     the accumulated value in the shared counter file.  If this is not the
     same as the number of processes, then there is a bug in this example!
 * Side effects:
     o During execution, a file named countfile is created in the
       current directory (DESTROYING any pre-existing file with that
       name). If the program terminates normally, it will remove this
       file. If the program terminates abnormally (for example, via
       control-C), then that file may still exist.
     o During execution, two POSIX named semaphores are created in a virtual
       filesystem under /dev/shm (SHared Memory for Linux; Solaris used /tmp).
       For instance, a semaphore named "/foo" would have an entry called
       "/dev/shm/sem.foo".  Thus, there is the possibility of name clashes!
       The semaphores below are customized with somewhat unique names
       (incorporating the pid in the name).
       If the program terminates normally, it will remove these "files".
       If the program terminates abnormally (for example, via
       control-C), these files may still exist and will cause name clashes
       on subsequent runs (until the OS periodically cleans out /dev/shm),
       manifesting in a segmentation fault.  Fix the problem with:
       rm /dev/shm/sem.*
 * Routines called (the most significant ones):
     lseek read write open close
     sem_open sem_post sem_wait sem_close sem_unlink
     CHK (a macro available from edoras.sdsu.edu:~cs570/CHK.h)
     'man sem_open' gives link library information. The
     following line should serve to compile the program on edoras:
               gcc -g semex.c -pthread -o semex
*/
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include "CHK.h"
#define COURSEID "570"
#define SEMNAMESIZE 30

int main  (int argc, char *argv[]) {
  int i, fd;
  pid_t mainpid, childpid;
  int nproc; /* total number of participating processes */
  sem_t *pmutx; /* semaphore guarding access to shared data */
  sem_t *pdone; /* semaphore used by main to wait till everyone done */
  int count; /* local copy of counter */
  char semaphoreMutx[SEMNAMESIZE];
  char semaphoreDone[SEMNAMESIZE];
  /* Try to make a "name" for the semaphore that will not clash.
     See man sem_open, and the commentary under "Side effects" at the
     beginning of this file. */
  sprintf(semaphoreMutx,"%s%ldmutx",COURSEID,(long)getuid());
  sprintf(semaphoreDone,"%s%lddone",COURSEID,(long)getuid());

  if ( (argc != 2) || ((nproc = atoi(argv[1])) <= 0) ) {
    fprintf (stderr, "Usage: %s number_of_processes\n", argv[0]);
    exit(1);
  }

  /* create and initialize the count file: */
  CHK(fd = open("countfile",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR));
  count = 0;
  CHK(lseek(fd,0,SEEK_SET));
  assert(sizeof(count) == write(fd,&count,sizeof(count)));

  /* create and initialize the semaphores: */
  CHK(SEM_FAILED != (pmutx = sem_open(semaphoreMutx,O_RDWR|O_CREAT|O_EXCL,S_IRUSR|S_IWUSR,1)));
  CHK(SEM_FAILED != (pdone = sem_open(semaphoreDone,O_RDWR|O_CREAT|O_EXCL,S_IRUSR|S_IWUSR,0)));
  
  CHK(mainpid = getpid());
  for (i = 1; i < nproc;  ++i) { /* create  nproc - 1  coworkers: */
    CHK(childpid = fork());
    if (0 == childpid) { /* make each child get out of this loop  */
      break;             /* or they will start making grandkids!  */
    }
  }
  srandom(getpid()); /* using random sleeps to provoke race conditions */
  printf("mainpid=%d, mypid=%d, ppid=%d\n", mainpid, getpid(), getppid());
  CHK(sem_wait(pmutx)); /* request access to critical section */

  /* begin critical section -- read count, increment count, write count */
  sleep(random()%2);
  CHK(lseek(fd,0,SEEK_SET));
  assert(sizeof(count) == read(fd,&count,sizeof(count)));
  count++;
  sleep(random()%2);
  CHK(lseek(fd,0,SEEK_SET));
  assert(sizeof(count) == write(fd,&count,sizeof(count)));
  /* end critical section */
 
  CHK(sem_post(pmutx)); /* release critical section */
  CHK(sem_post(pdone));
  /* parent process has done a read and write (just like the other children do),
     and now waits for everyone else to complete, and then prints the result: */
  if ( mainpid == getpid()) { /* the children skip the rest of the code */
    int answer;
      for (i=0;i<nproc;i++) {
        CHK(sem_wait(pdone));
      }
    CHK(lseek(fd,0,SEEK_SET));
    assert(sizeof(answer) == read(fd,&answer,sizeof(answer)));
/* use '!=' on the above line to illustrate what happens when you have a
   failed assertion; to make the code misbehave, change the == to !=
   and recompile. Don't forget to remove the /dev/shm/sem files AND ./countfile
   after a failed run (since a failed run will abort before it gets to the
   cleanup lines below) */
    printf("The final sum is %d\n",answer);
    CHK(close(fd));
    CHK(unlink("countfile"));
    CHK(sem_close(pmutx));
    CHK(sem_unlink(semaphoreMutx));
    CHK(sem_close(pdone));
    CHK(sem_unlink(semaphoreDone));
  }
  exit(0);
}

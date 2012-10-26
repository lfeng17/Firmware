/****************************************************************************
 * examples/elf/tests/signal/signal.c
 *
 *   Copyright (C) 2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/****************************************************************************
 * Definitions
 ****************************************************************************/

#define USEC_PER_MSEC 1000
#define MSEC_PER_SEC  1000
#define USEC_PER_SEC  (USEC_PER_MSEC * MSEC_PER_SEC)
#define SHORT_DELAY   (USEC_PER_SEC / 3)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int sigusr1_rcvd = 0;
static int sigusr2_rcvd = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sigusr1_sighandler
 ****************************************************************************/

/* NOTE: it is necessary for functions that are referred to by function pointers
 * pointer to be declared with global scope (at least for ARM).  Otherwise,
 * a relocation type that is not supported by ELF is generated by GCC.
 */

void sigusr1_sighandler(int signo)
{
  printf("sigusr1_sighandler: Received SIGUSR1, signo=%d\n", signo);
  sigusr1_rcvd = 1;
}

/****************************************************************************
 * Name: sigusr2_sigaction
 ***************************************************************************/

/* NOTE: it is necessary for functions that are referred to by function pointers
 *  pointer to be declared with global scope (at least for ARM).  Otherwise,
 * a relocation type that is not supported by ELF is generated by GCC.
 */

#ifdef __USE_POSIX199309
void sigusr2_sigaction(int signo, siginfo_t *siginfo, void *arg)
{
  printf("sigusr2_sigaction: Received SIGUSR2, signo=%d siginfo=%p arg=%p\n",
         signo, siginfo, arg);

#ifdef HAVE_SIGQUEUE
  if (siginfo)
    {
      printf("  si_signo  = %d\n",  siginfo->si_signo);
      printf("  si_errno  = %d\n",  siginfo->si_errno);
      printf("  si_code   = %d\n",  siginfo->si_code);
      printf("  si_pid    = %d\n",  siginfo->si_pid);
      printf("  si_uid    = %d\n",  siginfo->si_uid);
      printf("  si_status = %d\n",  siginfo->si_status);
      printf("  si_utime  = %ld\n", (long)siginfo->si_utime);
      printf("  si_stime  = %ld\n", (long)siginfo->si_stime);
      printf("  si_value  = %d\n",  siginfo->si_value.sival_int);
      printf("  si_int    = %d\n",  siginfo->si_int);
      printf("  si_ptr    = %p\n",  siginfo->si_ptr);
      printf("  si_addr   = %p\n",  siginfo->si_addr);
      printf("  si_band   = %ld\n",  siginfo->si_band);
      printf("  si_fd     = %d\n",  siginfo->si_fd);
    }
#endif
  sigusr2_rcvd = 1;
}
#else
void sigusr2_sigaction(int signo)
{
  printf("sigusr2_sigaction: Received SIGUSR2, signo=%d\n", signo);
  sigusr2_rcvd = 1;
}

#endif

/****************************************************************************
 * Name: sigusr2_sighandler
 ****************************************************************************/

static void sigusr2_sighandler(int signo)
{
  printf("sigusr2_sighandler: Received SIGUSR2, signo=%d\n", signo);
  sigusr2_rcvd = 1;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, char **argv)
{
  struct sigaction act;
  struct sigaction oact;
  void (*old_sigusr1_sighandler)(int signo);
  void (*old_sigusr2_sighandler)(int signo);
  pid_t mypid = getpid();
#if defined(__USE_POSIX199309) && defined(HAVE_SIGQUEUE)
  sigval_t sigval;
#endif
  int status;

  printf("Setting up signal handlers from pid=%d\n", mypid);

  /* Set up so that sigusr1_sighandler will respond to SIGUSR1 */

  old_sigusr1_sighandler = signal(SIGUSR1, sigusr1_sighandler);
  if (old_sigusr1_sighandler == SIG_ERR)
    {
      fprintf(stderr, "Failed to install SIGUSR1 handler, errno=%d\n",
              errno);
      exit(1);
    }

  printf("Old SIGUSR1 sighandler at %p\n", old_sigusr1_sighandler);
  printf("New SIGUSR1 sighandler at %p\n", sigusr1_sighandler);

  /* Set up so that sigusr2_sigaction will respond to SIGUSR2 */

  memset(&act, 0, sizeof(struct sigaction));
  act.sa_sigaction = sigusr2_sigaction;
  act.sa_flags     = SA_SIGINFO;

  (void)sigemptyset(&act.sa_mask);

  status = sigaction(SIGUSR2, &act, &oact);
  if (status != 0)
    {
      fprintf(stderr, "Failed to install SIGUSR2 handler, errno=%d\n",
              errno);
      exit(2);
    }

  printf("Old SIGUSR2 sighandler at %p\n", oact.sa_handler);
  printf("New SIGUSR2 sighandler at %p\n", sigusr2_sigaction);
  printf("Raising SIGUSR1 from pid=%d\n", mypid);

  fflush(stdout); usleep(SHORT_DELAY);

  /* Send SIGUSR1 to ourselves via raise() */

  status = raise(SIGUSR1);
  if (status != 0)
    {
      fprintf(stderr, "Failed to raise SIGUSR1, errno=%d\n", errno);
      exit(3);
    }

  usleep(SHORT_DELAY);
  printf("SIGUSR1 raised from pid=%d\n", mypid);

  /* Verify that we received SIGUSR1 */

  if (sigusr1_rcvd == 0)
    {
      fprintf(stderr, "SIGUSR1 not received\n");
      exit(4);
    }
  sigusr1_rcvd = 0;

  /* Send SIGUSR2 to ourselves */

  printf("Killing SIGUSR2 from pid=%d\n", mypid);
  fflush(stdout); usleep(SHORT_DELAY);

#if defined(__USE_POSIX199309) && defined(HAVE_SIGQUEUE)
  /* Send SIGUSR2 to ourselves via sigqueue() */

  sigval.sival_int = 87;
  status = sigqueue(mypid, SIGUSR2, sigval);
  if (status != 0)
    {
      fprintf(stderr, "Failed to queue SIGUSR2, errno=%d\n", errno);
      exit(5);
    }

  usleep(SHORT_DELAY);
  printf("SIGUSR2 queued from pid=%d, sigval=97\n", mypid);
#else
  /* Send SIGUSR2 to ourselves via kill() */

  status = kill(mypid, SIGUSR2);
  if (status != 0)
    {
      fprintf(stderr, "Failed to kill SIGUSR2, errno=%d\n", errno);
      exit(5);
    }

  usleep(SHORT_DELAY);
  printf("SIGUSR2 killed from pid=%d\n", mypid);
#endif
  /* Verify that SIGUSR2 was received */

  if (sigusr2_rcvd == 0)
    {
      fprintf(stderr, "SIGUSR2 not received\n");
      exit(6);
    }
  sigusr2_rcvd = 0;

  /* Remove the sigusr2_sigaction handler and replace the SIGUSR2
   * handler with sigusr2_sighandler.
   */

  printf("Resetting SIGUSR2 signal handler from pid=%d\n", mypid);

  old_sigusr2_sighandler = signal(SIGUSR2, sigusr2_sighandler);
  if (old_sigusr2_sighandler == SIG_ERR)
    {
      fprintf(stderr, "Failed to install SIGUSR2 handler, errno=%d\n",
              errno);
      exit(7);
    }

  printf("Old SIGUSR2 sighandler at %p\n", old_sigusr2_sighandler);
  printf("New SIGUSR2 sighandler at %p\n", sigusr2_sighandler);

  /* Verify that the handler that was removed was sigusr2_sigaction */

  if ((void*)old_sigusr2_sighandler != (void*)sigusr2_sigaction)
    {
      fprintf(stderr,
              "Old SIGUSR2 signhanlder (%p) is not sigusr2_sigation (%p)\n",
              old_sigusr2_sighandler, sigusr2_sigaction);
      exit(8);
    }

  /* Send SIGUSR2 to ourselves via kill() */

  printf("Killing SIGUSR2 from pid=%d\n", mypid);
  fflush(stdout); usleep(SHORT_DELAY);

  status = kill(mypid, SIGUSR2);
  if (status != 0)
    {
      fprintf(stderr, "Failed to kill SIGUSR2, errno=%d\n", errno);
      exit(9);
    }

  usleep(SHORT_DELAY);
  printf("SIGUSR2 killed from pid=%d\n", mypid);

  /* Verify that SIGUSR2 was received */

  if (sigusr2_rcvd == 0)
    {
      fprintf(stderr, "SIGUSR2 not received\n");
      exit(10);
    }
  sigusr2_rcvd = 0;

  return 0;
}

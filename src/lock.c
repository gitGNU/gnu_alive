/* lock.c - Lock file management for GNU Alive.
 *
 * Copyright (C) 2005 Jakob Eriksson <jakob!eriksson()vmlinux!org>
 * Copyright (C) 2003-2005 Joachim Nilsson <joachim!nilsson()member!fsf!org>
 * Copyright (C) 2002, 2003 Torgny Lyon <torgny()enterprise!hb!se>
 *
 * GNU Alive is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * GNU Alive is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef HAVE_FCNTL
#include <sys/fcntl.h>
#endif
#include <unistd.h>

#include "log.h"

extern int errno;
extern char *fallback_pid_files[];


/*
 * Virtualize away flock differences detected by autoconf.
 *
 * Truncating: http://www.unixguide.net/unix/faq/3.3.shtml
 * Locking: http://www.erlenstar.demon.co.uk/unix/faq_3.html#SEC35
 */

/* Functions for setting and getting a file lock ***********************/
#if defined(HAVE_FCNTL)
#define DECLARE_FLOCK(l)                        \
  struct flock (l) = { .l_whence = SEEK_SET,    \
                       .l_start  = 0,           \
                       .l_len    = 0,           \
                       .l_type   = F_WRLCK, }

static int __flock_set(int fd)
{
  DECLARE_FLOCK(lock);

  return fcntl (fd, F_SETLK, &lock);
}

static pid_t __flock_get(int fd)
{
  DECLARE_FLOCK(lock);

  if (fcntl (fd, F_GETLK, &lock) < 0)
    return -1;

  if (F_UNLCK == lock.l_type)
    return 0;

  return lock.l_pid;
}

#elif defined(HAVE_LOCKF)
static int __flock_set(fd)
{
  lseek(fd, 0, SEEK_SET);

  return lockf (fd, F_TLOCK, 0);
}

static pid_t __flock_get(int fd)
{
  if (!lockf (fd, F_TLOCK, 0))
    {
      return lockf (fd, F_ULOCK, 0);
    }

  return 0;
}

#elif defined(HAVE_FLOCK)
#define __flock_set(fd) \
  flock (fd, LOCK_EX | LOCK_NB)

static pid_t __flock_get(int fd)
{
  if (!flock(fd, LOCK_EX | LOCK_NB))
    {
      return flock(fd, LOCK_UN);
    }

  return 0;
}
#else  /* Neither fcntl(), lockf() or flock() */
#define __flock_set(fd) (0)
#define __flock_get(fd) (0)
#endif  /* HAVE_FCNTL */

/* Truncate the lockfile length ***************************************/
#if defined(F_FREESP)
static int __ftruncate(int fd)
{
  struct flock lock = init_flock();

  return fcntl (fd, F_FREESP, &lock);
}
#elif defined (HAVE_FTRUNCATE)
#define __ftruncate(fd) ftruncate (fd, (off_t) 0)
#else  /* !F_FREESP */
#define __ftruncate(fd) (0)
#endif

/*
 * flock_set
 * Lock the PID file and truncate to zero length.
 * XXX - Funny, it's not really the end of the world if we
 *       cannot truncate it.  Since we open with O_TRUNC and O_CREAT
 *       chances are good we've actually already truncated it...
 *       So why does it get such a detrimental role, what's really
 *       interesting is that we
 *             1) Write at START of file and
 *             2) Get an exclusive lock.
 */
static int flock_set(int fd)
{
  return (__flock_set(fd) || __ftruncate(fd));
}

/*
 * flock_get
 * Acquire lock and return the corresponding FILE ptr.
 */
static FILE *flock_get (int fd)
{
  int result = __flock_get(fd);

  if (result)
    {
      ERR("Failed to get lock: %s", strerror (errno));
    }

  return fdopen(fd, "r");
}


/**
 * lock_create - Create a lockfile and store a PID
 * @file: List of files.
 * @pid:  PID to store in lockfile.
 *
 * This function tries to open a lockfile from a @file list.
 * When one is opened it is locked and the given @pid is
 * stored therein.
 *
 * Returns: -1 on error 0 when PID written OK.
 */

int lock_create (char **file, pid_t pid)
{
  int fd, fallback;
  FILE *fp;

  fallback = 0;
  do
    {
      /* Open the lock file, truncate any old to zero and set permissions 644 */
      fd = open (*file, O_WRONLY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE | S_IRGRP | S_IROTH);
      if (-1 == fd)
        {
          *file = fallback_pid_files [fallback++];
          if (!*file)
            return -1;
        }
    }
  while (-1 == fd);

  if (flock_set(fd))
    {
      close (fd);
      return -1;
    }

  /* fd=>FILE* */
  fp = fdopen(fd, "w");
  if (!fp)
    {
      close(fd);
      return -1;
    }

  /* Verify that the entry is written. */
  if (fprintf(fp, "%ld\n", (long)pid) < 0)
    {
      fclose(fp);
      return -1;
    }

  /* Flush to backing store */
  fflush(fp);
  fclose(fp);

  return 0;
}


/**
 * lock_read - Find the pid of the daemon process.
 * @file:      PID file.
 *
 * Return pid of a daemon process which has got a write lock on the
 * lock @file.
 *
 * If the given @file cannot be opened, accessed, or found a set of
 * backup alternatives is also be searched. If all these also fail
 * the list of running processes is also searched, where the first
 * matching process ID will be returned. (So if there are more than
 * one matching daemon process only the first will be returned.)
 *
 * Returns: 0 if not locked and -1 on error, otherwise the PID.
 */

pid_t lock_read (char **file)
{
  FILE *fp;
  int fd, fallback;
  pid_t pid;

  fallback = 0;
  do
    {
      DBG(_("Looking for PID file: %s."), *file);

      fd = open (*file, O_RDONLY);
      if (-1 == fd)
        {
          *file = fallback_pid_files [fallback++];
          if (!*file)
            {
               /* This makes us depend on procps and coreutils in GNU/Linux
                * but only coreutils in GNU/Hurd.
                * Question is: Do we want this?
                * Answer:      No, do not allow daemon to start if no lockfile.
                */
/*                system ("ps --no-heading -C alive | head -1 | cut -f 1 -d ' ' >"); */

              DBG(_("Cannot find any PID file, daemon not running."));
              if (ENOENT == errno)
                return 0;
              else
                return -1;
            }
        }
    }
  while (fd == -1);

  fp = flock_get (fd);
  if (!fp)
    {
      /* XXX - Ugly code to replace fscanf() in case we fail at
       * XXX - fdopen() ... why use fdopen? Can we fix this as well?
       */
      int i;
      char str[10];

      ERR(_("Failed badly in flock_get(): %s"), strerror (errno));
      i = read (fd, str, 10);
      if (i > 0 && i < 10)
        {
          str[i] = 0;
          pid = atol (str);
        }
      close (fd);
    }
  else
    {
      fscanf (fp, "%d", (int *)&pid);
      fclose (fp);              /* Closes fd as well... */
    }

  /* Old version running?
   * XXX - Maybe prompt user to upgrade/restart/reload daemon?
   */
  if (strstr (*file, "qadsl"))
    {
      ERR(_("Old qadsl version running: %s, with PID = %d"), *file, pid);
    }
  else
    {
      DBG(_("Found PID file %s with PID = %d"), *file, pid);
    }

  return pid;
}


/**
 * lock_remove - Unlock and remove lockfile.
 * @file:        PID file.
 *
 * Unlocks and removes the PID @file.
 *
 * Returns: 0 when OK, nonzero on error.
 */

int
lock_remove (char *file)
{
  int result;

  result = unlink (file);
  if (-1 == result)
    {
      if (ENOENT == errno)
        result = 0;
    }

  return result;
}


/* Local Variables:
 * mode: C;
 * c-file-style: gnu;
 * indent-tabs-mode: nil;
 * End:
 */

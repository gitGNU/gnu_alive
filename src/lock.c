/* lock.c - Lock file management for GNU Alive.
 *
 * Copyright (C) 2005 Jakob Eriksson <jakob!eriksson()vmlinux!org>
 * Copyright (C) 2003, 2004 Joachim Nilsson <joachim!nilsson()member!fsf!org>
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
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>

#include "log.h"

extern int errno;
extern char *fallback_pid_files[];


/**
 * write_pid - Write the pid of the daemon process to file.
 * @file:      PID file.
 * @pid:       Process ID to write.
 *
 * This function is responsible for writing the @pid of the
 * auto-login daemon to the specified PID @file.
 *
 * If the specified @file does not exist a set of backup
 * alternatives should be used. If neither of these is
 * available (read-only file system?) it is OK to return error.
 *
 * Returns: A positive integer representing the PID, or
 *          -1 on error.
 */



/*
 * Virtualize away flock differences detected by autoconf
 */

#if defined(LOCK_FCNTL) || defined(HAVE_FCNTL_F_FREESP)
static struct flock init_lock()
{
  struct flock lock;

  lock.l_whence = SEEK_SET;
  lock.l_start = 0;
  lock.l_len = 0;
  lock.l_type = F_WRLCK;
}
#endif

static pid_t FLOCK_GET_FALLBACK(int fd)
{
  FILE *fp;
  pid_t pid;

  fp = fdopen(fd, "r");
  fscanf(fp, "%d", &pid);
  fclose(fp); /* XXX is fd closed now too? can we remove this line and let lock_read() close instead? */

  return pid;
}

#if defined(LOCK_FCNTL)
static int _FLOCK(int fd)
{
  struct flock lock = init_lock();

  return fcntl (fd, F_SETLK, &lock);
}

static pid_t FLOCK_GET(int fd)
{
  struct flock lock = init_lock();

  if (fcntl(fd, F_GETLK, &lock) < 0)
    return -1;
  if (F_UNLCK == lock.l_type)
    return 0;
  return lock.l_pid;
}
#elif defined(LOCK_LOCKF)
static int _FLOCK(fd)
{
  lseek(fd, 0, SEEK_SET);
  return lockf (fd, F_TLOCK, 0);
}

static pid_t FLOCK_GET(int fd)
{
  if (!flock(fd, LOCK_EX | LOCK_NB))
    {
      if (flock(fd, LOCK_UN) == -1)
        return -1;
      return 0;
    }

  return FLOCK_GET_FALLBACK(fd);
}
#elif defined(LOCK_FLOCK)
#define _FLOCK(fd) flock (fd, LOCK_EX | LOCK_NB)

static pid_t FLOCK_GET(int fd)
{
  if (!lockf(fd, F_TLOCK, 0))
    {
      if (!lockf(fd, F_ULOCK, 0))
        return -1;
      return 0;
    }

  return FLOCK_GET_FALLBACK(fd);
}

#else
#define _FLOCK(fd) (0)
#define FLOCK_GET(fd) FLOCK_GET_FALLBACK(fd)
#endif


#ifdef HAVE_FCNTL_F_FREESP
static int FREESP(int fd)
{
  struct flock lock;

  lock.l_whence = SEEK_SET;
  lock.l_start = 0;
  lock.l_len = 0;
  
  return fcntl (fd, F_FREESP, &lock);
}
#else
#define FREESP(fd) ftruncate (fd, (off_t) 0)
#endif


static int FLOCK(int fd)
{
  return _FLOCK(fd) || FREESP(fd);
}

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
          if (NULL == *file)
            return -1;
        }
    }
  while (-1 == fd);

  if (FLOCK(fd))
    {
      close (fd);
      return -1;
    }

  fp = fdopen(fd, "w");
  if (NULL == fp)
    {
      close(fd);
      return -1;
    }

  if (fprintf(fp, "%ld\n", (long) pid) < 0)
    {
      fclose(fp);
      return -1;
    }
  fflush(fp);

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

pid_t lock_read (char **file, int verbose)
{
  int fd, fallback;
  pid_t pid;

  fallback = 0;
  do
    {
      DEBUG(_("Looking for PID file: %s."), *file);

      fd = open(*file, O_RDONLY);
      if (-1 == fd)
        {
          *file = fallback_pid_files [fallback++];
          if (NULL == *file)
            {
               /* This makes us depend on procps and coreutils in GNU/Linux
                * but only coreutils in GNU/Hurd.
                * Question is: Do we want this?
                * Answer:      No, do not allow daemon to start if no lockfile.
                */
/*                system ("ps --no-heading -C alive | head -1 | cut -f 1 -d ' ' >"); */

              DEBUG(_("Cannot find any PID file, daemon not running."));
              if (ENOENT == errno)
                return 0;
              else
                return -1;
            }
        }
    }
  while (fd == -1);

  pid = FLOCK_GET(fd);
  close(fd);

  /* Old version running?
   * XXX - Maybe prompt user to upgrade/restart/reload daemon?
   */
  if (strstr (*file, "qadsl"))
    {
      ERROR(_("Old qadsl version running: %s, with PID = %d"), *file, pid);
    }
  else
    {
      DEBUG(_("Found PID file %s with PID = %d"), *file, pid);
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

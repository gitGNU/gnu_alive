/* lock.c - Lock file management for qADSL.
 * 
 * Copyright (C) 2003 Joachim Nilsson <joachim!nilsson()member!fsf!org>
 * Copyright (C) 2002, 2003 Torgny Lyon <torgny()enterprise!hb!se>
 *
 * qADSL is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * qADSL is distributed in the hope that it will be useful, but
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

int
lock_create (char **file, pid_t pid)
{
  int fd, fallback;
  FILE *fp;

#if defined(LOCK_FCNTL)
  struct flock lock;
#endif
#ifdef HAVE_FCNTL_F_FREESP
  struct flock tlock;
#endif

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
  while (fd == -1);

#if defined(LOCK_FCNTL)
  lock.l_type = F_WRLCK;
  lock.l_start = 0;
  lock.l_whence = SEEK_SET;
  lock.l_len = 0;
  if (fcntl (fd, F_SETLK, &lock) == -1) 
    {
      close(fd);
      return -1;
    }
#elif defined(LOCK_FLOCK)
  if (flock(fd,  LOCK_EX | LOCK_NB) == -1) 
    {
      close(fd);
      return -1;
    }
#elif defined(LOCK_LOCKF)
  lseek(fd, 0, SEEK_SET);
  if (lockf(fd, F_TLOCK, 0) == -1) 
    {
      close(fd);
      return -1;
    }
#endif

#ifdef HAVE_FCNTL_F_FREESP
  tlock.l_whence = SEEK_SET;
  tlock.l_start = 0;
  tlock.l_len = 0;
  if (fcntl(fd, F_FREESP, &tlock) != 0) 
    {
      close (fd);
      return -1;
    }
#else
  if (ftruncate(fd, (off_t) 0) == -1) 
    {
      close(fd);
      return -1;
    }
#endif
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

pid_t
lock_read (char **file)
{
  int fd, fallback;
  FILE *fp;
  pid_t pid;
#if defined(LOCK_FCNTL)
  struct flock lock;
#endif

  fallback = 0;
  do
    {
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
/*                system ("ps --no-heading -C qadsl | head -1 | cut -f 1 -d ' ' >"); */
               
              if (ENOENT == errno)
                return 0;
              else
                return -1;
            }
        }
    }
  while (fd == -1);

#if defined(LOCK_FCNTL)
  lock.l_type = F_WRLCK;
  lock.l_start = 0;
  lock.l_whence = SEEK_SET;
  lock.l_len = 0;
  if (fcntl(fd, F_GETLK, &lock) < 0) {
    close(fd);
    return -1;
  }
  close(fd);
  if (F_UNLCK == lock.l_type)
    return 0;
  return lock.l_pid;
#elif defined(LOCK_FLOCK)
  if(flock(fd, LOCK_EX | LOCK_NB) == 0) {
    if (flock(fd, LOCK_UN) == -1)
      return -1;
    return 0;
  }
#elif defined(LOCK_LOCKF)
  if(lockf(fd, F_TLOCK, 0) == 0) {
    if (lockf(fd, F_ULOCK, 0) == 0)
      return -1;
    return 0;
  }
#endif

  fp = fdopen(fd, "r");
  fscanf(fp, "%d", &pid);
  fclose(fp);
  close(fd);

  return pid;
}


/*
 * Unlock and remove the lockfile. Returns 0 on error, nonzero otherwise.
 */
int
lock_remove (char *file)
{
  int result;

  result = unlink (file);
  if (result == -1) 
    {
      if (errno == ENOENT)
        result = 0;
    }

  return result;
}


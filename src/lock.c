/* lock.c - Lock file management for qADSL.
 * 
 * Copyright (C) 2003 Joachim Nilsson <joachim()gnufans!org>
 * Copyright (C) 2002,03 Torgny Lyon <torgny()enterprise!hb!se>
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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"

extern int errno;



/**
 * write_pid - Write the pid of the daemon process to file.
 * @file:      PID file.
 * @pid:       Process ID to write.
 *
 * This function is responsible for writing the @pid of the
 * auto-login daemon to the specified PID @file.
 */

int
lock_create (char *file, pid_t pid)
{
  int fd;
  FILE *fp;

#if defined(LOCK_FCNTL)
  struct flock lock;
#endif
#ifdef HAVE_FCNTL_F_FREESP
  struct flock tlock;
#endif
  fd = open(file, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd == -1)
    {
      return -1;
    }

#if defined(LOCK_FCNTL)
  lock.l_type = F_WRLCK;
  lock.l_start = 0;
  lock.l_whence = SEEK_SET;
  lock.l_len = 0;
  if (fcntl(fd, F_SETLK, &lock) == -1) 
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
  if (fp == NULL) 
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


/*
 * Return pid of process which has got a write lock on the
 * lock file, 0 if not locked and -1 on error.
 */

pid_t
lock_read (char *file)
{
	int fd;
	FILE *fp;
	pid_t pid;
#if defined(LOCK_FCNTL)
	struct flock lock;
#endif

	fd = open(file, O_RDONLY);
	if (fd == -1) 
	  {
	    if (errno == ENOENT)
	      return 0;
	    else
	      return -1;
	  }

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
	if (lock.l_type == F_UNLCK)
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


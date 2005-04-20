#ifndef LOCK_DEV
#define LOCK_DEV


/*
 * Create an advisory lock on the device to prevent concurrent writes.
 * Uses either lockf, flock, or fcntl locking methods.  See the Makefile
 * and the Configure files for how to specify the proper method.
 */

int lock_dev(int fd, int mode, struct device *dev)
{
#if (defined(HAVE_FLOCK) && defined (LOCK_EX) && defined(LOCK_NB))
	/**/
#else /* FLOCK */

#if (defined(HAVE_LOCKF) && defined(F_TLOCK))
	/**/
#else /* LOCKF */

#if (defined(F_SETLK) && defined(F_WRLCK))
	struct flock flk;

#endif /* FCNTL */
#endif /* LOCKF */
#endif /* FLOCK */

	if(IS_NOLOCK(dev))
		return 0;

#if (defined(HAVE_FLOCK) && defined (LOCK_EX) && defined(LOCK_NB))
	if (flock(fd, (mode ? LOCK_EX : LOCK_SH)|LOCK_NB) < 0)
#else /* FLOCK */

#if (defined(HAVE_LOCKF) && defined(F_TLOCK))
	if (mode && lockf(fd, F_TLOCK, 0) < 0)
#else /* LOCKF */

#if (defined(F_SETLK) && defined(F_WRLCK))
	flk.l_type = mode ? F_WRLCK : F_RDLCK;
	flk.l_whence = 0;
	flk.l_start = 0L;
	flk.l_len = 0L;

	if (fcntl(fd, F_SETLK, &flk) < 0)
#endif /* FCNTL */
#endif /* LOCKF */
#endif /* FLOCK */
	{
		if(errno == EINVAL
#ifdef  EOPNOTSUPP 
		   || errno ==  EOPNOTSUPP
#endif
		  )
			return 0;
		else
			return 1;
	}
	return 0;
}


#endif

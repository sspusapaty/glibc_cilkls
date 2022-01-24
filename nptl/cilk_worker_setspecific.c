/* Copyright (C) 2002-2021 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "pthreadP.h"
#include "cilk.h"
#include <shlib-compat.h>

void reg_is_cilk_thread(int (*func)(void)) {
    is_cilk_thread = func;
}

void reg_cilk_current(pthread_t (*func)(void)) {
    cilk_current = func;
}

void reg_get_cilkls_mutex(pthread_mutex_t* (*func)(void)) {
    get_cilkls_mutex = func;
}

int
___cilk_worker_setspecific (pthread_key_t key, const void *value)
{
  struct pthread *self;
  unsigned int idx1st;
  unsigned int idx2nd;
  struct pthread_key_data *level2;
  unsigned int seq;
  pthread_mutex_t* mutex = NULL;

  // TODO replace with pointer equality THREAD_SELF == current()
  if (!is_cilk_thread())
    return __pthread_setspecific(key, value);
  
  self = (struct pthread*)(cilk_current());
  mutex = get_cilkls_mutex();

  pthread_mutex_lock(mutex);

  /* Special case access to the first 2nd-level block.  This is the
     usual case.  */
  if (__glibc_likely (key < PTHREAD_KEY_2NDLEVEL_SIZE))
    {
      /* Verify the key is sane.  */
        if (KEY_UNUSED ((seq = __pthread_keys[key].seq))) {
            pthread_mutex_unlock(mutex);
	        /* Not valid.  */
            return EINVAL;
        }

      level2 = &self->specific_1stblock[key];

      /* Remember that we stored at least one set of data.  */
      if (value != NULL)
	    self->specific_used = true; // TODO: check if safe
    }
  else
    {
      if (key >= PTHREAD_KEYS_MAX
              || KEY_UNUSED ((seq = __pthread_keys[key].seq))) {
        pthread_mutex_unlock(mutex);
	    /* Not valid.  */
	    return EINVAL;
      }

      idx1st = key / PTHREAD_KEY_2NDLEVEL_SIZE;
      idx2nd = key % PTHREAD_KEY_2NDLEVEL_SIZE;

      /* This is the second level array.  Allocate it if necessary.  */
      level2 = self->specific[idx1st]; // TODO: check if safe
      if (level2 == NULL)
	{
        if (value == NULL) {
            pthread_mutex_unlock(mutex);
            /* We don't have to do anything.  The value would in any case
               be NULL.  We can save the memory allocation.  */
            return 0;
        }

	  level2
	    = (struct pthread_key_data *) calloc (PTHREAD_KEY_2NDLEVEL_SIZE,
						  sizeof (*level2));
      if (level2 == NULL) {
        pthread_mutex_unlock(mutex);
	    return ENOMEM;
      }

	  self->specific[idx1st] = level2; // TODO: check if safe
	}

      /* Pointer to the right array element.  */
      level2 = &level2[idx2nd];

      /* Remember that we stored at least one set of data.  */
      self->specific_used = true; // TODO: check if safe
    }

  /* Store the data and the sequence number so that we can recognize
     stale data.  */
  level2->seq = seq;
  level2->data = (void *) value;

  pthread_mutex_unlock(mutex);
  return 0;
}
versioned_symbol (libc, ___cilk_worker_setspecific, cilk_worker_setspecific,
		  GLIBC_2_34);
libc_hidden_ver (___cilk_worker_setspecific, __cilk_worker_setspecific)
#ifndef SHARED
strong_alias (___cilk_worker_setspecific, __cilk_worker_setspecific)
#endif

#if OTHER_SHLIB_COMPAT (libc, GLIBC_2_0, GLIBC_2_34)
compat_symbol (libpthread, ___cilk_worker_setspecific, __cilk_worker_setspecific,
	       GLIBC_2_0);
compat_symbol (libpthread, ___cilk_worker_setspecific, cilk_worker_setspecific,
	       GLIBC_2_0);
#endif

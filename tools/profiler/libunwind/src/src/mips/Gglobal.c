/* libunwind - a platform-independent unwind library
   Copyright (C) 2008 CodeSourcery

This file is part of libunwind.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

#include "unwind_i.h"
#include "dwarf_i.h"

HIDDEN pthread_mutex_t mips_lock = PTHREAD_MUTEX_INITIALIZER;
HIDDEN int tdep_needs_initialization = 1;

/* FIXME: I'm pretty sure we don't need this at all for MIPS, but "generic"
   code (include/dwarf_i.h) seems to expect it to be here at present.  */

HIDDEN uint8_t dwarf_to_unw_regnum_map[] =
  {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
  };

HIDDEN void
tdep_init (void)
{
  intrmask_t saved_mask;

  sigfillset (&unwi_full_mask);

  lock_acquire (&mips_lock, saved_mask);
  {
    if (!tdep_needs_initialization)
      /* another thread else beat us to it... */
      goto out;

    mi_init ();

    dwarf_init ();

#ifndef UNW_REMOTE_ONLY
    mips_local_addr_space_init ();
#endif
    tdep_needs_initialization = 0;	/* signal that we're initialized... */
  }
 out:
  lock_release (&mips_lock, saved_mask);
}

#ifndef SYS_FEATURES_H
#define SYS_FEATURES_H
  
  // Newlib nees this switch to enable clock_gettime etc.
  // Also, we'll need posix timers sooner or later
#define _POSIX_TIMERS 1



#ifndef __GNUC_PREREQ
#define __GNUC_PREREQ(A, B) 0 /* Nei */
#endif

#ifndef __GNUC_PREREQ__
#define __GNUC_PREREQ__(A, B) __GNUC_PREREQ(A, B)
#endif

#define __GLIBC_PREREQ__(A, B) 1 /* Jo.  */
#define __GLIBC_PREREQ(A, B) 1 /* Jo.  */

#endif

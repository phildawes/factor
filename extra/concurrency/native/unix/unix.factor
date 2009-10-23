! Copyright (C) 2009 Phil Dawes.
! See http://factorcode.org/license.txt for BSD license.
USING: accessors alien.c-types alien.syntax classes.struct
concurrency.native destructors kernel libc math memoize system ;
IN: concurrency.native.unix

! hack so that pthread sizes don't have to be hardcoded for each platform/libc-version

STRUCT: pthread_size_info
{ sizeof_pthread_mutex_t int }
{ sizeof_pthread_mutexattr_t int }
{ sizeof_pthread_cond_t int }
{ sizeof_pthread_condattr_t int } ;

FUNCTION: pthread_size_info* pthread_sizes ( ) ;

C-TYPE: pthread_mutexattr_t
C-TYPE: pthread_mutex_t
C-TYPE: pthread_condattr_t
C-TYPE: pthread_cond_t

FUNCTION: int pthread_mutexattr_init ( pthread_mutexattr_t* __attr ) ;
FUNCTION: int pthread_mutexattr_destroy ( pthread_mutexattr_t* __attr ) ;

FUNCTION: int pthread_mutex_init ( pthread_mutex_t* __mutex, pthread_mutexattr_t* __mutexattr ) ;
FUNCTION: int pthread_mutex_destroy ( pthread_mutex_t* __mutex ) ;
FUNCTION: int pthread_mutex_trylock ( pthread_mutex_t* __mutex ) ;
FUNCTION: int pthread_mutex_lock ( pthread_mutex_t* __mutex ) ;
FUNCTION: int pthread_mutex_unlock ( pthread_mutex_t* __mutex ) ;

FUNCTION: int pthread_condattr_init ( pthread_condattr_t* __attr ) ;
FUNCTION: int pthread_condattr_destroy ( pthread_condattr_t* __attr ) ;

FUNCTION: int pthread_cond_init ( pthread_cond_t* __cond, pthread_condattr_t* __cond_attr ) ;
FUNCTION: int pthread_cond_destroy ( pthread_cond_t* __cond ) ;
FUNCTION: int pthread_cond_signal ( pthread_cond_t* __cond ) ;
FUNCTION: int pthread_cond_broadcast ( pthread_cond_t* __cond ) ;
FUNCTION: int pthread_cond_wait ( pthread_cond_t* __cond, pthread_mutex_t* __mutex ) ;

MEMO: pthread-size-table ( -- n )
    pthread_sizes pthread_size_info memory>struct ;

: ok? ( result -- )
    zero? [ "got nonzero result from pthreads api " throw ] unless ;

! must be free'd
: new_pthread_mutexattr ( -- mutexattr* )
    pthread-size-table sizeof_pthread_mutexattr_t>> malloc
    [ pthread_mutexattr_init ok? ] keep ;

! must be free'd
: new_pthread_mutex ( mutexattr* -- mutex* )
    pthread-size-table sizeof_pthread_mutex_t>> malloc
    [ swap pthread_mutex_init ok? ] keep ;

! must be free'd
: new_pthread_condattr ( -- condattr* )
    pthread-size-table sizeof_pthread_condattr_t>> malloc
    [ pthread_condattr_init ok? ] keep ;

! must be free'd
: new_pthread_cond ( condattr* -- cond* )
    pthread-size-table sizeof_pthread_cond_t>> malloc
    [ swap pthread_cond_init ok? ] keep ;

M: unix (native-lock) ( -- mutex )
    new_pthread_mutexattr
    [ new_pthread_mutex  ]
    [ dup pthread_mutexattr_destroy ok? free ] bi ;

M: unix acquire-lock ( lock -- )
    underlying>> pthread_mutex_lock ok? ;

M: unix release-lock ( lock -- )
    underlying>> pthread_mutex_unlock ok? ;

M: native-lock dispose* ( lock -- )
    underlying>> [ pthread_mutex_destroy ] keep free ok? ;

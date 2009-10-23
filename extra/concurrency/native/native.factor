! Copyright (C) 2009 Phil Dawes.
! See http://factorcode.org/license.txt for BSD license.
USING: accessors combinators continuations destructors fry kernel
system vocabs.loader ;
IN: concurrency.native

TUPLE: native-lock < disposable underlying ;

HOOK: (native-lock) os ( -- underlying )
HOOK: acquire-lock os ( lock -- )
HOOK: release-lock os ( lock -- )

: <native-lock> ( -- native-lock )
    native-lock new-disposable (native-lock) >>underlying ;

: with-lock ( lock quot -- )
    '[ acquire-lock @ ] over '[ _ release-lock ] [ ] cleanup ; inline

{
    { [ os unix? ] [ "concurrency.native.unix" ] } 
    { [ os windows? ] [ "concurrency.native.windows" ] }
} cond require

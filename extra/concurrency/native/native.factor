! Copyright (C) 2009 Phil Dawes.
! See http://factorcode.org/license.txt for BSD license.
USING: accessors assocs combinators concurrency.locks continuations
destructors fry kernel namespaces system vocabs.loader ;
IN: concurrency.native

TUPLE: native-lock < disposable underlying ;

HOOK: (native-lock) os ( -- underlying )
HOOK: acquire-lock os ( lock -- )
HOOK: release-lock os ( lock -- )

: <native-lock> ( -- native-lock )
    native-lock new-disposable (native-lock) >>underlying ;

: with-native-lock ( lock quot -- )
    '[ acquire-lock @ ] over '[ _ release-lock ] [ ] cleanup ; inline

: local-vm-locks ( -- hash )
    \ local-vm-locks get-global ;

\ local-vm-locks [ H{ } clone ] initialize

: local-lock ( native-lock -- local-lock )
  underlying>> local-vm-locks [ drop <lock> ] cache ;

: with-dual-lock ( native-lock quot -- )
    over local-lock [ with-native-lock ] with-lock ; inline

{
    { [ os unix? ] [ "concurrency.native.unix" ] } 
    { [ os windows? ] [ "concurrency.native.windows" ] }
} cond require

! Copyright (C) 2009 Phil Dawes.
! See http://factorcode.org/license.txt for BSD license.

USING: accessors alien arrays combinators command-line
concurrency.messaging fry io io.ports kernel math
math.parser namespaces sequences serialize
splitting system threads vocabs.loader concurrency.native prettyprint ;
IN: native-thread-test.io

HOOK: create-pipe-handles os ( -- in out )
HOOK: handle>output-stream os ( handle -- output-stream )
HOOK: arg>output-handle os ( integer -- handle )    

TUPLE: vm-pipe send-handle native-lock ;
C: <vm-pipe> vm-pipe

TUPLE: remote-vm-thread < vm-pipe thread-id ;
C: <remote-vm-thread> remote-vm-thread 

SYMBOL: local-vm-pipe

: >command-line-arg ( pipe-handle lock-handle thread-id -- str )        
    3array [ number>string ] map "," join "-parentvm=" prepend ;

: parentvm-arg ( -- str )
    local-vm-pipe get
    [ send-handle>> dup alien? [ alien-address ] when ]
    [ native-lock>> underlying>> alien-address ] bi
    self id>> >command-line-arg ;

: parentvm-arg-from-command-line ( -- str )
    (command-line) [ "-parentvm=" head? ] find nip ;

: thread>remote-vm-thread ( thread -- rvp )
    local-vm-pipe get [ send-handle>> ] [ native-lock>> ] bi
    rot id>> <remote-vm-thread> ;

M: thread (serialize) 
    thread>remote-vm-thread (serialize) ;

: >output-stream ( remote-vm-thread -- output-port )
    send-handle>> handle>output-stream ;

: send-to-locked-stream ( msg remote-vm-thread -- )
    dup native-lock>> [ thread-id>> 2array serialize flush ] with-dual-lock ;
!    thread-id>> 2array serialize flush ;

M: remote-vm-thread send ( msg remote-vm-thread -- )
    dup >output-stream [ send-to-locked-stream ] with-output-stream* ;

: shutdown-pipe ( -- )
  local-vm-pipe f set ;    

: read-pipe-forever ( -- )
    deserialize
    [ first2 thread send read-pipe-forever ]
    [ shutdown-pipe ] if* ;

: start-vm-pipe-reader ( input-stream -- )
    '[ _ [ read-pipe-forever ] with-input-stream* ] "vm-pipe-reader" spawn drop ;

: args>remote-vm-thread ( pipe-handle lock-handle thread-id -- rvp )
    [ arg>output-handle ] [ <alien> native-lock new swap >>underlying ] [ ] tri* <remote-vm-thread> ;

: parentvm-arg-to-remote-thread ( arg -- remote-vm-thread )
    "-parentvm=" length tail "," split [ string>number ] map first3
    args>remote-vm-thread ;

: start-vm-comms ( -- )
    local-vm-pipe get [
        create-pipe-handles
        [ <input-port> start-vm-pipe-reader ]
        [ <native-lock> <vm-pipe> local-vm-pipe set ] bi*
    ] unless ;

: send-our-pipe-to-parent ( parentvm-args -- )
    parentvm-arg-to-remote-thread self swap send ;

: (child-main) ( parentvm-arg -- )
    send-our-pipe-to-parent
    receive call( -- ) ;

: child-main ( -- )
    start-vm-comms 
    parentvm-arg-from-command-line
    (child-main) ;

MAIN: child-main

{
    { [ os unix? ] [ "native-thread-test.io.unix" ] } 
    { [ os windows? ] [ "native-thread-test.io.windows" ] }
} cond require

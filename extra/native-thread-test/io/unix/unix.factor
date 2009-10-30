! Copyright (C) 2009 Phil Dawes.
! See http://factorcode.org/license.txt for BSD license.
USING: accessors io.backend.unix io.ports kernel native-thread-test.io system io.pipes ;
IN: native-thread-test.io.unix

M: unix create-pipe-handles ( -- in out )
    (pipe) [ in>> ] [ out>> fd>> ] bi ;

M: unix handle>output-stream
    <fd> <output-port> ;

M: unix arg>output-handle ;  ! nothing to do
    
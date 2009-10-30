! Copyright (C) 2009 Phil Dawes.
! See http://factorcode.org/license.txt for BSD license.
USING: accessors alien destructors io io.backend.windows io.buffers
io.pipes.windows.nt io.ports kernel math.bitwise native-thread-test.io
sequences system windows.kernel32 io.files.windows io.backend.windows.nt ;
IN: native-thread-test.io.windows

TUPLE: synchronous-output-port < output-port ;

: <synchronous-output-port> ( handle -- output-port )
    synchronous-output-port <buffered-port> ;

: write-synchronously ( port -- n )
    make-FileArgs dup setup-write WriteFile drop lpNumberOfBytesRet>> first ;

M: synchronous-output-port stream-flush ( port -- )
    [
        dup buffer>> buffer-empty?
        [ drop ]
        [
            [ write-synchronously ]
            [ finish-write ]
            [ port-flush ] tri ] if
    ] with-destructors ;

: open-write-end ( name -- handle )
    GENERIC_WRITE
    { FILE_SHARE_READ FILE_SHARE_WRITE } flags
    default-security-attributes
    OPEN_EXISTING
    0   !    i.e. not FILE_FLAG_OVERLAPPED, this is synchronous
    f
    CreateFile ;

M: winnt create-pipe-handles ( -- in out )
    unique-pipe-name
    [ [ create-named-pipe ] with-destructors ]
    [ open-write-end ] bi ;

M: winnt handle>output-stream
    <win32-file> <synchronous-output-port> ;

M: winnt arg>output-handle <alien> ;
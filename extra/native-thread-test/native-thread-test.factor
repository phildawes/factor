USING: alien.c-types alien.syntax concurrency.messaging io
io.encodings.utf16n io.encodings.utf8 io.files kernel namespaces
native-thread-test.io sequences system threads unix.utilities ;
IN: native-thread-test

FUNCTION: void* start_standalone_factor_in_new_thread ( int argc, char** argv ) ;

HOOK: native-string-encoding os ( -- encoding )
M: windows native-string-encoding utf16n ;
M: unix native-string-encoding utf8 ;

: start-vm-in-native-thread ( args -- native-thread-handle )
    \ vm get-global prefix 
    [ length ] [ native-string-encoding strings>alien ] bi 
     start_standalone_factor_in_new_thread ;

: start-vm-in-native-thread-with-io ( -- remote-thread )
    { "-run=native-thread-test.io" "-quiet" } parentvm-arg suffix start-vm-in-native-thread drop receive ;

: spawn-vm ( quot -- remote-thread )
    start-vm-comms
    start-vm-in-native-thread-with-io
    [ send ] keep ; 

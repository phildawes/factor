! Copyright (C) 2009 Phil Dawes.
! See http://factorcode.org/license.txt for BSD license.
USING: accessors alien alien.accessors alien.c-types alien.syntax
classes classes.struct continuations fry kernel libc math ;
IN: concurrent-queue

TYPEDEF: void* cell
TYPEDEF: int lock 

STRUCT: node
{ value cell }
{ next node* }
;

: <node> ( cell -- node )
    node malloc-struct swap >>value f >>next ;

STRUCT: queue
{ first node* }
{ last node* }
{ producer-lock lock }
{ consumer-lock lock } ;

: <queue> ( -- queue )
    queue malloc-struct
    f <node> [ >>first ] [ >>last ] bi
    0 >>producer-lock 0 >>consumer-lock ;

: *node ( alien -- node )
    node memory>struct ;

: field-address ( struct fieldname -- alien )
    swap [ class offset-of ] [ (underlying)>> <displaced-alien> ] bi ;

! lets pretend this is atomic for now
: atomic-exchange-4 ( c-ptr int -- int )
    swap [ nip *int ] [ 0 set-alien-unsigned-4 ] 2bi ;

: acquire-spinlock ( alien -- )
    '[ _ 1 atomic-exchange-4 zero? not ] loop ;

: release-spinlock ( alien -- )
    0 swap 0 set-alien-unsigned-4 ;

: with-spinlock ( lockptr quot -- )
    dupd '[ _ acquire-spinlock @ ] swap '[ _ release-spinlock ] [ ] cleanup ; inline

: with-producer-lock ( queue quot -- )
    [ "producer-lock" field-address ] dip with-spinlock ; inline

: with-consumer-lock ( queue quot -- )
    [ "consumer-lock" field-address ] dip with-spinlock ; inline

: append-node ( node queue -- )
    [ last>> *node (>>next) ]
    [ (>>last) ] 2bi ;
    
: produce ( queue c-ptr -- )
    <node> over '[ _ _ append-node ] with-producer-lock ;

: next-becomes-first ( queue -- firstnode )
    [ first>> *node next>> dup ] [ (>>first) ] bi ;

: take-value ( node -- value )
    [ value>> ] [ f swap (>>value) ] bi ;

: extract-node ( queue -- firstnode value )
    [ next-becomes-first ] [ first>> *node take-value ] bi ;

: value-to-consume? ( queue -- ? )
    first>> *node next>> ;

: free-node ( node -- )
    free ;

: consume ( queue -- c-ptr/f )
    dup 
    [ dup value-to-consume?
      [ extract-node ]
      [ drop f f ] if
    ] with-consumer-lock
    [ free-node ] dip ;

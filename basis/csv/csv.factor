! Copyright (C) 2007, 2008 Phil Dawes
! See http://factorcode.org/license.txt for BSD license.
USING: accessors combinators combinators.short-circuit io io.files
kernel make namespaces sequences unicode.categories ;
IN: csv

SYMBOL: delimiter

CHAR: , delimiter set-global

TUPLE: dialect delimiter quotechar lineterminator ;

: <dialect> ( -- d )
  dialect new
  CHAR: , >>delimiter
  CHAR: " >>quotechar
  "\n" >>lineterminator ;

SYMBOL: thedialect

<dialect> thedialect set-global

<PRIVATE

: delimiter> ( -- ch ) thedialect get delimiter>> ; inline
: quotechar> ( -- ch ) thedialect get quotechar>> ; inline
: lineterminator> ( -- str ) thedialect get lineterminator>> ; inline

DEFER: quoted-field ( -- endchar )
    
: trim-whitespace ( str -- str )
    [ blank? ] trim ; inline

: skip-to-field-end ( -- endchar )
    "\n" delimiter> suffix read-until nip ; inline
  
: not-quoted-field ( -- endchar )
    "\n" delimiter> suffix quotechar> suffix read-until
    dup {
        { quotechar> [ 2drop quoted-field ] }
        { delimiter>  [ swap trim-whitespace % ] }
        { CHAR: \n    [ swap trim-whitespace % ] }
        { f           [ swap trim-whitespace % ] }
    } case ;
  
: maybe-escaped-quote ( -- endchar )
    read1 dup {
        { quotechar>    [ , quoted-field ] }
        { delimiter> [ ] }
        { CHAR: \n   [ ] }
        [ 2drop skip-to-field-end ]
    } case ;
  
: quoted-field ( -- endchar )
    "" quotechar> suffix read-until
    drop % maybe-escaped-quote ;

: field ( -- sep string )
    [ not-quoted-field ] "" make  ;

: (row) ( -- sep )
    field , 
    dup delimiter> = [ drop (row) ] when ;

: row ( -- eof? array[string] )
    [ (row) ] { } make ;

: (csv) ( -- )
    row
    dup [ empty? ] all? [ drop ] [ , ] if
    [ (csv) ] when ;
  
PRIVATE>

: csv-row ( stream -- row )
    [ row nip ] with-input-stream* ;

: csv* ( -- rows )
    [ (csv) ] { } make dup last { "" } = [ but-last ] when ;
  
: csv ( stream -- rows )
    [ csv* ] with-input-stream ;

: file>csv ( path encoding -- csv )
    <file-reader> csv ;

: with-dialect ( dialect quot -- )
    [ thedialect ] dip with-variable ; inline

: with-delimiter ( ch quot -- )
    [ <dialect> swap >>delimiter ] dip with-dialect ; inline


<PRIVATE

: needs-escaping? ( cell -- ? )
    [ { [ "" lineterminator> suffix quotechar> suffix member? ] [ delimiter> = ] } 1|| ] any? ; inline

: escape-quotes ( cell -- cell' )
    [
        [
            [ , ]
            [ dup quotechar> = [ , ] [ drop ] if ] bi
        ] each
    ] "" make ; inline

: quotechar-as-str ( -- s ) "" quotechar> suffix ; inline 

: enclose-in-quotes ( cell -- cell' )
    quotechar-as-str dup surround ; inline
    
: escape-if-required ( cell -- cell' )
    dup needs-escaping?
    [ escape-quotes enclose-in-quotes ] when ; inline

PRIVATE>
    
: write-row ( row -- )
    [ delimiter> write1 ]
    [ escape-if-required write ] interleave lineterminator> write ; inline
    
: write-csv ( rows stream -- )
    [ [ write-row ] each ] with-output-stream ;

: csv>file ( rows path encoding -- ) <file-writer> write-csv ;

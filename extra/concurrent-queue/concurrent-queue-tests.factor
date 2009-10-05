USING: alien.syntax concurrent-queue fry kernel math sequences
tools.test ;

FROM: concurrent-queue => produce ;


[ f ] [ <queue> consume ] unit-test
[ ALIEN: 35 ] [ <queue> [ ALIEN: 35 produce ] [ consume ] bi ] unit-test

[ ALIEN: 35 ALIEN: 36 ALIEN: 37 ]
[ <queue>
  [ { ALIEN: 35 ALIEN: 36 ALIEN: 37 } [ produce ] with each ]
  [ '[ _ consume ] 3 swap times ] bi
] unit-test
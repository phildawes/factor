USING: concurrency.native destructors tools.test ;
IN: concurrency.native.tests

! make sure native locks run without errors
[ ] [ <native-lock> [ [ ] with-lock ] with-disposal ] unit-test
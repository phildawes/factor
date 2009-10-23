! Copyright (C) 2009 Phil Dawes.
! See http://factorcode.org/license.txt for BSD license.
USING: windows.kernel32 windows.types alien.syntax kernel destructors
accessors concurrency.native system ;
IN: concurrency.native.windows

LIBRARY: kernel32

FUNCTION: HANDLE CreateMutexW ( LPSECURITY_ATTRIBUTES lMutexAttributes, BOOL linitialOwner, LPCSTR lName ) ;
ALIAS: CreateMutex CreateMutexW
FUNCTION: BOOL ReleaseMutex ( HANDLE mutex ) ;

FUNCTION: HANDLE CreateSemaphoreW ( LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximunCount, LPCTSTR lpName ) ;
ALIAS: CreateSemaphore CreateSemaphoreW
FUNCTION: BOOL ReleaseSemaphore ( HANDLE hSemaphore, LONG lReleaseCount, LPLONG lpPreviousCount ) ;

: check-error ( handle -- handle )
    dup [ "win32 threading function returned 0" throw ] unless ;

M: windows (native-lock) ( -- HANDLE )
    f 0 f CreateMutex check-error ;

M: windows acquire-lock ( lock -- )
    underlying>> INFINITE WaitForSingleObject [ "Error locking win32 mutex" throw ] unless ;

M: windows release-lock ( lock -- )
    underlying>> ReleaseMutex [ "Error releasing win32 mutex" throw ] unless ;

M: native-lock dispose* ( lock -- )
    underlying>> CloseHandle drop ;

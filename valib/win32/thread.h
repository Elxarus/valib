/*
  Thread and related classes

  Thread   - abstract base for thread classes
  CritSec  - critical section
  AutoLock - automatic lock
*/

#ifndef CTHREAD_H
#define CTHREAD_H

#include <windows.h>

class Thread
{
private:
  HANDLE f_thread;
  DWORD  f_threadId;
  bool   f_suspended;

  static DWORD WINAPI ThreadProc(LPVOID param);

protected:
  volatile bool   f_terminate;
  virtual DWORD process() = 0;

public:
  Thread(bool suspended = false);
  virtual ~Thread();

  void suspend();
  void resume();
  void terminate(int timeout_ms = 1000, DWORD exit_code = 0);

  HANDLE handle()      { return f_thread; }
  bool   terminating() { return f_terminate; }
  bool   terminated()  { return !f_thread; }
  bool   suspended()   { return f_thread? f_suspended: true; }
};


class CritSec 
{
protected:
  // Disallow critical section object copy
  CritSec(const CritSec &);
  CritSec &operator=(const CritSec &);

  CRITICAL_SECTION crit_sec;
  int lock_count;

public:
  CritSec()     { InitializeCriticalSection(&crit_sec); lock_count = 0; };
  ~CritSec()    { DeleteCriticalSection(&crit_sec);                     };

  void lock()   { EnterCriticalSection(&crit_sec);      lock_count++;   };
  void unlock() { LeaveCriticalSection(&crit_sec);      lock_count--;   };
};


class AutoLock 
{
protected:
  // Disallow autolock object copy
  AutoLock(const AutoLock &);
  AutoLock &operator=(const AutoLock &);

  CritSec *lock;

public:
  AutoLock(CritSec *_lock)
  {
    lock = _lock;
    lock->lock();
  };

  ~AutoLock() 
  {
    lock->unlock();
  };
};


#endif

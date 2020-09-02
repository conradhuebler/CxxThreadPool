# CxxThreadPool

Simple, header-only Thread Pool Class for C++11 ( or newer ). Inspired by the Qt ThreadPool Class. A progress bar will be printed out to cerr.

# Usage

Include the CxxThreadPool.h in your project and link your project against pthread ( at least on *nx systems ).

Subclass CxxThread and reimplement
```cpp
virtual int execute() = 0;
```
like for example
```cpp
inline int execute()
{
    printf("Thread  started  ...\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    printf("Thread finished! .. \n",);
    return 0;
}
```

Use
```cpp
CxxThreadPool *pool = new CxxThreadPool;
pool->setActiveThreadCount(32);
```
to control the number of active threads and add a single CxxThread derived object with
```cpp
pool->addThread(thread);
```
.

Finally, run all threads in queued and in parallel with
```cpp
pool->StartAndWait();
```

Increase verbosity by defining
```cpp
#define _CxxThreadPool_Verbose
```
Prevent of a progress bar appearing
```cpp
#define _CxxThreadPool_NoBar
```
Define width of the progress bar
```cpp
#define _CxxThreadPool_BarWidth
```
Wakeup timeout for to check threads in milliseconds ( default = 100 )
```cpp
#define _CxxThreadPool_TimeOut 100
```
before including the header file.

Have a lot of fun.

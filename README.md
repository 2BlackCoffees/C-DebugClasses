# C++ DebugClasses

These 2 files allow to perform intrusiv debug and performance analyze of a C++ Program.

These are most macros. 
Before starting make sure the configuration matches your requirements.
In TraceDebug.hpp, you will find following defines that might need to be configured for your project:
```  
  TRACE_DEBUG_HPP_DEBUG_LOCAL: Will create a main to make a local example run, you certainly do not want to keep this one in your project. Comment it out.

  ENABLE_THREAD_SAFE:          If you are ** not ** running a multi threaded program comment this define.
  
  WRITE_OUTPUT_TO_FILE:        If not commented, write outputs into a file. Comment to write to std::out or qDebug if you are using Qt.

  USE_QT_DEBUG:                Commented, writes to std::out. Otherwise uses qDebug: However if WRITE_OUTPUT_TO_FILE is defined, then output will anyway be written into a file.
  
  UNIT_TRACE_DEBUG_NANO:       If commented, traces are displayed in ms. If not comented, traces are displayed in ns.

 ```

Following macros are available:


## DISPLAY_IMMEDIATE_DEBUG_VALUE(value)

    Displays:
    
```
      EpochSince1969:ThreadId:FileName:LineNumber (functionName) value = <ItsValue>
```

        - EpochSince1969 is useful if many process are communicationg with each other to be able to understand synchronization problems.
        - ThreadId is not displayed if ENABLE_THREAD_SAFE is not enabled.
        - A number of blank spaces defining the deepness of the hierarchy will be displayed providing information on the hierarchical deepnes of the call.    

    Example:
    
```c++
      int f3() {
        int a = 5;
        DISPLAY_IMMEDIATE_DEBUG_VALUE(a);    
        return a;
      }
```

    Will output something similar to:
```
      1469684456654.831543ms:140137838221056:TraceDebug.cpp:359 (f3)  a = 5
```

## DISPLAY_DEBUG_VALUE(functionNameReturningAValue())

    Displays:
    
```
      EpochSince1969:ThreadId:Processing functionNameReturningAValue From  FileName:LineNumber (fromFunctionName)
      EpochSince1969:ThreadId:->FileName:LineNumber (fromFunctionName) functionNameReturningAValue() = <ItsValue>
```
        - EpochSince1969 is useful if many process are communicationg with each other to be able to understand synchronization problems.
        - ThreadId is not displayed if ENABLE_THREAD_SAFE is not enabled.
        - A number of blank spaces defining the deepness of the hierarchy will be displayed providing information on the hierarchical deepnes of the call.    

    Example:
    
```c++
      int f3() {
        int a = 5;
        return a;    
      }
      int f2() {
        DISPLAY_DEBUG_VALUE(f3());
        return f3();
      }
```

    Will output something similar to:
    
```
      1469684456654.476807ms:140137855014784:Processing f3()  From TraceDebug.cpp:364 (f2)
      1469684456655.108154ms:140137838221056:->TraceDebug.cpp:364 (f2)  f3() = 5
```

## DISPLAY_DEBUG_MESSAGE("My message")

    Displays:
    
```
      EpochSince1969:ThreadId:FileName:LineNumber (functionName) My message
```

        - EpochSince1969 is useful if many process are communicationg with each other to be able to understand synchronization problems.
        - ThreadId is not displayed if ENABLE_THREAD_SAFE is not enabled.
        - A number of blank spaces defining the deepness of the hierarchy will be displayed providing information on the hierarchical deepnes of the call.    

    
    Example:
    
```
      void f3() {
        DISPLAY_DEBUG_MESSAGE("My message");    
      }
```
    
    Will output something similar to:
    
```
      1469695056816.514648ms:139934998378368:TraceDebug.cpp:374 (f3) My message
```
      
## START_TRACE_PERFORMANCE(uniqueKey)     

    Provides timing in the current scope.
    
    Displays:
    
```
      EpochSince1969:ThreadId:FileName:LineNumber (functionName) [uniqueKey] Start measure
      EpochSince1969:ThreadId:FileName:LineNumber (functionName) [uniqueKey] , <End measure> - <Start measure> = <Time spent>
```
        - EpochSince1969 is useful if many process are communicationg with each other to be able to understand synchronization problems.
        - ThreadId is not displayed if ENABLE_THREAD_SAFE is not enabled.
        - A number of blank spaces defining the deepness of the hierarchy will be displayed providing information on the hierarchical deepnes of the call.    
    
    Example:
    
```c++
      int f3() {
        START_TRACE_PERFORMANCE(f3);
        int a = 5;
        return a;    
      }
```
    
    Will output something similar to:    
    
```
      1469684456655.628174ms:140137855014784:TraceDebug.cpp:357 (f3) [f3]  Start measure
      1469684456655.718994ms:140137855014784:TraceDebug.cpp:357 (f3) [f3], <End measure> - <Start measure> = 0.091847ms
```

## ADD_TRACE_PERFORMANCE(uniqueKey, userInfo)

     Provides intermediate timing in the scope of the referenced key of START_TRACE_PERFORMANCE.
     
     Displays:
     
```
      EpochSince1969:ThreadId:FileName:LineNumber (functionName) [uniqueKey] Start measure
      EpochSince1969:ThreadId:FileName:LineNumber (functionName) [uniqueKey] , <userInfo> - <Start measure> = <Time spent 1>, <End measure> - <Start measure> = <Time spent 2>, Full time: <Full time spent>
```

    Example:
    
```c++
      int test() {
        START_TRACE_PERFORMANCE(test);
        doSth1();
        ADD_TRACE_PERFORMANCE("Before doSth2");
        doSth2();
      }
```

    Will output something similar to:   
    
```
      1469684456653.974365ms:140137855014784:TraceDebug.cpp:375 (test) [test]  Start measure
      1469684456655.876709ms:140137855014784:TraceDebug.cpp:375 (test) [test], <Before doSth2> - <Start measure> = 1.549560ms, <End measure> - <Before doSth2> = 0.359068ms, Full time: 1.908628ms
```

## SET_TRACE_PERFORMANCE_CACHE_DEEPNESS(integer value)
    If the integer value is greater than 0, then all information are stored in a cahe. Once the number of cached lines reached the inger value, all is printed once.
    The time to flush the cash is as well indicated.
    
## DISPLAY_START_TRACE_PERFORMANCE(boolean)
    If set to true, then the first line associated to the macro START_TRACE_PERFORMANCE is displayed (default behaviour).
    If set to false, only the resulting time is displayed.
    

     
As an example, a small C++ example:

```c++
int f3() {
  START_TRACE_PERFORMANCE(f3);
  int a = 5;
  DISPLAY_IMMEDIATE_DEBUG_VALUE(a);
  return a;
}
int f2() {
  START_TRACE_PERFORMANCE(f2);
  DISPLAY_DEBUG_VALUE(f3());
  return 2;
}
int f1() {
  START_TRACE_PERFORMANCE(f1);
  DISPLAY_DEBUG_VALUE(f2() - 1);
  return 1;
}

void test() {
  std::thread thread1(f1);
  START_TRACE_PERFORMANCE(test);
  DISPLAY_DEBUG_VALUE(f1());
  ADD_TRACE_PERFORMANCE(test, "This is the middle");
  f2();
  thread1.join();
}

int main()
{
  std::cout << "\n\n ****  Without cache enabled **** \n\n";
  SET_TRACE_PERFORMANCE_CACHE_DEEPNESS(0);
  test();
  std::cout << "\n\n ****  With cache enabled **** \n\n";
  SET_TRACE_PERFORMANCE_CACHE_DEEPNESS(50);
  test();
}
```

Provides following output:

```

 ****  Without cache enabled **** 

1469712120722.829346ms:139700164831104:TraceDebug.cpp:376 (test) [test]  Start measure
1469712120722.878418ms:139700148037376:TraceDebug.cpp:368 (f1) [f1]  Start measure
  1469712120722.959717ms:139700148037376:Processing f2() - 1  From TraceDebug.cpp:369 (f1)
    1469712120723.001465ms:139700148037376:TraceDebug.cpp:363 (f2) [f2]  Start measure
      1469712120723.034180ms:139700148037376:Processing f3()  From TraceDebug.cpp:364 (f2)
        1469712120723.078125ms:139700148037376:TraceDebug.cpp:357 (f3) [f3]  Start measure
  1469712120723.059814ms:139700164831104:Processing f1()  From TraceDebug.cpp:377 (test)
    1469712120723.146484ms:139700164831104:TraceDebug.cpp:368 (f1) [f1]  Start measure
      1469712120723.190674ms:139700164831104:Processing f2() - 1  From TraceDebug.cpp:369 (f1)
        1469712120723.224854ms:139700164831104:TraceDebug.cpp:363 (f2) [f2]  Start measure
          1469712120723.250732ms:139700164831104:Processing f3()  From TraceDebug.cpp:364 (f2)
            1469712120723.290527ms:139700164831104:TraceDebug.cpp:357 (f3) [f3]  Start measure
          1469712120723.104248ms:139700148037376:TraceDebug.cpp:359 (f3)  a = 5
        1469712120723.372314ms:139700148037376:TraceDebug.cpp:357 (f3) [f3], <Start measure> - <Start measure> = 0.214817ms, <End measure> - <Start measure> = 0.077399ms, Full time: 0.292216ms
      1469712120723.428955ms:139700148037376:->TraceDebug.cpp:364 (f2)  f3() = 5
    1469712120723.463867ms:139700148037376:TraceDebug.cpp:363 (f2) [f2], <Start measure> - <Start measure> = 0.228599ms, <End measure> - <Start measure> = 0.237513ms, Full time: 0.466112ms
              1469712120723.441895ms:139700164831104:TraceDebug.cpp:359 (f3)  a = 5
  1469712120723.500244ms:139700148037376:->TraceDebug.cpp:369 (f1)  f2() - 1 = 1
1469712120723.575928ms:139700148037376:TraceDebug.cpp:368 (f1) [f1], <Start measure> - <Start measure> = 0.270867ms, <End measure> - <Start measure> = 0.429771ms, Full time: 0.700638ms
          1469712120723.658691ms:139700164831104:->TraceDebug.cpp:364 (f2)  f3() = 5
      1469712120723.706055ms:139700164831104:->TraceDebug.cpp:369 (f1)  f2() - 1 = 1
  1469712120723.744141ms:139700164831104:->TraceDebug.cpp:377 (test)  f1() = 1
    1469712120723.775146ms:139700164831104:TraceDebug.cpp:363 (f2) [f2]  Start measure
      1469712120723.802246ms:139700164831104:Processing f3()  From TraceDebug.cpp:364 (f2)
        1469712120723.857178ms:139700164831104:TraceDebug.cpp:357 (f3) [f3]  Start measure
          1469712120723.883545ms:139700164831104:TraceDebug.cpp:359 (f3)  a = 5
        1469712120723.910645ms:139700164831104:TraceDebug.cpp:357 (f3) [f3], <End measure> - <Start measure> = 0.055173ms
      1469712120723.940918ms:139700164831104:->TraceDebug.cpp:364 (f2)  f3() = 5
    1469712120723.966553ms:139700164831104:TraceDebug.cpp:363 (f2) [f2], <End measure> - <Start measure> = 0.190994ms
1469712120724.017090ms:139700164831104:TraceDebug.cpp:376 (test) [test], <This is the middle> - <Start measure> = 0.943374ms, <End measure> - <This is the middle> = 0.248315ms, Full time: 1.191689ms
1469712120724.057129ms:139700164831104:TraceDebug.cpp:374 (test)  


 ****  With cache enabled **** 

1469712120724.100830ms:139700164831104:TraceDebug.cpp:376 (test) [test]  Start measure
1469712120724.136475ms:139700148037376:TraceDebug.cpp:368 (f1) [f1]  Start measure
  1469712120724.116211ms:139700164831104:Processing f1()  From TraceDebug.cpp:377 (test)
    1469712120724.184082ms:139700164831104:TraceDebug.cpp:368 (f1) [f1]  Start measure
  1469712120724.174561ms:139700148037376:Processing f2() - 1  From TraceDebug.cpp:369 (f1)
    1469712120724.210693ms:139700148037376:TraceDebug.cpp:363 (f2) [f2]  Start measure
      1469712120724.201416ms:139700164831104:Processing f2() - 1  From TraceDebug.cpp:369 (f1)
        1469712120724.237305ms:139700164831104:TraceDebug.cpp:363 (f2) [f2]  Start measure
      1469712120724.231201ms:139700148037376:Processing f3()  From TraceDebug.cpp:364 (f2)
        1469712120724.264160ms:139700148037376:TraceDebug.cpp:357 (f3) [f3]  Start measure
          1469712120724.280518ms:139700148037376:TraceDebug.cpp:359 (f3)  a = 5
        1469712120724.300049ms:139700148037376:TraceDebug.cpp:357 (f3) [f3], <End measure> - <Start measure> = 0.036052ms
          1469712120724.252197ms:139700164831104:Processing f3()  From TraceDebug.cpp:364 (f2)
            1469712120724.332520ms:139700164831104:TraceDebug.cpp:357 (f3) [f3]  Start measure
      1469712120724.319580ms:139700148037376:->TraceDebug.cpp:364 (f2)  f3() = 5
    1469712120724.361816ms:139700148037376:TraceDebug.cpp:363 (f2) [f2], <Start measure> - <Start measure> = 0.029279ms, <End measure> - <Start measure> = 0.122675ms, Full time: 0.151954ms
              1469712120724.351807ms:139700164831104:TraceDebug.cpp:359 (f3)  a = 5
            1469712120724.396973ms:139700164831104:TraceDebug.cpp:357 (f3) [f3], <End measure> - <Start measure> = 0.065986ms
  1469712120724.385254ms:139700148037376:->TraceDebug.cpp:369 (f1)  f2() - 1 = 1
1469712120724.425537ms:139700148037376:TraceDebug.cpp:368 (f1) [f1], <Start measure> - <Start measure> = 0.052087ms, <End measure> - <Start measure> = 0.240887ms, Full time: 0.292974ms
          1469712120724.415039ms:139700164831104:->TraceDebug.cpp:364 (f2)  f3() = 5
      1469712120724.469238ms:139700164831104:->TraceDebug.cpp:369 (f1)  f2() - 1 = 1
  1469712120724.490479ms:139700164831104:->TraceDebug.cpp:377 (test)  f1() = 1
    1469712120724.505371ms:139700164831104:TraceDebug.cpp:363 (f2) [f2]  Start measure
      1469712120724.517578ms:139700164831104:Processing f3()  From TraceDebug.cpp:364 (f2)
        1469712120724.530518ms:139700164831104:TraceDebug.cpp:357 (f3) [f3]  Start measure
          1469712120724.541748ms:139700164831104:TraceDebug.cpp:359 (f3)  a = 5
        1469712120724.553223ms:139700164831104:TraceDebug.cpp:357 (f3) [f3], <End measure> - <Start measure> = 0.023913ms
      1469712120724.565430ms:139700164831104:->TraceDebug.cpp:364 (f2)  f3() = 5
    1469712120724.578369ms:139700164831104:TraceDebug.cpp:363 (f2) [f2], <End measure> - <Start measure> = 0.073292ms
1469712120724.594971ms:139700164831104:TraceDebug.cpp:376 (test) [test], <This is the middle> - <Start measure> = 0.402124ms, <End measure> - <This is the middle> = 0.094128ms, Full time: 0.496252ms

 ```

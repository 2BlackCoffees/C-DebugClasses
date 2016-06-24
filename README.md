# C++ DebugClasses

These 2 files allow to perform intrusiv debug and performance analyze of a C++ Program.


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
void main()
{
  // Disable cache related to performance analysis for the sake of the example.
  {
    SET_TRACE_PERFORMANCE_CACHE_DEEPNESS(0);
    START_TRACE_PERFORMANCE(main);
    DISPLAY_DEBUG_VALUE(f1());
    ADD_TRACE_PERFORMANCE(main, "This is the middle");
    f2();
  }
  
  // Enable again: trace informtion will not be dispayed concurrently to the timing measures
  {
    std::cout << "\n\n  **** Measures with cache enabled ****\n" << std::endl;
    SET_TRACE_PERFORMANCE_CACHE_DEEPNESS(500);
    START_TRACE_PERFORMANCE(main);
    DISPLAY_DEBUG_VALUE(f1());
    ADD_TRACE_PERFORMANCE(main, "This is the middle");
    f2();
  }
}
```

Provides following output:

```
  Processing f1()  From TraceDebug.cpp:225 (main)
      Processing f2() - 1  From TraceDebug.cpp:216 (f1)
          Processing f3()  From TraceDebug.cpp:211 (f2)
              TraceDebug.cpp:206 (f3)  a = 5
           (Full elapsed: 0.000000 ms) TraceDebug.cpp:204 (f3) [f3], <End measure> - <Start measure> = 2.000000 ms
          ->TraceDebug.cpp:211 (f2)  f3() = 5
       (Full elapsed: 8.000000 ms) TraceDebug.cpp:210 (f2) [f2], <End measure> - <Start measure> = 13.000000 ms
      ->TraceDebug.cpp:216 (f1)  f2() - 1 = 1
   (Full elapsed: 16.000000 ms) TraceDebug.cpp:215 (f1) [f1], <End measure> - <Start measure> = 25.000000 ms
  ->TraceDebug.cpp:225 (main)  f1() = 1
      Processing f3()  From TraceDebug.cpp:211 (f2)
          TraceDebug.cpp:206 (f3)  a = 5
       (Full elapsed: 32.000000 ms) TraceDebug.cpp:204 (f3) [f3], <End measure> - <Start measure> = 2.000000 ms
      ->TraceDebug.cpp:211 (f2)  f3() = 5
   (Full elapsed: 40.000000 ms) TraceDebug.cpp:210 (f2) [f2], <End measure> - <Start measure> = 13.000000 ms
 (Full elapsed: 45.000000 ms) TraceDebug.cpp:224 (main) [main], <This is the middle> - <Start measure> = 39.000000 ms, <End measure> - <This is the middle> = 18.000000 ms, Full time: 57.000000 ms


  **** Measures with cache enabled ****
 
  Processing f1()  From TraceDebug.cpp:235 (main)
      Processing f2() - 1  From TraceDebug.cpp:216 (f1)
          Processing f3()  From TraceDebug.cpp:211 (f2)
              TraceDebug.cpp:206 (f3)  a = 5
          ->TraceDebug.cpp:211 (f2)  f3() = 5
      ->TraceDebug.cpp:216 (f1)  f2() - 1 = 1
  ->TraceDebug.cpp:235 (main)  f1() = 1
      Processing f3()  From TraceDebug.cpp:211 (f2)
          TraceDebug.cpp:206 (f3)  a = 5
      ->TraceDebug.cpp:211 (f2)  f3() = 5
           (Full elapsed: 73.000000 ms) TraceDebug.cpp:204 (f3) [f3], <End measure> - <Start measure> = 3.000000 ms
       (Full elapsed: 75.000000 ms) TraceDebug.cpp:210 (f2) [f2], <End measure> - <Start measure> = 9.000000 ms
   (Full elapsed: 77.000000 ms) TraceDebug.cpp:215 (f1) [f1], <End measure> - <Start measure> = 15.000000 ms
       (Full elapsed: 84.000000 ms) TraceDebug.cpp:204 (f3) [f3], <End measure> - <Start measure> = 2.000000 ms
   (Full elapsed: 86.000000 ms) TraceDebug.cpp:210 (f2) [f2], <End measure> - <Start measure> = 7.000000 ms
 (Full elapsed: 86.000000 ms) TraceDebug.cpp:234 (main) [main], <This is the middle> - <Start measure> = 21.000000 ms, <End measure> - <This is the middle> = 7.000000 ms, Full time: 28.000000 ms
 (Full elapsed: 86.000000 ms) TraceDebug.cpp:234 (main) [main], <This is the middle> - <Start measure> = 21.000000 ms, <End measure> - <This is the middle> = 7.000000 ms, Full time: 28.000000 ms
 (Full elapsed: 130.000000 ms) TraceDebug.cpp:234 (main) [main], <This is the middle> - <Start measure> = 21.000000 ms, <End measure> - <This is the middle> = 7.000000 ms, <Start Printing cache> - <End measure> = 0.000000 ms, <Done Printing cache> - <Start Printing cache> = 44.000000 ms, Full time: 72.000000 ms
```

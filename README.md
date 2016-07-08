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
    SET_TRACE_PERFORMANCE_CACHE_DEEPNESS(5);
    START_TRACE_PERFORMANCE(main);
    DISPLAY_DEBUG_VALUE(f1());
    ADD_TRACE_PERFORMANCE(main, "This is the middle");
    f2();
  }
}
```

Provides following output:

```
  Processing f1()  From TraceDebug.cpp:254 (main)
      Processing f2() - 1  From TraceDebug.cpp:245 (f1)
          Processing f3()  From TraceDebug.cpp:240 (f2)
              TraceDebug.cpp:235 (f3)  a = 5
           (Full elapsed: 0.000000 ms) TraceDebug.cpp:233 (f3) [f3], <End measure> - <Start measure> = 6.000000 ms
          ->TraceDebug.cpp:240 (f2)  f3() = 5
       (Full elapsed: 13.000000 ms) TraceDebug.cpp:239 (f2) [f2], <End measure> - <Start measure> = 28.000000 ms
      ->TraceDebug.cpp:245 (f1)  f2() - 1 = 1
   (Full elapsed: 25.000000 ms) TraceDebug.cpp:244 (f1) [f1], <End measure> - <Start measure> = 49.000000 ms
  ->TraceDebug.cpp:254 (main)  f1() = 1
      Processing f3()  From TraceDebug.cpp:240 (f2)
          TraceDebug.cpp:235 (f3)  a = 5
       (Full elapsed: 45.000000 ms) TraceDebug.cpp:233 (f3) [f3], <End measure> - <Start measure> = 4.000000 ms
      ->TraceDebug.cpp:240 (f2)  f3() = 5
   (Full elapsed: 56.000000 ms) TraceDebug.cpp:239 (f2) [f2], <End measure> - <Start measure> = 19.000000 ms
 (Full elapsed: 62.000000 ms) TraceDebug.cpp:253 (main) [main], <This is the middle> - <Start measure> = 67.000000 ms, <End measure> - <This is the middle> = 25.000000 ms, Full time: 92.000000 ms


  **** Measures with cache enabled ****

  Processing f1()  From TraceDebug.cpp:264 (main)
      Processing f2() - 1  From TraceDebug.cpp:245 (f1)
          Processing f3()  From TraceDebug.cpp:240 (f2)
              TraceDebug.cpp:235 (f3)  a = 5
           (Full elapsed: 85.000000 ms) TraceDebug.cpp:233 (f3) [f3], <End measure> - <Start measure> = 0.000000 ms
           (Full elapsed: 85.000000 ms) TraceDebug.cpp:233 (f3) [f3], <End measure> - <Start measure> = 0.000000 ms
           (Full elapsed: 115.000000 ms) TraceDebug.cpp:233 (f3) [f3], <End measure> - <Start measure> = 0.000000 ms, <Start Printing cache> - <End measure> = 0.000000 ms, <Done Printing cache> - <Start Printing cache> = 30.000000 ms, Full time: 30.000000 ms
          ->TraceDebug.cpp:240 (f2)  f3() = 5
       (Full elapsed: 129.000000 ms) TraceDebug.cpp:239 (f2) [f2], <End measure> - <(***!!! Printing inducted 30.000000 ms overhead in this measure !!!***)Start measure> = 44.000000 ms
      ->TraceDebug.cpp:245 (f1)  f2() - 1 = 1
   (Full elapsed: 129.000000 ms) TraceDebug.cpp:244 (f1) [f1], <End measure> - <(***!!! Printing inducted 30.000000 ms overhead in this measure !!!***)Start measure> = 44.000000 ms
  ->TraceDebug.cpp:264 (main)  f1() = 1
    INFO: Reserved more space for cache. Now set to 10
      Processing f3()  From TraceDebug.cpp:240 (f2)
          TraceDebug.cpp:235 (f3)  a = 5
       (Full elapsed: 129.000000 ms) TraceDebug.cpp:233 (f3) [f3], <End measure> - <Start measure> = 0.000000 ms
      ->TraceDebug.cpp:240 (f2)  f3() = 5
        INFO: Reserved more space for cache. Now set to 20
   (Full elapsed: 129.000000 ms) TraceDebug.cpp:239 (f2) [f2], <End measure> - <Start measure> = 0.000000 ms
 (Full elapsed: 129.000000 ms) TraceDebug.cpp:263 (main) [main], <This is the middle> - <(***!!! Printing inducted 30.000000 ms overhead in this measure !!!***)Start measure> = 44.000000 ms, <End measure> - <This is the middle> = 0.000000 ms, Full time: 44.000000 ms
 (Full elapsed: 129.000000 ms) TraceDebug.cpp:263 (main) [main], <This is the middle> - <(***!!! Printing inducted 30.000000 ms overhead in this measure !!!***)Start measure> = 44.000000 ms, <End measure> - <This is the middle> = 0.000000 ms, Full time: 44.000000 ms
 (Full elapsed: 219.000000 ms) TraceDebug.cpp:263 (main) [main], <This is the middle> - <(***!!! Printing inducted 30.000000 ms overhead in this measure !!!***)Start measure> = 44.000000 ms, <End measure> - <This is the middle> = 0.000000 ms, <Start Printing cache> - <End measure> = 0.000000 ms, <Done Printing cache> - <Start Printing cache> = 90.000000 ms, Full time: 134.000000 ms
 ```

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
0.061682 ms: Processing f1()  From TraceDebug.cpp:272 (main)
     0.101155 ms: Processing f2() - 1  From TraceDebug.cpp:263 (f1)
         0.112363 ms: Processing f3()  From TraceDebug.cpp:258 (f2)
             0.120368 ms TraceDebug.cpp:253 (f3)  a = 5
          (Full elapsed: 0.000422 ms) TraceDebug.cpp:251 (f3) [f3], <End measure> - <Start measure> = 0.013742 ms
         0.142666 ms ->TraceDebug.cpp:258 (f2)  f3() = 5
      (Full elapsed: 0.016892 ms) TraceDebug.cpp:257 (f2) [f2], <End measure> - <Start measure> = 0.040930 ms
     0.154986 ms ->TraceDebug.cpp:263 (f1)  f2() - 1 = 1
  (Full elapsed: 0.028964 ms) TraceDebug.cpp:262 (f1) [f1], <End measure> - <Start measure> = 0.064852 ms
 0.166372 ms ->TraceDebug.cpp:272 (main)  f1() = 1
     0.176165 ms: Processing f3()  From TraceDebug.cpp:258 (f2)
         0.183521 ms TraceDebug.cpp:253 (f3)  a = 5
      (Full elapsed: 0.056520 ms) TraceDebug.cpp:251 (f3) [f3], <End measure> - <Start measure> = 0.007694 ms
     0.193883 ms ->TraceDebug.cpp:258 (f2)  f3() = 5
  (Full elapsed: 0.067016 ms) TraceDebug.cpp:257 (f2) [f2], <End measure> - <Start measure> = 0.026332 ms
(Full elapsed: 0.073725 ms) TraceDebug.cpp:271 (main) [main], <This is the middle> - <Start measure> = 0.117390 ms, <End measure> - <This is the middle> = 0.035336 ms, Full time: 0.152726 ms


 **** Measures with cache enabled ****

 0.221901 ms: Processing f1()  From TraceDebug.cpp:282 (main)
     0.228825 ms: Processing f2() - 1  From TraceDebug.cpp:263 (f1)
         0.235176 ms: Processing f3()  From TraceDebug.cpp:258 (f2)
             0.240904 ms TraceDebug.cpp:253 (f3)  a = 5
          (Full elapsed: 0.112704 ms) TraceDebug.cpp:251 (f3) [f3], <End measure> - <Start measure> = 0.006171 ms
          (Full elapsed: 0.116124 ms) TraceDebug.cpp:251 (f3) [f3], <End measure> - <Start measure> = 0.006171 ms
          (Full elapsed: 0.126592 ms) TraceDebug.cpp:251 (f3) [f3], <End measure> - <Start measure> = 0.006171 ms, <Start Printing cache> - <End measure> = 0.003960 ms, <Done Printing cache> - <Start Printing cache> = 0.010057 ms, Full time: 0.020188 ms
         0.274333 ms ->TraceDebug.cpp:258 (f2)  f3() = 5
      (Full elapsed: 0.146267 ms) TraceDebug.cpp:257 (f2) [f2], <End measure> - <(***!!! Printing inducted 0.010057 ms overhead in this measure !!!***)Start measure> = 0.045981 ms
     0.282282 ms ->TraceDebug.cpp:263 (f1)  f2() - 1 = 1
  (Full elapsed: 0.153964 ms) TraceDebug.cpp:262 (f1) [f1], <End measure> - <(***!!! Printing inducted 0.010057 ms overhead in this measure !!!***)Start measure> = 0.059860 ms
 0.289692 ms ->TraceDebug.cpp:282 (main)  f1() = 1
   0.295003 ms TraceDebug.cpp:146 (CacheOrPrintOutputs)  INFO: Reserved more space for cache. Now set to 10
     0.314314 ms: Processing f3()  From TraceDebug.cpp:258 (f2)
         0.321314 ms TraceDebug.cpp:253 (f3)  a = 5
      (Full elapsed: 0.193197 ms) TraceDebug.cpp:251 (f3) [f3], <End measure> - <Start measure> = 0.006661 ms
     0.329303 ms ->TraceDebug.cpp:258 (f2)  f3() = 5
       0.334049 ms TraceDebug.cpp:146 (CacheOrPrintOutputs)  INFO: Reserved more space for cache. Now set to 20
  (Full elapsed: 0.207240 ms) TraceDebug.cpp:257 (f2) [f2], <End measure> - <Start measure> = 0.037276 ms
(Full elapsed: 0.212542 ms) TraceDebug.cpp:281 (main) [main], <This is the middle> - <(***!!! Printing inducted 0.010057 ms overhead in this measure !!!***)Start measure> = 0.080931 ms, <End measure> - <This is the middle> = 0.044893 ms, Full time: 0.125824 ms
(Full elapsed: 0.217651 ms) TraceDebug.cpp:281 (main) [main], <This is the middle> - <(***!!! Printing inducted 0.010057 ms overhead in this measure !!!***)Start measure> = 0.080931 ms, <End measure> - <This is the middle> = 0.044893 ms, Full time: 0.125824 ms
(Full elapsed: 0.240087 ms) TraceDebug.cpp:281 (main) [main], <This is the middle> - <(***!!! Printing inducted 0.010057 ms overhead in this measure !!!***)Start measure> = 0.080931 ms, <End measure> - <This is the middle> = 0.044893 ms, <*** WARNING:Cache was resized when displaying trace informations. This resize inducted 0.013461 ms overhead in this measure !!!***)> - <End measure> = 0.005595 ms, <Start Printing cache> - <*** WARNING:Cache was resized when displaying trace informations. This resize inducted 0.013461 ms overhead in this measure !!!***)> = 0.000000 ms, <Done Printing cache> - <Start Printing cache> = 0.022181 ms, Full time: 0.153600 ms
 ```

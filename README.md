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
0.132222 ms:TraceDebug.cpp:279 (main) [main]  Start measure
  0.213242 ms: Processing f1()  From TraceDebug.cpp:280 (main)
    0.255236 ms:TraceDebug.cpp:270 (f1) [f1]  Start measure
      0.278640 ms: Processing f2() - 1  From TraceDebug.cpp:271 (f1)
        0.302037 ms:TraceDebug.cpp:265 (f2) [f2]  Start measure
          0.323583 ms: Processing f3()  From TraceDebug.cpp:266 (f2)
            0.351862 ms:TraceDebug.cpp:259 (f3) [f3]  Start measure
              0.377792 ms TraceDebug.cpp:261 (f3)  a = 5
           (Full elapsed: 0.000835 ms) TraceDebug.cpp:259 (f3) [f3], <End measure> - <Start measure> = 0.062327 ms
          0.452046 ms ->TraceDebug.cpp:266 (f2)  f3() = 5
       (Full elapsed: 0.059631 ms) TraceDebug.cpp:265 (f2) [f2], <End measure> - <Start measure> = 0.175375 ms
      0.496799 ms ->TraceDebug.cpp:271 (f1)  f2() - 1 = 1
   (Full elapsed: 0.103325 ms) TraceDebug.cpp:270 (f1) [f1], <End measure> - <Start measure> = 0.266869 ms
  0.541907 ms ->TraceDebug.cpp:280 (main)  f1() = 1
    0.569143 ms:TraceDebug.cpp:265 (f2) [f2]  Start measure
      0.592811 ms: Processing f3()  From TraceDebug.cpp:266 (f2)
        0.615757 ms:TraceDebug.cpp:259 (f3) [f3]  Start measure
          0.636929 ms TraceDebug.cpp:261 (f3)  a = 5
       (Full elapsed: 0.243254 ms) TraceDebug.cpp:259 (f3) [f3], <End measure> - <Start measure> = 0.043874 ms
      0.682120 ms ->TraceDebug.cpp:266 (f2)  f3() = 5
   (Full elapsed: 0.286123 ms) TraceDebug.cpp:265 (f2) [f2], <End measure> - <Start measure> = 0.134042 ms
 (Full elapsed: 0.311000 ms) TraceDebug.cpp:279 (main) [main], <This is the middle> - <Start measure> = 0.433048 ms, <End measure> - <This is the middle> = 0.165432 ms, Full time: 0.598480 ms


  **** Measures with cache enabled ****

0.786120 ms:TraceDebug.cpp:289 (main) [main]  Start measure
  0.797677 ms: Processing f1()  From TraceDebug.cpp:290 (main)
    0.809260 ms:TraceDebug.cpp:270 (f1) [f1]  Start measure
      0.818241 ms: Processing f2() - 1  From TraceDebug.cpp:271 (f1)
        0.827950 ms:TraceDebug.cpp:265 (f2) [f2]  Start measure
          0.838968 ms TraceDebug.cpp:151 (CacheOrPrintOutputs)  INFO: Reserved more space for cache. Now set to 10
          0.853475 ms: Processing f3()  From TraceDebug.cpp:266 (f2)
            0.864193 ms:TraceDebug.cpp:259 (f3) [f3]  Start measure
              0.871838 ms TraceDebug.cpp:261 (f3)  a = 5
           (Full elapsed: 0.466746 ms) TraceDebug.cpp:259 (f3) [f3], <End measure> - <Start measure> = 0.018476 ms
           (Full elapsed: 0.475647 ms) TraceDebug.cpp:259 (f3) [f3], <End measure> - <Start measure> = 0.018476 ms
           (Full elapsed: 0.587147 ms) TraceDebug.cpp:259 (f3) [f3], <End measure> - <Start measure> = 0.018476 ms, <*** WARNING:Cache was resized when displaying trace informations. This resize inducted 0.015796 ms overhead in this measure !!!***)> - <End measure> = 0.010625 ms, <Start Printing cache> - <*** WARNING:Cache was resized when displaying trace informations. This resize inducted 0.015796 ms overhead in this measure !!!***)> = 0.000000 ms, <Done Printing cache> - <Start Printing cache> = 0.109265 ms, Full time: 0.138366 ms
          1.070623 ms ->TraceDebug.cpp:266 (f2)  f3() = 5
       (Full elapsed: 0.666961 ms) TraceDebug.cpp:265 (f2) [f2], <End measure> - <(***!!! Printing inducted 0.109265 ms overhead in this measure !!!***)Start measure> = 0.254871 ms
      1.091738 ms ->TraceDebug.cpp:271 (f1)  f2() - 1 = 1
   (Full elapsed: 0.685788 ms) TraceDebug.cpp:270 (f1) [f1], <End measure> - <(***!!! Printing inducted 0.109265 ms overhead in this measure !!!***)Start measure> = 0.292859 ms
  1.109616 ms ->TraceDebug.cpp:290 (main)  f1() = 1
    1.122439 ms:TraceDebug.cpp:265 (f2) [f2]  Start measure
      1.131981 ms: Processing f3()  From TraceDebug.cpp:266 (f2)
        1.141266 ms:TraceDebug.cpp:259 (f3) [f3]  Start measure
          1.148679 ms TraceDebug.cpp:261 (f3)  a = 5
       (Full elapsed: 0.742472 ms) TraceDebug.cpp:259 (f3) [f3], <End measure> - <Start measure> = 0.017387 ms
       (Full elapsed: 0.750460 ms) TraceDebug.cpp:259 (f3) [f3], <End measure> - <Start measure> = 0.017387 ms
       (Full elapsed: 0.856408 ms) TraceDebug.cpp:259 (f3) [f3], <End measure> - <Start measure> = 0.017387 ms, <Start Printing cache> - <End measure> = 0.009101 ms, <Done Printing cache> - <Start Printing cache> = 0.105123 ms, Full time: 0.131611 ms
      1.321475 ms ->TraceDebug.cpp:266 (f2)  f3() = 5
   (Full elapsed: 0.916761 ms) TraceDebug.cpp:265 (f2) [f2], <End measure> - <(***!!! Printing inducted 0.105123 ms overhead in this measure !!!***)Start measure> = 0.210145 ms
 (Full elapsed: 0.930947 ms) TraceDebug.cpp:289 (main) [main], <(***!!! Printing inducted 0.105123 ms overhead in this measure !!!***)This is the middle> - <(***!!! Printing inducted 0.105123 ms overhead in this measure !!!***)(***!!! Printing inducted 0.109265 ms overhead in this measure !!!***)Start measure> = 0.331769 ms, <End measure> - <(***!!! Printing inducted 0.105123 ms overhead in this measure !!!***)This is the middle> = 0.229509 ms, Full time: 0.561278 ms
 (Full elapsed: 0.944264 ms) TraceDebug.cpp:289 (main) [main], <(***!!! Printing inducted 0.105123 ms overhead in this measure !!!***)This is the middle> - <(***!!! Printing inducted 0.105123 ms overhead in this measure !!!***)(***!!! Printing inducted 0.109265 ms overhead in this measure !!!***)Start measure> = 0.331769 ms, <End measure> - <(***!!! Printing inducted 0.105123 ms overhead in this measure !!!***)This is the middle> = 0.229509 ms, Full time: 0.561278 ms
 (Full elapsed: 1.007400 ms) TraceDebug.cpp:289 (main) [main], <(***!!! Printing inducted 0.105123 ms overhead in this measure !!!***)This is the middle> - <(***!!! Printing inducted 0.105123 ms overhead in this measure !!!***)(***!!! Printing inducted 0.109265 ms overhead in this measure !!!***)Start measure> = 0.331769 ms, <End measure> - <(***!!! Printing inducted 0.105123 ms overhead in this measure !!!***)This is the middle> = 0.229509 ms, <Start Printing cache> - <End measure> = 0.014557 ms, <Done Printing cache> - <Start Printing cache> = 0.060623 ms, Full time: 0.636458 ms
 ```

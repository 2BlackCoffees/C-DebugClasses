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
1469435661722.346680ms:TraceDebug.cpp:275 (main) [main]  Start measure
  1469435661722.382812ms:Processing f1()  From TraceDebug.cpp:276 (main)
    1469435661722.399658ms:TraceDebug.cpp:266 (f1) [f1]  Start measure
      1469435661722.409912ms:Processing f2() - 1  From TraceDebug.cpp:267 (f1)
        1469435661722.419922ms:TraceDebug.cpp:261 (f2) [f2]  Start measure
          1469435661722.428223ms:Processing f3()  From TraceDebug.cpp:262 (f2)
            1469435661722.437256ms:TraceDebug.cpp:255 (f3) [f3]  Start measure
              1469435661722.449219ms:TraceDebug.cpp:257 (f3)  a = 5
            1469435661722.462646ms:TraceDebug.cpp:255 (f3) [f3], <End measure> - <Start measure> = 0.025036ms
          1469435661722.479736ms:->TraceDebug.cpp:262 (f2)  f3() = 5
        1469435661722.489258ms:TraceDebug.cpp:261 (f2) [f2], <End measure> - <Start measure> = 0.070508ms
      1469435661722.501709ms:->TraceDebug.cpp:267 (f1)  f2() - 1 = 1
    1469435661722.510986ms:TraceDebug.cpp:266 (f1) [f1], <End measure> - <Start measure> = 0.113042ms
  1469435661722.520264ms:->TraceDebug.cpp:276 (main)  f1() = 1
    1469435661722.531006ms:TraceDebug.cpp:261 (f2) [f2]  Start measure
      1469435661722.540039ms:Processing f3()  From TraceDebug.cpp:262 (f2)
        1469435661722.549561ms:TraceDebug.cpp:255 (f3) [f3]  Start measure
          1469435661722.557129ms:TraceDebug.cpp:257 (f3)  a = 5
        1469435661722.565918ms:TraceDebug.cpp:255 (f3) [f3], <End measure> - <Start measure> = 0.016571ms
      1469435661722.573730ms:->TraceDebug.cpp:262 (f2)  f3() = 5
    1469435661722.581787ms:TraceDebug.cpp:261 (f2) [f2], <End measure> - <Start measure> = 0.051237ms
1469435661722.592041ms:TraceDebug.cpp:275 (main) [main], <This is the middle> - <Start measure> = 0.184564ms, <End measure> - <This is the middle> = 0.064162ms, Full time: 0.248726ms


  **** Measures with cache enabled ****

1469435661722.615479ms:TraceDebug.cpp:285 (main) [main]  Start measure
  1469435661722.620605ms:Processing f1()  From TraceDebug.cpp:286 (main)
    1469435661722.625488ms:TraceDebug.cpp:266 (f1) [f1]  Start measure
      1469435661722.629639ms:Processing f2() - 1  From TraceDebug.cpp:267 (f1)
        1469435661722.634033ms:TraceDebug.cpp:261 (f2) [f2]  Start measure
          1469435661722.639160ms:TraceDebug.cpp:151 (CacheOrPrintOutputs)  INFO: Reserved more space for cache. Now set to 10
          1469435661722.645020ms:Processing f3()  From TraceDebug.cpp:262 (f2)
            1469435661722.650146ms:TraceDebug.cpp:255 (f3) [f3]  Start measure
              1469435661722.653809ms:TraceDebug.cpp:257 (f3)  a = 5
            1469435661722.657959ms:TraceDebug.cpp:255 (f3) [f3], <End measure> - <Start measure> = 0.008190ms
            1469435661722.662109ms:TraceDebug.cpp:255 (f3) [f3], <End measure> - <Start measure> = 0.008190ms
            1469435661722.706055ms:TraceDebug.cpp:255 (f3) [f3], <End measure> - <Start measure> = 0.008190ms, <*** WARNING:Cache was resized when displaying trace informations. This resize inducted 0.006613ms overhead in this measure !!!***)> - <End measure> = 0.004783ms, <Start Printing cache> - <*** WARNING:Cache was resized when displaying trace informations. This resize inducted 0.006613ms overhead in this measure !!!***)> = 0.000000ms, <Done Printing cache> - <Start Printing cache> = 0.043055ms, Full time: 0.056028ms
          1469435661722.731689ms:->TraceDebug.cpp:262 (f2)  f3() = 5
        1469435661722.736328ms:TraceDebug.cpp:261 (f2) [f2], <End measure> - <(***!!! Printing inducted 0.043055ms overhead in this measure !!!***)Start measure> = 0.102570ms
      1469435661722.740723ms:->TraceDebug.cpp:267 (f1)  f2() - 1 = 1
    1469435661722.746094ms:TraceDebug.cpp:266 (f1) [f1], <End measure> - <(***!!! Printing inducted 0.043055ms overhead in this measure !!!***)Start measure> = 0.120719ms
  1469435661722.750244ms:->TraceDebug.cpp:286 (main)  f1() = 1
    1469435661722.755859ms:TraceDebug.cpp:261 (f2) [f2]  Start measure
      1469435661722.760498ms:Processing f3()  From TraceDebug.cpp:262 (f2)
        1469435661722.764893ms:TraceDebug.cpp:255 (f3) [f3]  Start measure
          1469435661722.768311ms:TraceDebug.cpp:257 (f3)  a = 5
        1469435661722.772461ms:TraceDebug.cpp:255 (f3) [f3], <End measure> - <Start measure> = 0.007834ms
        1469435661722.776123ms:TraceDebug.cpp:255 (f3) [f3], <End measure> - <Start measure> = 0.007834ms
        1469435661722.816650ms:TraceDebug.cpp:255 (f3) [f3], <End measure> - <Start measure> = 0.007834ms, <Start Printing cache> - <End measure> = 0.004138ms, <Done Printing cache> - <Start Printing cache> = 0.040124ms, Full time: 0.052096ms

 ```

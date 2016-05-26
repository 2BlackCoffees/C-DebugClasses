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
  START_TRACE_PERFORMANCE(main);
  DISPLAY_DEBUG_VALUE(f1());
  ADD_TRACE_PERFORMANCE(main, "This is the middle");
  f2();
}
```

Provides following output:
```
Processing f1()  From TraceDebug.cpp:183 (main)

    Processing f2() - 1  From TraceDebug.cpp:177 (f1)

        Processing f3()  From TraceDebug.cpp:172 (f2)

            TraceDebug.cpp:167 (f3)  a = 5

         (Full elapsed: 0.000000 ms) TraceDebug.cpp:165 (f3) [f3], <End measure> - <Start measure> = 2.000200 ms

        ->TraceDebug.cpp:172 (f2)  f3() = 5

     (Full elapsed: 5.000500 ms) TraceDebug.cpp:171 (f2) [f2], <End measure> - <Start measure> = 9.000900 ms

    ->TraceDebug.cpp:177 (f1)  f2() - 1 = 1

 (Full elapsed: 11.001100 ms) TraceDebug.cpp:176 (f1) [f1], <End measure> - <Start measure> = 17.001700 ms

->TraceDebug.cpp:183 (main)  f1() = 1

    Processing f3()  From TraceDebug.cpp:172 (f2)

        TraceDebug.cpp:167 (f3)  a = 5

     (Full elapsed: 20.002000 ms) TraceDebug.cpp:165 (f3) [f3], <End measure> - <Start measure> = 2.000200 ms

    ->TraceDebug.cpp:172 (f2)  f3() = 5

 (Full elapsed: 25.002500 ms) TraceDebug.cpp:171 (f2) [f2], <End measure> - <Start measure> = 9.000900 ms

(Full elapsed: 29.002900 ms) TraceDebug.cpp:182 (main) [main], <This is the middle> - <Start measure> = 25.002500 ms, <End measure> - <This is the middle> = 13.001300 ms, Full time: 38.003800 ms
```

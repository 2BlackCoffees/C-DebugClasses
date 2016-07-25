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

 ****  Without cache enabled **** 

1469454756441.994385ms:139661736146816:TraceDebug.cpp:280 (test) [test]  Start measure
1469454756442.174072ms:139661719353088:TraceDebug.cpp:273 (f1) [f1]  Start measure
  1469454756442.119385ms:139661736146816:Processing f1()  From TraceDebug.cpp:281 (test)
    1469454756442.380371ms:139661736146816:TraceDebug.cpp:273 (f1) [f1]  Start measure
  1469454756442.276367ms:139661719353088:Processing f2() - 1  From TraceDebug.cpp:274 (f1)
    1469454756442.548584ms:139661719353088:TraceDebug.cpp:268 (f2) [f2]  Start measure
      1469454756442.451172ms:139661736146816:Processing f2() - 1  From TraceDebug.cpp:274 (f1)
        1469454756442.684570ms:139661736146816:TraceDebug.cpp:268 (f2) [f2]  Start measure
          1469454756442.746094ms:139661736146816:Processing f3()  From TraceDebug.cpp:269 (f2)
            1469454756442.807373ms:139661736146816:TraceDebug.cpp:262 (f3) [f3]  Start measure
              1469454756442.854736ms:139661736146816:TraceDebug.cpp:264 (f3)  a = 5
            1469454756442.944092ms:139661736146816:TraceDebug.cpp:262 (f3) [f3], <End measure> - <Start measure> = 0.138534ms
          1469454756443.015625ms:139661736146816:->TraceDebug.cpp:269 (f2)  f3() = 5
        1469454756443.069336ms:139661736146816:TraceDebug.cpp:268 (f2) [f2], <Start measure> - <Start measure> = 0.148332ms, <End measure> - <Start measure> = 0.383801ms, Full time: 0.532133ms
      1469454756442.628174ms:139661719353088:Processing f3()  From TraceDebug.cpp:269 (f2)
        1469454756443.235596ms:139661719353088:TraceDebug.cpp:262 (f3) [f3]  Start measure
          1469454756443.310791ms:139661719353088:TraceDebug.cpp:264 (f3)  a = 5
        1469454756443.375732ms:139661719353088:TraceDebug.cpp:262 (f3) [f3], <End measure> - <Start measure> = 0.146252ms
      1469454756443.432617ms:139661719353088:->TraceDebug.cpp:269 (f2)  f3() = 5
    1469454756443.494629ms:139661719353088:TraceDebug.cpp:268 (f2) [f2]Not enough trace to display results.
  1469454756443.537842ms:139661719353088:->TraceDebug.cpp:274 (f1)  f2() - 1 = 1
      1469454756443.129883ms:139661736146816:->TraceDebug.cpp:274 (f1)  f2() - 1 = 1
    1469454756443.652832ms:139661736146816:TraceDebug.cpp:273 (f1) [f1], <Start measure> - <Start measure> = 0.212976ms, <End measure> - <Start measure> = 1.271733ms, Full time: 1.484709ms
  1469454756443.741943ms:139661736146816:->TraceDebug.cpp:281 (test)  f1() = 1
    1469454756443.818604ms:139661736146816:TraceDebug.cpp:268 (f2) [f2]  Start measure
      1469454756443.872314ms:139661736146816:Processing f3()  From TraceDebug.cpp:269 (f2)
        1469454756443.927002ms:139661736146816:TraceDebug.cpp:262 (f3) [f3]  Start measure
1469454756444.010498ms:139661719353088:TraceDebug.cpp:273 (f1) [f1]Not enough trace to display results.
          1469454756443.977783ms:139661736146816:TraceDebug.cpp:264 (f3)  a = 5
        1469454756444.150391ms:139661736146816:TraceDebug.cpp:262 (f3) [f3], <End measure> - <Start measure> = 0.224016ms
      1469454756444.203369ms:139661736146816:->TraceDebug.cpp:269 (f2)  f3() = 5
    1469454756444.249512ms:139661736146816:TraceDebug.cpp:268 (f2) [f2], <End measure> - <Start measure> = 0.433730ms
1469454756444.309814ms:139661736146816:TraceDebug.cpp:280 (test) [test], <This is the middle> - <Start measure> = 1.811563ms, <End measure> - <This is the middle> = 0.511786ms, Full time: 2.323349ms
Closing scope


 ****  With cache enabled **** 

1469454756444.458984ms:139661736146816:TraceDebug.cpp:280 (test) [test]  Start measure
1469454756444.510010ms:139661719353088:TraceDebug.cpp:273 (f1) [f1]  Start measure
  1469454756444.567139ms:139661719353088:Processing f2() - 1  From TraceDebug.cpp:274 (f1)
  1469454756444.571045ms:139661736146816:Processing f1()  From TraceDebug.cpp:281 (test)
    1469454756444.611328ms:139661736146816:TraceDebug.cpp:273 (f1) [f1]  Start measure
    1469454756444.669434ms:139661719353088:TraceDebug.cpp:268 (f2) [f2]  Start measure
      1469454756444.640869ms:139661736146816:Processing f2() - 1  From TraceDebug.cpp:274 (f1)
        1469454756444.720703ms:139661736146816:TraceDebug.cpp:268 (f2) [f2]  Start measure
      1469454756444.708984ms:139661719353088:Processing f3()  From TraceDebug.cpp:269 (f2)
        1469454756444.766357ms:139661719353088:TraceDebug.cpp:262 (f3) [f3]  Start measure
          1469454756444.751465ms:139661736146816:Processing f3()  From TraceDebug.cpp:269 (f2)
            1469454756444.812988ms:139661736146816:TraceDebug.cpp:262 (f3) [f3]  Start measure
          1469454756444.797119ms:139661719353088:TraceDebug.cpp:264 (f3)  a = 5
        1469454756444.861816ms:139661719353088:TraceDebug.cpp:262 (f3) [f3], <Start measure> - <Start measure> = 0.048843ms, <End measure> - <Start measure> = 0.044643ms, Full time: 0.093486ms
              1469454756444.841553ms:139661736146816:TraceDebug.cpp:264 (f3)  a = 5
            1469454756444.930908ms:139661736146816:TraceDebug.cpp:262 (f3) [f3]Not enough trace to display results.
      1469454756444.907959ms:139661719353088:->TraceDebug.cpp:269 (f2)  f3() = 5
    1469454756444.974609ms:139661719353088:TraceDebug.cpp:268 (f2) [f2], <Start measure> - <Start measure> = 0.056501ms, <End measure> - <Start measure> = 0.251670ms, Full time: 0.308171ms
          1469454756444.956787ms:139661736146816:->TraceDebug.cpp:269 (f2)  f3() = 5
        1469454756445.034424ms:139661736146816:TraceDebug.cpp:268 (f2) [f2]Not enough trace to display results.
  1469454756445.013672ms:139661719353088:->TraceDebug.cpp:274 (f1)  f2() - 1 = 1
1469454756445.075684ms:139661719353088:TraceDebug.cpp:273 (f1) [f1], <Start measure> - <Start measure> = 0.108841ms, <End measure> - <Start measure> = 0.463549ms, Full time: 0.572390ms
      1469454756445.058594ms:139661736146816:->TraceDebug.cpp:274 (f1)  f2() - 1 = 1
    1469454756445.131836ms:139661736146816:TraceDebug.cpp:273 (f1) [f1]Not enough trace to display results.
  1469454756445.150635ms:139661736146816:->TraceDebug.cpp:281 (test)  f1() = 1
    1469454756445.178223ms:139661736146816:TraceDebug.cpp:268 (f2) [f2]  Start measure
      1469454756445.200439ms:139661736146816:Processing f3()  From TraceDebug.cpp:269 (f2)
        1469454756445.223877ms:139661736146816:TraceDebug.cpp:262 (f3) [f3]  Start measure
          1469454756445.246094ms:139661736146816:TraceDebug.cpp:264 (f3)  a = 5
        1469454756445.270508ms:139661736146816:TraceDebug.cpp:262 (f3) [f3], <End measure> - <Start measure> = 0.047996ms
      1469454756445.296143ms:139661736146816:->TraceDebug.cpp:269 (f2)  f3() = 5
    1469454756445.319336ms:139661736146816:TraceDebug.cpp:268 (f2) [f2], <End measure> - <Start measure> = 0.141516ms
1469454756445.349854ms:139661736146816:TraceDebug.cpp:280 (test) [test], <This is the middle> - <Start measure> = 0.714934ms, <End measure> - <This is the middle> = 0.179659ms, Full time: 0.894593ms

 ```

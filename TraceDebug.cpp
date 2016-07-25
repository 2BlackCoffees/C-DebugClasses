// //////////////////////////////////////////////////////////////////////
//
// This file is under the license http://www.apache.org/licenses/LICENSE-2.0
//
// //////////////////////////////////////////////////////////////////////

#include "TraceDebug.hpp"
#ifdef ENABLE_TRACE_DEBUG
// ==============================================================================================================================
#ifdef ENABLE_THREAD_SAFE
std::map<std::thread::id, unsigned int>                                 DebugTrace::debugPrintDeepness;
std::recursive_mutex                                                    DebugTrace::the_mutex;
#else
unsigned int                                                            DebugTrace::debugPrintDeepness = 0;
#endif
#ifdef WRITE_OUTPUT_TO_FILE
std::ofstream                                                           DebugTrace::outputFile;
#endif
unsigned int                                                            DebugTrace::traceCacheDeepness = 0;
std::atomic<double>                                                     DebugTrace::reserveTime(0);
bool                                                                    DebugTrace::traceActive = true;
bool                                                                    DebugTrace::displayStartTracePerformance = true;
std::vector<std::string>                                                DebugTrace::localCache;
std::map<std::string, int>                                              DebugTrace::mapFileNameToLine;
std::map<std::string,
         std::vector<std::pair<std::string,
                               std::chrono::steady_clock::time_point>>> DebugTrace::mapFileNameFunctionNameToVectorTimingInfo;

// ==============================================================================================================================
std::string DebugTrace::GetUniqueKey(const std::string & string1,
                                     const std::string & string2,
                                     const std::string & string3) {
  return std::move(string1 + string2 + string3);
}

// ==============================================================================================================================
DebugTrace::DebugTrace(const std::string & functionName, const std::string & fileName,
                       int lineNumber, const std::string & uniqueKey): debugPerformanceMustBeDisplayed(true) {
  keyDebugPerformanceToErase = GetUniqueKey(fileName, functionName, uniqueKey);
  GET_THREAD_SAFE_GUARD;
  IncreaseDebugPrintDeepness();
  lineHeader = fileName + ":" + std::to_string(lineNumber) + " (" + functionName + ") [" + uniqueKey + "]";

  // Automatically add a trace point when constructor is called
  const std::string startMeasure = "Start measure";
  AddTrace(std::chrono::steady_clock::now(), startMeasure);
  if(displayStartTracePerformance) {
    DebugTrace::PrintString(DebugTrace::GetDiffTimeSinceStartAndThreadId() + ":" + lineHeader + "  " + startMeasure, true);
  }
}

// ==============================================================================================================================
DebugTrace::DebugTrace(const std::string & functionName, const std::string & fileName, int lineNumber) {
  std::string uniqueKey = GetUniqueKey(fileName, functionName);
  // If key does not exist or key exists and current line is being accessed
  GET_THREAD_SAFE_GUARD;
  if(mapFileNameToLine.find(uniqueKey) == mapFileNameToLine.end() ||
     mapFileNameToLine[uniqueKey] == lineNumber) {
    // Remember the key name to erase in destructor
    keyDebugPrintToErase = uniqueKey;
    // Save line number
    mapFileNameToLine[uniqueKey] = lineNumber;
    // Increase deepness (this will increase the number of spaces when displaying the output thus giving a more comprehensive
    // path of the call stack being involved)
    IncreaseDebugPrintDeepness();
    // Notify the deepness must be decremented in desctructor (which is not the case for performance measurement)
    debugPrintMustBeDecremented = true;
  }
}

// ==============================================================================================================================
DebugTrace::~DebugTrace() {
  GET_THREAD_SAFE_GUARD;
  if(debugPerformanceMustBeDisplayed) {
    DisplayPerformanceMeasure();
    mapFileNameFunctionNameToVectorTimingInfo.erase(keyDebugPerformanceToErase);
  }

  auto getAllDebugPrintDeepness = GetAllDebugPrintDeepness();
  if((debugPrintMustBeDecremented || debugPerformanceMustBeDisplayed) && GetDebugPrintDeepness() > 0) {
    DecreaseDebugPrintDeepness();
    if(debugPrintMustBeDecremented) {
      mapFileNameToLine.erase(keyDebugPrintToErase);
    }
  }

  if(GetAllDebugPrintDeepness() == 0) {
    mapFileNameToLine.clear();
    traceCacheDeepness = 0;
    CacheOrPrintTimings("Closing scope");
#ifdef WRITE_OUTPUT_TO_FILE
    outputFile.close();
#endif
  }
}

// ==============================================================================================================================
void DebugTrace::DisplayPerformanceMeasure() {
  // Automatically an end of measure trace points when getting out of scope
  AddTrace(std::chrono::steady_clock::now(), "End measure");
  CacheOrPrintTimings(GetPerformanceResults());
}

// ==============================================================================================================================
void DebugTrace::CacheOrPrintTimings(std::string&& output) {
  // Is the cache enabled ?
  if(traceCacheDeepness > 1) {
    localCache.push_back(output);
    // Print all cache information when maximum cache size happened
    if(localCache.size() > traceCacheDeepness - 1 || GetDebugPrintDeepness() <= 0) {
      auto startPrintingCacheTime = std::chrono::steady_clock::now();
      localCache.push_back(GetPerformanceResults());
      for(const std::string & str: localCache) {
        PRINT_RESULT(str);
      }
      localCache.clear();
      if(reserveTime > 0) {
        AddTrace(startPrintingCacheTime, "*** WARNING:Cache was resized when displaying trace informations. This resize inducted " +
                 std::to_string(reserveTime) + std::string(UNIT_TRACE_DEBUG)+ " overhead in this measure !!!***)");
        reserveTime = 0;
      }
      // Update all still existing trace points that their measures will be impacted because of the cache display
      AddTrace(startPrintingCacheTime, "Start Printing cache");
      auto endPrintingCacheTime = std::chrono::steady_clock::now();
      AddTrace(endPrintingCacheTime, "Done Printing cache");
      PRINT_RESULT(GetPerformanceResults());
      for(auto& tmpPair: mapFileNameFunctionNameToVectorTimingInfo) {
        for(auto& pairElement: tmpPair.second) {
          pairElement.first = "(***!!! Printing inducted " +
                               std::to_string(std::chrono::duration <double, UNIT_TRACE_TEMPLATE_TYPE> (
                                   endPrintingCacheTime - startPrintingCacheTime).count()) +
                               std::string(UNIT_TRACE_DEBUG)+ " overhead in this measure !!!***)" + pairElement.first;
        }
      }
    }
  } else {
    if(localCache.size() > 0) {
      for(const std::string & str: localCache) {
        PRINT_RESULT(str);
      }
      localCache.clear();
    }
    // Display results without caching information
    PRINT_RESULT(output);
  }
}

// ==============================================================================================================================
void DebugTrace::CacheOrPrintOutputs(std::string&& output) {
  // Is the cache enabled ?
  if(traceCacheDeepness > 1) {
    localCache.push_back(output);
    if(localCache.size() > traceCacheDeepness - 1) {
      auto startResizing = std::chrono::steady_clock::now();
      if(SetTracePerformanceCacheDeepness(traceCacheDeepness * 2)) {
        DISPLAY_DEBUG_MESSAGE("INFO: Reserved more space for cache. Now set to " + std::to_string(traceCacheDeepness));
      }
      reserveTime = reserveTime + std::chrono::duration <double, UNIT_TRACE_TEMPLATE_TYPE> (
                                         std::chrono::steady_clock::now() - startResizing).count();
    }
  } else {
    // Display results without caching information
    PRINT_RESULT(output);
  }

}

// ==============================================================================================================================
void DebugTrace::AddTrace(std::chrono::steady_clock::time_point timePoint, const std::string & variableName) {

  GET_THREAD_SAFE_GUARD;
  std::pair<std::string, std::chrono::steady_clock::time_point> tmpPair = std::make_pair(variableName, timePoint);

  auto vectorTimingInfoIt = mapFileNameFunctionNameToVectorTimingInfo.find(keyDebugPerformanceToErase);

  if(vectorTimingInfoIt == mapFileNameFunctionNameToVectorTimingInfo.end()) {
    std::vector<std::pair<std::string, std::chrono::steady_clock::time_point>> tmpVector;
    tmpVector.push_back(tmpPair);
    mapFileNameFunctionNameToVectorTimingInfo[keyDebugPerformanceToErase] = std::move(tmpVector);
  } else {
    vectorTimingInfoIt->second.push_back(tmpPair);
  }

}

// ==============================================================================================================================
void DebugTrace::ActiveTrace(bool activate) {
  GET_THREAD_SAFE_GUARD;
  traceActive = activate;
}

// ==============================================================================================================================
bool DebugTrace::IsTraceActive() {
  GET_THREAD_SAFE_GUARD;
  return traceActive;
}

// ==============================================================================================================================
std::string DebugTrace::getSpaces() {
  if(GetDebugPrintDeepness() > 1) {
    return std::string(2 * (GetDebugPrintDeepness() - 1), ' ');
  }
  return "";
}

// ==============================================================================================================================
void DebugTrace::PrintString(const std::string & inStr, bool showHierarchy) {
  GET_THREAD_SAFE_GUARD;
  std::string str;
  if(showHierarchy) {
    str = DebugTrace::getSpaces() + inStr;
  } else {
    str = inStr;
  }
  CacheOrPrintOutputs(std::move(str));
}

// ==============================================================================================================================
std::string DebugTrace::GetPerformanceResults() {
  auto performanceInfos = mapFileNameFunctionNameToVectorTimingInfo[keyDebugPerformanceToErase];

  std::string tmp = DebugTrace::getSpaces() + DebugTrace::GetDiffTimeSinceStartAndThreadId() + ":" + lineHeader;
  auto size = performanceInfos.size() - 1;

  if(size > 0) {
    for(decltype(size) index = 0; index < size; ++index)
    {
      const auto& valueMin = performanceInfos[index];
      const auto& valueMax = performanceInfos[index + 1];
      if(!tmp.empty())
      {
        tmp += ", ";
      }
      tmp += "<" + valueMax.first + "> - <" + valueMin.first + "> = " +
          std::to_string(std::chrono::duration <double, UNIT_TRACE_TEMPLATE_TYPE> (valueMax.second - valueMin.second).count()) +
          std::string(UNIT_TRACE_DEBUG);
    }
    if(size > 1)
    {
      const auto& valueMin = performanceInfos[0];
      const auto& valueMax = performanceInfos[size];
      tmp += ", Full time: " +
          std::to_string(std::chrono::duration <double, UNIT_TRACE_TEMPLATE_TYPE> (valueMax.second - valueMin.second).count()) +
          std::string(UNIT_TRACE_DEBUG);
    }
  } else {
    tmp += "Not enough trace to display results.";
  }
  return tmp;
}

// ==============================================================================================================================
// ==============================================================================================================================
// ==============================================================================================================================

// Compile with MSVC2013: 
// cl /EHsc TraceDebug.cpp
// Compile with gcc: 
// g++ -std=c++11 -o TraceDebug TraceDebug.cpp -pthread
#ifdef TRACE_DEBUG_HPP_DEBUG_LOCAL
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

/* This program will output:

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
*/
#endif
#endif

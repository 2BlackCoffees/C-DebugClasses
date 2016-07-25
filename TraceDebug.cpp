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

  if(getAllDebugPrintDeepness == 0) {
    mapFileNameToLine.clear();
  }
#ifdef WRITE_OUTPUT_TO_FILE
  if(getAllDebugPrintDeepness == 0) {
    outputFile.close();
  }
#endif
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
      traceCacheDeepness *= 2;
      localCache.reserve(traceCacheDeepness);
      DISPLAY_DEBUG_MESSAGE("INFO: Reserved more space for cache. Now set to " + std::to_string(traceCacheDeepness));
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
// g++ -std=c++11 -o TraceDebug TraceDebug.cpp
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
int main()
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

/* This program will output:
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
*/
#endif
#endif

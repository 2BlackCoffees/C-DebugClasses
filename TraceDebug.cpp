// //////////////////////////////////////////////////////////////////////
//
// This file is under the license http://www.apache.org/licenses/LICENSE-2.0
//
// //////////////////////////////////////////////////////////////////////

#include "TraceDebug.hpp"
#ifdef ENABLE_TRACE_DEBUG
// ==============================================================================================================================
int                                                                     DebugTrace::debugPrintDeepness = 0;
uint64_t                                                                DebugTrace::traceCacheDeepness = 500;
std::atomic<double>                                                     DebugTrace::reserveTime = 0;
bool                                                                    DebugTrace::traceActive = true;
std::vector<std::string>                                                DebugTrace::localCache;
std::map<std::string, int>                                              DebugTrace::mapFileNameToLine;
std::map<std::string,
         std::vector<std::pair<std::string,
                               std::chrono::system_clock::time_point>>> DebugTrace::mapFileNameFunctionNameToVectorTimingInfo;

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
  ++debugPrintDeepness;
  lineHeader = fileName + ":" + std::to_string(lineNumber) + " (" + functionName + ") [" + uniqueKey + "]";

  // Automatically add a trace point when constructor is called
  AddTrace(std::chrono::steady_clock::now(), "Start measure");
}

// ==============================================================================================================================
DebugTrace::DebugTrace(const std::string & functionName, const std::string & fileName, int lineNumber) {
  std::string uniqueKey = GetUniqueKey(fileName, functionName);
  // If key does not exist or key exists and current line is being accessed
  if(mapFileNameToLine.find(uniqueKey) == mapFileNameToLine.end() ||
     mapFileNameToLine[uniqueKey] == lineNumber) {
    // Remember the key name to erase in destructor
    keyDebugPrintToErase = uniqueKey;
    // Save line number
    mapFileNameToLine[uniqueKey] = lineNumber;
    // Increase deepness (this will increase the number of spaces when displaying the output thus giving a more comprehensive
    // path of the call stack being involved)
    ++debugPrintDeepness;
    // Notify the deepness must be decremented in desctructor (which is not the case for performance measurement)
    debugPrintMustBeDecremented = true;
  }
}

// ==============================================================================================================================
DebugTrace::~DebugTrace() {
  if((debugPrintMustBeDecremented || debugPerformanceMustBeDisplayed) && debugPrintDeepness > 0) {
    --debugPrintDeepness;
    if(debugPrintMustBeDecremented) {
      mapFileNameToLine.erase(keyDebugPrintToErase);
    }
  }
  if(debugPrintDeepness == 0) {
    mapFileNameToLine.clear();
  }
  if(debugPerformanceMustBeDisplayed) {
    DisplayPerformanceMeasure();
    mapFileNameFunctionNameToVectorTimingInfo.erase(keyDebugPerformanceToErase);
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
    if(localCache.size() > traceCacheDeepness - 1 || debugPrintDeepness <= 0) {
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
void DebugTrace::AddTrace(std::chrono::system_clock::time_point timePoint, const std::string & variableName) {

  std::pair<std::string, std::chrono::system_clock::time_point> tmpPair = std::make_pair(variableName, timePoint);

  auto vectorTimingInfoIt = mapFileNameFunctionNameToVectorTimingInfo.find(keyDebugPerformanceToErase);

  if(vectorTimingInfoIt == mapFileNameFunctionNameToVectorTimingInfo.end()) {
    std::vector<std::pair<std::string, std::chrono::system_clock::time_point>> tmpVector;
    tmpVector.push_back(tmpPair);
    mapFileNameFunctionNameToVectorTimingInfo[keyDebugPerformanceToErase] = std::move(tmpVector);
  } else {
    vectorTimingInfoIt->second.push_back(tmpPair);
  }

}

// ==============================================================================================================================
void DebugTrace::ActiveTrace(bool activate) {
  traceActive = activate;
}

// ==============================================================================================================================
bool DebugTrace::IsTraceActive() {
  return traceActive;
}

// ==============================================================================================================================
std::string DebugTrace::getSpaces() {
  if(debugPrintDeepness > 1) {
    return std::string(2 * (debugPrintDeepness - 1), ' ');
  }
  return "";
}

// ==============================================================================================================================
void DebugTrace::PrintString(const std::string & inStr, bool showHierarchy) {
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
  static auto firstAccess = std::chrono::steady_clock::now();
  auto now = std::chrono::steady_clock::now();
  auto performanceInfos = mapFileNameFunctionNameToVectorTimingInfo[keyDebugPerformanceToErase];

  std::string tmp = DebugTrace::getSpaces() + " (Full elapsed: " +
      std::to_string(std::chrono::duration <double, UNIT_TRACE_TEMPLATE_TYPE> (now - firstAccess).count()) +
      std::string(UNIT_TRACE_DEBUG) + ") " + lineHeader;
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

// Compile with MSVC2013: cl /EHsc TraceDebug.cpp
#define TRACE_DEBUG_HPP_DEBUG_LOCAL
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

/* This program will output:
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

*/
#endif
#endif

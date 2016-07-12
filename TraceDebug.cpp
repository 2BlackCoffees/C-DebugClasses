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
std::chrono::time_point<std::chrono::system_clock>                      DebugTrace::startingTime =
                                                                                               std::chrono::system_clock::now();
unsigned int                                                            DebugTrace::traceCacheDeepness = 0;
std::atomic<double>                                                     DebugTrace::reserveTime(0);
bool                                                                    DebugTrace::traceActive = true;
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
  AddTrace(std::chrono::steady_clock::now(), "Start measure");
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
  if(debugPerformanceMustBeDisplayed) {
    DisplayPerformanceMeasure();
    mapFileNameFunctionNameToVectorTimingInfo.erase(keyDebugPerformanceToErase);
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

*/
#endif
#endif

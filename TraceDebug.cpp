// //////////////////////////////////////////////////////////////////////
//
// This file is under the license http://www.apache.org/licenses/LICENSE-2.0
//
// //////////////////////////////////////////////////////////////////////

#include "TraceDebug.hpp"
#ifdef ENABLE_TRACE_DEBUG
// ==============================================================================================================================
#ifdef ENABLE_THREAD_SAFE
std::map<std::thread::id, unsigned int>                                 TraceDebug::debugPrintDeepness;
std::recursive_mutex                                                    TraceDebug::the_mutex;
#else
unsigned int                                                            TraceDebug::debugPrintDeepness = 0;
#endif
#ifdef WRITE_OUTPUT_TO_FILE
std::ofstream                                                           TraceDebug::outputFile;
#endif
unsigned int                                                            TraceDebug::traceCacheDeepness = 0;
bool                                                                    TraceDebug::traceActive = true;
bool                                                                    TraceDebug::displayStartTracePerformance = true;
std::vector<std::string>                                                TraceDebug::localCache;
std::map<std::string, int>                                              TraceDebug::mapFileNameToLine;
std::map<std::string,
         std::vector<std::pair<std::string,
                               std::chrono::steady_clock::time_point>>> TraceDebug::mapFileNameFunctionNameToVectorTimingInfo;
static Guard                                                            guard;

// ==============================================================================================================================
std::string TraceDebug::GetUniqueKey(const std::string & string1,
                                     const std::string & string2,
                                     const std::string & string3) {
  return std::move(string1 + string2 + string3);
}

// ==============================================================================================================================
TraceDebug::TraceDebug(const std::string & functionName, const std::string & fileName,
                       int lineNumber, const std::string & uniqueKey): debugPerformanceMustBeDisplayed(true) {
  keyDebugPerformanceToErase = GetUniqueKey(fileName, functionName, uniqueKey);
  GET_THREAD_SAFE_GUARD;
  IncreaseDebugPrintDeepness();
  lineHeader = fileName + ":" + std::to_string(lineNumber) + " (" + functionName + ") [" + uniqueKey + "]";

  // Automatically add a trace point when constructor is called
  const std::string startMeasure = "Start measure";
  AddTrace(std::chrono::steady_clock::now(), startMeasure);
  if(displayStartTracePerformance) {
    TraceDebug::PrintString(TraceDebug::GetDiffTimeSinceStartAndThreadId() + ":" + lineHeader + "  " + startMeasure, true);
  }
}

// ==============================================================================================================================
TraceDebug::TraceDebug(const std::string & functionName, const std::string & fileName, int lineNumber) {
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
TraceDebug::~TraceDebug() {
  GET_THREAD_SAFE_GUARD;
  // Display performance informations
  if(debugPerformanceMustBeDisplayed) {
    DisplayPerformanceMeasure();
    mapFileNameFunctionNameToVectorTimingInfo.erase(keyDebugPerformanceToErase);
  }

  // Manage hierachy information (number of spaces)
  if((debugPrintMustBeDecremented || debugPerformanceMustBeDisplayed) && GetDebugPrintDeepness() > 0) {
    DecreaseDebugPrintDeepness();
    if(debugPrintMustBeDecremented) {
      mapFileNameToLine.erase(keyDebugPrintToErase);
    }
  }

  if(GetAllDebugPrintDeepness() == 0) {
    mapFileNameToLine.clear();
  }

}

// ==============================================================================================================================
void TraceDebug::DisplayPerformanceMeasure() {
  // Automatically add an end of measure trace points when getting out of scope
  AddTrace(std::chrono::steady_clock::now(), "End measure");
  std::string timingInformation = GetPerformanceResults();
  if(!timingInformation.empty()) CacheOrPrintTimings(std::move(timingInformation));
}

// ==============================================================================================================================
void TraceDebug::CacheOrPrintTimings(std::string&& output) {
  // Is the cache enabled ?
  if(traceCacheDeepness > 1) {
    localCache.push_back(output);
    // Print all cache information when maximum cache size happened
    if(localCache.size() >= traceCacheDeepness) {
      auto startPrintingCacheTime = std::chrono::steady_clock::now();
      localCache.push_back(GetPerformanceResults());
      PrintCache();
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
    PRINT_RESULT(output);
  }
}

// ==============================================================================================================================
void TraceDebug::CacheOrPrintOutputs(std::string&& output) {
  // If the cache is enabled, store output into cache
  // If the cache reached its limit print it out
  if(traceCacheDeepness > 1) {
    localCache.push_back(output);
    if(localCache.size() > traceCacheDeepness - 1) {
      PrintCache();
    }
  } else {
    // Display results without caching information
    PRINT_RESULT(output);
  }

}

// ==============================================================================================================================
void TraceDebug::AddTrace(std::chrono::steady_clock::time_point timePoint, const std::string & variableName) {

  GET_THREAD_SAFE_GUARD;
  // Associate name of variable with time information
  std::pair<std::string, std::chrono::steady_clock::time_point> tmpPair = std::make_pair(variableName, timePoint);

  // Append structure to the map mapFileNameFunctionNameToVectorTimingInfo referenced by the key keyDebugPerformanceToErase
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
void TraceDebug::ActiveTrace(bool activate) {
  GET_THREAD_SAFE_GUARD;
  traceActive = activate;
}

// ==============================================================================================================================
bool TraceDebug::IsTraceActive() {
  GET_THREAD_SAFE_GUARD;
  return traceActive;
}

// ==============================================================================================================================
std::string TraceDebug::getSpaces() {
  if(GetDebugPrintDeepness() > 1) {
    return std::string(2 * (GetDebugPrintDeepness() - 1), ' ');
  }
  return "";
}

// ==============================================================================================================================
void TraceDebug::PrintString(const std::string & inStr, bool showHierarchy) {
  GET_THREAD_SAFE_GUARD;
  std::string str;
  if(showHierarchy) {
    str = TraceDebug::getSpaces() + inStr;
  } else {
    str = inStr;
  }
  CacheOrPrintOutputs(std::move(str));
}

// ==============================================================================================================================
std::string TraceDebug::GetPerformanceResults() {
  // Get performance info for key keyDebugPerformanceToErase
  auto performanceInfos = mapFileNameFunctionNameToVectorTimingInfo[keyDebugPerformanceToErase];

  std::string tmp = TraceDebug::getSpaces() + TraceDebug::GetDiffTimeSinceStartAndThreadId() + ":" + lineHeader;

  // If the number of information stored is greater than 1 a difference can be computed
  auto size = performanceInfos.size() - 1;
  if(size > 0) {
    // Compute all timing differences
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
  } else if (size == 1) {
    // We have 1 timing information only, no difference can be computed
    tmp += ": Not enough trace to display results.";
  } else {
    tmp = "";
  }
  return tmp;
}

// ==============================================================================================================================
void TraceDebug::SetTracePerformanceCacheDeepness(unsigned int cacheDeepness) {
  GET_THREAD_SAFE_GUARD;
  if(cacheDeepness != traceCacheDeepness) {
    traceCacheDeepness = cacheDeepness;
    // Set a little bigger as the time for displaying output will
    // automatically be added.
    localCache.reserve(traceCacheDeepness + 3);
  }
}

// ==============================================================================================================================
void TraceDebug::Finalize() {
  // This method is called by a guard statically created that will automatically expire when the program expires.
  GET_THREAD_SAFE_GUARD;
  TraceDebug::PrintCache();
#ifdef WRITE_OUTPUT_TO_FILE
  if(outputFile.is_open()) outputFile.close();
#endif

}

// ==============================================================================================================================
std::string TraceDebug::GetDiffTimeSinceStartAndThreadId() {
  std::chrono::duration <double, UNIT_TRACE_TEMPLATE_TYPE> elapsedTime =
      std::chrono::system_clock::now().time_since_epoch();
  std::string returnValue = std::to_string(elapsedTime.count()) + UNIT_TRACE_DEBUG;
#ifdef ENABLE_THREAD_SAFE
  std::ostringstream buffer;
  buffer << std::this_thread::get_id();
  returnValue += ":" + buffer.str();
#endif
  return returnValue;
}

// ==============================================================================================================================
void TraceDebug::DisplayStartTracePerformance(bool inDisplayStartTracePerformance) {
  GET_THREAD_SAFE_GUARD;
  displayStartTracePerformance = inDisplayStartTracePerformance;
}

// ==============================================================================================================================
void TraceDebug::PrintCache() {
  if(localCache.size() > 0) {
    for(const std::string & str: localCache) {
      PRINT_RESULT(str);
    }
    localCache.clear();
  }

}

// ==============================================================================================================================
void TraceDebug::IncreaseDebugPrintDeepness() {
#ifdef ENABLE_THREAD_SAFE
  ++debugPrintDeepness[std::this_thread::get_id()];
#else
  ++debugPrintDeepness;
#endif
}

// ==============================================================================================================================
void TraceDebug::DecreaseDebugPrintDeepness() {
#ifdef ENABLE_THREAD_SAFE
  --debugPrintDeepness[std::this_thread::get_id()];
#else
  --debugPrintDeepness;
#endif
}

// ==============================================================================================================================
unsigned int TraceDebug::GetDebugPrintDeepness() {
#ifdef ENABLE_THREAD_SAFE
  return debugPrintDeepness[std::this_thread::get_id()];
#else
  return debugPrintDeepness;
#endif
}

// ==============================================================================================================================
unsigned int TraceDebug::GetAllDebugPrintDeepness() {
#ifdef ENABLE_THREAD_SAFE
  return std::accumulate(debugPrintDeepness.begin(), debugPrintDeepness.end(), 0,
                         [](unsigned int a, std::pair<std::thread::id, unsigned int> b) {
    return a + b.second;
  });
#else
  return GetDebugPrintDeepness();
#endif
}

// ==============================================================================================================================
#ifdef WRITE_OUTPUT_TO_FILE
void TraceDebug::WriteToFile(const std::string& stringToWrite, const std::string& fileName) {
  GET_THREAD_SAFE_GUARD;
  if(!outputFile.is_open()) {
    // Search for a non existing filename
    std::string tmpFileName = fileName + "-" + std::to_string(GETPID);
    struct stat buffer;
    for(int index = 0; stat(tmpFileName.c_str(), &buffer) == 0; ++index) {
      tmpFileName = fileName + std::to_string(index);
    }
    outputFile.open(tmpFileName + ".log", std::ofstream::out);
  }
  outputFile << stringToWrite << "\n";
}
#endif


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

void test(std::string&& message) {
  DISPLAY_DEBUG_MESSAGE(message);
  std::thread thread1(f1);
  START_TRACE_PERFORMANCE(test);
  DISPLAY_DEBUG_VALUE(f1());
  ADD_TRACE_PERFORMANCE(test, "This is the middle");
  f2();
  thread1.join();
}

int main()
{
  SET_TRACE_PERFORMANCE_CACHE_DEEPNESS(0);
  test("\n\n\n ****  Without cache enabled **** \n");
  SET_TRACE_PERFORMANCE_CACHE_DEEPNESS(50);
  test("\n\n\n ****  With cache enabled **** \n");
}

/* This program will output:

 ****  Without cache enabled ****

1469712120722.829346ms:139700164831104:TraceDebug.cpp:376 (test) [test]  Start measure
1469712120722.878418ms:139700148037376:TraceDebug.cpp:368 (f1) [f1]  Start measure
  1469712120722.959717ms:139700148037376:Processing f2() - 1  From TraceDebug.cpp:369 (f1)
    1469712120723.001465ms:139700148037376:TraceDebug.cpp:363 (f2) [f2]  Start measure
      1469712120723.034180ms:139700148037376:Processing f3()  From TraceDebug.cpp:364 (f2)
        1469712120723.078125ms:139700148037376:TraceDebug.cpp:357 (f3) [f3]  Start measure
  1469712120723.059814ms:139700164831104:Processing f1()  From TraceDebug.cpp:377 (test)
    1469712120723.146484ms:139700164831104:TraceDebug.cpp:368 (f1) [f1]  Start measure
      1469712120723.190674ms:139700164831104:Processing f2() - 1  From TraceDebug.cpp:369 (f1)
        1469712120723.224854ms:139700164831104:TraceDebug.cpp:363 (f2) [f2]  Start measure
          1469712120723.250732ms:139700164831104:Processing f3()  From TraceDebug.cpp:364 (f2)
            1469712120723.290527ms:139700164831104:TraceDebug.cpp:357 (f3) [f3]  Start measure
          1469712120723.104248ms:139700148037376:TraceDebug.cpp:359 (f3)  a = 5
        1469712120723.372314ms:139700148037376:TraceDebug.cpp:357 (f3) [f3], <Start measure> - <Start measure> = 0.214817ms, <End measure> - <Start measure> = 0.077399ms, Full time: 0.292216ms
      1469712120723.428955ms:139700148037376:->TraceDebug.cpp:364 (f2)  f3() = 5
    1469712120723.463867ms:139700148037376:TraceDebug.cpp:363 (f2) [f2], <Start measure> - <Start measure> = 0.228599ms, <End measure> - <Start measure> = 0.237513ms, Full time: 0.466112ms
              1469712120723.441895ms:139700164831104:TraceDebug.cpp:359 (f3)  a = 5
  1469712120723.500244ms:139700148037376:->TraceDebug.cpp:369 (f1)  f2() - 1 = 1
1469712120723.575928ms:139700148037376:TraceDebug.cpp:368 (f1) [f1], <Start measure> - <Start measure> = 0.270867ms, <End measure> - <Start measure> = 0.429771ms, Full time: 0.700638ms
          1469712120723.658691ms:139700164831104:->TraceDebug.cpp:364 (f2)  f3() = 5
      1469712120723.706055ms:139700164831104:->TraceDebug.cpp:369 (f1)  f2() - 1 = 1
  1469712120723.744141ms:139700164831104:->TraceDebug.cpp:377 (test)  f1() = 1
    1469712120723.775146ms:139700164831104:TraceDebug.cpp:363 (f2) [f2]  Start measure
      1469712120723.802246ms:139700164831104:Processing f3()  From TraceDebug.cpp:364 (f2)
        1469712120723.857178ms:139700164831104:TraceDebug.cpp:357 (f3) [f3]  Start measure
          1469712120723.883545ms:139700164831104:TraceDebug.cpp:359 (f3)  a = 5
        1469712120723.910645ms:139700164831104:TraceDebug.cpp:357 (f3) [f3], <End measure> - <Start measure> = 0.055173ms
      1469712120723.940918ms:139700164831104:->TraceDebug.cpp:364 (f2)  f3() = 5
    1469712120723.966553ms:139700164831104:TraceDebug.cpp:363 (f2) [f2], <End measure> - <Start measure> = 0.190994ms
1469712120724.017090ms:139700164831104:TraceDebug.cpp:376 (test) [test], <This is the middle> - <Start measure> = 0.943374ms, <End measure> - <This is the middle> = 0.248315ms, Full time: 1.191689ms
1469712120724.057129ms:139700164831104:TraceDebug.cpp:374 (test)


 ****  With cache enabled ****

1469712120724.100830ms:139700164831104:TraceDebug.cpp:376 (test) [test]  Start measure
1469712120724.136475ms:139700148037376:TraceDebug.cpp:368 (f1) [f1]  Start measure
  1469712120724.116211ms:139700164831104:Processing f1()  From TraceDebug.cpp:377 (test)
    1469712120724.184082ms:139700164831104:TraceDebug.cpp:368 (f1) [f1]  Start measure
  1469712120724.174561ms:139700148037376:Processing f2() - 1  From TraceDebug.cpp:369 (f1)
    1469712120724.210693ms:139700148037376:TraceDebug.cpp:363 (f2) [f2]  Start measure
      1469712120724.201416ms:139700164831104:Processing f2() - 1  From TraceDebug.cpp:369 (f1)
        1469712120724.237305ms:139700164831104:TraceDebug.cpp:363 (f2) [f2]  Start measure
      1469712120724.231201ms:139700148037376:Processing f3()  From TraceDebug.cpp:364 (f2)
        1469712120724.264160ms:139700148037376:TraceDebug.cpp:357 (f3) [f3]  Start measure
          1469712120724.280518ms:139700148037376:TraceDebug.cpp:359 (f3)  a = 5
        1469712120724.300049ms:139700148037376:TraceDebug.cpp:357 (f3) [f3], <End measure> - <Start measure> = 0.036052ms
          1469712120724.252197ms:139700164831104:Processing f3()  From TraceDebug.cpp:364 (f2)
            1469712120724.332520ms:139700164831104:TraceDebug.cpp:357 (f3) [f3]  Start measure
      1469712120724.319580ms:139700148037376:->TraceDebug.cpp:364 (f2)  f3() = 5
    1469712120724.361816ms:139700148037376:TraceDebug.cpp:363 (f2) [f2], <Start measure> - <Start measure> = 0.029279ms, <End measure> - <Start measure> = 0.122675ms, Full time: 0.151954ms
              1469712120724.351807ms:139700164831104:TraceDebug.cpp:359 (f3)  a = 5
            1469712120724.396973ms:139700164831104:TraceDebug.cpp:357 (f3) [f3], <End measure> - <Start measure> = 0.065986ms
  1469712120724.385254ms:139700148037376:->TraceDebug.cpp:369 (f1)  f2() - 1 = 1
1469712120724.425537ms:139700148037376:TraceDebug.cpp:368 (f1) [f1], <Start measure> - <Start measure> = 0.052087ms, <End measure> - <Start measure> = 0.240887ms, Full time: 0.292974ms
          1469712120724.415039ms:139700164831104:->TraceDebug.cpp:364 (f2)  f3() = 5
      1469712120724.469238ms:139700164831104:->TraceDebug.cpp:369 (f1)  f2() - 1 = 1
  1469712120724.490479ms:139700164831104:->TraceDebug.cpp:377 (test)  f1() = 1
    1469712120724.505371ms:139700164831104:TraceDebug.cpp:363 (f2) [f2]  Start measure
      1469712120724.517578ms:139700164831104:Processing f3()  From TraceDebug.cpp:364 (f2)
        1469712120724.530518ms:139700164831104:TraceDebug.cpp:357 (f3) [f3]  Start measure
          1469712120724.541748ms:139700164831104:TraceDebug.cpp:359 (f3)  a = 5
        1469712120724.553223ms:139700164831104:TraceDebug.cpp:357 (f3) [f3], <End measure> - <Start measure> = 0.023913ms
      1469712120724.565430ms:139700164831104:->TraceDebug.cpp:364 (f2)  f3() = 5
    1469712120724.578369ms:139700164831104:TraceDebug.cpp:363 (f2) [f2], <End measure> - <Start measure> = 0.073292ms
1469712120724.594971ms:139700164831104:TraceDebug.cpp:376 (test) [test], <This is the middle> - <Start measure> = 0.402124ms, <End measure> - <This is the middle> = 0.094128ms, Full time: 0.496252ms
*/
#endif
#endif



/*
The MIT License (MIT)

Copyright (c) 2BlackCoffees 2016

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

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
#ifdef USE_QT_DEBUG
QBuffer                                                                 TraceDebug::qDebugBuffer;
QDebug *                                                                TraceDebug::qDebugLogger = nullptr;
#endif
unsigned int                                                            TraceDebug::traceCacheDeepness = 0;
bool                                                                    TraceDebug::traceActive = true;
bool                                                                    TraceDebug::displayStartTracePerformance = true;
std::vector<std::string>                                                TraceDebug::localCache;
std::map<std::string, int>                                              TraceDebug::mapFileNameToLine;
std::map<std::string,
         std::vector<std::pair<std::string,
                               std::chrono::steady_clock::time_point>>>
                                                                        TraceDebug::mapFileNameFunctionNameToVectorTimingInfo;

// ==============================================================================================================================
std::string TraceDebug::GetUniqueKey(const std::string & string1,
                                     const std::string & string2,
                                     const std::string& string3)
{
  return string1 + string2 + string3;
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
      tmp += "<" + valueMax.first + "> - <" + valueMin.first + "> = "
             + std::to_string(
                       std::chrono::duration<double, UNIT_TRACE_TEMPLATE_TYPE>(
                               valueMax.second - valueMin.second)
                               .count())
             + std::string(UNIT_TRACE_DEBUG);
    }
    if(size > 1)
    {
      const auto& valueMin = performanceInfos[0];
      const auto& valueMax = performanceInfos[size];
      tmp += ", Full time: "
             + std::to_string(
                       std::chrono::duration<double, UNIT_TRACE_TEMPLATE_TYPE>(
                               valueMax.second - valueMin.second)
                               .count())
             + std::string(UNIT_TRACE_DEBUG);
    }
  }
  else if (size == 1)
  {
    // We have 1 timing information only, no difference can be computed
    tmp += ": Not enough trace to display results.";
  }
  else
  {
    tmp = "";
  }
  return tmp;
}

// ==============================================================================================================================
void TraceDebug::SetTracePerformanceCacheDeepness(unsigned int cacheDeepness)
{
  GET_THREAD_SAFE_GUARD;
  if (cacheDeepness != traceCacheDeepness)
  {
    traceCacheDeepness = cacheDeepness;
    // Set a little bigger as the time for displaying output will
    // automatically be added.
    localCache.reserve(traceCacheDeepness + 3);
  }
}

// ==============================================================================================================================
void TraceDebug::Finalize()
{
  // This method is called by a guard statically created that will
  // automatically expire when the program expires.
  GET_THREAD_SAFE_GUARD;
  TraceDebug::PrintCache();
#ifdef WRITE_OUTPUT_TO_FILE
  if (outputFile.is_open())
    outputFile.close();
#endif
}

// ==============================================================================================================================
std::string TraceDebug::GetDiffTimeSinceStartAndThreadId()
{
  std::chrono::duration <double, UNIT_TRACE_TEMPLATE_TYPE> elapsedTime =
      std::chrono::system_clock::now().time_since_epoch();
  std::string returnValue =
          std::to_string(elapsedTime.count()) + UNIT_TRACE_DEBUG;
#ifdef ENABLE_THREAD_SAFE
  std::ostringstream buffer;
  buffer << std::this_thread::get_id();
  returnValue += ":" + buffer.str();
#endif
  return returnValue;
}

// ==============================================================================================================================
void TraceDebug::DisplayStartTracePerformance(
        bool inDisplayStartTracePerformance)
{
  GET_THREAD_SAFE_GUARD;
  displayStartTracePerformance = inDisplayStartTracePerformance;
}

// ==============================================================================================================================
void TraceDebug::PrintCache()
{
  if (localCache.size() > 0)
  {
    for (const std::string& str : localCache)
    {
      PRINT_RESULT(str);
    }
    localCache.clear();
  }
}

// ==============================================================================================================================
void TraceDebug::IncreaseDebugPrintDeepness()
{
#ifdef ENABLE_THREAD_SAFE
  ++debugPrintDeepness[std::this_thread::get_id()];
#else
  ++debugPrintDeepness;
#endif
}

// ==============================================================================================================================
void TraceDebug::DecreaseDebugPrintDeepness()
{
#ifdef ENABLE_THREAD_SAFE
  --debugPrintDeepness[std::this_thread::get_id()];
#else
  --debugPrintDeepness;
#endif
}

// ==============================================================================================================================
unsigned int TraceDebug::GetDebugPrintDeepness()
{
#ifdef ENABLE_THREAD_SAFE
  return debugPrintDeepness[std::this_thread::get_id()];
#else
  return debugPrintDeepness;
#endif
}

// ==============================================================================================================================
unsigned int TraceDebug::GetAllDebugPrintDeepness()
{
#ifdef ENABLE_THREAD_SAFE
  return std::accumulate(
          debugPrintDeepness.begin(), debugPrintDeepness.end(), 0,
                         [](unsigned int a, std::pair<std::thread::id, unsigned int> b) {
    return a + b.second;
  });
#else
  return GetDebugPrintDeepness();
#endif
}

// ==============================================================================================================================
#ifdef WRITE_OUTPUT_TO_FILE
void TraceDebug::WriteToFile(const std::string& stringToWrite,
                             const std::string& fileName)
{
  GET_THREAD_SAFE_GUARD;
  static Guard guardOnLeavingProgram;
  if (!outputFile.is_open())
  {
    // Search for a non existing filename
    std::string tmpFileName = fileName + "-" + std::to_string(GETPID);
    struct stat buffer;
    for (int index = 0; stat(tmpFileName.c_str(), &buffer) == 0; ++index)
    {
      tmpFileName = fileName + std::to_string(index);
    }
    outputFile.open(tmpFileName + ".log", std::ofstream::out);
  }
  outputFile << stringToWrite << "\n";
  // We need the output immidiately
  outputFile.flush();
}
#endif


// ==============================================================================================================================
// ==============================================================================================================================
// ==============================================================================================================================

// Compile with MSVC2013: 
// cl /EHsc /DEBUG /Zi [/O2] TraceDebug.cpp
// Compile with gcc: 
// g++ -std=c++11 -o TraceDebug TraceDebug.cpp -pthread
#ifdef TRACE_DEBUG_HPP_DEBUG_LOCAL
#include <future>
#include <ctime>
std::mutex m_mutex;
int f3()
{
  START_TRACE_PERFORMANCE(f3);
  int a = 5;
  DISPLAY_IMMEDIATE_DEBUG_VALUE(a);
  return a;
}
int f2()
{
  START_TRACE_PERFORMANCE(f2);
  DISPLAY_DEBUG_VALUE(f3());
  return 2;
}

// This fake functions is used to add traces when reverse engineering to Enterprise Architect 
int before_f1_mutex()
{
  return std::rand() % 1000;
}

// This fake functions is used to add traces when reverse engineering to Enterprise Architect 
int after_f1_mutex(int value)
{
  return std::rand() % 1000 + value;
}

int f1()
{
  auto value = before_f1_mutex();
  std::lock_guard<std::mutex> lock(m_mutex);
  value += after_f1_mutex(value);
  START_TRACE_PERFORMANCE(f1);
  DISPLAY_DEBUG_VALUE(f2() - 1);
  return value;
}

void test(std::string&& message)
{
  DISPLAY_DEBUG_MESSAGE(message);
  std::srand(std::time(0));
  auto handle1 = std::async(std::launch::async, f1);
  auto handle2 = std::async(std::launch::async, f1);
  auto handle3 = std::async(std::launch::async, f1);
  START_TRACE_PERFORMANCE(test);
  DISPLAY_DEBUG_VALUE(f1());
  ADD_TRACE_PERFORMANCE(test, "This is the middle");
  f2();
  handle1.get();
  handle2.get();
  handle3.get();
}

// This fake functions is used to add traces when reverse engineering to Enterprise Architect 
void starting_again()
{
}

int main()
{
  while(true)
  {
    starting_again();
    std::cout << "Sleeping ..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << " ... Done" << std::endl;
    SET_TRACE_PERFORMANCE_CACHE_DEEPNESS(0);
    test("\n\n\n ****  Without cache enabled **** \n");
    SET_TRACE_PERFORMANCE_CACHE_DEEPNESS(50);
    test("\n\n\n ****  With cache enabled **** \n");
  }
}

/* This program will output:

 ****  Without cache enabled ****

1497256660314.479492ms:6648:TraceDebug.cpp:457 (test) [test]  Start measure
1497256660332.482910ms:2320:TraceDebug.cpp:445 (f1) [f1]  Start measure
  1497256660332.482910ms:6648:Processing f1()  From TraceDebug.cpp:458 (test)
  1497256660346.485840ms:2320:Processing f2() - 1  From TraceDebug.cpp:446 (f1)
    1497256660379.492432ms:2320:TraceDebug.cpp:423 (f2) [f2]  Start measure
      1497256660393.495117ms:2320:Processing f3()  From TraceDebug.cpp:424 (f2)
        1497256660408.498291ms:2320:TraceDebug.cpp:416 (f3) [f3]  Start measure
          1497256660427.501953ms:2320:TraceDebug.cpp:418 (f3)  a = 5
        1497256660441.504883ms:2320:TraceDebug.cpp:416 (f3) [f3], <End measure> - <Start measure> = 32.918209ms
      1497256660408.498291ms:2320:->TraceDebug.cpp:424 (f2)  f3() = 5
    1497256660478.512207ms:2320:TraceDebug.cpp:423 (f2) [f2], <End measure> - <Start measure> = 98.423626ms
  1497256660379.492432ms:2320:->TraceDebug.cpp:446 (f1)  f2() - 1 = 1
1497256660515.519531ms:2320:TraceDebug.cpp:445 (f1) [f1], <End measure> - <Start measure> = 182.301203ms
1497256660540.524658ms:6872:TraceDebug.cpp:445 (f1) [f1]  Start measure
  1497256660559.528320ms:6872:Processing f2() - 1  From TraceDebug.cpp:446 (f1)
    1497256660577.531982ms:6872:TraceDebug.cpp:423 (f2) [f2]  Start measure
      1497256660594.535400ms:6872:Processing f3()  From TraceDebug.cpp:424 (f2)
        1497256660608.538086ms:6872:TraceDebug.cpp:416 (f3) [f3]  Start measure
          1497256660621.540771ms:6872:TraceDebug.cpp:418 (f3)  a = 5
        1497256660636.543701ms:6872:TraceDebug.cpp:416 (f3) [f3], <End measure> - <Start measure> = 27.813547ms
      1497256660608.538086ms:6872:->TraceDebug.cpp:424 (f2)  f3() = 5
    1497256660669.550293ms:6872:TraceDebug.cpp:423 (f2) [f2], <End measure> - <Start measure> = 92.224794ms
  1497256660577.531982ms:6872:->TraceDebug.cpp:446 (f1)  f2() - 1 = 1
1497256660703.557129ms:6872:TraceDebug.cpp:445 (f1) [f1], <End measure> - <Start measure> = 162.611639ms
1497256660723.561279ms:7316:TraceDebug.cpp:445 (f1) [f1]  Start measure
  1497256660735.563721ms:7316:Processing f2() - 1  From TraceDebug.cpp:446 (f1)
    1497256660750.566650ms:7316:TraceDebug.cpp:423 (f2) [f2]  Start measure
      1497256660767.570068ms:7316:Processing f3()  From TraceDebug.cpp:424 (f2)
        1497256660786.573730ms:7316:TraceDebug.cpp:416 (f3) [f3]  Start measure
          1497256660800.576660ms:7316:TraceDebug.cpp:418 (f3)  a = 5
        1497256660815.579590ms:7316:TraceDebug.cpp:416 (f3) [f3], <End measure> - <Start measure> = 28.510737ms
      1497256660786.573730ms:7316:->TraceDebug.cpp:424 (f2)  f3() = 5
    1497256660844.585449ms:7316:TraceDebug.cpp:423 (f2) [f2], <End measure> - <Start measure> = 94.334691ms
  1497256660749.566406ms:7316:->TraceDebug.cpp:446 (f1)  f2() - 1 = 1
1497256660869.590332ms:7316:TraceDebug.cpp:445 (f1) [f1], <End measure> - <Start measure> = 146.832130ms
    1497256660883.593262ms:6648:TraceDebug.cpp:445 (f1) [f1]  Start measure
      1497256660891.594727ms:6648:Processing f2() - 1  From TraceDebug.cpp:446 (f1)
        1497256660900.596680ms:6648:TraceDebug.cpp:423 (f2) [f2]  Start measure
          1497256660908.598145ms:6648:Processing f3()  From TraceDebug.cpp:424 (f2)
            1497256660916.599854ms:6648:TraceDebug.cpp:416 (f3) [f3]  Start measure
              1497256660925.601563ms:6648:TraceDebug.cpp:418 (f3)  a = 5
            1497256660932.603027ms:6648:TraceDebug.cpp:416 (f3) [f3], <End measure> - <Start measure> = 15.942627ms
          1497256660916.599854ms:6648:->TraceDebug.cpp:424 (f2)  f3() = 5
        1497256660951.606689ms:6648:TraceDebug.cpp:423 (f2) [f2], <End measure> - <Start measure> = 51.503705ms
      1497256660900.596680ms:6648:->TraceDebug.cpp:446 (f1)  f2() - 1 = 1
    1497256660970.610596ms:6648:TraceDebug.cpp:445 (f1) [f1], <End measure> - <Start measure> = 86.397284ms
  1497256660364.489502ms:6648:->TraceDebug.cpp:458 (test)  f1() = 940
    1497256660992.614990ms:6648:TraceDebug.cpp:423 (f2) [f2]  Start measure
      1497256661000.616699ms:6648:Processing f3()  From TraceDebug.cpp:424 (f2)
        1497256661009.618408ms:6648:TraceDebug.cpp:416 (f3) [f3]  Start measure
          1497256661017.620117ms:6648:TraceDebug.cpp:418 (f3)  a = 5
        1497256661027.622070ms:6648:TraceDebug.cpp:416 (f3) [f3], <End measure> - <Start measure> = 18.031997ms
      1497256661009.618408ms:6648:->TraceDebug.cpp:424 (f2)  f3() = 5
    1497256661045.625488ms:6648:TraceDebug.cpp:423 (f2) [f2], <End measure> - <Start measure> = 52.701977ms
1497256661056.627686ms:6648:TraceDebug.cpp:457 (test) [test], <This is the middle> - <Start measure> = 678.305191ms, <End measure> - <This is the middle> = 63.276756ms, Full time: 741.581947ms
1497256661077.632080ms:6648:TraceDebug.cpp:452 (test)


 ****  With cache enabled ****

1497256661077.632080ms:6648:TraceDebug.cpp:457 (test) [test]  Start measure
1497256661077.632080ms:7316:TraceDebug.cpp:445 (f1) [f1]  Start measure
  1497256661077.632080ms:6648:Processing f1()  From TraceDebug.cpp:458 (test)
  1497256661077.632080ms:7316:Processing f2() - 1  From TraceDebug.cpp:446 (f1)
    1497256661077.632080ms:7316:TraceDebug.cpp:423 (f2) [f2]  Start measure
      1497256661077.632080ms:7316:Processing f3()  From TraceDebug.cpp:424 (f2)
        1497256661077.632080ms:7316:TraceDebug.cpp:416 (f3) [f3]  Start measure
          1497256661077.632080ms:7316:TraceDebug.cpp:418 (f3)  a = 5
        1497256661077.632080ms:7316:TraceDebug.cpp:416 (f3) [f3], <End measure> - <Start measure> = 0.056816ms
      1497256661077.632080ms:7316:->TraceDebug.cpp:424 (f2)  f3() = 5
    1497256661077.632080ms:7316:TraceDebug.cpp:423 (f2) [f2], <End measure> - <Start measure> = 0.146256ms
  1497256661077.632080ms:7316:->TraceDebug.cpp:446 (f1)  f2() - 1 = 1
1497256661077.632080ms:7316:TraceDebug.cpp:445 (f1) [f1], <End measure> - <Start measure> = 0.257322ms
1497256661077.632080ms:2320:TraceDebug.cpp:445 (f1) [f1]  Start measure
  1497256661077.632080ms:2320:Processing f2() - 1  From TraceDebug.cpp:446 (f1)
    1497256661077.632080ms:2320:TraceDebug.cpp:423 (f2) [f2]  Start measure
      1497256661077.632080ms:2320:Processing f3()  From TraceDebug.cpp:424 (f2)
        1497256661077.632080ms:2320:TraceDebug.cpp:416 (f3) [f3]  Start measure
          1497256661077.632080ms:2320:TraceDebug.cpp:418 (f3)  a = 5
        1497256661077.632080ms:2320:TraceDebug.cpp:416 (f3) [f3], <End measure> - <Start measure> = 0.037388ms
      1497256661077.632080ms:2320:->TraceDebug.cpp:424 (f2)  f3() = 5
    1497256661077.632080ms:2320:TraceDebug.cpp:423 (f2) [f2], <End measure> - <Start measure> = 0.111066ms
  1497256661077.632080ms:2320:->TraceDebug.cpp:446 (f1)  f2() - 1 = 1
1497256661077.632080ms:2320:TraceDebug.cpp:445 (f1) [f1], <End measure> - <Start measure> = 0.188043ms
    1497256661077.632080ms:6648:TraceDebug.cpp:445 (f1) [f1]  Start measure
      1497256661077.632080ms:6648:Processing f2() - 1  From TraceDebug.cpp:446 (f1)
        1497256661077.632080ms:6648:TraceDebug.cpp:423 (f2) [f2]  Start measure
          1497256661077.632080ms:6648:Processing f3()  From TraceDebug.cpp:424 (f2)
            1497256661077.632080ms:6648:TraceDebug.cpp:416 (f3) [f3]  Start measure
              1497256661077.632080ms:6648:TraceDebug.cpp:418 (f3)  a = 5
            1497256661077.632080ms:6648:TraceDebug.cpp:416 (f3) [f3], <End measure> - <Start measure> = 0.036289ms
          1497256661077.632080ms:6648:->TraceDebug.cpp:424 (f2)  f3() = 5
        1497256661077.632080ms:6648:TraceDebug.cpp:423 (f2) [f2], <End measure> - <Start measure> = 0.104102ms
      1497256661077.632080ms:6648:->TraceDebug.cpp:446 (f1)  f2() - 1 = 1
    1497256661077.632080ms:6648:TraceDebug.cpp:445 (f1) [f1], <End measure> - <Start measure> = 0.176680ms
  1497256661077.632080ms:6648:->TraceDebug.cpp:458 (test)  f1() = 1696
1497256661077.632080ms:6872:TraceDebug.cpp:445 (f1) [f1]  Start measure
    1497256661077.632080ms:6648:TraceDebug.cpp:423 (f2) [f2]  Start measure
  1497256661077.632080ms:6872:Processing f2() - 1  From TraceDebug.cpp:446 (f1)
    1497256661077.632080ms:6872:TraceDebug.cpp:423 (f2) [f2]  Start measure
      1497256661077.632080ms:6648:Processing f3()  From TraceDebug.cpp:424 (f2)
      1497256661077.632080ms:6872:Processing f3()  From TraceDebug.cpp:424 (f2)
        1497256661077.632080ms:6648:TraceDebug.cpp:416 (f3) [f3]  Start measure
          1497256661078.632080ms:6648:TraceDebug.cpp:418 (f3)  a = 5
        1497256661078.632080ms:6648:TraceDebug.cpp:416 (f3) [f3], <End measure> - <Start measure> = 0.055350ms
        1497256661078.632080ms:6872:TraceDebug.cpp:416 (f3) [f3]  Start measure
      1497256661077.632080ms:6648:->TraceDebug.cpp:424 (f2)  f3() = 5
    1497256661078.632080ms:6648:TraceDebug.cpp:423 (f2) [f2], <Start measure> - <Start measure> = 0.039221ms, <End measure> - <Start measure> = 0.164584ms, Full time: 0.203805ms
          1497256661078.632080ms:6872:TraceDebug.cpp:418 (f3)  a = 5
*/
#endif
#endif

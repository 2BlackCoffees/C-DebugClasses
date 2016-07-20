// //////////////////////////////////////////////////////////////////////
//
// This file is under the license http://www.apache.org/licenses/LICENSE-2.0
// Accessible on GitHub: https://github.com/2BlackCoffees/C-DebugClasses
//
// //////////////////////////////////////////////////////////////////////

#ifndef __TRACE_DEBUG_HPP
#define __TRACE_DEBUG_HPP

#include <map>
#include <utility>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <chrono>
#include <sstream>
#include <atomic>
#include <cstring>
#include <thread>
#include <mutex>
#include <numeric>

// Comment this line to completely disable traces
#define ENABLE_TRACE_DEBUG
#ifdef ENABLE_TRACE_DEBUG

  // Decomment this line when adding TraceDebug to your project
  #define TRACE_DEBUG_HPP_DEBUG_LOCAL

  // Uncomment to disable threadsafe
  #define ENABLE_THREAD_SAFE
  #ifdef ENABLE_THREAD_SAFE
    #define GET_THREAD_SAFE_GUARD std::lock_guard<std::recursive_mutex> guard(the_mutex);
  #else
    #define GET_THREAD_SAFE_GUARD
  #endif


  // Select here micro or milliseconds traces
  //#define UNIT_TRACE_DEBUG_NANO
  #ifdef UNIT_TRACE_DEBUG_NANO
    #define UNIT_TRACE_DEBUG " ns"
    #define UNIT_TRACE_TEMPLATE_TYPE std::nano
  #else
    #define UNIT_TRACE_DEBUG " ms"
    #define UNIT_TRACE_TEMPLATE_TYPE std::milli
  #endif

  // If not commented, write outputs a file. Comment to write to std::out or qDebug
  //#define WRITE_OUTPUT_TO_FILE
  // Commented writes to std::out. Otherwise uses qDebug.
  //#define USE_QT_DEBUG
  #ifndef WRITE_OUTPUT_TO_FILE
    #ifdef USE_QT_DEBUG
      #include <QDebug>
      #define PRINT_RESULT(string_to_print) qDebug() << QString::fromUtf8(string_to_print.c_str());
    #else
      #define PRINT_RESULT(string_to_print) std::cout << string_to_print << std::endl;
    #endif
  #else
    #define PRINT_RESULT(string_to_print) DebugTrace::WriteToFile(string_to_print, "TraceDebug.log");
  #endif

  // =============================================================================================

  #ifdef _WIN32
    #define __func__ __FUNCTION__
    #define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
  #else
    #define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
  #endif

  #define TOKENPASTE(x, y) x ## y
  #define TOKENPASTE_EXPAND(x, y) TOKENPASTE(x , y)

  // Activate traces
  #define DISPLAY_DEBUG_ACTIVE_TRACE DebugTrace::ActiveTrace(true)
  // Deactivate traces: any call to any debug macro will not be displaying anything. However the hierarchy is kept up to date.
  // When traces are activated back the last computed hierarchy will be used to insert the right number of blank spaces
  #define DISPLAY_DEBUG_DEACTIVE_TRACE DebugTrace::ActiveTrace(false)
  // Display a value specifying the expression, its value and many blank spaces defining the deepness of the hierarchy,
  // the filename, line number and function name are all displayed.
  // This macro should be used when deeper hierachy will be created to compute the expected value.
#define DISPLAY_DEBUG_VALUE(value) \
  DebugTrace TOKENPASTE_EXPAND(__Unused, __LINE__)(__func__, __FILENAME__, __LINE__); \
  if(DebugTrace::IsTraceActive()) { \
    std::stringstream TOKENPASTE_EXPAND(__UnusedStream1, __LINE__);\
    std::stringstream TOKENPASTE_EXPAND(__UnusedStream2, __LINE__);\
    TOKENPASTE_EXPAND(__UnusedStream1, __LINE__) << DebugTrace::GetDiffTimeSinceStart() << ": Processing " << #value << "  From " << __FILENAME__ << ":" << __LINE__ << " ("<< __func__ << ")"; \
    DebugTrace::PrintString(TOKENPASTE_EXPAND(__UnusedStream1, __LINE__).str(), true);\
    TOKENPASTE_EXPAND(__UnusedStream2, __LINE__) << DebugTrace::GetDiffTimeSinceStart() << " ->" << __FILENAME__ << ":" << __LINE__ << " ("<< __func__ << ")  " << #value << " = " << (value); \
    DebugTrace::PrintString(TOKENPASTE_EXPAND(__UnusedStream2, __LINE__).str(), true);\
  }
  // Display a value specifying the expression, its value and many blank spaces defining the deepness of the hierarchy,
  // the filename, line number and function name are all displayed.
  // This macro should be used when NO deeper hierachy is required to compute the expected value.
#define DISPLAY_IMMEDIATE_DEBUG_VALUE(value) \
  DebugTrace TOKENPASTE_EXPAND(__Unused, __LINE__)(__func__, __FILENAME__, __LINE__); \
  if(DebugTrace::IsTraceActive()) { \
    std::stringstream TOKENPASTE_EXPAND(__UnusedStream, __LINE__);\
    TOKENPASTE_EXPAND(__UnusedStream, __LINE__) << DebugTrace::GetDiffTimeSinceStart() << " " << __FILENAME__ << ":" << __LINE__ << " ("<< __func__ << ")  " << #value << " = " << (value); \
    DebugTrace::PrintString(TOKENPASTE_EXPAND(__UnusedStream, __LINE__).str(), true);\
  }
  // Display a message and preceeded by blank spaces defining the deepness of the hierarchy
  #define DISPLAY_DEBUG_MESSAGE(message) { \
      DebugTrace TOKENPASTE_EXPAND(__Unused, __LINE__)(__func__, __FILENAME__, __LINE__); \
      std::stringstream TOKENPASTE_EXPAND(__UnusedStream, __LINE__);\
      TOKENPASTE_EXPAND(__UnusedStream, __LINE__) << DebugTrace::GetDiffTimeSinceStart() << " " << __FILENAME__ << ":" << __LINE__ << " ("<< __func__ << ")  " << message; \
      if(DebugTrace::IsTraceActive()) { DebugTrace::PrintString(TOKENPASTE_EXPAND(__UnusedStream, __LINE__).str(), true); }\
  }
  // Display a value specifying the expression, its value and preceed them with blank spaces defining the deepness of the hierarchy
  // deeper hierarchy will however not be displayed
  #define DISPLAY_DEBUG_VALUE_NON_HIERARCHICALLY(value) \
      DebugTrace TOKENPASTE_EXPAND(__Unused_Debug, __LINE__)(__func__, __FILENAME__, __LINE__); \
      if(DebugTrace::IsTraceActive()) { \
        DISPLAY_DEBUG_DEACTIVE_TRACE; \
        std::stringstream TOKENPASTE_EXPAND(__UnusedStream, __LINE__);\
        TOKENPASTE_EXPAND(__UnusedStream, __LINE__) << DebugTrace::GetDiffTimeSinceStart() << " " << __BASE_FILE__ << ":" << __LINE__ << " ("<< __func__ << ")  " << #value << " = " << (value) << std::endl; \
        DebugTrace::PrintString(TOKENPASTE_EXPAND(__UnusedStream, __LINE__).str(), true);\
        DISPLAY_DEBUG_ACTIVE_TRACE; \
      }
  // This macro will display the full time between the point it is created to its end of scope.    
  #define START_TRACE_PERFORMANCE(unique_key) \
    DebugTrace TOKENPASTE_EXPAND(unique_key, _Performance_Variable)(__func__, __FILENAME__, __LINE__, #unique_key);
  // this macro allows to create several measurement points between START_TRACE_PERFORMANCE creation and its end of scope
  #define ADD_TRACE_PERFORMANCE(unique_key, userInfo) \
    auto TOKENPASTE_EXPAND(unique_key, __LINE__) = std::chrono::steady_clock::now();\
    TOKENPASTE_EXPAND(unique_key, _Performance_Variable).AddTrace(TOKENPASTE_EXPAND(unique_key, __LINE__), userInfo);
  // Define deepness of cache: Set below 2, caching is deactivated: all results are displayed when available.
  // Displaying has a huge cost of performance, thus enabling the cache allows to have a more reliable measure.
  // Once the cache is full it is displayed and all measures not yet done will notify the inducted time overhead.
#define SET_TRACE_PERFORMANCE_CACHE_DEEPNESS(cache_deepness) \
  DebugTrace::SetTracePerformanceCacheDeepness(cache_deepness);
#define DISPLAY_START_TRACE_PERFORMANCE(displayStartTracePerformance) \
  DebugTrace::DisplayStartTracePerformance(displayStartTracePerformance);

  class DebugTrace {
      // How many objects DebugTrace in nested scopes were created
#ifdef ENABLE_THREAD_SAFE
      static std::map<std::thread::id, unsigned int> debugPrintDeepness;
#else
      static unsigned int debugPrintDeepness;
#endif
      // How many elements to be cached
      static unsigned int traceCacheDeepness;
      // How much time required if reserve must be applied on localCache
      static std::atomic<double> reserveTime;
      // Traces are active / Inactive
      static bool traceActive;
      // Display a message when starting a trace performance if true
      // Will display only final result if false
      static bool displayStartTracePerformance;
      // Local cache to be used instead of the output
      static std::vector<std::string> localCache;
      // Key is filename + functioname, Value is line number
      static std::map<std::string, int> mapFileNameToLine;
      // Key is filename + functioname + unique key,
      // Value is a vector of pair containing a variable name as first and timing as second
      static std::map<std::string, std::vector<std::pair<std::string,
                                               std::chrono::steady_clock::time_point>>> mapFileNameFunctionNameToVectorTimingInfo;      
      static std::chrono::time_point<std::chrono::system_clock> startingTime;
#ifdef WRITE_OUTPUT_TO_FILE
      static std::ofstream outputFile;
#endif
      // Mutex
#ifdef ENABLE_THREAD_SAFE
      static std::recursive_mutex the_mutex;
#endif

      bool debugPrintMustBeDecremented = false;
      bool debugPerformanceMustBeDisplayed = false;
      std::string keyDebugPrintToErase;
      // For performance analyse, contains filename + functioname + unique key,
      std::string keyDebugPerformanceToErase;
      std::string lineHeader;
      std::string GetUniqueKey(const std::string & string1,
                               const std::string & string2,
                               const std::string & string3 = "");

    public:
      DebugTrace(const std::string & functionName, const std::string & fileName, int lineNumber, const std::string &uniqueKey);
      DebugTrace(const std::string & functionName, const std::string & fileName, int lineNumber = __LINE__);
      ~DebugTrace();

      static void ActiveTrace(bool activate);
      static bool IsTraceActive();
      static void PrintString(const std::string & inStr, bool showHierarchy);
      static void SetTracePerformanceCacheDeepness(unsigned int cacheDeepness) {
        GET_THREAD_SAFE_GUARD;
        traceCacheDeepness = cacheDeepness;
        // Set a little bigger as the time for displaying output will
        // automatically be added.
        localCache.reserve(traceCacheDeepness + 3);
      }
      static std::string GetDiffTimeSinceStart() {
        std::chrono::duration <double, UNIT_TRACE_TEMPLATE_TYPE> elapsedTime =
            std::chrono::system_clock::now() - startingTime;
        return std::to_string(elapsedTime.count()) + UNIT_TRACE_DEBUG;
      }
      static void DisplayStartTracePerformance(bool inDisplayStartTracePerformance) {
        GET_THREAD_SAFE_GUARD;
        displayStartTracePerformance = inDisplayStartTracePerformance;
      }

      void  AddTrace(std::chrono::steady_clock::time_point timePoint, const std::string & variableName);
  private:
      static std::string getSpaces();
      static void CacheOrPrintOutputs(std::string &&output);
#ifdef WRITE_OUTPUT_TO_FILE
      static void WriteToFile(const std::string& stringToWrite, const std::string& fileName) {
        GET_THREAD_SAFE_GUARD;
        if(!outputFile.is_open()) {
          outputFile.open(fileName);
        }
        outputFile << stringToWrite << "\n";
      }
#endif
      std::string GetPerformanceResults();
      void DisplayPerformanceMeasure();
      void CacheOrPrintTimings(std::string &&output);
      void IncreaseDebugPrintDeepness() {
#ifdef ENABLE_THREAD_SAFE
        ++debugPrintDeepness[std::this_thread::get_id()];
#else
        ++debugPrintDeepness;
#endif
      }
      void DecreaseDebugPrintDeepness() {
#ifdef ENABLE_THREAD_SAFE
        --debugPrintDeepness[std::this_thread::get_id()];
#else
        --debugPrintDeepness;
#endif
      }
      static unsigned int GetDebugPrintDeepness() {
#ifdef ENABLE_THREAD_SAFE
        return debugPrintDeepness[std::this_thread::get_id()];
#else
        return debugPrintDeepness;
#endif
      }

      static unsigned int GetAllDebugPrintDeepness() {
#ifdef ENABLE_THREAD_SAFE
        return std::accumulate(debugPrintDeepness.begin(), debugPrintDeepness.end(), 0,
                               [](unsigned int a, std::pair<std::thread::id, unsigned int> b) {
                                   return a + b.second;
                                });
#else
        return GetDebugPrintDeepness();
#endif
      }

  };
#else
  #define DISPLAY_DEBUG_ACTIVE_TRACE
  #define DISPLAY_DEBUG_DEACTIVE_TRACE
  #define DISPLAY_DEBUG_VALUE(value)
  #define DISPLAY_IMMEDIATE_DEBUG_VALUE(value)
  #define DISPLAY_DEBUG_MESSAGE(message)
  #define DISPLAY_DEBUG_VALUE_NON_HIERARCHICALLY(value)
  #define START_TRACE_PERFORMANCE(unique_key)
  #define ADD_TRACE_PERFORMANCE(unique_key, userInfo)
  #define SET_TRACE_PERFORMANCE_CACHE_DEEPNESS(cache_deepness)
#endif
#endif

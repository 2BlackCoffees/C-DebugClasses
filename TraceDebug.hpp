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

  // Uncomment to disable threadsafe (Optimization)
  #define ENABLE_THREAD_SAFE

  // If not commented, write outputs into a file. Comment to write to std::out or qDebug
  //#define WRITE_OUTPUT_TO_FILE

  // Commented writes to std::out. Otherwise uses qDebug: However if WRITE_OUTPUT_TO_FILE is defined, then
  // output will be written into a file
  //#define USE_QT_DEBUG


  // If defined, traces are printed in ns otherwise in ms
  //#define UNIT_TRACE_DEBUG_NANO


// =============================================================================================

  #ifdef ENABLE_THREAD_SAFE
    #define GET_THREAD_SAFE_GUARD std::lock_guard<std::recursive_mutex> guard(the_mutex);
  #else
    #define GET_THREAD_SAFE_GUARD
  #endif

  #ifdef UNIT_TRACE_DEBUG_NANO
    #define UNIT_TRACE_DEBUG "ns"
    #define UNIT_TRACE_TEMPLATE_TYPE std::nano
  #else
    #define UNIT_TRACE_DEBUG "ms"
    #define UNIT_TRACE_TEMPLATE_TYPE std::milli
  #endif

  #ifndef WRITE_OUTPUT_TO_FILE
    #ifdef USE_QT_DEBUG
      #include <QDebug>
      #define PRINT_RESULT(string_to_print) qDebug() << QString::fromUtf8(string_to_print.c_str());
    #else
      #define PRINT_RESULT(string_to_print) std::cout << string_to_print << std::endl;
    #endif
  #else
    #include <iomanip>
    #include <sys/stat.h>
    #ifdef _WIN32
      #include <process.h>
      #define GETPID _getpid()
    #else
      #include<sys/types.h>
      #include <unistd.h>
      #define GETPID getpid()
    #endif
    #define PRINT_RESULT(string_to_print) TraceDebug::WriteToFile(string_to_print, "TraceDebug");
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

// =============================================================================================

  // Activate traces
  #define DISPLAY_DEBUG_ACTIVE_TRACE TraceDebug::ActiveTrace(true)
  // Deactivate traces: any call to any debug macro will not be displaying anything. However the hierarchy is kept up to date.
  // When traces are activated back the last computed hierarchy will be used to insert the right number of blank spaces
  #define DISPLAY_DEBUG_DEACTIVE_TRACE TraceDebug::ActiveTrace(false)
  // Display a value specifying the expression, its value and many blank spaces defining the deepness of the hierarchy,
  // the filename, line number and function name are all displayed.
  // This macro should be used when deeper hierachy will be created to compute the expected value.
#define DISPLAY_DEBUG_VALUE(value) \
  TraceDebug TOKENPASTE_EXPAND(__Unused, __LINE__)(__func__, __FILENAME__, __LINE__); \
  if(TraceDebug::IsTraceActive()) { \
    std::stringstream TOKENPASTE_EXPAND(__UnusedStream1, __LINE__);\
    std::stringstream TOKENPASTE_EXPAND(__UnusedStream2, __LINE__);\
    TOKENPASTE_EXPAND(__UnusedStream1, __LINE__) << TraceDebug::GetDiffTimeSinceStartAndThreadId() << ":Processing " << #value << "  From " << __FILENAME__ << ":" << __LINE__ << " ("<< __func__ << ")"; \
    TraceDebug::PrintString(TOKENPASTE_EXPAND(__UnusedStream1, __LINE__).str(), true);\
    TOKENPASTE_EXPAND(__UnusedStream2, __LINE__) << TraceDebug::GetDiffTimeSinceStartAndThreadId() << ":->" << __FILENAME__ << ":" << __LINE__ << " ("<< __func__ << ")  " << #value << " = " << (value); \
    TraceDebug::PrintString(TOKENPASTE_EXPAND(__UnusedStream2, __LINE__).str(), true);\
  }
  // Display a value specifying the expression, its value and many blank spaces defining the deepness of the hierarchy,
  // the filename, line number and function name are all displayed.
  // This macro should be used when NO deeper hierachy is required to compute the expected value.
#define DISPLAY_IMMEDIATE_DEBUG_VALUE(value) \
  TraceDebug TOKENPASTE_EXPAND(__Unused, __LINE__)(__func__, __FILENAME__, __LINE__); \
  if(TraceDebug::IsTraceActive()) { \
    std::stringstream TOKENPASTE_EXPAND(__UnusedStream, __LINE__);\
    TOKENPASTE_EXPAND(__UnusedStream, __LINE__) << TraceDebug::GetDiffTimeSinceStartAndThreadId() << ":" << __FILENAME__ << ":" << __LINE__ << " ("<< __func__ << ")  " << #value << " = " << (value); \
    TraceDebug::PrintString(TOKENPASTE_EXPAND(__UnusedStream, __LINE__).str(), true);\
  }
  // Display a message and preceeded by blank spaces defining the deepness of the hierarchy
  #define DISPLAY_DEBUG_MESSAGE(message) { \
      TraceDebug TOKENPASTE_EXPAND(__Unused, __LINE__)(__func__, __FILENAME__, __LINE__); \
      std::stringstream TOKENPASTE_EXPAND(__UnusedStream, __LINE__);\
      TOKENPASTE_EXPAND(__UnusedStream, __LINE__) << TraceDebug::GetDiffTimeSinceStartAndThreadId() << ":" << __FILENAME__ << ":" << __LINE__ << " ("<< __func__ << ")  " << message; \
      if(TraceDebug::IsTraceActive()) { TraceDebug::PrintString(TOKENPASTE_EXPAND(__UnusedStream, __LINE__).str(), true); }\
  }
  // Display a value specifying the expression, its value and preceed them with blank spaces defining the deepness of the hierarchy
  // deeper hierarchy will however not be displayed
  #define DISPLAY_DEBUG_VALUE_NON_HIERARCHICALLY(value) \
      TraceDebug TOKENPASTE_EXPAND(__Unused_Debug, __LINE__)(__func__, __FILENAME__, __LINE__); \
      if(TraceDebug::IsTraceActive()) { \
        DISPLAY_DEBUG_DEACTIVE_TRACE; \
        std::stringstream TOKENPASTE_EXPAND(__UnusedStream, __LINE__);\
        TOKENPASTE_EXPAND(__UnusedStream, __LINE__) << TraceDebug::GetDiffTimeSinceStartAndThreadId() << ":" << __BASE_FILE__ << ":" << __LINE__ << " ("<< __func__ << ")  " << #value << " = " << (value) << std::endl; \
        TraceDebug::PrintString(TOKENPASTE_EXPAND(__UnusedStream, __LINE__).str(), true);\
        DISPLAY_DEBUG_ACTIVE_TRACE; \
      }
  // This macro will display the full time between the point it is created to its end of scope.    
  #define START_TRACE_PERFORMANCE(unique_key) \
    TraceDebug TOKENPASTE_EXPAND(unique_key, _Performance_Variable)(__func__, __FILENAME__, __LINE__, #unique_key);
  // this macro allows to create several measurement points between START_TRACE_PERFORMANCE creation and its end of scope
  #define ADD_TRACE_PERFORMANCE(unique_key, userInfo) \
    auto TOKENPASTE_EXPAND(unique_key, __LINE__) = std::chrono::steady_clock::now();\
    TOKENPASTE_EXPAND(unique_key, _Performance_Variable).AddTrace(TOKENPASTE_EXPAND(unique_key, __LINE__), userInfo);
  // Define deepness of cache: Set below 2, caching is deactivated: all results are displayed when available.
  // Displaying has a huge cost of performance, thus enabling the cache allows to have a more reliable measure.
  // Once the cache is full it is displayed and all measures not yet done will notify the inducted time overhead.
  #define SET_TRACE_PERFORMANCE_CACHE_DEEPNESS(cache_deepness) \
    TraceDebug::SetTracePerformanceCacheDeepness(cache_deepness);
  // Specifies whether the line Start measure should be displayed when creating a new START_TRACE_PERFORMANCE
  // By default it is displayed, it can be removed specifying DISPLAY_START_TRACE_PERFORMANCE(false) and thus
  // only the diff time in a scope will be displayed.
  #define DISPLAY_START_TRACE_PERFORMANCE(displayStartTracePerformance) \
    TraceDebug::DisplayStartTracePerformance(displayStartTracePerformance);

  class TraceDebug {
      // How many objects TraceDebug in nested scopes were created
#ifdef ENABLE_THREAD_SAFE
      static std::map<std::thread::id, unsigned int> debugPrintDeepness;
#else
      static unsigned int debugPrintDeepness;
#endif
      // How many elements to be cached
      static unsigned int traceCacheDeepness;
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
      TraceDebug(const std::string & functionName, const std::string & fileName, int lineNumber, const std::string &uniqueKey);
      TraceDebug(const std::string & functionName, const std::string & fileName, int lineNumber = __LINE__);
      ~TraceDebug();
      void  AddTrace(std::chrono::steady_clock::time_point timePoint, const std::string & variableName);

      static void ActiveTrace(bool activate);
      static bool IsTraceActive();
      static void PrintString(const std::string & inStr, bool showHierarchy);
      static void SetTracePerformanceCacheDeepness(unsigned int cacheDeepness);
      static void Finalize();
      static std::string GetDiffTimeSinceStartAndThreadId();
      static void DisplayStartTracePerformance(bool inDisplayStartTracePerformance);

  private:
      std::string GetPerformanceResults();
      void DisplayPerformanceMeasure();
      void CacheOrPrintTimings(std::string &&output);
      void IncreaseDebugPrintDeepness();
      void DecreaseDebugPrintDeepness();

      static std::string getSpaces();
      static void CacheOrPrintOutputs(std::string &&output);
#ifdef WRITE_OUTPUT_TO_FILE
      static std::ofstream outputFile;
      static void WriteToFile(const std::string& stringToWrite, const std::string& fileName);
#endif
      static void PrintCache();
      static unsigned int GetDebugPrintDeepness();
      static unsigned int GetAllDebugPrintDeepness();

  };

  class Guard {
  public:
    ~Guard() {
      TraceDebug::Finalize();
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
  #define DISPLAY_START_TRACE_PERFORMANCE(displayStartTracePerformance)
#endif
#endif

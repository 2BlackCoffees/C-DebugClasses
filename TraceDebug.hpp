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
#include <chrono>
#include <sstream>

#define ENABLE_TRACE_DEBUG
#ifdef ENABLE_TRACE_DEBUG
  #ifdef _WIN32
    #define __func__ __FUNCTION__
    #define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
  #else
    #define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
  #endif

  //#define UNIT_TRACE_DEBUG_NANO
  #ifdef UNIT_TRACE_DEBUG_NANO
    #define UNIT_TRACE_DEBUG " ns"
    #define UNIT_TRACE_TEMPLATE_TYPE std::nano
  #else
    #define UNIT_TRACE_DEBUG " ms"
    #define UNIT_TRACE_TEMPLATE_TYPE std::milli
  #endif
  //#define USE_QT_DEBUG
  #ifdef USE_QT_DEBUG
    #include <QDebug>
    #define PRINT_RESULT(string_to_print) qDebug() << QString::fromUtf8(string_to_print.c_str());
  #else
    #define PRINT_RESULT(string_to_print) std::cout << string_to_print << std::endl;
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
    std::stringstream stream1;\
    std::stringstream stream2;\
    stream1 << "Processing " << #value << "  From " << __FILENAME__ << ":" << __LINE__ << " ("<< __func__ << ")"; \
    DebugTrace::PrintString(stream1.str(), true);\
    stream2 << "->" << __FILENAME__ << ":" << __LINE__ << " ("<< __func__ << ")  " << #value << " = " << (value); \
    DebugTrace::PrintString(stream2.str(), true);\
  }
  // Display a value specifying the expression, its value and many blank spaces defining the deepness of the hierarchy,
  // the filename, line number and function name are all displayed.
  // This macro should be used when NO deeper hierachy is required to compute the expected value.
#define DISPLAY_IMMEDIATE_DEBUG_VALUE(value) \
  DebugTrace TOKENPASTE_EXPAND(__Unused, __LINE__)(__func__, __FILENAME__, __LINE__); \
  if(DebugTrace::IsTraceActive()) { \
    std::stringstream stream;\
    stream << __FILENAME__ << ":" << __LINE__ << " ("<< __func__ << ")  " << #value << " = " << (value); \
    DebugTrace::PrintString(stream.str(), true);\
  }
  // Display a message and preceeded by blank spaces defining the deepness of the hierarchy
  #define DISPLAY_DEBUG_MESSAGE(message) \
      DebugTrace TOKENPASTE_EXPAND(__Unused, __LINE__)(__func__, __FILENAME__, __LINE__); \
      if(DebugTrace::IsTraceActive()) { DebugTrace::PrintString(message, true); }
  // Display a value specifying the expression, its value and preceed them with blank spaces defining the deepness of the hierarchy
  // deeper hierarchy will however not be displayed
  #define DISPLAY_DEBUG_VALUE_NON_HIERARCHICALLY(value) \
      DebugTrace TOKENPASTE_EXPAND(__Unused_Debug, __LINE__)(__func__, __FILENAME__, __LINE__); \
      if(DebugTrace::IsTraceActive()) { \
        DISPLAY_DEBUG_DEACTIVE_TRACE; \
        std::stringstream stream;\
        stream << __BASE_FILE__ << ":" << __LINE__ << " ("<< __func__ << ")  " << #value << " = " << (value) << std::endl; \
        DebugTrace::PrintString(stream.str(), true);\
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

  class DebugTrace {
      static int debugPrintDeepness;
      static uint64_t tracePerformanceCacheDeepness;
      static bool traceActive;
      static std::vector<std::string> localCache;
      static std::map<std::string, int> mapFileNameToLine;
      static std::map<std::string, std::vector<std::pair<std::string,
                                               std::chrono::system_clock::time_point>>> mapFileNameFunctionNameToVectorTimingInfo;
      bool debugPrintMustBeDecremented = false;
      bool debugPerformanceMustBeDisplayed = false;
      std::string keyDebugPrintToErase;
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
      static std::string getSpaces();
      static void PrintString(const std::string & inStr, bool showHierarchy);
      static void SetTracePerformanceCacheDeepness(uint64_t cacheDeepness) {
        tracePerformanceCacheDeepness = cacheDeepness;
        // Set a little bigger as the time for displaying output will
        // automatically be added.
        localCache.reserve(tracePerformanceCacheDeepness + 3);
      }

      std::string GetPerformanceResults();
      void  AddTrace(std::chrono::system_clock::time_point timePoint, const std::string & variableName);
  private:
      void DisplayPerformanceMeasure();
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

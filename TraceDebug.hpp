// //////////////////////////////////////////////////////////////////////
//
// This file is under the license http://www.apache.org/licenses/LICENSE-2.0
// Accessible on GitHub: https://github.com/2BlackCoffees/C-DebugClasses
//
// //////////////////////////////////////////////////////////////////////

#ifndef __TRACE_DEBUG_HPP
#define __TRACE_DEBUG_HPP

// See here an example of usage in source code:

// In file main.cpp @ line 368
// DISPLAY_DEBUG_VALUE(f1());
// In file f1.cpp @ line 83
// DISPLAY_DEBUG_VALUE(f2());
// In file f2.cpp @ line 651
// DISPLAY_DEBUG_VALUE(f3());
// In file f2.cpp @ line 660
// DISPLAY_DEBUG_VALUE(value);

// Output will be:

//Processing f1() From main.cpp:368 (main)
// Processing f2() From f1.cpp:83 (f1)
// Processing f3() From f2.cpp:651 (f2)
// ->f2.cpp:651 (f2) f3() = 1
// Processing value From f2.cpp:660 (f2)
// ->f2.cpp:660 (f2) value = 1
// ->f1.cpp:83 (f1) f2() = 1
//->main.cpp:368 (main) f1() = 1

// Analyze of performances
// -----------------------

// void ClassName::anyFct() {
//   START_TRACE_PERFORMANCE(my_unique_id);
//   /* function body */
// }

// Will produce
// "ClassName::anyFct (my_unique_id) filename.cpp:362 (Full elapsed: 398.000000 ms), End measure - Start measure = 3.000000 ms"

// Nested functions can be analyzed as well and any trace point can be easily added:

// void ClassName::anyFct() {
//   START_TRACE_PERFORMANCE(my_unique_id);
//   for (...) {
//     START_TRACE_PERFORMANCE(my_unique_id1);
//     // code here
//     ADD_TRACE_PERFORMANCE(my_unique_id1, "User information");
//     // code here
//     Usage of my_unique_id instead of my_unique_id1 is possible too: many information will be provided on one line
//   }
//   /* function body */
// }

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

  #ifdef UNIT_TRACE_DEBUG_NANO
    #define UNIT_TRACE_DEBUG " ns"
    #define UNIT_TRACE_TEMPLATE_TYPE std::nano
  #else
    #define UNIT_TRACE_DEBUG " ms"
    #define UNIT_TRACE_TEMPLATE_TYPE std::milli
  #endif
  #define USE_QT_DEBUG
  #ifdef USE_QT_DEBUG
    #include <QDebug>
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
  #define START_TRACE_PERFORMANCE(unique_key) \
    DebugTrace TOKENPASTE_EXPAND(unique_key, _Performance_Variable)(__func__, __FILENAME__, __LINE__, #unique_key);
  #define ADD_TRACE_PERFORMANCE(unique_key, userInfo) \
    auto TOKENPASTE_EXPAND(unique_key, __LINE__) = std::chrono::steady_clock::now();\
    TOKENPASTE_EXPAND(unique_key, _Performance_Variable).AddTrace(TOKENPASTE_EXPAND(unique_key, __LINE__), userInfo);


  class DebugTrace {
      static int debugPrintDeepness;
      static bool traceActive;
      static std::map<std::string, int> mapFileNameToLine;
      static std::map<std::string, std::vector<std::pair<std::string,
                                               std::chrono::system_clock::time_point>>>
                mapFileNameFunctionNameToVectorTimingInfo;
      bool debugPrintMustBeDecremented;
      bool debugPerformanceMustBeDisplayed;
      std::string keyDebugPrintToErase;
      std::string keyDebugPerformanceToErase;
      std::string lineHeader;
      std::string GetUniqueKey(const std::string & string1,
                               const std::string & string2,
                               const std::string & string3 = "");

    public:
      DebugTrace(const std::string &functionName, const std::string &fileName, int lineNumber, const std::string &uniqueKey);

      DebugTrace(const std::string & functionName,
                 const std::string & fileName,
                 int lineNumber = __LINE__);
      ~DebugTrace();
      static void PrintPerformanceInfos();

      static void ActiveTrace(bool activate);
      static bool IsTraceActive();
      static std::string getSpaces();
      static void PrintString(const std::string& inStr, bool showHierarchy);

      std::string GetPerformanceResults();
      void  AddTrace(std::chrono::system_clock::time_point timePoint, const std::string &variableName);
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
#endif
#endif

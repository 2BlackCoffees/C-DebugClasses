// //////////////////////////////////////////////////////////////////////
//
// This file is under the license http://www.apache.org/licenses/LICENSE-2.0
//
// //////////////////////////////////////////////////////////////////////

#include "TraceDebug.hpp"
#ifdef ENABLE_TRACE_DEBUG
int DebugTrace::debugPrintDeepness = 0;
bool DebugTrace::traceActive = true;
std::map<std::string, int> DebugTrace::mapFileNameToLine;
std::map<std::string,
             std::vector<std::pair<std::string,
                                   std::chrono::system_clock::time_point>>>
                          DebugTrace::mapFileNameFunctionNameToVectorTimingInfo;
std::string DebugTrace::GetUniqueKey(const std::string &string1,
                                     const std::string &string2,
                                     const std::string &string3) {
  return string1 + string2 + string3;
}

DebugTrace::DebugTrace(const std::string& functionName,
                       const std::string& fileName,
                       int lineNumber,
                       const std::string& uniqueKey)
{
  keyDebugPerformanceToErase = GetUniqueKey(fileName, functionName, uniqueKey);
  debugPerformanceMustBeDisplayed = true;
  ++debugPrintDeepness;
  lineHeader = fileName + ":" + std::to_string(lineNumber) + " (" +
      functionName + ") [" + uniqueKey + "]";
  AddTrace(std::chrono::steady_clock::now(), "Start measure");
}

DebugTrace::DebugTrace(const std::string &functionName,
                       const std::string &fileName,
                       int lineNumber): debugPrintMustBeDecremented(false)  {
  debugPerformanceMustBeDisplayed = false;
  std::string uniqueKey = GetUniqueKey(fileName, functionName);
  bool savedFileName = mapFileNameToLine.find(uniqueKey) != mapFileNameToLine.end();
  if(!savedFileName || mapFileNameToLine[uniqueKey] == lineNumber) {
    keyDebugPrintToErase = uniqueKey;
    mapFileNameToLine[uniqueKey] = lineNumber;
    ++debugPrintDeepness;
    debugPrintMustBeDecremented = true;
  }
}

DebugTrace::~DebugTrace() {
  if((debugPrintMustBeDecremented || debugPerformanceMustBeDisplayed) &&
     debugPrintDeepness > 0) {
    --debugPrintDeepness;
    if(debugPrintMustBeDecremented) {
      mapFileNameToLine.erase(keyDebugPrintToErase);
    }
  }
  if(debugPrintDeepness == 0) {
    mapFileNameToLine.clear();
  }
  if(debugPerformanceMustBeDisplayed) {
    AddTrace(std::chrono::steady_clock::now(), "End measure");
#ifdef USE_QT_DEBUG
      qDebug() << QString::fromUtf8(GetPerformanceResults().c_str());
#else
    std::cout << GetPerformanceResults() << std::endl;
#endif
    mapFileNameFunctionNameToVectorTimingInfo.erase(keyDebugPerformanceToErase);
  }
}

void DebugTrace::AddTrace(std::chrono::system_clock::time_point timePoint,
                          const std::string & variableName) {
  std::pair<std::string, std::chrono::system_clock::time_point> tmpPair =
      std::make_pair(variableName, timePoint);

  auto vectorTimingInfoIt =
      mapFileNameFunctionNameToVectorTimingInfo.find(keyDebugPerformanceToErase);
  if(vectorTimingInfoIt == mapFileNameFunctionNameToVectorTimingInfo.end()) {
    std::vector<std::pair<std::string,
                          std::chrono::system_clock::time_point>> tmpVector;
    tmpVector.push_back(tmpPair);
    mapFileNameFunctionNameToVectorTimingInfo[keyDebugPerformanceToErase] = tmpVector;
  } else {
    vectorTimingInfoIt->second.push_back(tmpPair);
  }

}

void DebugTrace::PrintPerformanceInfos()
{
  std::string output = "";

}

void DebugTrace::ActiveTrace(bool activate) {
  traceActive = activate;
}

bool DebugTrace::IsTraceActive() {
  return traceActive;
}

std::string DebugTrace::getSpaces() {
  if(debugPrintDeepness > 1) {
    return std::string(2 * (debugPrintDeepness - 1), ' ');
  }
  return "";
}

void DebugTrace::PrintString(const std::string& inStr, bool showHierarchy) {
  std::string str;
  if(showHierarchy) {
    str = DebugTrace::getSpaces() + inStr;
  } else {
    str = inStr;
  }
#ifdef USE_QT_DEBUG
  qDebug() << QString::fromUtf8(str.c_str());
#else
  std::cout << str << std::endl;
#endif
}

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
          std::to_string(
            std::chrono::duration <double, UNIT_TRACE_TEMPLATE_TYPE> (
              valueMax.second - valueMin.second).count()) +
          std::string(UNIT_TRACE_DEBUG);
    }
    if(size > 1)
    {
      const auto& valueMin = performanceInfos[0];
      const auto& valueMax = performanceInfos[size];
      tmp += ", Full time: " + std::to_string(std::chrono::duration <double, UNIT_TRACE_TEMPLATE_TYPE> (
                           valueMax.second - valueMin.second).count()) + std::string(UNIT_TRACE_DEBUG);
    }
  } else {
    tmp += "Not enough trace to display results.";
  }
  return tmp;
}

//cl /EHsc TraceDebug.cpp
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
  START_TRACE_PERFORMANCE(main);
  DISPLAY_DEBUG_VALUE(f1());
  ADD_TRACE_PERFORMANCE(main, "This is the middle");
  f2();
}

/* This program will output:
  Processing f1()  From TraceDebug.cpp:183 (main)
      Processing f2() - 1  From TraceDebug.cpp:177 (f1)
          Processing f3()  From TraceDebug.cpp:172 (f2)
              TraceDebug.cpp:167 (f3)  a = 5
           (Full elapsed: 0.000000 ms) TraceDebug.cpp:165 (f3) [f3], <End measure> - <Start measure> = 2.000200 ms
          ->TraceDebug.cpp:172 (f2)  f3() = 5
       (Full elapsed: 5.000500 ms) TraceDebug.cpp:171 (f2) [f2], <End measure> - <Start measure> = 9.000900 ms
      ->TraceDebug.cpp:177 (f1)  f2() - 1 = 1
   (Full elapsed: 11.001100 ms) TraceDebug.cpp:176 (f1) [f1], <End measure> - <Start measure> = 17.001700 ms
  ->TraceDebug.cpp:183 (main)  f1() = 1
      Processing f3()  From TraceDebug.cpp:172 (f2)
          TraceDebug.cpp:167 (f3)  a = 5
       (Full elapsed: 20.002000 ms) TraceDebug.cpp:165 (f3) [f3], <End measure> - <Start measure> = 2.000200 ms
      ->TraceDebug.cpp:172 (f2)  f3() = 5
   (Full elapsed: 25.002500 ms) TraceDebug.cpp:171 (f2) [f2], <End measure> - <Start measure> = 9.000900 ms
 (Full elapsed: 29.002900 ms) TraceDebug.cpp:182 (main) [main], <This is the middle> - <Start measure> = 25.002500 ms, <End measure> - <This is the middle> = 13.001300 ms, Full time: 38.003800 ms
*/
#endif
#endif

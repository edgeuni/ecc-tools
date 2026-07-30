add_library(
  icts_source_data_manager_adapter_sdc
  ${ICTS_DATA_MANAGER_ADAPTER_SDC}/SdcClockReader.cc
  ${ICTS_DATA_MANAGER_ADAPTER_SDC}/clock_parser/SdcClockParser.cc
  ${ICTS_DATA_MANAGER_ADAPTER_SDC}/clock_parser/SdcClockCommands.cc
  ${ICTS_DATA_MANAGER_ADAPTER_SDC}/clock_parser/SdcClockEvaluator.cc
  ${ICTS_DATA_MANAGER_ADAPTER_SDC}/clock_parser/SdcClockValue.cc
  ${ICTS_DATA_MANAGER_ADAPTER_SDC}/clock_trace/ClockTraceResolver.cc
  ${ICTS_DATA_MANAGER_ADAPTER_SDC}/clock_trace/ClockTracePins.cc
  ${ICTS_DATA_MANAGER_ADAPTER_SDC}/clock_trace/ClockTraceOwnership.cc
  ${ICTS_DATA_MANAGER_ADAPTER_SDC}/clock_trace/ClockTraceResolve.cc
)

target_include_directories(
  icts_source_data_manager_adapter_sdc
  PUBLIC
  ${ICTS_DATA_MANAGER_ADAPTER_SDC}
  ${ICTS_DATA_MANAGER_ADAPTER}
  ${ICTS_DATA_MANAGER}
  PRIVATE
  ${ICTS_DATA_MANAGER_ADAPTER_SDC}/clock_parser
  ${ICTS_DATA_MANAGER_ADAPTER_SDC}/clock_trace
  ${ICTS_TOOLKIT}
  ${HOME_DATABASE}/data/design
  ${HOME_DATABASE}/data/design/db_design
  ${HOME_DATABASE}/data/design/db_layout
  ${HOME_DATABASE}/manager/builder
)

target_link_libraries(
  icts_source_data_manager_adapter_sdc
  PRIVATE
  idm
  icts_source_data_manager_config
  icts_source_toolkit_logger
  liberty
)

if(DEBUG_ICTS_SOURCE_DATA_MANAGER_ADAPTER_SDC)
  if(MSVC)
    target_compile_options(icts_source_data_manager_adapter_sdc PRIVATE /Zi /Od /Ob0)
    target_link_options(icts_source_data_manager_adapter_sdc PRIVATE /DEBUG)
  else()
    target_compile_options(icts_source_data_manager_adapter_sdc PRIVATE -g3 -O0 -fno-inline -fno-omit-frame-pointer)
  endif()
endif()

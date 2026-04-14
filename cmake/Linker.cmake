macro(chart_view_configure_linker project_name)
  set(chart_view_USER_LINKER_OPTION
    "DEFAULT"
      CACHE STRING "Linker to be used")
    set(chart_view_USER_LINKER_OPTION_VALUES "DEFAULT" "SYSTEM" "LLD" "GOLD" "BFD" "MOLD" "SOLD" "APPLE_CLASSIC" "MSVC")
  set_property(CACHE chart_view_USER_LINKER_OPTION PROPERTY STRINGS ${chart_view_USER_LINKER_OPTION_VALUES})
  list(
    FIND
    chart_view_USER_LINKER_OPTION_VALUES
    ${chart_view_USER_LINKER_OPTION}
    chart_view_USER_LINKER_OPTION_INDEX)

  if(${chart_view_USER_LINKER_OPTION_INDEX} EQUAL -1)
    message(
      STATUS
        "Using custom linker: '${chart_view_USER_LINKER_OPTION}', explicitly supported entries are ${chart_view_USER_LINKER_OPTION_VALUES}")
  endif()

  set_target_properties(${project_name} PROPERTIES LINKER_TYPE "${chart_view_USER_LINKER_OPTION}")
endmacro()

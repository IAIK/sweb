# This macro adds a post-build hook to the specified target in order to attach
# SWEB-readable debug info.

macro(ADD_DEBUG_INFO TARGET)
  set(dbg_binary $<TARGET_FILE:${TARGET}>)

  add_custom_command(TARGET ${TARGET}
      POST_BUILD
      COMMAND "${CMAKE_BINARY_DIR}/add-dbg" "${dbg_binary}" "${dbg_binary}.dbg"
      COMMAND "${OBJCOPY_EXECUTABLE}" --remove-section .swebdbg "${dbg_binary}"
      COMMAND "${OBJCOPY_EXECUTABLE}" --add-section .swebdbg="${dbg_binary}.dbg"
                                      --set-section-flags .swebdbg=noload,readonly "${dbg_binary}"
      COMMAND rm -f "${dbg_binary}.dbg"
  )

  # Requires add-dbg to be built first.
  add_dependencies(${TARGET} add-dbg)
endmacro(ADD_DEBUG_INFO)

target_include_directories(kernel PUBLIC
  ../../include/usb
  ../../include/usb/usbd
)

add_project_library(arch_arm_usb)

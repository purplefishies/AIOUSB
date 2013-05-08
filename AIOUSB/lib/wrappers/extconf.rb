require 'mkmf'


$libs = append_library($libs, "usb-1.0")
$libs = append_library($libs, "pthread")
$libs = append_library($libs, "aiousbdbg")
$libs = append_library($libs, "aiousbcppdbg")



create_makefile('AIOUSB')


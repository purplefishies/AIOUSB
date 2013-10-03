#!/usr/bin/env python

"""
setup.py file for SWIG example
"""

from distutils.core import setup, Extension
import os

aiousb_module = Extension('_AIOUSB',
                           sources=[
                                    'AIOUSB_wrap.c'
                                    ],
                          libraries=["usb-1.0","pthread","aiousb","aiousbdbg"],
                          include_dirs=["/usr/include/libusb-1.0",
                                        os.environ.get("AIOUSB_ROOT") + "/lib"
                                        ]
                           )
#aiousb_module.extra_compile_args = ['-Wl,--whole-archive  -lusb-1.0'];

setup(name = '_AIOUSB',
      version = '0.1',
      author      = "Jimi Damon",
      description = """Acces I/O python wrappers""",
      ext_modules = [aiousb_module],
      py_modules = ["AIOUSB"],
      )

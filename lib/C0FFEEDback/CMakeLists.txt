set(LLVM_LINK_COMPONENTS
  Support
  )

add_clang_library(clangC0FFEEDback
  EventHandler.cpp  

  LINK_LIBS
  clangAST
  clangBasic
  clangLex
  clangSema
  )

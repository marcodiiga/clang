set(LLVM_LINK_COMPONENTS
  Core
  MC
  Support
  )

add_clang_library(clangC0FFEED
  C0FFEED.cpp  

  LINK_LIBS
  clangAST
  clangBasic
  clangLex
  clangSema
  )

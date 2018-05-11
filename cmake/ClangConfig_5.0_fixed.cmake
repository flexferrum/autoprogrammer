# This file allows users to call find_package(Clang) and pick up our targets.


# Compute the installation prefix from this LLVMConfig.cmake file location.
get_filename_component(CLANG_INSTALL_PREFIX "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(CLANG_INSTALL_PREFIX "${CLANG_INSTALL_PREFIX}" PATH)
get_filename_component(CLANG_INSTALL_PREFIX "${CLANG_INSTALL_PREFIX}" PATH)
get_filename_component(CLANG_INSTALL_PREFIX "${CLANG_INSTALL_PREFIX}" PATH)

find_package(LLVM REQUIRED CONFIG
             HINTS "${CLANG_INSTALL_PREFIX}/lib/cmake/llvm")

set(CLANG_EXPORTED_TARGETS "clangBasic;clangLex;clangParse;clangAST;clangDynamicASTMatchers;clangASTMatchers;clangSema;clangCodeGen;clangAnalysis;clangEdit;clangRewrite;clangARCMigrate;clangDriver;clangSerialization;clangRewriteFrontend;clang
Frontend;clangFrontendTool;clangToolingCore;clangToolingRefactor;clangTooling;clangIndex;clangStaticAnalyzerCore;clangStaticAnalyzerCheckers;clangStaticAnalyzerFrontend;clangFormat;clang;clang-format;clang-import-test;clangApplyReplacements;c
langReorderFields;modularize;clangTidy;clangTidyAndroidModule;clangTidyBoostModule;clangTidyBugproneModule;clangTidyCERTModule;clangTidyCppCoreGuidelinesModule;clangTidyGoogleModule;clangTidyHICPPModule;clangTidyLLVMModule;clangTidyMiscModule
;clangTidyModernizeModule;clangTidyMPIModule;clangTidyPerformanceModule;clangTidyPlugin;clangTidyReadabilityModule;clang-tidy;clangTidyUtils;clangChangeNamespace;clangQuery;clangMove;clangDaemon;clangd;clangIncludeFixer;clangIncludeFixerPlugi
n;findAllSymbols;libclang")
set(CLANG_CMAKE_DIR "${CLANG_INSTALL_PREFIX}/lib/cmake/clang")
set(CLANG_INCLUDE_DIRS "${CLANG_INSTALL_PREFIX}/include")

# Provide all our library targets to users.

list (APPEND CMAKE_MODULE_PATH ${CLANG_CMAKE_DIR})

include(ClangTargets)
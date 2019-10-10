# these static libraries are available pre-compiled from
# https://github.com/lkeegan/libsbml-static
LIBS += \
    $$PWD/libsbml/lib/libsbml-static.a \
    $$PWD/expat/lib/libexpat.a \
    $$PWD/symengine/lib/libsymengine.a \
    $$PWD/gmp/lib/libgmp.a \
    $$PWD/gmp/lib/libgmpxx.a \
    $$PWD/spdlog/lib/libspdlog.a \
    $$PWD/libtiff/lib/libtiff.a \
    $$PWD/muparser/lib/libmuparser.a \

# these LLVM core static libraries are available pre-compiled from
# https://github.com/lkeegan/llvm-static
LIBS += \
    $$PWD/llvm/lib/libLLVMX86Disassembler.a \
    $$PWD/llvm/lib/libLLVMX86AsmParser.a \
    $$PWD/llvm/lib/libLLVMVectorize.a \
    $$PWD/llvm/lib/libLLVMX86CodeGen.a \
    $$PWD/llvm/lib/libLLVMX86Desc.a \
    $$PWD/llvm/lib/libLLVMX86Info.a \
    $$PWD/llvm/lib/libLLVMMCDisassembler.a \
    $$PWD/llvm/lib/libLLVMX86AsmPrinter.a \
    $$PWD/llvm/lib/libLLVMX86Utils.a \
    $$PWD/llvm/lib/libLLVMSelectionDAG.a \
    $$PWD/llvm/lib/libLLVMGlobalISel.a \
    $$PWD/llvm/lib/libLLVMAsmPrinter.a \
    $$PWD/llvm/lib/libLLVMCodeGen.a \
    $$PWD/llvm/lib/libLLVMScalarOpts.a \
    $$PWD/llvm/lib/libLLVMAggressiveInstCombine.a \
    $$PWD/llvm/lib/libLLVMBitWriter.a \
    $$PWD/llvm/lib/libLLVMMCJIT.a \
    $$PWD/llvm/lib/libLLVMInstCombine.a \
    $$PWD/llvm/lib/libLLVMTransformUtils.a \
    $$PWD/llvm/lib/libLLVMExecutionEngine.a \
    $$PWD/llvm/lib/libLLVMTarget.a \
    $$PWD/llvm/lib/libLLVMAnalysis.a \
    $$PWD/llvm/lib/libLLVMProfileData.a \
    $$PWD/llvm/lib/libLLVMRuntimeDyld.a \
    $$PWD/llvm/lib/libLLVMObject.a \
    $$PWD/llvm/lib/libLLVMMCParser.a \
    $$PWD/llvm/lib/libLLVMBitReader.a \
    $$PWD/llvm/lib/libLLVMMC.a \
    $$PWD/llvm/lib/libLLVMDebugInfoCodeView.a \
    $$PWD/llvm/lib/libLLVMDebugInfoMSF.a \
    $$PWD/llvm/lib/libLLVMAsmParser.a \
    $$PWD/llvm/lib/libLLVMCore.a \
    $$PWD/llvm/lib/libLLVMBinaryFormat.a \
    $$PWD/llvm/lib/libLLVMSupport.a \
    $$PWD/llvm/lib/libLLVMDemangle.a \

# these static libraries should be compiled first in their directories
win32: LIBS += $$PWD/qcustomplot/release/libqcustomplot.a
unix: LIBS += $$PWD/qcustomplot/libqcustomplot.a
LIBS += $$PWD/triangle/triangle.o

# dune requires some definitions
DEFINES += DUNE_LOGGING_VENDORED_FMT=0 ENABLE_GMP=1 ENABLE_UG=1 MUPARSER_STATIC UG_USE_NEW_DIMENSION_DEFINES

# spdlog defines
DEFINES += SPDLOG_FMT_EXTERNAL

# on windows dune-copasi pdelab_expression_adapter doesn't compile unless we first remove unicode support
# NB: may be due to compiling muparser without unicode support?
DEFINES -= UNICODE _UNICODE

# these static libraries are available pre-compiled from
# https://github.com/lkeegan/dune-copasi-static
LIBS += \
    $$PWD/dune/lib/libdune-logging.a \
    $$PWD/dune/lib/libdunepdelab.a \
    $$PWD/dune/lib/libdunegrid.a \
    $$PWD/dune/lib/libdunegeometry.a \
    $$PWD/dune/lib/libugS3.a \
    $$PWD/dune/lib/libugS2.a \
    $$PWD/dune/lib/libugL.a \
    $$PWD/dune/lib/libdunecommon.a \
    $$PWD/dune/lib/libdune_copasi_lib.a \
    $$PWD/fmt/lib/libfmt.a \

# include QT and ext headers as system headers to suppress compiler warnings
QMAKE_CXXFLAGS += \
    -isystem "$$PWD/libsbml/include" \
    -isystem "$$PWD/gmp/include" \
    -isystem "$$PWD/symengine/include" \
    -isystem "$$PWD/qcustomplot" \
    -isystem "$$PWD/triangle" \
    -isystem "$$PWD/spdlog/include" \
    -isystem "$$PWD/llvm/include" \
    -isystem "$$PWD/muparser/include" \
    -isystem "$$PWD/libtiff/include" \
    -isystem "$$PWD/fmt/include" \
    -isystem "$$PWD/dune/include" \

# LLVM on linux also needs libdl:
unix:!mac {
    LIBS += -ldl
}

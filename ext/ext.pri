include(../topdir.pri)

# these static libraries are available pre-compiled from
# https://github.com/lkeegan/libsbml-static
LIBS += \
    $${TOPDIR}/ext/libsbml/lib/libsbml-static.a \
    $${TOPDIR}/ext/expat/lib/libexpat.a \
    $${TOPDIR}/ext/symengine/lib/libsymengine.a \
    $${TOPDIR}/ext/gmp/lib/libgmp.a \
    $${TOPDIR}/ext/gmp/lib/libgmpxx.a \
    $${TOPDIR}/ext/spdlog/lib/libspdlog.a \
    $${TOPDIR}/ext/libtiff/lib/libtiff.a \
    $${TOPDIR}/ext/muparser/lib/libmuparser.a \

# these LLVM core static libraries are available pre-compiled from
# https://github.com/lkeegan/llvm-static
LIBS += \
    $${TOPDIR}/ext/llvm/lib/libLLVMX86Disassembler.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMX86AsmParser.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMVectorize.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMX86CodeGen.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMX86Desc.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMX86Info.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMMCDisassembler.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMX86AsmPrinter.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMX86Utils.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMSelectionDAG.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMGlobalISel.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMAsmPrinter.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMCodeGen.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMScalarOpts.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMAggressiveInstCombine.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMBitWriter.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMMCJIT.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMInstCombine.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMTransformUtils.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMExecutionEngine.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMTarget.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMAnalysis.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMProfileData.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMRuntimeDyld.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMObject.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMMCParser.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMBitReader.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMMC.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMDebugInfoCodeView.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMDebugInfoMSF.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMAsmParser.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMCore.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMBinaryFormat.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMSupport.a \
    $${TOPDIR}/ext/llvm/lib/libLLVMDemangle.a \

# these static libraries should be compiled first in their directories
#win32: LIBS += $${TOPDIR}/ext/qcustomplot/release/libqcustomplot.a
LIBS += \
    $${TOPDIR}/ext/qcustomplot/libqcustomplot.a \
    $${TOPDIR}/ext/triangle/triangle.o \

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
    $${TOPDIR}/ext/dune/lib/libdune-logging.a \
    $${TOPDIR}/ext/dune/lib/libdunepdelab.a \
    $${TOPDIR}/ext/dune/lib/libdunegrid.a \
    $${TOPDIR}/ext/dune/lib/libdunegeometry.a \
    $${TOPDIR}/ext/dune/lib/libugS3.a \
    $${TOPDIR}/ext/dune/lib/libugS2.a \
    $${TOPDIR}/ext/dune/lib/libugL.a \
    $${TOPDIR}/ext/dune/lib/libdunecommon.a \
    $${TOPDIR}/ext/dune/lib/libdune_copasi_lib.a \
    $${TOPDIR}/ext/fmt/lib/libfmt.a \

# include QT and ext headers as system headers to suppress compiler warnings
QMAKE_CXXFLAGS += \
    -isystem "$${TOPDIR}/ext/libsbml/include" \
    -isystem "$${TOPDIR}/ext/gmp/include" \
    -isystem "$${TOPDIR}/ext/symengine/include" \
    -isystem "$${TOPDIR}/ext/qcustomplot" \
    -isystem "$${TOPDIR}/ext/triangle" \
    -isystem "$${TOPDIR}/ext/spdlog/include" \
    -isystem "$${TOPDIR}/ext/llvm/include" \
    -isystem "$${TOPDIR}/ext/muparser/include" \
    -isystem "$${TOPDIR}/ext/libtiff/include" \
    -isystem "$${TOPDIR}/ext/fmt/include" \
    -isystem "$${TOPDIR}/ext/dune/include" \
    -isystem "$${TOPDIR}/ext/catch" \

# LLVM on linux also needs libdl:
unix:!mac {
    LIBS += -ldl
}

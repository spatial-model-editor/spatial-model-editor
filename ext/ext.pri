# these static libraries are available pre-compiled from
# https://github.com/lkeegan/libsbml-static
LIBS += \
    $$PWD/libsbml/lib/libsbml-static.a \
    $$PWD/expat/lib/libexpat.a \
    $$PWD/symengine/lib/libsymengine.a \
    $$PWD/gmp/lib/libgmp.a \
    $$PWD/gmp/lib/libgmpxx.a \
    $$PWD/spdlog/lib/spdlog/libspdlog.a \
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
DEFINES += DUNE_LOGGING_VENDORED_FMT=1 ENABLE_GMP=1 ENABLE_UG=1 HAVE_CONFIG_H MUPARSER_STATIC UG_USE_NEW_DIMENSION_DEFINES

# on windows dune-copasi pdelab_expression_adapter doesn't compile unless we first remove unicode support
# NB: may be due to compiling muparser without unicode support?
DEFINES -= UNICODE _UNICODE

# these static libraries are available pre-compiled from
# https://github.com/lkeegan/dune-copasi-static
LIBS += \
    $$PWD/dune/dune-logging/build-cmake/lib/libdune-logging.a \
    $$PWD/dune/dune-logging/build-cmake/lib/libdune-logging-fmt.a \
    $$PWD/dune/dune-pdelab/build-cmake/lib/libdunepdelab.a \
    $$PWD/dune/dune-grid/build-cmake/lib/libdunegrid.a \
    $$PWD/dune/dune-geometry/build-cmake/lib/libdunegeometry.a \
    $$PWD/dune/dune-uggrid/build-cmake/lib/libugS3.a \
    $$PWD/dune/dune-uggrid/build-cmake/lib/libugS2.a \
    $$PWD/dune/dune-uggrid/build-cmake/lib/libugL.a \
    $$PWD/dune/dune-common/build-cmake/lib/libdunecommon.a \
    $$PWD/dune/dune-copasi/build-cmake/lib/libdune_copasi_lib.a \

INCLUDEPATH += \
    $$PWD/dune/dune-copasi \
    $$PWD/dune/dune-copasi/build-cmake \
    $$PWD/dune/dune-uggrid/low \
    $$PWD/dune/dune-uggrid/gm \
    $$PWD/dune/dune-uggrid/dom \
    $$PWD/dune/dune-uggrid/np \
    $$PWD/dune/dune-uggrid/ui \
    $$PWD/dune/dune-uggrid/np/algebra \
    $$PWD/dune/dune-uggrid/np/udm \
    $$PWD/dune/dune-multidomaingrid \
    $$PWD/dune/dune-pdelab \
    $$PWD/dune/dune-logging \
    $$PWD/dune/dune-common \
    $$PWD/dune/dune-uggrid \
    $$PWD/dune/dune-geometry \
    $$PWD/dune/dune-typetree \
    $$PWD/dune/dune-istl \
    $$PWD/dune/dune-grid \
    $$PWD/dune/dune-localfunctions \
    $$PWD/dune/dune-functions \
    $$PWD/dune/dune-logging/vendor/fmt/include \
    $$PWD/dune/dune-logging/build-cmake/dune/vendor/fmt \

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
    -isystem "$$PWD/build/dune-copasi" \
    -isystem "$$PWD/build/dune-copasi/build-cmake" \
    -isystem "$$PWD/build/dune-uggrid/low" \
    -isystem "$$PWD/build/dune-uggrid/gm" \
    -isystem "$$PWD/build/dune-uggrid/dom" \
    -isystem "$$PWD/build/dune-uggrid/np" \
    -isystem "$$PWD/build/dune-uggrid/ui" \
    -isystem "$$PWD/build/dune-uggrid/np/algebra" \
    -isystem "$$PWD/build/dune-uggrid/np/udm" \
    -isystem "$$PWD/build/dune-multidomaingrid" \
    -isystem "$$PWD/build/dune-pdelab" \
    -isystem "$$PWD/build/dune-logging" \
    -isystem "$$PWD/build/dune-common" \
    -isystem "$$PWD/build/dune-uggrid" \
    -isystem "$$PWD/build/dune-geometry" \
    -isystem "$$PWD/build/dune-typetree" \
    -isystem "$$PWD/build/dune-istl" \
    -isystem "$$PWD/build/dune-grid" \
    -isystem "$$PWD/build/dune-localfunctions" \
    -isystem "$$PWD/build/dune-functions" \
    -isystem "$$PWD/build/dune-logging/build-cmake/dune/vendor/fmt" \

# LLVM on linux also needs libdl:
unix:!mac {
    LIBS += -ldl
}

include(../topdir.pri)

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
    $${TOPDIR}/ext/dune/lib/libdune_copasi_md_lib_fv_cg.a \
    $${TOPDIR}/ext/dune/lib/libdune_copasi_sd_lib_cg.a \
    $${TOPDIR}/ext/dune/lib/libdune_copasi_sd_lib_fv.a \
    $${TOPDIR}/ext/dune/lib/libdune_copasi_lib.a \
    $${TOPDIR}/ext/dune/lib/libdune-logging.a \
    $${TOPDIR}/ext/dune/lib/libdunepdelab.a \
    $${TOPDIR}/ext/dune/lib/libdunegrid.a \
    $${TOPDIR}/ext/dune/lib/libdunegeometry.a \
    $${TOPDIR}/ext/dune/lib/libugS3.a \
    $${TOPDIR}/ext/dune/lib/libugS2.a \
    $${TOPDIR}/ext/dune/lib/libugL.a \
    $${TOPDIR}/ext/dune/lib/libdunecommon.a \

# these static libraries are available pre-compiled from
# https://github.com/lkeegan/libsbml-static
LIBS += \
    $${TOPDIR}/ext/lib/libsbml-static.a \
    $${TOPDIR}/ext/lib/libexpat.a \
    $${TOPDIR}/ext/lib/libsymengine.a \
    $${TOPDIR}/ext/lib/libgmp.a \
    $${TOPDIR}/ext/lib/libgmpxx.a \
    $${TOPDIR}/ext/lib/libspdlog.a \
    $${TOPDIR}/ext/lib/libfmt.a \
    $${TOPDIR}/ext/lib/libtiff.a \
    $${TOPDIR}/ext/lib/libmuparser.a \
    $${TOPDIR}/ext/lib/libtbb.a \

# these LLVM core static libraries are available pre-compiled from
# https://github.com/lkeegan/llvm-static
LIBS += \
    $${TOPDIR}/ext/lib/libLLVMAsmParser.a \
    $${TOPDIR}/ext/lib/libLLVMMCJIT.a \
    $${TOPDIR}/ext/lib/libLLVMExecutionEngine.a \
    $${TOPDIR}/ext/lib/libLLVMRuntimeDyld.a \
    $${TOPDIR}/ext/lib/libLLVMX86CodeGen.a \
    $${TOPDIR}/ext/lib/libLLVMAsmPrinter.a \
    $${TOPDIR}/ext/lib/libLLVMDebugInfoDWARF.a \
    $${TOPDIR}/ext/lib/libLLVMGlobalISel.a \
    $${TOPDIR}/ext/lib/libLLVMSelectionDAG.a \
    $${TOPDIR}/ext/lib/libLLVMCodeGen.a \
    $${TOPDIR}/ext/lib/libLLVMTarget.a \
    $${TOPDIR}/ext/lib/libLLVMBitWriter.a \
    $${TOPDIR}/ext/lib/libLLVMScalarOpts.a \
    $${TOPDIR}/ext/lib/libLLVMInstCombine.a \
    $${TOPDIR}/ext/lib/libLLVMAggressiveInstCombine.a \
    $${TOPDIR}/ext/lib/libLLVMVectorize.a \
    $${TOPDIR}/ext/lib/libLLVMTransformUtils.a \
    $${TOPDIR}/ext/lib/libLLVMAnalysis.a \
    $${TOPDIR}/ext/lib/libLLVMProfileData.a \
    $${TOPDIR}/ext/lib/libLLVMX86AsmParser.a \
    $${TOPDIR}/ext/lib/libLLVMX86Desc.a \
    $${TOPDIR}/ext/lib/libLLVMObject.a \
    $${TOPDIR}/ext/lib/libLLVMBitReader.a \
    $${TOPDIR}/ext/lib/libLLVMCore.a \
    $${TOPDIR}/ext/lib/libLLVMRemarks.a \
    $${TOPDIR}/ext/lib/libLLVMBitstreamReader.a \
    $${TOPDIR}/ext/lib/libLLVMMCParser.a \
    $${TOPDIR}/ext/lib/libLLVMX86Disassembler.a \
    $${TOPDIR}/ext/lib/libLLVMX86Info.a \
    $${TOPDIR}/ext/lib/libLLVMMCDisassembler.a \
    $${TOPDIR}/ext/lib/libLLVMMC.a \
    $${TOPDIR}/ext/lib/libLLVMBinaryFormat.a \
    $${TOPDIR}/ext/lib/libLLVMDebugInfoCodeView.a \
    $${TOPDIR}/ext/lib/libLLVMDebugInfoMSF.a \
    $${TOPDIR}/ext/lib/libLLVMX86Utils.a \
    $${TOPDIR}/ext/lib/libLLVMSupport.a \
    $${TOPDIR}/ext/lib/libLLVMDemangle.a \

# these static libraries should be compiled first in their directories
LIBS += \
    $${TOPDIR}/ext/qcustomplot/libqcustomplot.a \
    $${TOPDIR}/ext/triangle/triangle.o \

# include QT and ext headers as system headers to suppress compiler warnings
QMAKE_CXXFLAGS += \
    -isystem "$${TOPDIR}/ext/qcustomplot" \
    -isystem "$${TOPDIR}/ext" \
    -isystem "$${TOPDIR}/ext/catch" \
    -isystem "$${TOPDIR}/ext/include" \
    -isystem "$${TOPDIR}/ext/dune/include" \

# LLVM on linux also needs libdl:
unix:!mac {
    LIBS += -ldl
}

# these static libraries are available pre-compiled from
# https://github.com/lkeegan/libsbml-static
LIBS += \
    $$PWD/libsbml/lib/libsbml-static.a \
    $$PWD/expat/lib/libexpat.a \
    $$PWD/symengine/lib/libsymengine.a \
    $$PWD/gmp/lib/libgmp.a \
    $$PWD/spdlog/lib/spdlog/libspdlog.a \

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
LIBS += \
    $$PWD/qcustomplot/libqcustomplot.a \
    $$PWD/triangle/triangle.o \

# include QT and ext headers as system headers to suppress compiler warnings
QMAKE_CXXFLAGS += \
    -isystem "$$PWD/libsbml/include" \
    -isystem "$$PWD/gmp/include" \
    -isystem "$$PWD/symengine/include" \
    -isystem "$$PWD/qcustomplot" \
    -isystem "$$PWD/triangle" \
    -isystem "$$PWD/spdlog/include" \
    -isystem "$$PWD/llvm/include" \

# LLVM on linux also needs libdl:
unix:!mac {
    LIBS += -ldl
}

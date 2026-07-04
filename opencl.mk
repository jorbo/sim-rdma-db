# Definition of include file locations
ifneq ($(HOST_ARCH), x86)
	xrt_path = $(SYSROOT)/opt/xilinx/xrt/
	OPENCL_INCLUDE:= $(xrt_path)/include/xrt
else
	xrt_path = $(XILINX_XRT)
	OPENCL_INCLUDE:= $(xrt_path)/include
endif

VIVADO_INCLUDE:= $(XILINX_VIVADO)/include
# hls_stream.h / ap_int.h ship with Vitis HLS since the 2020.2 toolchain
# split; host code that includes kernel headers needs them too.
HLS_INCLUDE:= $(XILINX_HLS)/include
opencl_CXXFLAGS=-I$(OPENCL_INCLUDE) -I$(VIVADO_INCLUDE) -I$(HLS_INCLUDE)
OPENCL_LIB:= $(xrt_path)/lib
# libxilinxopencl provides the xcl* extension entry points (e.g.
# xclGetMemObjDeviceAddress), which the ICD libOpenCL does not export.
opencl_LDFLAGS=-L$(OPENCL_LIB) -lOpenCL -lxilinxopencl -pthread

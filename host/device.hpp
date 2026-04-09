#ifndef DEVICE_HPP
#define DEVICE_HPP


#include "myopencl.hpp"


std::vector<cl::Device> get_devices(const std::string& vendor_name);
std::vector<cl::Device> get_xil_devices();
std::vector<unsigned char> read_binary_file(const std::string& xclbin_file_name);


#endif

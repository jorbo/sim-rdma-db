#include "device.hpp"
#include <fstream>
#include <iostream>
#include <CL/cl_ext_xilinx.h>


using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::cerr;


vector<cl::Device> get_devices(const string& vendor_name) {
	size_t i;
	cl_int err;
	vector<cl::Platform> platforms;
	OCL_CHECK(err, err = cl::Platform::get(&platforms));
	cl::Platform platform;

	for (i = 0; i < platforms.size(); i++) {
		platform = platforms[i];
		OCL_CHECK(err,
			string platformName = platform.getInfo<CL_PLATFORM_NAME>(&err)
		);
		if (!(platformName.compare(vendor_name))) {
			cout << "Found Platform" << endl;
			cout << "Platform Name: " << platformName.c_str() << endl;
			break;
		}
	}
	if (i == platforms.size()) {
		cerr << "Error: Failed to find Xilinx platform" << endl;
		cout << "Found the following platforms : " << endl;
		for (size_t j = 0; j < platforms.size(); j++) {
			platform = platforms[j];
			OCL_CHECK(err,
				string platformName = platform.getInfo<CL_PLATFORM_NAME>(&err)
			);
			cout << "Platform Name: " << platformName.c_str() << endl;
		}
		exit(EXIT_FAILURE);
	}
	// Getting ACCELERATOR Devices and selecting 1st such device
	vector<cl::Device> devices;
	OCL_CHECK(err,
		err = platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices
	));
	return devices;
}

vector<cl::Device> get_xil_devices() {
	return get_devices("Xilinx");
}


vector<unsigned char> read_binary_file(const string& xclbin_file_name) {
	cout << "INFO: Reading " << xclbin_file_name << endl;
	FILE* fp;
	if ((fp = fopen(xclbin_file_name.c_str(), "r")) == nullptr) {
		printf("ERROR: %s xclbin not available please build\n",
			xclbin_file_name.c_str());
		exit(EXIT_FAILURE);
	}
	// Loading XCL Bin into char buffer
	cout << "Loading: '" << xclbin_file_name.c_str() << "'\n";
	std::ifstream bin_file(xclbin_file_name.c_str(), std::ifstream::binary);
	bin_file.seekg(0, bin_file.end);
	auto nb = bin_file.tellg();
	bin_file.seekg(0, bin_file.beg);
	vector<unsigned char> buf;
	buf.resize(nb);
	bin_file.read(reinterpret_cast<char*>(buf.data()), nb);
	return buf;
}

#ifndef HOST_HPP
#define HOST_HPP


#include "run-tree.hpp"
#include "bootstrap.hpp"


TreeOutput run_fpga_tree(TreeInput& input, const RdmaConfig& rdma,
                         std::string const& binaryFile);


#endif

#include "run-tree.hpp"
extern "C" {
#include "../krnl/core/memory.h"
#include "../krnl/core/node.h"
};


TreeInput::TreeInput() {
	memory.resize(MEM_SIZE);
	mem_reset_all(memory.data());
}

TreeOutput::TreeOutput() {
	memory.resize(MEM_SIZE);
}

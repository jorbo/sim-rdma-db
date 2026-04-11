#include "run-tree.hpp"
extern "C" {
#include "../krnl/core/memory.h"
#include "../krnl/core/node.h"
};


TreeInput::TreeInput() : root(bptr_make(0, 0)) {
	memory.resize(MEM_SIZE);
	mem_context_t ctx = mem_context_local(0, memory.data());
	mem_reset_all(&ctx);
}

TreeOutput::TreeOutput() : root(bptr_make(0, 0)) {
	memory.resize(MEM_SIZE);
}

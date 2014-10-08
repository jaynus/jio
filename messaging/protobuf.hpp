#pragma once

#ifdef _DEBUG
#pragma comment(lib, "libprotobufD.lib")
#else
#pragma comment(lib, "libprotobuf.lib")
#endif

namespace jio {
	namespace messaging {
		int test(void) { return -4; }
	}
}
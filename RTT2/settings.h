#pragma once

#define RTT2_PRINT_LOG

#define RTT2_EPSILON (1e-6)

namespace rtt2 {
#ifdef RTT2_USE_FLOAT
	typedef float rtt2_float;
#else
	typedef double rtt2_float;
#endif
}

#pragma once
#include <oneapi/tbb/enumerable_thread_specific.h>

template <typename T>
using ThreadLocal = tbb::enumerable_thread_specific<T>;
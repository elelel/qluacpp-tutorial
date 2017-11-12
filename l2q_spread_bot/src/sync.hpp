#pragma once

#include <mutex>

extern std::recursive_mutex mutex;
using lock_guard = std::lock_guard<std::recursive_mutex>;

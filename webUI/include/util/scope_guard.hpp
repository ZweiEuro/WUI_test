#include <mutex>

namespace mg8
{
    using ScopeGuard = std::unique_lock<std::mutex>;
}
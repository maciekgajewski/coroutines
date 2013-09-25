#include "context.hpp"

namespace coroutines {

context::context(coroutine_scheduler* parent)
    : _parent(parent)
{
}

}

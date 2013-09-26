#include "coroutine.hpp"
#include "context.hpp"

#include "monitor.hpp"

namespace coroutines {

monitor::monitor()
{
}

monitor::~monitor()
{
    assert(_waiting.empty());
}

void monitor::wait()
{
    coroutine* coro = coroutine::current_corutine();
    assert(coro);

    std::cout << "MONITOR: '" << coro->name() << "' will wait" << std::endl;

    coro->yield([coro, this]()
    {
        _waiting.push(std::move(*coro));
    });
}

void monitor::wake_all()
{
    context* ctx = context::current_context();
    assert(ctx);

    std::list<coroutine> waiting;
    _waiting.get_all(waiting);

    std::cout << "MONITOR: waking up " << waiting.size() << " coroutines" << std::endl;

    ctx->enqueue(waiting);
}

}

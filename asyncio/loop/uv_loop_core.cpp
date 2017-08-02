#include <sstream>

#include "loop_exception.hpp"
#include "uv_loop_core.hpp"
#include "uv_timer_handle.hpp"

using namespace std;
USING_ASYNNCIO_NAMESPACE;

UVLoopCore::UVLoopCore() : _activeHandles(0) {
  _loop = new uv_loop_t;
  uv_loop_init(_loop);
  _owner = true;
}

UVLoopCore::UVLoopCore(uv_loop_t *uvLoop) : _activeHandles(0) {
  _loop = uvLoop;
  _owner = false;
}

UVLoopCore::~UVLoopCore() { close(); }
void UVLoopCore::runOneIteration() { uv_run(_loop, UV_RUN_ONCE); }
void UVLoopCore::close() { closeUVLoopT(); }

size_t UVLoopCore::activeHandlesCount() { return _activeHandles; }
uint64_t UVLoopCore::time() { return uv_now(_loop); }

TimerHandle *UVLoopCore::callSoon(TimerCallback callback, void *data) {
  return callLater(0, callback, data);
}

TimerHandle *UVLoopCore::callSoonThreadSafe(TimerCallback callback,
                                            void *data) {
  return new UVTimerHandle(this, callback, data);
}

TimerHandle *UVLoopCore::callLater(uint64_t milliseconds,
                                   TimerCallback callback, void *data) {
  auto handle = new UVTimerHandle(this, callback, data);
  handle->setupTimer(milliseconds);
  return handle;
}

void UVLoopCore::closeUVLoopT() {
  if (_loop) {
    runOneIteration(); // clear pending closing callback;
  }
  if (_owner && _loop) {
    switch (int err = uv_loop_close(_loop)) {
    case 0:
      break;
    case UV_EBUSY:
      throw LoopBusyError();
      break;
    default:
      stringstream ss;
      ss << "uv_loop_close() failed. error: " << err;
      throw LoopException(ss.str());
    }
    delete _loop;
  }
  _loop = nullptr;
}
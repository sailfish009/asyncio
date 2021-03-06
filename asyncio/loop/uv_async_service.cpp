#include "uv_async_service.hpp"
#include "loop_exception.hpp"
#include "uv_handle.hpp"

using namespace std;
USING_ASYNNCIO_NAMESPACE;

UVAsyncService::UVAsyncService(uv_loop_t *uvLoop) : UVService(uvLoop) {
  setupService();
}

TimerHandle *UVAsyncService::callSoon(TimerCallback callback, void *data) {
  auto handle = new UVASyncTimerHandle(this);
  handle->reset(callback, data);
  handle->setupTimer();
  return (TimerHandle *)handle;
}

void UVAsyncService::doStartTimer(UVHandle *handle) {
  handle->addRef();
  pushTimer(handle);
  uvAsyncSend();
}

bool UVAsyncService::doCancelTimer(UVHandle *handle) {
  if (eraseTimer(handle)) {
    handle->subRef();
  }
  return true;
}

void UVAsyncService::addHandle() {
  lock_guard<mutex> lock(_mutex);
  UVService::addHandle();
}

void UVAsyncService::subHandle() {
  lock_guard<mutex> lock(_mutex);
  UVService::subHandle();
}

void UVAsyncService::setupService() {
  uv_async_t *uvAsync = new uv_async_t;
  int err = uv_async_init(_uvLoop, uvAsync, [](uv_async_t *h) {
    auto service = (UVAsyncService *)(h->data);
    service->processTimers();
  });
  if (err != 0) {
    delete uvAsync;
    throw LoopException("uv_async_init() failed.");
  }
  _uvAsync = uvAsync;
  _uvAsync->data = this;
}

void UVAsyncService::processTimers() {
  while (true) {
    UVHandle *handle = popTimer();
    if (handle) {
      handle->processTimer();
      handle->subRef();
    } else {
      return;
    }
  };
}

void UVAsyncService::uvAsyncSend() {
  int err = uv_async_send(_uvAsync);
  if (err != 0) {
    throw LoopException("uv_async_send() failed.");
  }
}

void UVAsyncService::pushTimer(UVHandle *handle) {
  lock_guard<mutex> lock(_mutex);
  _queue.push(handle);
}

UVHandle *UVAsyncService::popTimer() {
  lock_guard<mutex> lock(_mutex);
  if (!_queue.empty()) {
    UVHandle *handle = _queue.front();
    _queue.pop();
    return handle;
  } else {
    return nullptr;
  }
}

bool UVAsyncService::eraseTimer(UVHandle *handle) {
  lock_guard<mutex> lock(_mutex);
  return _queue.erase(handle);
}

void UVAsyncService::close() {
  UVService::close();

  uv_close((uv_handle_t *)(_uvAsync), [](uv_handle_t *h) {
    auto handle = (uv_async_t *)h;
    delete handle;
  });
}
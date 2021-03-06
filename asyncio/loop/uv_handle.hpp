#pragma once

#include <asyncio/common.hpp>

#include "loop_core.hpp"
#include "timer_handle.hpp"
#include "uv.h"

BEGIN_ASYNCIO_NAMESPACE;
class UVService;
class UVHandle : public TimerHandle {
public:
  UVHandle(UVService *service, TimerCallback callback = nullptr,
           void *data = nullptr);
  void reset(TimerCallback callback, void *data);

  void processTimer(); // promote protected process() to public
  void setupTimer();   // promote protected startTimer() to public

protected:
  void doStartTimer() override;
  bool executeTimer() override;
  bool cancelTimer() override;
  void doStopTimer() override;
  void recycle() override;
  uv_loop_t *getUVLoop();

protected:
  UVService *_service;
  TimerCallback _callback;
};

END_ASYNCIO_NAMESPACE;
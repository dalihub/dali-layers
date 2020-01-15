//
// Created by adam.b on 04/12/19.
//

#ifndef DALI_LAYERS_EVENT_SYNC_TIMER_H
#define DALI_LAYERS_EVENT_SYNC_TIMER_H

#include <dali/integration-api/events/touch-event-integ.h>
#include <memory>

namespace Dali
{
namespace Integration
{
class Event;
class HoverEvent;
class Core;
}

namespace Validation
{
struct TouchEventPayload
{
  Dali::Integration::TouchEvent  touchEvent;
  uint32_t                       frame;
};

/**
 * EventSyncTimer runs as often as possible and is responsible
 * for feeding DALi with user input. User input can come
 * either from pre-recorded file or be live.
 */
struct EventSyncTimer
{
  explicit EventSyncTimer( Dali::Integration::Core* core );

  ~EventSyncTimer();

  void Initialise();

  /** Starts timer */
  void Start();

  /** Queues new event.
   * Events come
   * @param event
   */
  void QueueEvent( TouchEventPayload event );

  struct Impl;
  std::unique_ptr<Impl> mImpl;

  Impl* GetImpl() const;
};
}
}

#endif //DALI_LAYERS_EVENT_SYNC_TIMER_H

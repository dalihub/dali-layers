//
// Created by adam.b on 04/12/19.
//

#include "event-sync-timer.h"

#include "autoreplay-layer.h"
#include <dali/devel-api/validation-layers.h>
#include <dali/integration-api/core.h>
#include <dali/public-api/adaptor-framework/timer.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
namespace Dali
{
namespace Validation
{

struct EventSyncTimer::Impl : public Dali::ConnectionTracker
{
  explicit Impl( EventSyncTimer* _owner ) :
   owner(_owner)
  {
  }

  bool running { true };

  virtual ~Impl()
  {
    running = false;
  }

  bool isReady{false};

  void Wait()
  {
    while( running && syncObject )
    {
      std::this_thread::yield();
    }
  }

  bool Tick()
  {
    isReady = true;
    bool expected = true;

    if(syncObject)
    {
      auto layer = AutoReplayLayer::Get();
      if( !touchEvents.empty() )
      {
        // wait
        int counter = 0;
        bool queued{ false };

        uint32_t currentFrame = layer->GetCurrentFrameCount();
        auto recordNotReplay = layer->GetMode() == Dali::Validation::Mode::RECORD;
        while(layer->LayerGetDeltaTime()
              && !touchEvents.empty()
              && (recordNotReplay || touchEvents.front().frame <= currentFrame) )
        {
          auto& event = touchEvents.front();

          uint32_t frame = event.frame;
          auto touchEvent{ event.touchEvent };
          touchEvents.pop();

          core->QueueEvent( touchEvent );

          printf("REP: Queued: [%d] %d\n", int(frame), int(touchEvent.time));
        }
      }
      syncObject = false;
    }
    return true;
  }

  bool HasEvents()
  {
    return !touchEvents.empty();
  }

  void WakeUp()
  {
    syncObject = true;
  }

  void QueueEvent( const TouchEventPayload& event )
  {
    touchEvents.push( event );
  }

  void Start()
  {
    timer = Dali::Timer::New(1);
    timer.TickSignal().Connect( this, &Impl::Tick );
    timer.Start();
  }

  EventSyncTimer* owner;
  Dali::Integration::Core* core;
  Dali::Timer     timer;
  std::queue<TouchEventPayload> touchEvents;
  std::mutex lockMutex;
  std::condition_variable conditionalVariable;
  std::atomic<bool> syncObject;

};

EventSyncTimer::EventSyncTimer( Dali::Integration::Core* core )
{
  mImpl = std::make_unique<Impl>( this );
  mImpl->core = core;
}

EventSyncTimer::~EventSyncTimer() = default;


void EventSyncTimer::Initialise()
{

}

void EventSyncTimer::Start()
{
  mImpl->Start();
}

void EventSyncTimer::QueueEvent( TouchEventPayload event )
{
  mImpl->QueueEvent( event );
}

EventSyncTimer::Impl* EventSyncTimer::GetImpl() const
{
  return mImpl.get();
}


} // Validation
} // Dali

/**
 * OVERRIDES
 */

namespace Dali
{
namespace Integration
{
/**
 *  Core::Initialize() is responsible for starting EventSyncTimer
 */
DALI_EXPORT_API void Core::Initialize()
{
  auto layer = Dali::Validation::AutoReplayLayer::Get();

  auto node = Dali::Validation::GetNextDispatchNode();

  if( Dali::Validation::IsLayerEnabled( nullptr ) )
  {
    // Initialise core
    Dali::Validation::InvokeDispatchNode( node, this );

    // Initialise timer
    layer->InitialiseEventSyncTimer( this );
  }
  else
  {
    Dali::Validation::InvokeDispatchNode( node, this );
  }
}

/**
 * Core::Update() is responsible for spoofing fixed timestep
 * and synchronizing timer with update.
 * @param elapsedSeconds
 * @param lastVSyncTimeMilliseconds
 * @param nextVSyncTimeMilliseconds
 * @param status
 * @param renderToFboEnabled
 * @param isRenderingToFbo
 */
DALI_EXPORT_API void Core::Update( float elapsedSeconds, uint32_t lastVSyncTimeMilliseconds, uint32_t nextVSyncTimeMilliseconds, UpdateStatus& status, bool renderToFboEnabled, bool isRenderingToFbo )
{
  auto layer = Dali::Validation::AutoReplayLayer::Get();
  uint32_t currentTime = layer->GetCurrentFrameCount() * layer->GetFrameTimeInMillis();

  auto node = Dali::Validation::GetNextDispatchNode();

  if( Dali::Validation::IsLayerEnabled( nullptr ) )
  {
    // let timer submit

    auto eventSyncTimer = layer->GetEventSyncTimer();
    if(eventSyncTimer->mImpl->isReady && eventSyncTimer->mImpl->HasEvents())
    {
      puts("Update Prepare()");
      eventSyncTimer->mImpl->WakeUp();
      eventSyncTimer->mImpl->Wait();
      puts("Update Now()");
    }

    if( layer->GetMode() == Validation::Mode::REPLAY &&
      !eventSyncTimer->mImpl->HasEvents() && layer->TerminateOnFinish() )
    {
      exit(0);
    }

    // Spoof fixed time step
    elapsedSeconds = 0.001f * float(layer->GetFrameTimeInMillis());
    lastVSyncTimeMilliseconds = currentTime;
    nextVSyncTimeMilliseconds = currentTime + layer->GetFrameTimeInMillis();
    Dali::Validation::InvokeDispatchNode( node, this,
                                          elapsedSeconds,
                                          lastVSyncTimeMilliseconds,
                                          nextVSyncTimeMilliseconds,
                                          &status,
                                          renderToFboEnabled,
                                          isRenderingToFbo
    );

    // Make sure DALi is updating
    status.needsNotification = true;
    status.keepUpdating = uint32_t(true);
  }
  else
  {
    Dali::Validation::InvokeDispatchNode( node, this,
                                          elapsedSeconds,
                                          lastVSyncTimeMilliseconds,
                                          nextVSyncTimeMilliseconds,
                                          &status,
                                          renderToFboEnabled,
                                          isRenderingToFbo
    );
  }
}
} // Integration
} // Dali
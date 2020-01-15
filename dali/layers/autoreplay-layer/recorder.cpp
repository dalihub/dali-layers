//
// Created by adam.b on 29/11/19.
//

#include <dali/internal/event/events/event-processor.h>
#include <dali/integration-api/events/event.h>
#include <dali/internal/event/common/scene-impl.h>

#include <dali/integration-api/events/touch-event-integ.h>
#include <dali/devel-api/validation-layers.h>
#include "autoreplay-layer.h"
#include "multi-point-event-serializer.h"
#include <queue>
#include "recorder.h"
#include "event-sync-timer.h"

using Dali::Integration::Event;

namespace
{
namespace
{

void SerializeTouchPoint( const Dali::Integration::TouchEvent& touchEvent )
{
  auto layer = Dali::Validation::AutoReplayLayer::Get();

  static FILE* outputFile = nullptr;
  if(!outputFile)
  {
    auto filePath = layer->GetRecordSimFilePath();
    outputFile = fopen( filePath.c_str(), "wb");
  }
  std::vector<uint8_t> buffer{};
  auto frame = layer->GetCurrentFrameCount();
  Dali::Integration::SerializeType( buffer, frame );
  Dali::Integration::Serialize( touchEvent, buffer );
  fwrite( buffer.data(), 1, buffer.size(), outputFile );
  fflush( outputFile );
}

}
}

namespace Dali
{
namespace Validation
{
struct Recorder::Impl
{
  Impl( Recorder* _owner ) :
    owner( _owner )
  {
  }

  ~Impl() = default;

  struct FrameTouchPair
  {
    uint32_t    frame;
    TouchEvent  touchEvent;
  };

  Recorder* owner;
  std::queue<FrameTouchPair> touches;
};

Recorder::Recorder()
{
  mImpl = std::make_unique<Impl>( this );
}

Recorder::~Recorder() = default;

} // Namespace Validation
} // Dali


/**
 * OVERRIDES
 */

// Dali::Integration
namespace Dali
{
namespace Integration
{
void Scene::QueueEvent( const Integration::Event& event )
{
  auto node = Dali::Validation::GetNextDispatchNode();
  auto layer = Dali::Validation::AutoReplayLayer::Get();

  /**
   * When recording, do not forward events to the Scene. Instead,
   * all the events are being cached for the synchronous dispatch.
   */
  if( Dali::Validation::IsLayerEnabled( nullptr ) && layer->GetMode() == Dali::Validation::Mode::RECORD )
  {
    if(event.type == Event::Type::Touch )
    {
      auto& touchEvent = static_cast<const TouchEvent&>(event);

      Dali::Validation::TouchEventPayload payload{};
      payload.touchEvent = touchEvent;
      payload.frame = layer->GetCurrentFrameCount();

      layer->GetEventSyncTimer()->QueueEvent( payload );
    }
  }
  else
  {
    Dali::Validation::InvokeDispatchNode( node, this, &event );
  }

}
} // Integration
} // Dali

// Dali::Internal
namespace Dali
{
namespace Internal
{
/**
 * Recording events must happen exactly in sync with vsync.
 * It may affect recording, only one event comes through during frame
 * and it's timed at the vsync.
 * Note! It may make recording mode feel sluggish but it's intended
 * in orded to get perfect sync during replay.
 */
void DALI_EXPORT_API EventProcessor::ProcessEvents()
{
  auto layer = Dali::Validation::AutoReplayLayer::Get();
  auto node = Dali::Validation::GetNextDispatchNode();
  auto mode = layer->GetMode();

  if( Dali::Validation::IsLayerEnabled( nullptr ) )
  {
    // before processing find out which events are processed in the frame
    MessageBuffer* queueToProcess = mCurrentEventQueue;

    // Count event
    uint32_t eventCount = 0;
    for( MessageBuffer::Iterator iter = queueToProcess->Begin(); iter.IsValid(); iter.Next() )
    {
      Event* event = reinterpret_cast< Event* >( iter.Get() );
      if(event->type == Event::Touch)
      {
        eventCount++;
        auto touchEvent = static_cast<Dali::Integration::TouchEvent*>(event);

        // spoof time when recording for each event
        touchEvent->time = layer->GetCurrentFrameCount() * layer->GetFrameTimeInMillis();
        printf("PADDLE TOUCH: [%d] %f, %f\n", int(layer->GetCurrentFrameCount()),
        touchEvent->GetPoint(0).GetScreenPosition().x, touchEvent->GetPoint(0).GetScreenPosition().y);
        if(mode == Dali::Validation::Mode::RECORD)
        {
          SerializeTouchPoint( *touchEvent );
        }
      }
    }

    Dali::Validation::InvokeDispatchNode( node, this );
  }
  else
  {
    Dali::Validation::InvokeDispatchNode( node, this );
  }
}
} // Internal
} // Dali

#include "multi-point-event-serializer.h"
#include "autoreplay-layer.h"
#include <dali/devel-api/validation-layers.h>
#include <dali/integration-api/events/touch-event-integ.h>
#include <dali/integration-api/scene.h>
#include <dali/integration-api/events/hover-event-integ.h>
#include "touch-event-messaging.h"
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

namespace Dali
{
namespace Integration
{
class TouchEventCombiner;
} // Integration

namespace Validation
{
void EnqueueTouchEvent( Dali::Validation::DispatchNode* node,
                        Dali::Integration::TouchEventCombiner* self,
                        const Dali::Integration::Point& point,
                        uint32_t time,
                        Dali::Integration::TouchEvent& touchEvent,
                        Dali::Integration::HoverEvent& hoverEvent );
} // Validation


namespace Integration
{

struct TouchEvent;
struct HoverEvent;

class TouchEventCombiner
{
public:

  enum EventDispatchType
  {
    DispatchTouch,      ///< The touch event should be dispatched.
    DispatchHover,      ///< The hover event should be dispatched.
    DispatchBoth,       ///< Both touch event and hover event should be dispatched.
    DispatchNone        ///< Neither touch event nor hover event should be dispatched.
  };

  EventDispatchType GetNextTouchEvent( const Point& point,
                                       uint32_t time,
                                       TouchEvent& touchEvent,
                                       HoverEvent& hoverEvent );
};

TouchEventCombiner::EventDispatchType TouchEventCombiner::GetNextTouchEvent( const Point& point,
                                                         uint32_t time,
                                                         TouchEvent& touchEvent,
                                                         HoverEvent& hoverEvent )
{
  auto layer = Dali::Validation::AutoReplayLayer::Get();
  auto node = Dali::Validation::GetNextDispatchNode();

  if( Dali::Validation::IsLayerEnabled( nullptr ) && layer->GetMode() == Dali::Validation::Mode::RECORD )
  {
    time = layer->GetCurrentFrameCount()*16;
    touchEvent.time = layer->LayerGetDeltaTime();
    auto type = Dali::Validation::InvokeReturnDispatchNode<TouchEventCombiner::EventDispatchType>(
      node, this, &point, time, &touchEvent, &hoverEvent );

    //Dali::Validation::EnqueueTouchEvent( node, this, point, time, touchEvent, hoverEvent );
    //Dali::Validation::DispatchTouchEvents();
    return type;
  }
  else
  {
    return Dali::Validation::InvokeReturnDispatchNode<TouchEventCombiner::EventDispatchType>
      (node, this, &point, time, &touchEvent, &hoverEvent );
  }
}

}

namespace Validation
{

struct TouchEventMessage
{
  Dali::Validation::DispatchNode*        node;
  Dali::Integration::Event*              event;
  Dali::Integration::Scene*              self;
  void Dispatch()
  {
    return Dali::Validation::InvokeDispatchNode(
      node, self, event );
  }
};

std::vector<TouchEventMessage> gTouchEventMessageQueue{};

void DispatchTouchEvents()
{
  for(auto& msg : gTouchEventMessageQueue )
  {
    msg.Dispatch();
  }
  gTouchEventMessageQueue.clear();
}

/*
void EnqueueTouchEvent( Dali::Validation::DispatchNode* node,
                        Dali::Integration::TouchEventCombiner* self,
                        const Dali::Integration::Point& point,
                        uint32_t time,
                        Dali::Integration::TouchEvent& touchEvent,
                        Dali::Integration::HoverEvent& hoverEvent )
{
  gTouchEventMessageQueue.emplace_back();
  auto& message = gTouchEventMessageQueue.back();
  message.node = node;
  message.time = time;
  message.touchEvent = &touchEvent;
  message.hoverEvent = &hoverEvent;
  message.point = const_cast<Dali::Integration::Point*>(&point);
  message.self = self;
}
*/
} // Validation

}

//
// Created by adam.b on 28/11/19.
//

#include "replayer.h"
#include "autoreplay-layer.h"
#include "event-sync-timer.h"
#include <dali/devel-api/validation-layers.h>
#include "multi-point-event-serializer.h"
#include <dali/integration-api/events/touch-event-integ.h>
#include <dali/integration-api/core.h>
#include <dali/public-api/adaptor-framework/timer.h>

#include <queue>

namespace Dali
{
namespace Validation
{
Replayer* Replayer::mInstance{nullptr};
struct Replayer::Impl : public Dali::ConnectionTracker
{

};

Replayer::Replayer()
{
  mInstance = this;
}

Replayer::~Replayer() = default;


void Replayer::Initialise(const std::string &filename)
{
  puts("Replayer init");
  mImpl = std::make_unique<Impl>();

  std::vector<Dali::Integration::TouchEvent> retval{};
  Dali::Integration::TouchEvent touchEvent{};
  std::vector<uint8_t> buffer{};

  FILE* f = fopen( filename.c_str(), "rb" );
  if(!f)
  {
    puts("ERROR");
  }
  fseek( f, 0, SEEK_END );
  auto fileLength = ftell(f);
  fseek( f, 0, SEEK_SET );
  buffer.resize( size_t(fileLength) );
  fread( buffer.data(), 1, fileLength, f );
  fclose( f );

  uint32_t offset = 0;
  while( offset < fileLength )
  {
    // Deserialize touch event
    uint32_t frame;
    Dali::Integration::DeserializeType( buffer, offset, frame );
    Dali::Integration::Deserialize( touchEvent, buffer, offset );
    printf("[%d] %f, %f\n",int(touchEvent.time),
           touchEvent.points[0].GetScreenPosition().x, touchEvent.points[0].GetScreenPosition().y);

    // Store touch event
    //mImpl->touchEvents.push( Impl::TouchEventWithFrame() = { frame, touchEvent } );
    auto layer = Dali::Validation::AutoReplayLayer::Get();
    if(!layer)
    {
      puts("No layer!");
    }
    // queue event for replaying
    layer->GetEventSyncTimer()->QueueEvent( {touchEvent, frame } );
  }
}

} // Namespace Validation

} // Namespace Dali

/*******************************************/
// HiJack Application::CreateAdaptor()
// Start player timer after Adaptor has been created
namespace Dali
{
namespace Integration
{
/**
 * UpdateManager::Update
 * @param elapsedSeconds
 * @param lastVSyncTimeMilliseconds
 * @param nextVSyncTimeMilliseconds
 * @param status
 * @param renderToFboEnabled
 * @param isRenderingToFbo
 */

} // Integration

namespace Internal
{
namespace Adaptor
{
namespace TimeService
{

/**
 * @brief Sleeps until the monotonic time specified since some unspecified starting point (usually the boot time).
 *
 * If the time specified has already passed, then it returns immediately.
 *
 * @param[in]  timeInNanoseconds  The time to sleep until
 *
 * @note The maximum value timeInNanoseconds can hold is 0xFFFFFFFFFFFFFFFF which is 1.844674407e+19. Therefore, this can overflow after approximately 584 years.
 */
void SleepUntil( uint64_t timeInNanoseconds )
{

}

} // Timeservice
} // Adaptor
} // Internal

} // Dali

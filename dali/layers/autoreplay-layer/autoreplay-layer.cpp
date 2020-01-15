#include <dali/devel-api/validation-layers.h>
#include <dali/devel-api/validation-layer-config.h>
#include <dali/internal/render/renderers/render-surface-frame-buffer.h>

#include "autoreplay-layer.h"
#include <GLES3/gl3.h>

#define LODEPNG_COMPILE_DISK
#define LODEPNG_COMPILE_ENCODER
#include "lodepng.h"
#include <queue>
#include "picojson.h"
#include "replayer.h"
#include "recorder.h"
#include "event-sync-timer.h"
#include <sys/time.h>

// Used by layer class
extern "C" const char* GetValidationLayerNameString();

#define assign( x ) x.result ? x.value : decltype(x.value)()

namespace Dali
{
namespace Validation
{

struct AutoReplayLayer::Impl
{
  explicit Impl( AutoReplayLayer* _owner ) :
    owner( _owner )
  {
  }

  uint32_t LayerGetCurrentMilliSeconds()
  {
    struct timeval tv;

    struct timespec tp;
    static clockid_t clockid;

    if (!clockid)
    {
#ifdef CLOCK_MONOTONIC_COARSE
      if (clock_getres(CLOCK_MONOTONIC_COARSE, &tp) == 0 &&
          (tp.tv_nsec / 1000) <= 1000 && clock_gettime(CLOCK_MONOTONIC_COARSE, &tp) == 0)
      {
        clockid = CLOCK_MONOTONIC_COARSE;
      }
      else
#endif
      if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0)
      {
        clockid = CLOCK_MONOTONIC;
      }
      else
      {
        clockid = ~0L;
      }
    }
    if (clockid != ~0L && clock_gettime(clockid, &tp) == 0)
    {
      return static_cast<uint32_t>( (tp.tv_sec * 1000 ) + (tp.tv_nsec / 1000000L) );
    }

    gettimeofday(&tv, nullptr);
    return static_cast<uint32_t>( (tv.tv_sec * 1000 ) + (tv.tv_usec / 1000) );
  }

  bool TerminateOnFinish()
  {
    return gTerminateOnFinish;
  }

  uint32_t LayerGetDeltaTime()
  {
    if(!gFirstRenderTime)
    {
      return 0u;
    }
    return LayerGetCurrentMilliSeconds() - gFirstRenderTime;
  }

  uint32_t GetCurrentFrameCount()
  {
    return gFrameCounter;
  }

  const std::string& GetRecordSimFilePath()
  {
    return gRecordSimFilePath;
  }

  const std::string& GetReplaySimFilePath()
  {
    return gReplaySimFilePath;
  }

  Mode GetMode()
  {
    return gCurrentMode;
  }

  void NotifyOnFirstPostRender()
  {
    if (!gFirstRenderTime)
    {
      gFirstRenderTime = LayerGetCurrentMilliSeconds();
    }
  }

  bool IsCaptureEnabled() const
  {
    return gCaptureEnabled;
  }

  void InitialiseReplayer()
  {
    replayer = std::make_unique<Replayer>();
    replayer->Initialise( gReplaySimFilePath );
  }

  void InitialiseRecorder()
  {
    recorder = std::make_unique<Recorder>();
  }

  void InitialiseEventSyncTimer( Dali::Integration::Core* core )
  {
    eventSyncTimer = std::make_unique<EventSyncTimer>( core );
    eventSyncTimer->Start();

    // Initialise sim file
    if( GetMode() == Dali::Validation::Mode::REPLAY )
    {
      InitialiseReplayer();
    }
    else if( GetMode() == Dali::Validation::Mode::RECORD )
    {
      InitialiseRecorder();
    }
  }

  uint32_t GetFrameTimeInMillis()
  {
    return 16u;
  }

  void ParseConfig()
  {
    this->config = std::make_unique<Config>();

    if(!config->Initialize( (std::string(GetValidationLayerNameString())+".json").c_str() ))
    {
      puts("ERROR: Configuration file not found!");
      return;
    }

    // common fields
    auto name = config->GetString( "layer.name" );
    auto library = config->GetString( "layer.library" );
    auto version = config->GetString( "layer.version" );

    // layer fields
    auto capturePrefix = config->GetString( "layer.config.capturePrefix" );
    // array, auto captureFrames = config->GetString( "layer.config.captureFrames" );
    auto recordSimFile = config->GetString( "layer.config.recordSimFilePath" );
    auto replaySimFile = config->GetString( "layer.config.replaySimFilePath" );
    auto modeRecord = config->GetBool( "layer.config.modeRecord" );
    auto modeReplay = config->GetBool( "layer.config.modeReplay" );
    auto captureEnabled = config->GetBool( "layer.config.captureEnabled" );
    auto terminateOnFinish = config->GetBool( "layer.config.terminateOnFinish" );
    auto captureInterval = config->GetInteger( "layer.config.captureInterval" );

    mCaptureInterval = uint32_t(assign( captureInterval ));
    gCapturePathPrefix = assign( capturePrefix );
    gRecordSimFilePath = assign( recordSimFile );
    gReplaySimFilePath = assign( replaySimFile );

    // Resolve operating mode
    auto rec = assign( modeRecord );
    auto rep = assign( modeReplay );
    if( rec && rep ) gCurrentMode = Dali::Validation::Mode::NONE;
    else if(rec) gCurrentMode = Dali::Validation::Mode::RECORD;
    else if(rep) gCurrentMode = Dali::Validation::Mode::REPLAY;

    gTerminateOnFinish = assign( terminateOnFinish );
    gCaptureEnabled = assign( captureEnabled );

    // frames to capture
    auto captureFrames = config->GetArrayOf<int>( "layer.config.captureFrames" );
    if( captureFrames.result )
    {
      for( auto& item : captureFrames.value )
      {
        gFrameCaptureQueue.push( item );
      }
    }
  }

  /**
   * Members
   */

  AutoReplayLayer* owner;

  std::unique_ptr<Config> config;

  // field to clean up
  uint32_t gFrameCounter {0u};
  std::queue<int> gFrameCaptureQueue;
  int gFrameCaptureNext{-1};

  std::string gCapturePathPrefix{};
  std::string gRecordSimFilePath{};
  std::string gReplaySimFilePath{};
  uint32_t mCaptureInterval{0u};
  Dali::Validation::Mode gCurrentMode{ Dali::Validation::Mode::NONE };
  bool gCaptureEnabled{false};

  uint32_t gFirstRenderTime{0u};
  bool gTerminateOnFinish{false};

  /**
   * EventSyncTimer
   */
  std::unique_ptr<EventSyncTimer> eventSyncTimer{};
  /**
   * Recorder
   */
  std::unique_ptr<Recorder> recorder;

  /**
   * Replayer
   */
  std::unique_ptr<Replayer> replayer;
};

uint32_t AutoReplayLayer::GetFrameTimeInMillis()
{
  return mImpl->GetFrameTimeInMillis();
}

AutoReplayLayer::AutoReplayLayer()
{
  mImpl = std::make_unique<AutoReplayLayer::Impl>( this );
}

uint32_t AutoReplayLayer::GetCurrentFrameCount()
{
  return mImpl->GetCurrentFrameCount();
}

const std::string& AutoReplayLayer::GetRecordSimFilePath()
{
  return mImpl->GetRecordSimFilePath();
}

const std::string& AutoReplayLayer::GetReplaySimFilePath()
{
  return mImpl->GetReplaySimFilePath();
}

uint32_t AutoReplayLayer::LayerGetCurrentMilliSeconds()
{
  return mImpl->LayerGetCurrentMilliSeconds();
}

uint32_t AutoReplayLayer::LayerGetDeltaTime()
{
  return mImpl->LayerGetDeltaTime();
}

bool AutoReplayLayer::TerminateOnFinish()
{
  return mImpl->TerminateOnFinish();
}

Mode AutoReplayLayer::GetMode()
{
  return mImpl->GetMode();
}

uint32_t AutoReplayLayer::GetCaptureInterval()
{
  return mImpl->mCaptureInterval;
}


void AutoReplayLayer::InitialiseEventSyncTimer( Dali::Integration::Core* core )
{
  mImpl->InitialiseEventSyncTimer( core );
}

AutoReplayLayer* AutoReplayLayer::Get()
{
  static AutoReplayLayer* instance { nullptr };
  if(!instance)
  {
    instance = new AutoReplayLayer();
  }

  return instance;
}

AutoReplayLayer::Impl* AutoReplayLayer::GetImpl() const
{
  return mImpl.get();
}

Recorder* AutoReplayLayer::GetRecorder() const
{
  return mImpl->recorder.get();
}


Replayer* AutoReplayLayer::GetReplayer() const
{
  return mImpl->replayer.get();
}

EventSyncTimer* AutoReplayLayer::GetEventSyncTimer() const
{
  return mImpl->eventSyncTimer.get();
}

} // Validation

/**
 * OVERRIDES
 */

namespace Internal
{
namespace Render
{
/**
 * PostRender used to capture  stuff
 */
void SurfaceFrameBuffer::PostRender()
{
  auto layer = Dali::Validation::AutoReplayLayer::Get();
  auto impl = layer->GetImpl();

  impl->NotifyOnFirstPostRender();

  //printf("PADDLE: Frame END = %d\n", impl->GetCurrentFrameCount());
  auto node = Dali::Validation::GetNextDispatchNode();

  //printf("Frame: %d, DT: %d\n", int(gFrameCounter), int(Dali::Validation::LayerGetDeltaTime()));
  if( Dali::Validation::IsLayerEnabled( nullptr ) && impl->gCaptureEnabled )
  {
    Dali::Validation::InvokeDispatchNode(node, this );

    if( impl->IsCaptureEnabled() && impl->gFrameCounter > 0 &&
      (( impl->gFrameCaptureNext > 0 && impl->gFrameCounter == impl->gFrameCaptureNext ) ||
        (impl->mCaptureInterval && (impl->gFrameCounter % impl->mCaptureInterval) == 0))
        )
    {
      // sync with replayer
      std::stringstream ss;
      ss << impl->gCapturePathPrefix;
      ss << "_";
      ss << impl->gFrameCounter;
      ss << ".png";

      printf("Capturing frame: %d to %s\n", int(impl->gFrameCounter),
             ss.str().c_str());
      auto positionSize = mSurface->GetPositionSize();
      std::vector<uint8_t> buffer{};
      buffer.resize( uint32_t(positionSize.width * positionSize.height * 4) );
      glReadPixels( 0, 0, positionSize.width, positionSize.height, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data() );

      // encode snapshot and save (will be upside-down)
      lodepng::encode( ss.str(), buffer, positionSize.width, positionSize.height );

      if(!impl->gFrameCaptureQueue.empty())
      {
        impl->gFrameCaptureNext = impl->gFrameCaptureQueue.front();
        impl->gFrameCaptureQueue.pop();
      }
      else
      {
        impl->gFrameCaptureNext = -1;
      }
    }
  }
  else
  {
    Dali::Validation::InvokeDispatchNode(node, this );
  }

  impl->gFrameCounter++;
  //printf("PADDLE: Frame BEGIN = %d\n", impl->GetCurrentFrameCount());
}

}
}
}

// Layer api, must be filled, layer is enabled by default
extern "C" const char* GetValidationLayerNameString()
{
  return "DALI_autoreplay_layer";
}

extern "C" const char* GetValidationLayerVersionString()
{
  return "0.0.1";
}

// optional, lazy initialization (may read configuration etc.)
extern "C" void ValidationLayerInit()
{
  // Lazy initialize layer class
  auto impl = Dali::Validation::AutoReplayLayer::Get()->GetImpl();

  // Parse JSON configuration file
  impl->ParseConfig();

  //exit(0);
}


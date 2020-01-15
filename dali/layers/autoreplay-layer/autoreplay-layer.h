#include <cstdint>
#include <memory>

namespace Dali
{
namespace Integration
{
class Core;
} // Integration

namespace Validation
{
struct Recorder;
struct Replayer;
class EventSyncTimer;
enum class Mode
{
  NONE,
  RECORD,
  REPLAY,
};

struct AutoReplayLayer
{
  AutoReplayLayer();

  /**
   * Static getter, only one instance of the layer allowed
   * @return
   */
  static AutoReplayLayer* Get();

  /**
   * Returns current frame count
   */
  uint32_t GetCurrentFrameCount();

  /**
   * returns config stuff, should be moved into separate class,
   * config should be driven via devel-api
   * TODO: clean up!
   * @return
   */
  const std::string& GetRecordSimFilePath();

  const std::string& GetReplaySimFilePath();

  uint32_t LayerGetCurrentMilliSeconds();

  uint32_t LayerGetDeltaTime();

  bool TerminateOnFinish();

  Mode              GetMode();

  uint32_t GetCaptureInterval();

  Recorder* GetRecorder() const;

  Replayer* GetReplayer() const;

  EventSyncTimer* GetEventSyncTimer() const;

  void InitialiseEventSyncTimer( Dali::Integration::Core* core );

  uint32_t GetFrameTimeInMillis();

  struct Impl;
  std::unique_ptr<Impl> mImpl;

  Impl* GetImpl() const;
};

} // Validation

} // Layer


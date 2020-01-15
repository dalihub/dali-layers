//
// Created by adam.b on 28/11/19.
//

#ifndef DALI_LAYERS_REPLAYER_H
#define DALI_LAYERS_REPLAYER_H

#include <string>
#include <memory>
#include <mutex>
namespace Dali
{
namespace Integration
{
class Core;
}
namespace Validation
{
struct Replayer
{
  Replayer();
  ~Replayer();

  void Initialise( const std::string& filename );

  static Replayer* Get()
  {
    return mInstance;
  }

  static Replayer* mInstance;

  struct Impl;
  std::unique_ptr<Impl> mImpl;
};

}
}

#endif //DALI_LAYERS_REPLAYER_H

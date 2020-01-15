//
// Created by adam.b on 29/11/19.
//

#ifndef DALI_LAYERS_RECORDER_H
#define DALI_LAYERS_RECORDER_H
#include <memory>

namespace Dali
{
namespace Validation
{

struct Recorder
{
  Recorder();

  ~Recorder();

  struct Impl;
  std::unique_ptr<Impl> mImpl;
};

} // Validation
} // Dali

#endif //DALI_LAYERS_RECORDER_H

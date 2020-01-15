// HiJack PostRender
#include <dali/public-api/actors/actor.h>

/**
 * These functions are necessary
 */
extern "C" const char* GetValidationLayerNameString()
{
  return "thread_checker-Layer";
}

extern "C" const char* GetValidationLayerVersionString()
{
  return "0.0.1";
}

extern "C" void ValidationLayerInit()
{

}


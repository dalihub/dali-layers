// HiJack PostRender
#include <dali/devel-api/validation-layers.h>
#include <dali/internal/window-system/common/window-render-surface.h>

/**
 * These functions are necessary
 */
extern "C" const char* GetValidationLayerNameString()
{
  return "sample_layer";
}

extern "C" const char* GetValidationLayerVersionString()
{
  return "0.0.1";
}

extern "C" void ValidationLayerInit()
{

}

namespace Dali
{
namespace Internal
{
namespace Adaptor
{
void WindowRenderSurface::PostRender( bool renderToFbo, bool replacingSurface, bool resizingSurface )
{
  auto node = Dali::Validation::GetNextDispatchNode();

  if( Dali::Validation::IsLayerEnabled( nullptr )  )
  {
    puts("Sample Layer running!");
    Dali::Validation::InvokeDispatchNode<void>(node, this, renderToFbo, replacingSurface, resizingSurface );
  }
  else
  {
    Dali::Validation::InvokeDispatchNode<void>(node, this, renderToFbo, replacingSurface, resizingSurface );
  }
}
}
}
}
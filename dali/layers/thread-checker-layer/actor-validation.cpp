//
// Created by adam.b on 15/01/20.
//

#include <dali/public-api/actors/actor.h>
#include <dali/devel-api/validation-layers.h>
#include <thread>
namespace Dali
{

void Actor::Remove(Actor child)
{
  auto id = std::this_thread::get_id();
  printf("Thread: %d: Actor::Remove()\n", id);
  auto node = Dali::Validation::GetNextDispatchNode();
  Dali::Validation::InvokeDispatchNode<void>(node, this, child );
}

void Actor::Add(Actor child)
{
  auto id = std::this_thread::get_id();
  printf("Thread: %d: Actor::Add()\n", id);
  auto node = Dali::Validation::GetNextDispatchNode();
  if( Dali::Validation::IsLayerEnabled( nullptr )  )
  {
    Dali::Validation::InvokeDispatchNode<void>(node, this, child );
  }
  else
  {
    Dali::Validation::InvokeDispatchNode<void>(node, this, child );
  }
}

}
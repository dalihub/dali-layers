#!/bin/bash

new_layer_name=$1

dali_layers_dir="$(dirname $(realpath $0))"/..
layer_dir=${dali_layers_dir}/dali/layers/${new_layer_name}-layer

layer_name=${new_layer_name}-layer
layer_name_underscore=$(echo $new_layer_name | sed -e 's/-/_/g;')_layer

# Create directory
mkdir ${layer_dir}
echo "Creating ${layer_dir}"

# Create JSON file
echo "Creating JSON config file"
cat << EOF | sed -e 's/LAYER_NAME/'${layer_name}'/g;' > ${layer_dir}/${layer_name_underscore}.json
{
  "layer" : {
    "name" : "LAYER_NAME",
    "library" : "libLAYER_NAME.so",
    "description" : "Layer description",
    "version" : "0.0.1",
    "config" : {
    },
  }
}
EOF

# Create cmake build file
echo "Creating CMAKE build file"
cat << EOF | sed -e "s/LAYER_NAME_VAR/${layer_name}/g;s/LAYER2_NAME_VAR/${layer_name_underscore}/g;" > ${layer_dir}/layer.cmake
SET( LAYER_SOURCES
     \${LAYERS_ROOT}/LAYER_NAME_VAR/LAYER_NAME_VAR.cpp)

SET( LAYER_NAME LAYER_NAME_VAR )

SET( LAYER_INSTALL_FILES \${LAYERS_ROOT}/LAYER_NAME_VAR/LAYER2_NAME_VAR.json )
EOF

# Create entry point
echo "Creating entry point"
cat << EOF > ${layer_dir}/${layer_name}.cpp
#include <dali/devel-api/validation-layers.h>
/**
 * These functions are necessary
 */
extern "C" const char* GetValidationLayerNameString()
{
  return "${layer_name_underscore}";
}

extern "C" const char* GetValidationLayerVersionString()
{
  return "0.0.1";
}

extern "C" void ValidationLayerInit()
{
  // Here initialisation code
}

// Sample override of Actor::Add() function

/**

#include <dali/public-api/actors/actor.h>

namespace Dali
{

void Actor::Add(Actor child)
{
  auto id = std::this_thread::get_id();
  printf("Actor::Add()\n");
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

} // Namespace Dali
**/
EOF
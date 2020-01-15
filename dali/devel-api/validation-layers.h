/*
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef DALI_VALIDATION_LAYER_DEVEL_H
#define DALI_VALIDATION_LAYER_DEVEL_H

#include <string>
#include <vector>

namespace Dali
{
namespace Validation
{

using DispatchNode = void;
DispatchNode* GetNextDispatchNode();

template<class RET, class OBJECT, class... Args>
RET InvokeReturnDispatchNode( DispatchNode* node, OBJECT* self, Args... args )
{
  auto func = reinterpret_cast<RET(*)(OBJECT*, Args...)>( node );
  return func( self, args...);
};


template<class O, class... Args>
void InvokeDispatchNode(DispatchNode *node, O *_this, Args... args)
{
  auto func = reinterpret_cast<void(*)(O*, Args...)>( node );
  func( _this, args... );
};

struct ValidationLayerInfo
{
  std::string name;
  std::string version;
  bool        enabled;
};

void SetLayerKeyValue( const char* key, const void* inValue, uint32_t size );
bool GetLayerKeyValue( const char* key, void* outValue, uint32_t size );

/**
 * Sets a value in the internal storage of the layer
 * @param[in] key Name of the key
 * @param[in] outValue Reference to value object
 */
template<class T>
void SetLayerKeyValue( const char* key, const T& inValue )
{
  SetLayerKeyValue( key, &inValue, sizeof(T) );
}

/**
 * Retrieves the value from the internal storage.
 * @param[in] key Name of the key
 * @param[out] outValue Reference to the output object
 *
 * @return True if the value exists, false otherwise
 */
template<class T>
bool GetLayerKeyValue( const char* key, T& outValue )
{
  return GetLayerKeyValue( key, &outValue, sizeof(T) );
}

/**
 * Tests whether the layer is currently enabled.
 *
 * @note The function Mmy be called from inside the actual layer. It must pass nullptr as
 * layerName parameter. In such case it MUST be called directly from the top-level
 * layer function!
 *
 * @param layerName Name of the layer to test
 * @return True if layer is enabled, false otherwise.
 */
bool IsLayerEnabled( const char* layerName );

/**
 * Enables named validation layer
 * @param layerName Name of the layer to enable
 */
void EnableLayer( const std::string& layerName );

/**
 * Disables named validation layer
 * @param layerName Name of the layer to disable
 */
void DisableLayer( const std::string& layerName );

/**
 * Disables all validation layers
 */
void DisableAllLayers();

/**
 * Enables all validation layers
 */
void EnableAllLayers();

/**
 * Reads layer configuration
 */
struct LayerConfig;

LayerConfig OpenLayerConfig( const std::string& name );

template<class T>
struct ConfigValue
{
  bool result;

};

template<class T>
ConfigValue<T> LayeConfigGetValue( const LayerConfig& config, std::string& path )
{

}

/**
 * Enumerates available layers
 * @return An array of ValidationLayerInfo structures
 */
std::vector<ValidationLayerInfo> EnumerateLayers();

} // namespace Validation

} // namespace Dali


#endif // DALI_VALIDATION_LAYER_DEVEL_H

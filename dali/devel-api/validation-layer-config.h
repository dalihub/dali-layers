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

#ifndef DALI_LAYERS_VALIDATION_LAYER_CONFIG_H
#define DALI_LAYERS_VALIDATION_LAYER_CONFIG_H

#include <memory>
#include <string>
#include <vector>
namespace Dali
{
namespace Validation
{
struct ConfigArray{};

class Config
{
public:

  Config();
  ~Config();

  template<class T>
  struct Result
  {
    bool result;
    T    value;
  };

  /**
   * Loads JSON configuration file for the layer.
   * The loader will look for the file in particular
   * paths.
   * @param configNameNoPath shouldn't contain path
   * @return True on success
   */
  bool Initialize( const char* configNameNoPath );

  /**
   * Get value as a string.
   * @param fieldPath Full path to the field. For example:
   * GetString( "layer.config.capturePrefix" )
   * @return returns
   */
  Result<std::string> GetString( const std::string& fieldPath, uint32_t arrayIndex = 0u );

  Result<int> GetInteger( const std::string& fieldPath, uint32_t arrayIndex = 0u );

  Result<float> GetFloat( const std::string& fieldPath, uint32_t arrayIndex = 0u );

  Result<bool> GetBool( const std::string& fieldPath, uint32_t arrayIndex = 0u );

  /**
   * Returns array of elements. All elements are assumed to be
   * of a single type.
   *
   * Note: it's quicker access to the array but there is no
   * support for env variables.
   */
  template <class T>
  Result<std::vector<T>> GetArrayOf( const std::string& fieldPath )
  {
    auto result = GetArrayOfAny( fieldPath, typeid(T), sizeof(T) );

    if(result.empty())
    {
      return {false};
    }

    // convert
    auto typedRawData = reinterpret_cast<T*>(result.data());
    auto size = result.size() / sizeof(T);

    std::vector<T> retval{};
    for( auto i = 0u; i < size; ++i )
    {
      retval.emplace_back( typedRawData[i] );
    }
    return {true, retval};
  }

private:

  std::vector<char> GetArrayOfAny( const std::string& fieldPath, const std::type_info& info, uint32_t typeSize );

  struct Impl;
  std::unique_ptr<Impl> mImpl;
};

} // Validation
} // Dali

#endif //DALI_LAYERS_VALIDATION_LAYER_CONFIG_H

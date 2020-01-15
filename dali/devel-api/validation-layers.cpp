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

#include <dali/devel-api/validation-layers.h>
#include <link.h>
#include <sstream>
#include <map>
#include <cstring>
#include <algorithm>
#include <mutex>
#include <thread>
#include "picojson.h"

namespace
{
/**
 * djb2 string hashing helper function
 */
uint32_t HashString( const char* string )
{
  uint32_t hash = 5381;
  while( int c = *string++ )
  {
    hash = hash * 33 + c;
  }
  return hash;
}
}

namespace Dali
{
namespace Validation
{
struct LayerConfig
{
  std::string     path{};
  picojson::value root{};

};

struct DispatchChain
{
  std::string           funcName;
  std::vector<void*>    func;
  std::vector<uint32_t> layerIndex;

  uint32_t reserved;
};

struct LayerInfo
{
  std::string name;
  std::string version;
  uint32_t    nameHash;
  std::string moduleName;
  uint32_t    moduleHash;
  bool enabled;
};

static std::vector<DispatchChain> g_validationDispatchChainStorage{};
static bool g_validationInitialized = false;
static std::vector<LayerInfo> g_validationLayerInfoStorage{};
static std::vector<std::string> g_dlOpenLibraries{};

static std::mutex g_validationLayerMutex{};
//#define SYNCHRONIZED(x) {std::lock_guard<std::mutex> lock(g_validationLayerMutex); x; }
#define SYNCHRONIZED(x) x;
std::vector<std::string> split_string(const std::string& str, char delim = ' ')
{
  std::vector<std::string> cont;
  std::stringstream ss(str);
  std::string token;
  while (std::getline(ss, token, delim)) {
    cont.push_back(token);
  }
  return cont;
}

std::string current_thread()
{
  std::stringstream ss{};
  ss << std::this_thread::get_id();
  return ss.str();
}
/**
 * Layers initialization uses DALI_INSTANCE_LAYERS
 *
 * 1. Layers must be attached using LD_PRELOAD
 * 2. If DALI_INSTANCE_LAYERS isn't set, all layers are enabled
 * 3. If DALI_INSTANCE_LAYERS is set, only specified layers are enabled
 * 4. DALI_INSTANCE_LAYERS determines the order of accessing layers, can be use to reorder them
 */
void initialize_layer( void* handle, const char* moduleName )
{
  // Find required layer-api symbols
  auto& localStorage = g_validationLayerInfoStorage;

  auto getNameProc = reinterpret_cast<const char*(*)()>(dlsym( handle, "GetValidationLayerNameString"));
  auto getVersionProc = reinterpret_cast<const char*(*)()>(dlsym( handle, "GetValidationLayerVersionString"));
  auto validationLayerInitProc = reinterpret_cast<void(*)()>(dlsym( handle, "ValidationLayerInit"));

  if( getNameProc )
  {
    // Initialise layer
    if(validationLayerInitProc)
    {
      validationLayerInitProc();
    }

    // add layer to the storage
    LayerInfo layerInfo{};
    layerInfo.name = getNameProc();
    layerInfo.version = getVersionProc();
    layerInfo.enabled = true;
    layerInfo.nameHash = HashString( layerInfo.name.c_str() );
    layerInfo.moduleName = moduleName;
    layerInfo.moduleHash = HashString( moduleName );
    g_validationLayerInfoStorage.emplace_back( layerInfo );
    printf("DALi Validation Layer: %s [ver.%s, %s] = %d\n",
           layerInfo.name.c_str(),
           layerInfo.version.c_str(),
           layerInfo.moduleName.c_str(),
           layerInfo.enabled);
  }
}

int IterateLibrariesCallback(struct dl_phdr_info *info,
                             size_t size, void *data)
{
  std::string module( info->dlpi_name );
  auto retval = reinterpret_cast<std::vector<std::string>*>(data);
  if(!module.empty())
  {
    retval->push_back( module );
  }
  return 0;
}

/**
 * Finds all libraries to be open when looking for particular symbol
 * Should be done only once
 */
void IterateLibraries()
{
  std::vector<std::string> allLibraries{};
  dl_iterate_phdr( IterateLibrariesCallback, &allLibraries );
  g_dlOpenLibraries = std::move(allLibraries);
}

/**
 * Looks for a symbol in all linked libraries
 * @param symbol
 * @return
 */
std::vector<void*> FindSymbolChain( const char* symbol )
{
  std::vector<void*> retval{};
  for(auto& libName : g_dlOpenLibraries )
  {
    auto handle = dlopen( libName.c_str(), RTLD_LAZY|RTLD_NOLOAD );
    auto sym = dlsym(handle, symbol);
    if(sym)
    {
      retval.emplace_back( sym );
    }
    dlclose( handle );
  }
}

void InitializeLayerModules()
{
  if(!g_validationInitialized)
  {
    IterateLibraries();

    std::for_each( g_dlOpenLibraries.begin(),
                   g_dlOpenLibraries.end(),
                   []( const std::string& libName ){
                     auto handle = dlopen( libName.c_str(), RTLD_LAZY|RTLD_NOLOAD );
                     initialize_layer( handle, libName.c_str() );
                     dlclose( handle );
                   } );

    // test env
    auto enabledLayers = getenv( "DALI_INSTANCE_LAYERS" );
    if( enabledLayers )
    {
      // disable all
      for(auto& layerInfo : g_validationLayerInfoStorage )
      {
        layerInfo.enabled = false;
      };

      // enable only specified layers
      auto layers = split_string( enabledLayers, ':' );
      for( auto& layerName : layers )
      {
        auto layerNameHash = HashString( layerName.c_str() );
        for(auto& layerInfo : g_validationLayerInfoStorage )
        {
          if( layerInfo.nameHash == layerNameHash )
          {
            layerInfo.enabled = true;
            break;
          }
        }
      }
    }

    g_validationInitialized = true;
  }
}

DispatchNode* GetNextDispatchNode()
{
  // find next symbol
  auto callerAddress = __builtin_return_address(0);
  Dl_info info{};
  dladdr( callerAddress, &info );

  bool chainFound = false;

  // find if chain has to be registered
  int keepTrying = 2;
  while( keepTrying )
  {
    for( auto& chain : g_validationDispatchChainStorage )
    {
      if( chain.funcName == info.dli_sname )
      {
        int index = 0;
        for( auto& addr : chain.func )
        {
          if( addr == info.dli_saddr )
          {
            break;
          }
          index++;
        }
        index++;
        if( index < chain.func.size() )
        {
          return chain.func[index];
        }
        chainFound = true;
        break;
      }
    }

    if(!chainFound)
    {
      InitializeLayerModules();

      // register chain and retry
      DispatchChain chain;
      chain.funcName = info.dli_sname;
      chain.reserved = 10; // max 10 libs (tizen)
      auto chainPtr = &chain;

      std::for_each( g_dlOpenLibraries.begin(),
                     g_dlOpenLibraries.end(),
                     [chainPtr]( const std::string& libName )
                     {
                       // obtain handle to the library
                       auto handle = dlopen( libName.c_str(), RTLD_LAZY|RTLD_NOLOAD );

                       // find symbol
                       auto sym = dlsym( handle, chainPtr->funcName.c_str());

                       // Find index of the module in the storage
                       auto it = std::find_if( g_validationLayerInfoStorage.begin(),
                                               g_validationLayerInfoStorage.end(),
                                               [&libName]( LayerInfo& val)
                       {
                         if(val.moduleHash == HashString( libName.c_str() ))
                         {
                           return true;
                         }
                       });

                       // push symbol to the array
                       chainPtr->layerIndex.push_back(
                         (it != g_validationLayerInfoStorage.end()) ?
                         uint32_t(it - g_validationLayerInfoStorage.begin()) :
                         uint32_t(0xffffffff) );

                       if(sym)
                       {
                         chainPtr->func.push_back( sym );
                       }
                       // close the library
                       dlclose(handle);
                     });

      puts("Pushing again");
      g_validationDispatchChainStorage.push_back(chain);
      puts("Pushed again");
    }
    --keepTrying;
  }

  return nullptr;
}

namespace
{
// the storage is a binary
struct Storage
{
  explicit Storage( uint32_t capacity = 0 )
  {
    data = nullptr;
    if(capacity)
    {
      data = malloc(capacity);
    }
    _capacity = capacity;
  }

  Storage( Storage&& o )
  {
    data = o.data;
    _capacity = o._capacity;
    o.data = nullptr;
    o._capacity = 0;
  }

  Storage& operator=(Storage&&o)
  {
    data = o.data;
    _capacity = o._capacity;
    o.data = nullptr;
    o._capacity = 0;
    return *this;
  }

  template<class T>
  explicit operator T()
  {
    return *reinterpret_cast<T*>(data);
  }

  ~Storage()
  {
    if(data)
      free(data);
  }

  void* data;
  uint32_t _capacity;
};

std::map<uint32_t, Storage> g_validationKeyValueStorage;
}

LayerInfo* GetLayerInfo( const char* layerName, void* callerAddress )
{
  InitializeLayerModules();

  LayerInfo* layerInfo{nullptr};
  if(!layerName)
  {
    Dl_info info{};
    dladdr( callerAddress, &info );
    auto moduleHash = HashString( info.dli_fname );
    for(auto& info : g_validationLayerInfoStorage )
    {
      if( info.moduleHash == moduleHash )
      {
        layerInfo = &info;
        break;
      }
    }
  }
  else
  {
    auto layerHash { HashString( layerName ) };
    for(auto& info : g_validationLayerInfoStorage )
    {
      if(info.nameHash == layerHash )
      {
        layerInfo = &info;
        break;
      }
    }
  }
  return layerInfo;
}

void EnableLayer( const std::string& layerName )
{
  LayerInfo* layerInfo{ GetLayerInfo(layerName.c_str(), __builtin_return_address(0)) };
  if(layerInfo)
  {
    layerInfo->enabled = true;
  }
}

void DisableLayer( const std::string& layerName  )
{
  LayerInfo* layerInfo{ GetLayerInfo(layerName.c_str(), __builtin_return_address(0)) };
  if(layerInfo)
  {
    layerInfo->enabled = false;
  }
}

void DisableAllLayers()
{
  InitializeLayerModules();
  for(auto& layerInfo : g_validationLayerInfoStorage )
  {
    layerInfo.enabled = false;
  }
}

void EnableAllLayers()
{
  InitializeLayerModules();
  for(auto& layerInfo : g_validationLayerInfoStorage )
  {
    layerInfo.enabled = true;
  }
}

std::vector<ValidationLayerInfo> EnumerateLayers()
{
  InitializeLayerModules();
  std::vector<ValidationLayerInfo> retval{};
  for(auto& layerInfo : g_validationLayerInfoStorage )
  {
    retval.emplace_back(
      ValidationLayerInfo() = {
        layerInfo.name,
        layerInfo.version,
        layerInfo.enabled
      }
    );
  }
  return retval;
}

void ResetLayerKeyValue( const char* key )
{

}

bool IsLayerEnabled( const char* layerName )
{
  LayerInfo* layerInfo{ GetLayerInfo(layerName, __builtin_return_address(0)) };

  // wrong layer?
  if( !layerInfo )
  {
    return false;
  }

  return layerInfo->enabled;
}

// Sets a value in the internal storage
void SetLayerKeyValue( const char* key, const void* inValue, uint32_t size )
{
  auto hash = HashString( key );
  auto it = g_validationKeyValueStorage.find( hash );

  // create new storage if needed
  if( it == g_validationKeyValueStorage.end() || (*it).second._capacity != size )
  {
    g_validationKeyValueStorage[hash] = std::move(Storage( size ));
    it = g_validationKeyValueStorage.find( hash );
  }

  auto storage = (*it).second.data;
  std::memcpy( storage, inValue, size );
}

// Retrieves the value from the internal storage
// returns false when there is no such value
bool GetLayerKeyValue( const char* key, void* outValue, uint32_t size )
{
  auto hash = HashString( key );
  auto it = g_validationKeyValueStorage.find( hash );

  if(it == g_validationKeyValueStorage.end())
  {
    return false;
  }

  // copy only if sizes match
  if( size == (*it).second._capacity )
  {
    std::memcpy( outValue, (*it).second.data, size );
    return true;
  }

  return false;
}

// returns function pointer from the layer (custom API)
void* GetLayerProc( const char* procName )
{
  return nullptr;
}

} // namespace Validation
} // namespace Dali

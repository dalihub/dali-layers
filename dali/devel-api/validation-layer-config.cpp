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

// CLASS HEADER
#include "validation-layer-config.h"

// THIRD-PARTY HEADERS
#include "picojson.h"

#define vllog(...) printf(__VA_ARGS__)

using namespace picojson;

namespace Dali
{
namespace Validation
{
struct Config::Impl
{
  explicit Impl( Config& owner ) :
  mOwner( owner )
  {

  }

  ~Impl() = default;

  // Pair of result and value
  struct JSONResult
  {
    bool result;
    picojson::value value;
  };

  bool Initialize( const std::string& filename )
  {
    // Check whether DESKTOP_PREFIX is set
    auto desktopPrefix = getenv("DESKTOP_PREFIX");

    // If not, use system /usr directory
    std::string installPrefixDir( desktopPrefix ? desktopPrefix : "/usr" );
    installPrefixDir += "/share/dali/layers/";

    std::vector<std::string> configLocations {
      "./",   // current working directory
      "/tmp", // temporary directory
      installPrefixDir // install directory
    };

    std::vector<char> buffer{};

    bool fileLoaded{ false };
    for( auto& path : configLocations )
    {
      auto str = path + filename;
      vllog( " Looking for %s...\n", str.c_str());
      FILE* file = fopen( str.c_str(), "rb" );

      if( !file )
      {
        continue;
      }

      fseek( file, 0, SEEK_END );
      buffer.resize( uint32_t(ftell(file)) );
      fseek( file, 0, SEEK_SET );
      fread( buffer.data(), 1, buffer.size(), file);
      fclose( file );

      picojson::value root;
      auto data = buffer.data();
      parse( root, data, data + buffer.size());
      if( root.is<object>() )
      {
        mRootObject = root;
        fileLoaded = true;

        vllog("Configuration loaded successfuly: %s\n", path.c_str());

        break;
      }

      // clear buffer and retry with next path
      buffer.clear();
    }

    return fileLoaded;
  }

  /**
   * Removes specified character from string, returns new string
   * @return
   */
  std::string RemoveChar( const std::string& input, char toRemove )
  {
    std::string retval;
    for( auto& c : input )
    {
      c != toRemove ? retval += c : 0;
    }
    return retval;
  }

  /**
   * Helper function to split string using single char separator
   * @return Array of substrings
   */
  std::vector<std::string> SplitString( const std::string& str, char separator )
  {
    std::string chunk("");
    std::vector<std::string> retval{};
    for( auto& c : str )
    {
      if( c != separator )
      {
        chunk += c;
      }
      else
      {
        retval.emplace_back( chunk );
        chunk.clear();
      }
    }
    if(!chunk.empty())
    {
      retval.emplace_back(chunk);
      return retval;
    }
  }

  JSONResult GetValueWithPath( const std::string& path )
  {
    // return false if root not initialised
    if(!mRootObject.is<object>())
    {
      return {false};
    }

    auto items = SplitString( path, '.' );
    auto currentObject = mRootObject;
    for( auto& item : items )
    {
      currentObject = currentObject.get( item );
    }
    return {true, currentObject};
  }

  /**
   * Each value passed as string can use optional environmental
   * variable substitution with syntax:
   * field : "$VARIABLE_NAME[=default_value]"
   * . For example:
   * capturePrefix : "$CAPTURE_PREFIX=/tmp"
   * Default value is optional.
   *
   * This mechanism allows to override configuration
   * using environmental variables.
   *
   * @param val
   */
  Config::Result<std::string> TryParseEnv( picojson::value& val )
  {
    Config::Result<std::string> result{false};
    if(val.is<std::string>())
    {
      auto str = val.get<std::string>();
      if(str[0] == '$')
      {
        // Test for default value
        std::string currentValue;
        auto split = SplitString( &str[1], '=' );
        if( split.size() == 2 )
        {
          currentValue = split.back();
        }

        std::string envVarName = split.front();

        auto envValue = getenv(envVarName.c_str());
        if(envValue)
        {
          currentValue = envValue;

        }
        result = { true, currentValue };
      }
    }
    return result;
  }

  bool ResolveArray( picojson::value& val, uint32_t arrayIndex )
  {
    bool isArray = ( val.is<picojson::array>() );
    if( isArray )
    {
      auto& array = val.get<picojson::array>();

      // index out of bounds
      if(array.size() <= arrayIndex )
      {
        return false;
      }

      // Overwrite the value object with a single element
      val = val.get<picojson::array>()[arrayIndex];
    }
    return true;
  }

  /**
   * Return as string
   */
  Config::Result<std::string> GetString( const std::string& fieldPath, uint32_t arrayIndex )
  {
    auto result = GetValueWithPath( fieldPath );
    Config::Result<std::string> retval{ false };

    auto keepTesting = result.result ? ResolveArray( result.value, arrayIndex ) : true;

    if( keepTesting && result.value.is<std::string>() )
    {
      auto envTestResult = TryParseEnv( result.value );
      if(envTestResult.result)
      {
        retval = envTestResult;
      }
      else
      {
        retval = {true, result.value.get<std::string>()};
      }
    }
    vllog( " * %s = %s\n", fieldPath.c_str(), retval.result ? retval.value.c_str() : "UNDEFINED" );
    return retval;
  }

  /**
   * Return as integer
   */
  Config::Result<int> GetInteger( const std::string& fieldPath, uint32_t arrayIndex )
  {
    auto result = GetValueWithPath( fieldPath );
    Config::Result<int> retval{ false };

    if( !result.result )
    {
      return {false};
    }

    if( ResolveArray( result.value, arrayIndex ) )
    {
      auto envTestResult = TryParseEnv( result.value );
      if(envTestResult.result)
      {
        // convert to integer
        retval = {true, int(strtol(envTestResult.value.c_str(), nullptr, 0))};
      }

      // the number will be implicitly converted into integer!
      if( result.value.is<double>() )
      {
        retval =  {true, int(result.value.get<double>())};
      }
    }

    vllog( " * %s = INT(%d)\n", fieldPath.c_str(), retval.result ? retval.value : 0 );

    return retval;
  }

  /**
   * Return as float
   */
  Config::Result<float> GetFloat( const std::string& fieldPath, uint32_t arrayIndex )
  {
    auto result = GetValueWithPath( fieldPath );
    Config::Result<float> retval{ false };
    if( !result.result )
    {
      return {false};
    }

    if( ResolveArray( result.value, arrayIndex ) )
    {
      auto envTestResult = TryParseEnv( result.value );
      if(envTestResult.result)
      {
        // convert to float
        retval = {true, float(strtod(envTestResult.value.c_str(), nullptr))};
      }

      // the number will be implicitly converted into float!
      if( result.value.is<double>() )
      {
        retval = {true, float(result.value.get<double>())};
      }
    }

    vllog( " * %s = FLOAT(%f)\n", fieldPath.c_str(), retval.result ? retval.value : 0 );

    return retval;
  }

  /**
 * Return as boolean
 */
  Config::Result<bool> GetBool( const std::string& fieldPath, uint32_t arrayIndex )
  {
    auto result = GetValueWithPath( fieldPath );
    Config::Result<bool> retval{ false };
    if( !result.result )
    {
      return {false};
    }

    if( ResolveArray( result.value, arrayIndex ) )
    {
      auto envTestResult = TryParseEnv( result.value );
      if(envTestResult.result)
      {
        // convert to bool
        // Accept values like "false/0/False" and "true/1/True".
        // Anything else returns false
        bool isTrue = envTestResult.value == "true" ||
                      envTestResult.value == "True" ||
                      envTestResult.value == "1";

        retval = {true, isTrue};
      }

      // the number will be implicitly converted into float!
      if( result.value.is<bool>() )
      {
        retval = {true, result.value.get<bool>()};
      }
    }
    vllog( " * %s = BOOL(%d)\n", fieldPath.c_str(), retval.result ? int(retval.value) : 0 );
    return retval;
  }

  template<class T>
  std::vector<char> CopyArrayWithType( picojson::value& inputArray )
  {
    auto value = inputArray.get<picojson::array>();
    std::vector<char> retval{};
    retval.resize( sizeof(T) * value.size() );
    auto retvalDataPtr = reinterpret_cast<T*>(retval.data());
    for(auto i = 0u; i < value.size(); ++i )
    {
      retvalDataPtr[i] = value[i].get<T>();
    }
    return retval;
  }

  // Special version for integer type
  std::vector<char> CopyArrayWithInt( picojson::value& inputArray )
  {
    auto value = inputArray.get<picojson::array>();
    std::vector<char> retval{};
    retval.resize( sizeof(int) * value.size() );
    auto retvalDataPtr = reinterpret_cast<int*>(retval.data());
    for(auto i = 0u; i < value.size(); ++i )
    {
      retvalDataPtr[i] = int(value[i].get<double>());
    }
    return retval;
  }

  std::vector<char> GetArrayOfAny( const std::string& fieldPath, const std::type_info& info, uint32_t typeSize )
  {
    auto result = GetValueWithPath( fieldPath );
    if( !result.result || !result.value.is<picojson::array>() )
    {
      return {false};
    }

    std::vector<char> retval{};
    if(info.hash_code() == typeid(std::string).hash_code())
    {
      retval = std::move( CopyArrayWithType<std::string>( result.value ) );
    }
    else if(info.hash_code() == typeid(double).hash_code())
    {
      retval = std::move( CopyArrayWithType<double>( result.value ) );
    }
    else if(info.hash_code() == typeid(bool).hash_code())
    {
      retval = std::move( CopyArrayWithType<bool>( result.value ) );
    }
    else if(info.hash_code() == typeid(int).hash_code())
    {
      retval = std::move( CopyArrayWithInt( result.value ) );
    }

    return retval;
  }

  Config&           mOwner;
  picojson::value   mRootObject{};
};

Config::Config()
{
  mImpl = std::make_unique<Impl>( *this );
}

Config::~Config() = default;

bool Config::Initialize(const char *configNameNoPath)
{
  return mImpl->Initialize( configNameNoPath );
}

Config::Result<std::string> Config::GetString( const std::string& fieldPath, uint32_t arrayIndex )
{
  return mImpl->GetString( fieldPath, arrayIndex );
}

Config::Result<int> Config::GetInteger( const std::string& fieldPath, uint32_t arrayIndex )
{
  return mImpl->GetInteger( fieldPath, arrayIndex );
}

Config::Result<float> Config::GetFloat( const std::string& fieldPath, uint32_t arrayIndex )
{
  return mImpl->GetFloat( fieldPath, arrayIndex );
}

Config::Result<bool> Config::GetBool( const std::string& fieldPath, uint32_t arrayIndex )
{
  return mImpl->GetBool( fieldPath, arrayIndex );
}

std::vector<char> Config::GetArrayOfAny( const std::string& fieldPath, const std::type_info& info, uint32_t typeSize )
{
  return std::move(mImpl->GetArrayOfAny( fieldPath, info, typeSize ));
}

} // Validation
} // Config
#ifndef DALI_INTEGRATION_MULTI_POINT_EVENT_SERIALIZER_H
#define DALI_INTEGRATION_MULTI_POINT_EVENT_SERIALIZER_H

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

// INTERNAL INCLUDES
#include <dali/public-api/common/vector-wrapper.h>
#include <dali/integration-api/events/multi-point-event-integ.h>

namespace Dali
{

namespace Integration
{

template<class T>
void SerializeType( std::vector<uint8_t>& buffer, const T& val )
{
  buffer.insert( buffer.end(), reinterpret_cast<const uint8_t*>(&val), reinterpret_cast<const uint8_t*>(&val)+sizeof(T) );
}

template<class T>
uint32_t DeserializeType( const std::vector<uint8_t>& buffer, uint32_t& offset, T& val )
{
  auto data = reinterpret_cast<const T*>( &buffer[offset] );
  std::copy( data, data+1, &val );
  offset += sizeof(T);
  return offset;
}

inline void Serialize( const MultiPointEvent& self, std::vector<uint8_t>& buffer )
{
  SerializeType( buffer, self.type );
  SerializeType( buffer, self.time );
  uint32_t pointCount { uint32_t(self.points.size()) };
  SerializeType( buffer, pointCount );
  for( auto& point : self.points )
  {
    SerializeType( buffer, point );
  }
}

inline void Deserialize( MultiPointEvent& self, std::vector<uint8_t>& buffer, uint32_t& offset )
{
  DeserializeType( buffer, offset, self.type );
  DeserializeType( buffer, offset, self.time );
  uint32_t pointCount{0u};
  DeserializeType( buffer, offset, pointCount );
  self.points.resize( pointCount );
  for( auto& point : self.points )
  {
    DeserializeType( buffer, offset, point );
  }
}

} // namespace Integration

} // namespace Dali

#endif // DALI_INTEGRATION_MULTI_POINT_EVENT_H

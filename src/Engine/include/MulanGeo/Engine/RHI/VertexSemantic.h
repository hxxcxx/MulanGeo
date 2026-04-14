/*
 * Vertex Semantic — what a vertex attribute MEANS
 *
 * Defines semantic slots for vertex attributes (position, normal,
 * texcoord, etc.) and their mapping to HLSL/GLSL names.
 */

#pragma once

#include <cstdint>

namespace MulanGeo::Engine {

// ============================================================
// Vertex Semantic
// ============================================================

enum class VertexSemantic : uint8_t {
    Position    = 0,   // float3  - object-space position
    Normal      = 1,   // float3  - object-space normal
    Tangent     = 2,   // float4  - tangent + handedness (w)
    Bitangent   = 3,   // float3
    Color0      = 4,   // ubyte4n or float4 - primary color / layer color
    Color1      = 5,   // ubyte4n or float4 - secondary color
    TexCoord0   = 6,   // float2
    TexCoord1   = 7,   // float2
    TexCoord2   = 8,   // float2
    TexCoord3   = 9,   // float2
    Indices     = 10,  // uint4   - bone / instance indices
    Weight      = 11,  // float4  - bone weights
    InstanceId  = 12,  // uint    - system-generated instance ID
    PickId      = 13,  // uint    - object ID for picking pass
    LayerId     = 14,  // uint    - CAD layer / group ID
    ObjectId    = 15,  // uint    - unique object ID within scene
    MaterialId  = 16,  // uint    - material index
    User0       = 17,  // float4  - user-defined
    User1       = 18,  // float4  - user-defined
    User2       = 19,  // float4  - user-defined
    User3       = 20,  // float4  - user-defined
    Count
};

constexpr const char* semanticName(VertexSemantic sem) {
    using enum VertexSemantic;
    switch (sem) {
        case Position:   return "POSITION";
        case Normal:     return "NORMAL";
        case Tangent:    return "TANGENT";
        case Bitangent:  return "BITANGENT";
        case Color0:     return "COLOR0";
        case Color1:     return "COLOR1";
        case TexCoord0:  return "TEXCOORD0";
        case TexCoord1:  return "TEXCOORD1";
        case TexCoord2:  return "TEXCOORD2";
        case TexCoord3:  return "TEXCOORD3";
        case Indices:    return "INDICES";
        case Weight:     return "WEIGHT";
        case InstanceId: return "INSTANCE_ID";
        case PickId:     return "PICK_ID";
        case LayerId:    return "LAYER_ID";
        case ObjectId:   return "OBJECT_ID";
        case MaterialId: return "MATERIAL_ID";
        case User0:      return "USER0";
        case User1:      return "USER1";
        case User2:      return "USER2";
        case User3:      return "USER3";
        default:         return "UNKNOWN";
    }
}

// HLSL semantic name (for shader generation)
constexpr const char* semanticHlsl(VertexSemantic sem) {
    using enum VertexSemantic;
    switch (sem) {
        case Position:   return "POSITION";
        case Normal:     return "NORMAL";
        case Tangent:    return "TANGENT";
        case Bitangent:  return "BINORMAL";
        case Color0:     return "COLOR";
        case Color1:     return "COLOR1";
        case TexCoord0:  return "TEXCOORD";
        case TexCoord1:  return "TEXCOORD1";
        case TexCoord2:  return "TEXCOORD2";
        case TexCoord3:  return "TEXCOORD3";
        case Indices:    return "BLENDINDICES";
        case Weight:     return "BLENDWEIGHT";
        case InstanceId: return "SV_InstanceID";
        case PickId:     return "PICK_ID";
        case LayerId:    return "LAYER_ID";
        case ObjectId:   return "OBJECT_ID";
        case MaterialId: return "MATERIAL_ID";
        default:         return "TEXCOORD";
    }
}

// GLSL variable prefix
constexpr const char* semanticGlsl(VertexSemantic sem) {
    using enum VertexSemantic;
    switch (sem) {
        case Position:   return "a_position";
        case Normal:     return "a_normal";
        case Tangent:    return "a_tangent";
        case Bitangent:  return "a_bitangent";
        case Color0:     return "a_color0";
        case Color1:     return "a_color1";
        case TexCoord0:  return "a_texcoord0";
        case TexCoord1:  return "a_texcoord1";
        case TexCoord2:  return "a_texcoord2";
        case TexCoord3:  return "a_texcoord3";
        case Indices:    return "a_indices";
        case Weight:     return "a_weight";
        case InstanceId: return "gl_InstanceID";
        case PickId:     return "a_pickId";
        case LayerId:    return "a_layerId";
        case ObjectId:   return "a_objectId";
        case MaterialId: return "a_materialId";
        case User0:      return "a_user0";
        case User1:      return "a_user1";
        case User2:      return "a_user2";
        case User3:      return "a_user3";
        default:         return "a_user";
    }
}

} // namespace MulanGeo::Engine

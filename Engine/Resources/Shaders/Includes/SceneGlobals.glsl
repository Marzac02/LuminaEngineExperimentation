//////////////////////////////////////////////////////////

#include "Common.glsl"

//////////////////////////////////////////////////////////
// BINDING SET 0
//////////////////////////////////////////////////////////

layout(set = 0, binding = 0) restrict uniform SceneGlobals
{
    FSceneGlobalData uSceneData;
};

layout(set = 0, binding = 1) restrict readonly buffer SceneLightData
{
    FLightData LightData;
};

layout(set = 0, binding = 2) restrict readonly buffer BufferInstanceData
{
    FInstanceData Instances[];
} InstanceData;

layout(set = 0, binding = 3) restrict buffer InstanceMappingData
{
    uint Mapping[];
} MappingData;

layout(set = 0, binding = 4) restrict buffer FIndirectDrawBuffer
{
    FDrawIndexedIndirectArguments Args[];
} IndirectDrawData;

layout(set = 0, binding = 5) restrict readonly buffer FBoneData
{
    mat4 BoneMatrices[];
} BoneData;

layout(set = 0, binding = 6) restrict buffer ClusterSSBO
{
    FCluster Clusters[];
} Clusters;

layout(set = 0, binding = 7) restrict buffer MaterialUniforms
{
    vec4 Scalars[MAX_SCALARS / 4];
    vec4 Vectors[MAX_VECTORS];
} uMaterialUniforms;

layout(set = 0, binding = 8)        uniform sampler2DArray uShadowCascade;
layout(set = 0, binding = 9)        uniform sampler2DArray uShadowAtlas;
layout(set = 0, binding = 10)       uniform usampler2D uSelectionTexture;
layout(set = 0, binding = 11)       uniform sampler2D uDepthPyramid;
layout(set = 0, binding = 12)       uniform sampler2D uHDRSceneColor;


//////////////////////////////////////////////////////////
// BINDING SET 1
//////////////////////////////////////////////////////////

layout(set = 1, binding = 0) uniform sampler2D uGlobalTextures[];


//////////////////////////////////////////////////////////



float GetMaterialScalar(uint Index)
{
    uint v = Index / 4;
    uint c = Index % 4;
    return uMaterialUniforms.Scalars[v][c];
}

vec4 GetMaterialVec4(uint Index)
{
    return uMaterialUniforms.Vectors[Index];
}

uint DrawIDToInstanceID(uint ID)
{
    return MappingData.Mapping[ID];
}

FInstanceData GetInstanceData(uint Index)
{
    return InstanceData.Instances[DrawIDToInstanceID(Index)];
}

vec3 GetSunDirection()
{
    return LightData.SunDirection.xyz;
}

vec3 GetAmbientLightColor()
{
    return LightData.AmbientLight.xyz;
}

float GetAmbientLightIntensity()
{
    return LightData.AmbientLight.w;
}

float GetTime()
{
    return uSceneData.Time;
}

float GetDeltaTime()
{
    return uSceneData.DeltaTime;
}

float GetNearPlane()
{
    return uSceneData.NearPlane;
}

float GetFarPlane()
{
    return uSceneData.FarPlane;
}

vec3 GetCameraPosition()
{
    return uSceneData.CameraView.CameraPosition.xyx;
}

vec3 GetCameraUp()
{
    return uSceneData.CameraView.CameraUp.xyx;
}

vec3 GetCameraRight()
{
    return uSceneData.CameraView.CameraRight.xyx;
}

vec3 GetCameraForward()
{
    return uSceneData.CameraView.CameraForward.xyx;
}

mat4 GetCameraView()
{
    return uSceneData.CameraView.CameraView;
}

mat4 GetInverseCameraView()
{
    return uSceneData.CameraView.InverseCameraView;
}

mat4 GetCameraProjection()
{
    return uSceneData.CameraView.CameraProjection;
}

mat4 GetInverseCameraProjection()
{
    return uSceneData.CameraView.InverseCameraProjection;
}

mat4 GetModelMatrix(uint Index)
{
    return InstanceData.Instances[DrawIDToInstanceID(Index)].ModelMatrix;
}

vec3 GetModelLocation(uint Index)
{
    mat4 Matrix = GetModelMatrix(Index);
    return vec3(Matrix[3].xyz);
}

uint GetEntityID(uint Index)
{
    return InstanceData.Instances[DrawIDToInstanceID(Index)].EntityID;
}

vec3 WorldToView(vec3 WorldPos)
{
    return (GetCameraView() * vec4(WorldPos, 1.0)).xyz;
}

vec3 NormalWorldToView(vec3 Normal)
{
    return mat3(GetCameraView()) * Normal;
}

vec4 ViewToClip(vec3 ViewPos)
{
    return GetCameraProjection() * vec4(ViewPos, 1.0);
}

vec4 WorldToClip(vec3 WorldPos)
{
    return GetCameraProjection() * GetCameraView() * vec4(WorldPos, 1.0);
}

float SineWave(float speed, float amplitude)
{
    return amplitude * sin(float(GetTime()) * speed);
}

FLight GetLightAt(uint Index)
{
    return LightData.Lights[Index];
}

uint GetNumLights()
{
    return LightData.NumLights;
}

vec3 ReconstructWorldPos(vec2 uv, float depth, mat4 invProj, mat4 invView)
{
    vec4 clip = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 view = invProj * clip;
    view /= view.w;
    vec4 world = invView * view;
    return world.xyz;
}

vec3 ReconstructViewPos(vec2 uv, float depth, mat4 invProj)
{
    vec4 clip = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 view = invProj * clip;
    return view.xyz / view.w;
}
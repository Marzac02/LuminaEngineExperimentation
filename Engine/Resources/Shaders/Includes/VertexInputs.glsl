
layout(location = 0) in vec3 inPosition;
layout(location = 1) in uint inNormal;
layout(location = 2) in uvec2 inUV;
layout(location = 3) in vec4 inColor;

#ifdef SKINNED_VERTEX
layout(location = 4) in uvec4 inJointIndices;
layout(location = 5) in uvec4 inJointWeights;
#endif

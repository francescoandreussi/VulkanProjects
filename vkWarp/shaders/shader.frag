#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D uvTexSamplerMS;
layout(binding = 2) uniform sampler2D uvTexSamplerLS;
layout(binding = 3) uniform sampler2D colorTexSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 firstLayerTexCoordMS = texture(uvTexSamplerMS, fragTexCoord);
    vec4 firstLayerTexCoordLS = texture(uvTexSamplerLS, fragTexCoord);
    float u = (firstLayerTexCoordMS.r * 256 + firstLayerTexCoordLS.r) / 65535;
    float v = (firstLayerTexCoordMS.g * 256 + firstLayerTexCoordLS.g) / 65535;
    float intensity = (firstLayerTexCoordMS.b * 256 + firstLayerTexCoordLS.b) / 65535;
    outColor = texture(colorTexSampler, vec2(u,v)) * intensity; //vec4(fragColor/*1.0, 0.0, 0.0*/, 1.0);
    //outColor = texture(colorTexSampler, fragTexCoord);
}
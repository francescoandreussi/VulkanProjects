#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D uvTexSamplerMS;
layout(binding = 2) uniform sampler2D uvTexSamplerLS;
layout(binding = 3) uniform sampler2D colorTexSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    /** 16-bits layered texture mapping */
    vec4 firstLayerTexCoordMS = texture(uvTexSamplerMS, fragTexCoord); // Most Significant 8-bits
    vec4 firstLayerTexCoordLS = texture(uvTexSamplerLS, fragTexCoord); // Least Significant 8-bits
    float u = (firstLayerTexCoordMS.r * 65280.0 + firstLayerTexCoordLS.r * 255.0) / 65535.0; // Computing 16-bits u coordinate
    float v = (firstLayerTexCoordMS.g * 65280.0 + firstLayerTexCoordLS.g * 255.0) / 65535.0; // Computing 16-bits v coordinate
    float intensity = (firstLayerTexCoordMS.b * 65280.0 + firstLayerTexCoordLS.b * 255.0) / 65535.0; // Computing 16-bits intensity
    outColor = texture(colorTexSampler, vec2(u,v)) * intensity; // Computing final colour

    /** 8-bits layered texture mapping */
    //vec4 firstLayerTexCoord = texture(uvTexSamplerMS, fragTexCoord);
    //outColor = texture(colorTexSampler, firstLayerTexCoord.rg) * firstLayerTexCoord.b;

    /** non-layered texture mapping */
    //outColor = texture(colorTexSampler, fragTexCoord);

    /** no texture mapping */
    //outColor = vec4(fragTexCoord, 0.0, 1.0);
}
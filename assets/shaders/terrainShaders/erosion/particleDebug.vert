#version 450 core

struct Particle
{
    vec4 motion;
    uvec4 state;
};

layout (std430, binding = 0) readonly buffer ParticleBuffer { Particle particles[]; };

uniform sampler2D u_height;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform int u_size;

out float f_Sediment;

void main()
{
    Particle particle = particles[gl_VertexID];
    vec2 position = particle.motion.xy;
    float water = uintBitsToFloat(particle.state.z);
    float sediment = uintBitsToFloat(particle.state.w);
	if (particle.state.y == 0u || particle.state.y >= 160u || water <= 0.0)
	{
		gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
		gl_PointSize = 0.0;
		f_Sediment = 0.0;
		return;
	}
    vec2 uv = (position + 0.5) / float(u_size);
    float height = texture(u_height, uv).r;
    vec3 worldPosition = vec3(position.x - float(u_size - 1) * 0.5, height + 0.18, position.y - float(u_size - 1) * 0.5);
    gl_Position = u_projection * u_view * vec4(worldPosition, 1.0);
    gl_PointSize = clamp(2.0 + water * 1.5, 2.0, 4.0);
    f_Sediment = clamp(sediment * 3.0, 0.0, 1.0);
}

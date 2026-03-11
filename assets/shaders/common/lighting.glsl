

struct Light
{
	vec3 direction;
	vec3 color;
	float intensivity;
};

struct Material
{
	vec3 diffuse;
	vec3 ambient;
	vec3 specular;
	float shininess;
};

uniform Light u_light;
uniform Material u_material;
uniform vec3 u_cameraPosition;

vec3 ApplyLighting(vec3 normal, vec3 albedo, vec3 fragPos)
{
	vec3 ambient = u_light.color * u_material.ambient;

	vec3 lightDir = normalize(-u_light.direction);

	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = u_light.color * (diff * u_material.diffuse);

	vec3 viewDir = normalize(u_cameraPosition - fragPos);
	vec3 reflectDir = reflect(-lightDir, normal);

	float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_material.shininess);
	vec3 specular = u_light.color * (spec * u_material.specular);

	return ((ambient + diffuse + specular) * u_light.intensivity) * albedo;
}
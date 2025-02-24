#version 410 core

struct LightComponent {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;

out vec4 fColor;

//matrices
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;

//lighting
uniform vec3 lightDir_d;    // directional light
uniform vec3 lightColor_d;  
uniform vec3 lightDir_p1;    // point light 1
uniform vec3 lightColor_p1;  
uniform vec3 lightPosEye_p1;
uniform vec3 lightPosEye_p2;
uniform vec3 lightPosEye_p3;
uniform vec3 lightPosEye_p4;
uniform float constant_p1;
uniform float linear_p1;
uniform float quadratic_p1;
uniform int pointlights_on;

// textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

//common conputation
vec4 fPosEye;
vec3 normalEye;


//components DIRECTIONAL light
//vec3 ambient_d;
float ambientStrength_d = 0.2f;
//vec3 diffuse_d;
//vec3 specular_d;
float specularStrength_d = 0.5f;
float shininess_d = 32.0f;
LightComponent component_d;     // contains: ambient,difuse,specular

//components POINT light 1
float ambientStrength_p1 = 0.002f;
float specularStrength_p1 = 0.1f;
float shininess_p1 = 32.0f;
LightComponent component_p1;

void commonComputation()
{
    //compute eye space coordinates
    fPosEye = view * model * vec4(fPosition, 1.0f);
    normalEye = normalize(normalMatrix * fNormal);
}

LightComponent computeDirLight(vec3 lightDir, vec3 lightColor, float ambientStrength, float specularStrength, float shininess)
{
    //normalize light direction
    vec3 lightDirN = normalize(view*vec4(lightDir,0.0f)).xyz;

    //compute view direction (in eye coordinates, the viewer is situated at the origin)
    vec3 viewDir = normalize(- fPosEye.xyz);

    LightComponent component;

    //compute ambient light
    component.ambient = ambientStrength * lightColor;

    //compute diffuse light
    component.diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;

    //compute specular light
    vec3 reflectDir = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0f), shininess);
    component.specular = specularStrength * specCoeff * lightColor;

    return component;
}

LightComponent computePosLight(vec3 lightDir, vec3 lightColor, vec3 lightPosEye, float constant, float linear, float quadratic, float ambientStrength, float specularStrength, float shininess)
{
    vec3 new_lightPosEye = (view * vec4(lightPosEye, 1.0f)).xyz;

    //compute light direction
	vec3 lightDirN = normalize(new_lightPosEye-fPosEye.xyz);
	
	//compute view direction 
	vec3 viewDir = normalize(- fPosEye.xyz);

    //compute half vector
	vec3 halfVector = normalize(lightDirN + viewDir);


	//compute distance to light
	float dist = length(new_lightPosEye - fPosEye.xyz);
	//compute attenuation
	float att = 1.0f / (constant + linear * dist + quadratic * (dist * dist));

    LightComponent component;
		
	//compute ambient light
	component.ambient = att * ambientStrength * lightColor;
	
	//compute diffuse light
	component.diffuse = att * max(dot(normalEye, lightDirN), 0.0f) * lightColor;

	//compute specular light
	float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), shininess);
	component.specular = att * specularStrength * specCoeff * lightColor;

    return component;
}

float computeFog()
{
 float fogDensity = 0.01f;
 float fragmentDistance = length(fPosEye);
 float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));

 return clamp(fogFactor, 0.0f, 1.0f);
}

float computeShadow(){
    vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    normalizedCoords = normalizedCoords * 0.5 + 0.5;

    float closestDepth = texture(shadowMap, normalizedCoords.xy).r;
    float currentDepth = normalizedCoords.z;

    float bias = 0.005f;
    float shadow = currentDepth-bias > closestDepth ? 1.0 : 0.0;
    if (normalizedCoords.z > 1.0f) return 0.0f;

    return shadow;
}


void main() 
{
    commonComputation();

    LightComponent component_d = computeDirLight(lightDir_d, lightColor_d, ambientStrength_d, specularStrength_d, shininess_d);
    LightComponent component_p1 = computePosLight(lightDir_p1, lightColor_p1, lightPosEye_p1, constant_p1, linear_p1, quadratic_p1, ambientStrength_p1, specularStrength_p1, shininess_p1);
    LightComponent component_p2 = computePosLight(lightDir_p1, lightColor_p1, lightPosEye_p2, constant_p1, linear_p1, quadratic_p1, ambientStrength_p1, specularStrength_p1, shininess_p1);
    LightComponent component_p3 = computePosLight(lightDir_p1, lightColor_p1, lightPosEye_p3, constant_p1, linear_p1, quadratic_p1, ambientStrength_p1, specularStrength_p1, shininess_p1);
    LightComponent component_p4 = computePosLight(lightDir_p1, lightColor_p1, lightPosEye_p4, constant_p1, linear_p1, quadratic_p1, ambientStrength_p1, specularStrength_p1, shininess_p1);

    vec3 ambient_point = component_p1.ambient + component_p2.ambient + component_p3.ambient + component_p4.ambient;
    vec3 diffuse_point = component_p1.diffuse + component_p2.diffuse + component_p3.diffuse + component_p4.diffuse;
    vec3 specular_point = component_p1.specular + component_p2.specular + component_p3.specular + component_p4.specular;

    vec3 ambient_final = component_d.ambient;
    vec3 diffuse_final = component_d.diffuse;
    vec3 specular_final = component_d.specular;

    if (pointlights_on == 1){
        ambient_final += ambient_point;
        diffuse_final += diffuse_point;
        specular_final += specular_point;
    }

    //fragment discarding
    vec4 colorFromTexture = texture(diffuseTexture, fTexCoords);
    if(colorFromTexture.a<0.1)
        discard;

    float shadow = computeShadow();
    //compute final vertex color
    vec3 color = min((ambient_final + (1.0f - shadow)*diffuse_final) * colorFromTexture.rgb + specular_final * (1.0f - shadow) * texture(specularTexture, fTexCoords).rgb, 1.0f);

    float fogFactor = computeFog();
    vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
    fColor = fogColor * (1 - fogFactor) + vec4(color * fogFactor, 1.0f);

    vec4 auxColor = vec4(color, 1.0f);
    fColor = fogColor * (1-fogFactor) + auxColor * fogFactor;

    //fColor = vec4(color, 1.0f);
}

#version 410 core

out vec4 fColor;

in vec2 fTexCoords;
uniform sampler2D diffuseTexture;

void main() {

    vec4 colorFromTexture = texture(diffuseTexture, fTexCoords);
    if(colorFromTexture.a<0.1)
        discard;

	fColor = vec4(1.0f);
}
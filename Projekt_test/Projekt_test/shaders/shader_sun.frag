#version 430 core

uniform sampler2D colorTexture; 
uniform bool isHighlighted;     
uniform vec3 highlightColor;    

in vec2 vecTex; 

out vec4 outColor;

void main()
{
    vec3 textureColor = texture(colorTexture, vecTex).xyz; 

    
    if(isHighlighted) {
        textureColor = mix(textureColor, highlightColor, 0.5);
    }

    outColor = vec4(textureColor, 1.0); 
}

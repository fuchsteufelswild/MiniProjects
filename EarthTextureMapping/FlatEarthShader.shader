#shader vertex
#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 text;

out vec4 vertexColor;
out vec3 normalCoord;
out vec3 position;
out vec2 texCoord;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

uniform float width;
uniform float height;
uniform float heightFactor;

uniform sampler2D heightMap;

uniform float heightMapOffset;


float CalculateHeight(vec3 vert)
{
	vec2 textureCoordinate;
	vec2 temp = vec2(vert.x + heightMapOffset, vert.z);
	if (temp.x > width)
		temp.x -= width;
	else if (temp.x < 0)
		temp.x += width;

	textureCoordinate.x = 1.0f - float(temp.x) / float(width);
	textureCoordinate.y = 1.0f - float(temp.y) / float(height);
	// vec4 heightColor = texture(heightMap, vec2(1.0f - (vert.x / float(width)), 1.0f - (vert.z / float(height))));
	vec4 heightColor = texture(heightMap, textureCoordinate);
	return heightFactor * heightColor.r;
}


bool IsInsideBounds(vec3 vert)
{
	if (vert.x >= 0.0f && vert.x <= width)
	{
		if (vert.z >= 0.0f && vert.z <= height)
		{
			return true;
		}
	}
	return false;
}


void SetNormalCoord(vec3 fPos)
{
	vec3 regulatedPosition;
	regulatedPosition.x = fPos.x;
	regulatedPosition.y = fPos.y;
	regulatedPosition.z = fPos.z;


	// Get each height in the neighhbourhood of the current vertex
	vec3 r0 = pos + vec3(1.0f, 0.0f, 0.0f);
	vec3 r1 = pos + vec3(1.0f, 0.0f, 1.0f);
	vec3 r2 = pos + vec3(0.0f, 0.0f, 1.0f);
	vec3 r3 = pos + vec3(-1.0f, 0.0f, 1.0f);
	vec3 r4 = pos + vec3(-1.0f, 0.0f, 0.0f);
	vec3 r5 = pos + vec3(-1.0f, 0.0f, -1.0f);
	vec3 r6 = pos + vec3(0.0f, 0.0f, -1.0f);
	vec3 r7 = pos + vec3(1.0f, 0.0f, -1.0f);

	r0.y = CalculateHeight(r0);
	r1.y = CalculateHeight(r1);
	r2.y = CalculateHeight(r2);
	r3.y = CalculateHeight(r3);
	r4.y = CalculateHeight(r4);
	r5.y = CalculateHeight(r5);
	r6.y = CalculateHeight(r6);
	r7.y = CalculateHeight(r7);

	if (IsInsideBounds(r0) && IsInsideBounds(r1))
	{
		normalCoord += normalize(cross(r1 - regulatedPosition, r0 - regulatedPosition));
	}
	/*if (IsInsideBounds(r0) && IsInsideBounds(r2))
	{
		normalCoord += normalize(cross(r1 - regulatedPosition, r0 - regulatedPosition));
	}*/
	if (IsInsideBounds(r1) && IsInsideBounds(r2))
	{
		normalCoord += normalize(cross(r2 - regulatedPosition, r1 - regulatedPosition));
	}
	if (IsInsideBounds(r2) && IsInsideBounds(r3))
	{
		normalCoord += normalize(cross(r3 - regulatedPosition, r2 - regulatedPosition));
	}
	if (IsInsideBounds(r3) && IsInsideBounds(r4))
	{
		normalCoord += normalize(cross(r4 - regulatedPosition, r3 - regulatedPosition));
	}
	if (IsInsideBounds(r4) && IsInsideBounds(r5))
	{
		normalCoord += normalize(cross(r5 - regulatedPosition, r4 - regulatedPosition));
	}
	/*if (IsInsideBounds(r4) && IsInsideBounds(r6))
	{
		normalCoord += normalize(cross(r5 - regulatedPosition, r4 - regulatedPosition));
	}*/
	if (IsInsideBounds(r5) && IsInsideBounds(r6))
	{
		normalCoord += normalize(cross(r6 - regulatedPosition, r5 - regulatedPosition));
	}
	if (IsInsideBounds(r6) && IsInsideBounds(r7))
	{
		normalCoord += normalize(cross(r7 - regulatedPosition, r6 - regulatedPosition));
	}
	if (IsInsideBounds(r7) && IsInsideBounds(r0))
	{
		normalCoord += normalize(cross(r0 - regulatedPosition, r7 - regulatedPosition));
	}

	normalCoord = normalize(normalCoord);

}


void main()
{
	vec3 finalPos;
	finalPos.x = pos.x;
	finalPos.y = 0.0f;
	finalPos.z = pos.z;
	

	normalCoord.x = 0;
	normalCoord.y = 0;
	normalCoord.z = 0;

	vertexColor = clamp(vec4(pos, 1.0f), 0.0f, 1.0f);

	vec2 temp = vec2(pos.x + heightMapOffset, pos.z);
	if (temp.x > width)
		temp.x -= (width);
	if (temp.x < 0)
		temp.x += (width);

	

	texCoord.x = 1.0f - float(temp.x) / float(width);
	texCoord.y = 1.0f - float(temp.y) / float(height);

	// finalPos.y = texture(heightMap, textureCoordinate).r * heightFactor;

	finalPos.y = CalculateHeight(finalPos);
	SetNormalCoord(finalPos);

	// normalCoord = vec3(0.0f, 1.0f, 0.0f);
	vec4 tempPosition = (u_Model * vec4(finalPos, 1.0f));

	position = tempPosition.xyz;
	
	gl_Position =  u_Projection * u_View * u_Model * vec4(finalPos, 1.0f);
}

#shader fragment
#version 330 core

struct Light
{
	vec4 diffuseLight;
	vec4 specularLight;
};

Light CalculateLight(vec4, vec4, int, int);

uniform vec4 u_Color;
layout(location = 0) out vec4 color;

uniform sampler2D textureSampler;

in vec4 vertexColor;
in vec3 normalCoord;
in vec3 position;
in vec2 texCoord;

uniform vec3 camPosition;
uniform vec3 lightPosition;

vec4 lightColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);

vec4 ambientReflectanceCoefficient = vec4(0.25f, 0.25f, 0.25f, 1.0f);
vec4 ambientColor = vec4(0.3f, 0.3f, 0.3f, 1.0f);

vec4 specularReflectanceCoefficient = vec4(1.0f, 1.0f, 1.0f, 1.0f);
vec4 specularLightColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
int specularExponent = 100;

vec4 diffuseReflectanceCoefficient = vec4(1.0f, 1.0f, 1.0f, 1.0f);
vec4 diffuseLightColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);

Light CalculateLight(vec3 lPos, vec4 lColor)
{
	vec3 lightDir = normalize(lPos - position);
	vec3 viewerDir = normalize(camPosition - position);
	vec3 halfwayVec = normalize(lightDir + viewerDir);

	float dotNormalLight = dot(lightDir, normalCoord); // Correlation between surface and the light

	vec4 resultSpec = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	if (dotNormalLight >= 0) // If lightdir and surface normal are on the same side
		resultSpec = specularReflectanceCoefficient * specularLightColor * pow(max(0, dot(halfwayVec, normalCoord)), specularExponent);

	return Light(diffuseReflectanceCoefficient * diffuseLightColor * max(0, dotNormalLight), resultSpec);
}

void main()
{
	vec4 finalTexture = texture(textureSampler, texCoord);
	Light rLight;
	vec4 resultingDiffuse = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	vec4 resultingSpecular = vec4(0.0f, 0.0f, 0.0f, 0.0f);

	rLight = CalculateLight(lightPosition, lightColor);

	resultingDiffuse += rLight.diffuseLight;
	resultingSpecular += rLight.specularLight;
	color = finalTexture * (ambientReflectanceCoefficient * ambientColor + resultingDiffuse) + resultingSpecular;
	// color = finalTexture + ambientColor + ambientColor;
}
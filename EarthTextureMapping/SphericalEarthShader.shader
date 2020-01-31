#shader vertex
#version 330 core

layout(location = 0) in vec2 pos;

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

uniform float horizontalSplit;
uniform float verticalSplit;

uniform sampler2D heightMap;

uniform float heightMapOffset;

vec3 GetWorldCoordinateOfPosition(vec2 vec)
{
	vec3 res;

	float theta = radians(180.0f * vec.y / 125.0f);
	float alpha = radians(360.0f * (vec.x / 250.0f));
	res.x = 350 * sin(theta) * cos(alpha);
	res.y = 350 * sin(theta) * sin(alpha);
	res.z = 350 * cos(theta);
	/*res.x = 350 * sin(theta) * cos(alpha);
	res.y = 350 * cos(theta);
	res.z = 350 * sin(theta)* sin(alpha);*/

	return res;
}

float CalculateHeight(vec2 vert)
{
	vec2 textureCoordinate;
	vec2 temp = vec2(vert.x + heightMapOffset, vert.y);
	if (temp.x > horizontalSplit)
		temp.x -= horizontalSplit;
	else if (temp.x < 0)
		temp.x += horizontalSplit;

	textureCoordinate.x = temp.x / float(horizontalSplit);
	textureCoordinate.y = temp.y / float(verticalSplit);
	// vec4 heightColor = texture(heightMap, vec2(1.0f - (vert.x / float(width)), 1.0f - (vert.z / float(height))));
	vec4 heightColor = texture(heightMap, textureCoordinate);
	return heightFactor * heightColor.r;
}


bool IsInsideBounds(vec2 vert)
{
	if (vert.x >= 0.0f && vert.x <= horizontalSplit)
	{
		if (vert.y >= 0.0f && vert.y <= verticalSplit)
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

	/*if (pos.y < 5 || pos.x < 5 || pos.y > 120 || pos.x > 245)
	{
		normalCoord = normalize(regulatedPosition);
		return;
	}*/
	
	// Get each height in the neighhbourhood of the current vertex

	vec2 tr0 = pos + vec2(1.0f, 0.0f);
	vec2 tr1 = pos + vec2(1.0f, 1.0f);
	vec2 tr2 = pos + vec2(0.0f, 1.0f);
	vec2 tr3 = pos + vec2(-1.0f, 1.0f);
	vec2 tr4 = pos + vec2(-1.0f, 0.0f);
	vec2 tr5 = pos + vec2(-1.0f, -1.0f);
	vec2 tr6 = pos + vec2(0.0f, -1.0f);
	vec2 tr7 = pos + vec2(1.0f, -1.0f);

	float h0 = CalculateHeight(tr0);
	float h1 = CalculateHeight(tr1);
	float h2 = CalculateHeight(tr2);
	float h3 = CalculateHeight(tr3);
	float h4 = CalculateHeight(tr4);
	float h5 = CalculateHeight(tr5);
	float h6 = CalculateHeight(tr6);
	float h7 = CalculateHeight(tr7);

	vec3 r0 = GetWorldCoordinateOfPosition(tr0);
	vec3 r1 = GetWorldCoordinateOfPosition(tr1);
	vec3 r2 = GetWorldCoordinateOfPosition(tr2);
	vec3 r3 = GetWorldCoordinateOfPosition(tr3);
	vec3 r4 = GetWorldCoordinateOfPosition(tr4);
	vec3 r5 = GetWorldCoordinateOfPosition(tr5);
	vec3 r6 = GetWorldCoordinateOfPosition(tr6);
	vec3 r7 = GetWorldCoordinateOfPosition(tr7);

	r0 += normalize(r0) * h0;
	r1 += normalize(r1) * h1;
	r2 += normalize(r2) * h2;
	r3 += normalize(r3) * h3;
	r4 += normalize(r4) * h4;
	r5 += normalize(r5) * h5;
	r6 += normalize(r6) * h6;
	r7 += normalize(r7) * h7;

	if (IsInsideBounds(tr0) && IsInsideBounds(tr1))
	{
		 normalCoord += normalize(cross(r1 - regulatedPosition, r0 - regulatedPosition));
	}
	if (IsInsideBounds(tr1) && IsInsideBounds(tr2))
	{
		normalCoord += normalize(cross(r2 - regulatedPosition, r1 - regulatedPosition));
	}
	if (IsInsideBounds(tr2) && IsInsideBounds(tr3))
	{
		normalCoord += normalize(cross(r3 - regulatedPosition, r2 - regulatedPosition));
	}
	if (IsInsideBounds(tr3) && IsInsideBounds(tr4))
	{
		normalCoord += normalize(cross(r4 - regulatedPosition, r3 - regulatedPosition));
	}
	if (IsInsideBounds(tr4) && IsInsideBounds(tr5))
	{
		normalCoord += normalize(cross(r5 - regulatedPosition, r4 - regulatedPosition));
	}
	if (IsInsideBounds(tr5) && IsInsideBounds(tr6))
	{
		normalCoord += normalize(cross(r6 - regulatedPosition, r5 - regulatedPosition));
	}
	if (IsInsideBounds(tr6) && IsInsideBounds(tr7))
	{
		normalCoord += normalize(cross(r7 - regulatedPosition, r6 - regulatedPosition));
	}
	if (IsInsideBounds(tr7) && IsInsideBounds(tr0))
	{
		normalCoord += normalize(cross(r0 - regulatedPosition, r7 - regulatedPosition));
	}

	normalCoord = normalize(normalCoord);

}


vec3 finalPos;



void main()
{
	finalPos = GetWorldCoordinateOfPosition(pos);
	/*finalPos.x = pos.x;
	finalPos.y = 0.0f;
	finalPos.z = pos.z;*/
	//
	finalPos += normalize(finalPos) * CalculateHeight(pos);
	normalCoord.x = 0;
	normalCoord.y = 0;
	normalCoord.z = 0;
	// normalCoord = normalize(finalPos);
	// vertexColor = clamp(vec4(pos, 1.0f), 0.0f, 1.0f);

	vec2 temp = vec2(pos.x + heightMapOffset, pos.y);
	if (temp.x > horizontalSplit)
		temp.x -= horizontalSplit;
	else if (temp.x < 0)
		temp.x += horizontalSplit;


	texCoord.x = temp.x / float(horizontalSplit);
	texCoord.y = temp.y / float(verticalSplit);

	// finalPos.y = texture(heightMap, textureCoordinate).r * heightFactor;

	// finalPos.y = CalculateHeight(finalPos);
	SetNormalCoord(finalPos);
	// normalCoord = normalize(finalPos);
	// normalCoord = normalize(finalPos);
	// normalCoord = normalize(finalPos);
	// finalPos += normalCoord * CalculateHeight(pos);

	vec4 tempPosition = (u_Model * vec4(finalPos, 1.0f));

	position = tempPosition.xyz;

	gl_Position = u_Projection * u_View * u_Model * vec4(finalPos, 1.0f);
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
	color = vec4(clamp(finalTexture * (ambientReflectanceCoefficient * ambientColor + resultingDiffuse) + resultingSpecular, 0.0f, 1.0f));
	// color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}
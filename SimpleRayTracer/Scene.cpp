#include "Scene.h"
#include "Camera.h"
#include "Light.h"
#include "Material.h"
#include "Shape.h"
#include "tinyxml2.h"

#include <future>
#include <thread>
#include <string>


using namespace tinyxml2;

// Find closest object in the path of the given ray
ReturnVal Scene::FindClosest(const Ray &ray)
{
	ReturnVal rt{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, nullptr};
	float minDis = 1e9;

	for(Shape* shape : objects)
	{
		ReturnVal hass = shape->intersect(ray);

		if(!hass.mat)
			continue;

		float dis = (hass.ip - ray.origin).Length();

		if (dis < minDis && dis > intTestEps)
		{
			minDis = dis;
			rt = hass;
		}
	}
	return rt;

}

// Calculate light for the given ray
void Scene::CalculateLight(const Ray &r, Vector3f& outColor, int depth)
{
	int shapeType;
	ReturnVal intersection = FindClosest(r);

	if(!intersection.mat) // No object in the path
	{
		outColor = backgroundColor;
		return;
	}

	outColor = ambientLight * intersection.mat->ambientRef; // Add ambient light to object

	Vector3f viewerDirection = r.origin - intersection.ip; // Camera look direction
	viewerDirection = viewerDirection.Normalize();

	for(PointLight* light : lights) // For each point light
	{
		Vector3f pointToLight = light->position - intersection.ip; // Light direction

		float distanceToLight = pointToLight.Length();

		pointToLight = pointToLight.Normalize();

		ReturnVal closestObjectInPath = FindClosest(Ray(intersection.ip + pointToLight * shadowRayEps, pointToLight)); // Find closest object in the path

		
		if(closestObjectInPath.mat) // Closest object in the path of the ray from intersection point to light exists
		{
			float distanceToObject = (closestObjectInPath.ip - intersection.ip).Length();

			if(distanceToObject > shadowRayEps && distanceToObject < distanceToLight - shadowRayEps) // Object is closer than light
			{
				continue;
			}
		}

		Vector3f lightIntensity = light->computeLightContribution(intersection.ip); // Resulting light intensity at the intersection point
		float dp = pointToLight.Dot(intersection.sn); // Correlation between light direction and the normal

		if(dp <= 0) // The are at right angles or light goes through the back face of the object
			continue;

		// Diffuse Reflection
		outColor = outColor + intersection.mat->diffuseRef * lightIntensity * dp;
		
		// Specular Reflection
		Vector3f add = pointToLight + viewerDirection; // Halfway vector 
		Vector3f halfwayVector = (add).Normalize(); 

		dp = halfwayVector.Dot(intersection.sn); // Correlation between halfway vector and the normal
		if(dp > 0)
			outColor = outColor + lightIntensity * std::pow(dp, intersection.mat->phongExp) * intersection.mat->specularRef; // Add specular reflection
	}

	if(depth < maxRecursionDepth)
	{
		Vector3f vrd = intersection.sn * 2 * viewerDirection.Dot(intersection.sn) - viewerDirection; // Viewer reflection direction
		vrd = vrd.Normalize();

		Vector3f mirCol; // Resulting mirror color
		CalculateLight(Ray(intersection.ip + vrd * shadowRayEps, vrd), mirCol, depth + 1); // Calculate color of the object for the viewer reflection direction ray

		if(!(mirCol.x == backgroundColor.x && mirCol.y == backgroundColor.y && mirCol.z == backgroundColor.z)) // If it intersects with an object
			outColor = outColor + mirCol * intersection.mat->mirrorRef;
	}

}

// Find the out color for the given ray
Color Scene::RenderPixel(const Ray &ray)
{
	Vector3f out;
	CalculateLight(ray, out, 0);

	unsigned char x, y, z;

	x = out.x > 255.0f ? 255 : out.x;
	y = out.y > 255.0f ? 255 : out.y;
	z = out.z > 255.0f ? 255 : out.z;

	return {x, y, z};
}

// Render Camera
void Scene::RenderCam(Camera *cam, Image &img, int iStart, int iEnd)
{
	for(int i = iStart; i < iEnd; ++i)
	{
		for(int j = 0; j < cam->imgPlane.nx; ++j)
		{
			Ray r = cam->getPrimaryRay(i, j);
			Color res = RenderPixel(r);
			img.setPixelValue(j, i, res);
		}
	}
}

/* 
 * Render the scene for each camera
 */
void Scene::renderScene(void)
{
	for(Camera* cam : cameras)
	{
		Image img(cam->imgPlane.nx, cam->imgPlane.ny);
		int rowDiff = cam->imgPlane.ny / 8;

		std::async(std::launch::async, [&] { RenderCam(cam, img, 0, rowDiff * 1); });
		std::async(std::launch::async, [&] { RenderCam(cam, img, rowDiff, rowDiff * 2); });
		std::async(std::launch::async, [&] { RenderCam(cam, img, rowDiff * 2, rowDiff * 3); });
		std::async(std::launch::async, [&] { RenderCam(cam, img, rowDiff * 3, rowDiff * 4); });
		std::async(std::launch::async, [&] { RenderCam(cam, img, rowDiff * 4, rowDiff * 5); });
		std::async(std::launch::async, [&] { RenderCam(cam, img, rowDiff * 5, rowDiff * 6); });
		std::async(std::launch::async, [&] { RenderCam(cam, img, rowDiff * 6, rowDiff * 7); });
		std::async(std::launch::async, [&] { RenderCam(cam, img, rowDiff * 7, cam->imgPlane.ny); });

		img.saveImage(cam->imageName);
	}

	
}

// Parses XML file. 
Scene::Scene(const char *xmlPath)
{
	pScene = this;

	const char *str;
	XMLDocument xmlDoc;
	XMLError eResult;
	XMLElement *pElement;

	maxRecursionDepth = 1;
	shadowRayEps = 0.001;

	eResult = xmlDoc.LoadFile(xmlPath);

	XMLNode *pRoot = xmlDoc.FirstChild();

	pElement = pRoot->FirstChildElement("MaxRecursionDepth");
	if(pElement != nullptr)
		pElement->QueryIntText(&maxRecursionDepth);

	pElement = pRoot->FirstChildElement("BackgroundColor");
	str = pElement->GetText();
	sscanf(str, "%f %f %f", &backgroundColor.r, &backgroundColor.g, &backgroundColor.b);

	pElement = pRoot->FirstChildElement("ShadowRayEpsilon");
	if(pElement != nullptr)
		pElement->QueryFloatText(&shadowRayEps);

	pElement = pRoot->FirstChildElement("IntersectionTestEpsilon");
	if(pElement != nullptr)
		eResult = pElement->QueryFloatText(&intTestEps);

	// Parse cameras
	pElement = pRoot->FirstChildElement("Cameras");
	XMLElement *pCamera = pElement->FirstChildElement("Camera");
	XMLElement *camElement;
	while(pCamera != nullptr)
	{
        int id;
        char imageName[64];
        Vector3f pos, gaze, up;
        ImagePlane imgPlane;

		eResult = pCamera->QueryIntAttribute("id", &id);
		camElement = pCamera->FirstChildElement("Position");
		str = camElement->GetText();
		sscanf(str, "%f %f %f", &pos.x, &pos.y, &pos.z);
		camElement = pCamera->FirstChildElement("Gaze");
		str = camElement->GetText();
		sscanf(str, "%f %f %f", &gaze.x, &gaze.y, &gaze.z);
		camElement = pCamera->FirstChildElement("Up");
		str = camElement->GetText();
		sscanf(str, "%f %f %f", &up.x, &up.y, &up.z);
		camElement = pCamera->FirstChildElement("NearPlane");
		str = camElement->GetText();
		sscanf(str, "%f %f %f %f", &imgPlane.left, &imgPlane.right, &imgPlane.bottom, &imgPlane.top);
		camElement = pCamera->FirstChildElement("NearDistance");
		eResult = camElement->QueryFloatText(&imgPlane.distance);
		camElement = pCamera->FirstChildElement("ImageResolution");	
		str = camElement->GetText();
		sscanf(str, "%d %d", &imgPlane.nx, &imgPlane.ny);
		camElement = pCamera->FirstChildElement("ImageName");
		str = camElement->GetText();
		strcpy(imageName, str);

		cameras.push_back(new Camera(id, imageName, pos, gaze, up, imgPlane));

		pCamera = pCamera->NextSiblingElement("Camera");
	}

	// Parse materals
	pElement = pRoot->FirstChildElement("Materials");
	XMLElement *pMaterial = pElement->FirstChildElement("Material");
	XMLElement *materialElement;
	while(pMaterial != nullptr)
	{
		materials.push_back(new Material());

		int curr = materials.size() - 1;
	
		eResult = pMaterial->QueryIntAttribute("id", &materials[curr]->id);
		materialElement = pMaterial->FirstChildElement("AmbientReflectance");
		str = materialElement->GetText();
		sscanf(str, "%f %f %f", &materials[curr]->ambientRef.r, &materials[curr]->ambientRef.g, &materials[curr]->ambientRef.b);
		materialElement = pMaterial->FirstChildElement("DiffuseReflectance");
		str = materialElement->GetText();
		sscanf(str, "%f %f %f", &materials[curr]->diffuseRef.r, &materials[curr]->diffuseRef.g, &materials[curr]->diffuseRef.b);
		materialElement = pMaterial->FirstChildElement("SpecularReflectance");
		str = materialElement->GetText();
		sscanf(str, "%f %f %f", &materials[curr]->specularRef.r, &materials[curr]->specularRef.g, &materials[curr]->specularRef.b);
		materialElement = pMaterial->FirstChildElement("MirrorReflectance");
		if(materialElement != nullptr)
		{
			str = materialElement->GetText();
			sscanf(str, "%f %f %f", &materials[curr]->mirrorRef.r, &materials[curr]->mirrorRef.g, &materials[curr]->mirrorRef.b);
		}
				else
		{
			materials[curr]->mirrorRef.r = 0.0;
			materials[curr]->mirrorRef.g = 0.0;
			materials[curr]->mirrorRef.b = 0.0;
		}
		materialElement = pMaterial->FirstChildElement("PhongExponent");
		if(materialElement != nullptr)
			materialElement->QueryIntText(&materials[curr]->phongExp);
		else
			materials[curr]->phongExp = 0;

		pMaterial = pMaterial->NextSiblingElement("Material");
	}

	// Parse vertex data
	pElement = pRoot->FirstChildElement("VertexData");
	int cursor = 0;
	Vector3f tmpPoint;
	str = pElement->GetText();
	while(str[cursor] == ' ' || str[cursor] == '\t' || str[cursor] == '\n')
		cursor++;
	while(str[cursor] != '\0')
	{
		for(int cnt = 0 ; cnt < 3 ; cnt++)
		{
			if(cnt == 0)
				tmpPoint.x = atof(str + cursor);
			else if(cnt == 1)
				tmpPoint.y = atof(str + cursor);
			else
				tmpPoint.z = atof(str + cursor);
			while(str[cursor] != ' ' && str[cursor] != '\t' && str[cursor] != '\n')
				cursor++; 
			while(str[cursor] == ' ' || str[cursor] == '\t' || str[cursor] == '\n')
				cursor++;
		}
		vertices.push_back(tmpPoint);
	}

	// Parse objects
	pElement = pRoot->FirstChildElement("Objects");
	
	// Parse spheres
	XMLElement *pObject = pElement->FirstChildElement("Sphere");
	XMLElement *objElement;
	while(pObject != nullptr)
	{
		int id;
		int matIndex;
		int cIndex;
		float R;

		eResult = pObject->QueryIntAttribute("id", &id);
		objElement = pObject->FirstChildElement("Material");
		eResult = objElement->QueryIntText(&matIndex);
		objElement = pObject->FirstChildElement("Center");
		eResult = objElement->QueryIntText(&cIndex);
		objElement = pObject->FirstChildElement("Radius");
		eResult = objElement->QueryFloatText(&R);

		objects.push_back(new Sphere(id, matIndex, cIndex, R, &vertices));

		pObject = pObject->NextSiblingElement("Sphere");
	}

	// Parse triangles
	pObject = pElement->FirstChildElement("Triangle");
	while(pObject != nullptr)
	{
		int id;
		int matIndex;
		int p1Index;
		int p2Index;
		int p3Index;

		eResult = pObject->QueryIntAttribute("id", &id);
		objElement = pObject->FirstChildElement("Material");
		eResult = objElement->QueryIntText(&matIndex);
		objElement = pObject->FirstChildElement("Indices");
		str = objElement->GetText();
		sscanf(str, "%d %d %d", &p1Index, &p2Index, &p3Index);

		objects.push_back(new Triangle(id, matIndex, p1Index, p2Index, p3Index, &vertices));

		pObject = pObject->NextSiblingElement("Triangle");
	}

	// Parse meshes
	pObject = pElement->FirstChildElement("Mesh");
	while(pObject != nullptr)
	{
		int id;
		int matIndex;
		int p1Index;
		int p2Index;
		int p3Index;
		int cursor = 0;
		int vertexOffset = 0;
		vector<Triangle> faces;
		vector<int> *meshIndices = new vector<int>;

		eResult = pObject->QueryIntAttribute("id", &id);
		objElement = pObject->FirstChildElement("Material");
		eResult = objElement->QueryIntText(&matIndex);
		objElement = pObject->FirstChildElement("Faces");
		objElement->QueryIntAttribute("vertexOffset", &vertexOffset);
		str = objElement->GetText();
		while(str[cursor] == ' ' || str[cursor] == '\t' || str[cursor] == '\n')
			cursor++;
		while(str[cursor] != '\0')
		{
			for(int cnt = 0 ; cnt < 3 ; cnt++)
			{
				if(cnt == 0)
					p1Index = atoi(str + cursor) + vertexOffset;
				else if(cnt == 1)
					p2Index = atoi(str + cursor) + vertexOffset;
				else
					p3Index = atoi(str + cursor) + vertexOffset;
				while(str[cursor] != ' ' && str[cursor] != '\t' && str[cursor] != '\n')
					cursor++; 
				while(str[cursor] == ' ' || str[cursor] == '\t' || str[cursor] == '\n')
					cursor++;
			}
			faces.push_back(*(new Triangle(-1, matIndex, p1Index, p2Index, p3Index, &vertices)));
			meshIndices->push_back(p1Index);
			meshIndices->push_back(p2Index);
			meshIndices->push_back(p3Index);
		}

		objects.push_back(new Mesh(id, matIndex, faces, meshIndices, &vertices));

		pObject = pObject->NextSiblingElement("Mesh");
	}

	// Parse lights
	int id;
	Vector3f position;
	Vector3f intensity;
	pElement = pRoot->FirstChildElement("Lights");

	XMLElement *pLight = pElement->FirstChildElement("AmbientLight");
	XMLElement *lightElement;
	str = pLight->GetText();
	sscanf(str, "%f %f %f", &ambientLight.r, &ambientLight.g, &ambientLight.b);

	pLight = pElement->FirstChildElement("PointLight");
	while(pLight != nullptr)
	{
		eResult = pLight->QueryIntAttribute("id", &id);
		lightElement = pLight->FirstChildElement("Position");
		str = lightElement->GetText();
		sscanf(str, "%f %f %f", &position.x, &position.y, &position.z);
		lightElement = pLight->FirstChildElement("Intensity");
		str = lightElement->GetText();
		sscanf(str, "%f %f %f", &intensity.r, &intensity.g, &intensity.b);

		lights.push_back(new PointLight(position, intensity));

		pLight = pLight->NextSiblingElement("PointLight");
	}
}


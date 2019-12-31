#include "Shape.h"
#include "Scene.h"
#include <cstdio>

#include "defs.h"

Shape::Shape(void)
{
}

Shape::Shape(int id, int matIndex)
    : id(id), matIndex(matIndex)
{
}

Sphere::Sphere(void)
{}

/* Constructor for sphere. */
Sphere::Sphere(int id, int matIndex, int cIndex, float R, vector<Vector3f> *pVertices)
    : Shape(id, matIndex)
{
    m_Radius = R;
    m_Center = (*pVertices)[cIndex - 1];
}

/* Sphere-ray intersection routine. */
ReturnVal Sphere::intersect(const Ray & ray) const
{
    Vector3f originToCenter = ray.origin - m_Center;

    float dirOtC = originToCenter.Dot(ray.direction); // correlation between ray direction and ray from center to origin
    float dd = ray.direction.Dot(ray.direction); // Direction ^ 2

    // Discriminant of the quadratic sphere equation
    float discriminant = dirOtC * dirOtC - dd * (originToCenter.Dot(originToCenter) - m_Radius * m_Radius); 

    // sqrt(< 0) -> nan
    if(discriminant < 0)
        return ReturnVal{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, nullptr};

    // Calculate closer intersection point
    discriminant = std::sqrt(discriminant);
    float t = -1 * dirOtC - discriminant;
    float ot = t / dd;

    if(ot > 0)
    {
        Vector3f point = ray.getPoint(ot); // intersection point
        Vector3f no = (point - m_Center) / m_Radius; // surface normal

        return ReturnVal{point, no, pScene->materials[matIndex - 1]};
    }

    return ReturnVal{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, nullptr};
}

Triangle::Triangle(void)
{}

/* Constructor for triangle. You will implement this. */
Triangle::Triangle(int id, int matIndex, int p1Index, int p2Index, int p3Index, vector<Vector3f> *pVertices)
    : Shape(id, matIndex)
{
    p0 = (*pVertices)[p1Index - 1];
    p1 = (*pVertices)[p2Index - 1];
    p2 = (*pVertices)[p3Index - 1];

    normal = (p1 - p0).Cross(p2 - p0);

    normal = normal.Normalize();
}

/* Triangle-ray intersection routine */
ReturnVal Triangle::intersect(const Ray & ray) const
{
    Vector3f r1 = {p0.x - p1.x, p0.x - p2.x, ray.direction.x};
    Vector3f r2 = {p0.y - p1.y, p0.y - p2.y, ray.direction.y};
    Vector3f r3 = {p0.z - p1.z, p0.z - p2.z, ray.direction.z}; 

    float detM = r1.x * (r2.y * r3.z - r2.z * r3.y) + r2.x * (r1.z * r3.y - r1.y * r3.z) + r3.x * (r1.y * r2.z - r1.z * r2.y);

    r1 = {p0.x - ray.origin.x, p0.x - p2.x, ray.direction.x};
    r2 = {p0.y - ray.origin.y, p0.y - p2.y, ray.direction.y};
    r3 = {p0.z - ray.origin.z, p0.z - p2.z, ray.direction.z};

    float det1 = r1.x * (r2.y * r3.z - r2.z * r3.y) + r2.x * (r1.z * r3.y - r1.y * r3.z) + r3.x * (r1.y * r2.z - r1.z * r2.y);

    r1 = {p0.x - p1.x, p0.x - ray.origin.x, ray.direction.x};
    r2 = {p0.y - p1.y, p0.y - ray.origin.y, ray.direction.y};
    r3 = {p0.z - p1.z, p0.z - ray.origin.z, ray.direction.z};

    float det2 = r1.x * (r2.y * r3.z - r2.z * r3.y) + r2.x * (r1.z * r3.y - r1.y * r3.z) + r3.x * (r1.y * r2.z - r1.z * r2.y);

    r1 = {p0.x - p1.x, p0.x - p2.x, p0.x - ray.origin.x};
    r2 = {p0.y - p1.y, p0.y - p2.y, p0.y - ray.origin.y};
    r3 = {p0.z - p1.z, p0.z - p2.z, p0.z - ray.origin.z};

    float det3 = r1.x * (r2.y * r3.z - r2.z * r3.y) + r2.x * (r1.z * r3.y - r1.y * r3.z) + r3.x * (r1.y * r2.z - r1.z * r2.y);

    float v1 = det1 / detM;
    float v2 = det2 / detM;
    float v3 = det3 / detM;

    if (v1 + v2 <= 1 + pScene->intTestEps && v1 >= 0 && v2 >= 0 && v3 >= 0)
    {
        Vector3f po = ray.getPoint(v3);

        return ReturnVal(po, normal, pScene->materials[matIndex - 1]);
    }
    return ReturnVal({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, nullptr);
}

Mesh::Mesh()
{}

/* Constructor for mesh */
Mesh::Mesh(int id, int matIndex, const vector<Triangle>& faces, vector<int> *pIndices, vector<Vector3f> *pVertices)
    : Shape(id, matIndex)
{

    for(int i = 0; i < faces.size(); ++i)
    {
        tris.push_back(Triangle(faces[i].id, faces[i].matIndex, (*pIndices)[i * 3 + 0], (*pIndices)[i * 3 + 1], (*pIndices)[i * 3 + 2], pVertices));
    }
}

/* Mesh-ray intersection routine */
ReturnVal Mesh::intersect(const Ray & ray) const
{
    ReturnVal rt{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, nullptr};
    float minDis = 1e9;

    for(int i = 0; i < tris.size(); ++i)
    {
        ReturnVal hass = tris[i].intersect(ray);

        if(!hass.mat)
            continue;

        float dis = (hass.ip - ray.origin).Length();
        if(dis < minDis)
        {
            minDis = dis;
            rt = hass;
        }
    }

    return rt;
}

#include "Camera.h"

#include <string.h>
#include <iostream>

#include <cmath>

Camera::Camera(int id,                      // Id of the camera
               const char* imageName,       // Name of the output PPM file 
               const Vector3f& pos,         // Camera position
               const Vector3f& gaze,        // Camera gaze direction
               const Vector3f& up,          // Camera up direction
               const ImagePlane& imgPlane)  // Image plane parameters
     :    imgPlane(imgPlane), m_Id(id), m_Pos(pos), m_Gaze(gaze), m_Up(up)

{
     strcpy(this->imageName, imageName);
     m_Gaze = m_Gaze.Normalize();
     m_Up = m_Up.Normalize();

     m_Right = (m_Gaze.Cross(m_Up)).Normalize();
}

/* Takes coordinate of an image pixel as row and col, and
 * returns the ray going through that pixel. 
 */
Ray Camera::getPrimaryRay(int row, int col) const
{
     float horizontalOffset = imgPlane.left + (col + 0.5f) * (imgPlane.right - imgPlane.left) / imgPlane.nx;
     float verticalOffset = imgPlane.top - (row + 0.5f) * (imgPlane.top - imgPlane.bottom) / imgPlane.ny;
     
     Vector3f rayDirection = m_Gaze * (imgPlane.distance) + m_Right * horizontalOffset + m_Up * verticalOffset;

     return Ray(m_Pos, rayDirection);
}


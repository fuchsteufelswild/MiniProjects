#include "KeyCommand.h"
#include "Camera.h"

#include "glm/glm/glm.hpp"

void MouseRotateHorizontalRightCommand::PerformAction(Camera& target)
{
	target.SetYaw(target.GetYaw() + 0.5f);
}

void MouseRotateVerticalUpCommand::PerformAction(Camera& target)
{
	target.SetPitch(target.GetPitch() + 0.5f);
}

void MouseRotateHorizontalLeftCommand::PerformAction(Camera& target)
{
	target.SetYaw(target.GetYaw() - 0.5f);
}

void MouseRotateVerticalDownCommand::PerformAction(Camera& target)
{
	target.SetPitch(target.GetPitch() - 0.5f);
}

void IncreaseHeightFactor::PerformAction(Camera& target)
{
	target.heightFactor += 0.5f;
}

void DecreaseHeightFactor::PerformAction(Camera& target)
{
	if (target.heightFactor <= 0.5)
	{
		target.heightFactor = 0;
		return;
	}
	target.heightFactor -= 0.5f;
}

void IncreaseHeightOffset::PerformAction(Camera& target)
{
	target.heightOffset += 1;
}

void DecreaseHeightOffset::PerformAction(Camera& target)
{
	target.heightOffset -= 1;
}

void MoveLightForward::PerformAction(Camera& target)
{
	target.lightPosition.z -= 5;
}

void MoveLightBackward::PerformAction(Camera& target)
{
	target.lightPosition.z += 5;
}

void MoveLightRight::PerformAction(Camera& target)
{
	target.lightPosition.x += 5;
}

void MoveLightLeft::PerformAction(Camera& target)
{
	target.lightPosition.x -= 5;
}

void MoveLightUp::PerformAction(Camera& target)
{
	target.lightPosition.y += 5;
}

void MoveLightDown::PerformAction(Camera& target)
{
	target.lightPosition.y -= 5;
}

void IncreaseCamSpeed::PerformAction(Camera& target)
{
	target.speed += 0.01f;
}

void DecreaseCamSpeed::PerformAction(Camera& target)
{
	target.speed -= 0.01f;
}
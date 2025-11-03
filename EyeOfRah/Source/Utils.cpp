//---------------------------------------------------------------------------------------
// file:	Utils.cpp
// author:	Leonard 
// email:	leonardjunyi.l@digipen.edu
//
// brief:	This file contains common utitlity functions to be used across the other files.
//
// Copyright � 2025 DigiPen, All rights reserved.
//---------------------------------------------------------------------------------------
#include "pch.hpp"
#include "Utils.hpp"

// MATH FUNCTIONS
int CustomCeil(float num) {
	int intPart = (int)num;
	if (num > intPart) {
		return intPart + 1;
	}
	else {
		return intPart;
	}
}

// CHECKER FUNCTIONS
bool IsAreaClicked(float area_center_x, float area_center_y, float area_width, float area_height, float click_x, float click_y)
{
	if ((click_x <= area_center_x + area_width / 2 && click_x >= area_center_x - area_width / 2) && (click_y <= area_center_y + area_height / 2 && click_y >= area_center_y - area_height / 2)) {
		return true;
	}
	else {
		return false;
	}
}

bool AreCirclesIntersecting(float c1_x, float c1_y, float r1, float c2_x, float c2_y, float r2)
{
	double d = (c1_x - c2_x) * (c1_x - c2_x) + (c1_y - c2_y) * (c1_y - c2_y);

	if (d <= (r2 - r1) * (r2 - r1)) {
		return true; // "Circle 1 is inside 2";
	}
	else if (d < (r1 + r2) * (r1 + r2)) {
		return true; // Circles are intersecting
	}
	else if (d == (r1 + r2) * (r1 + r2)) {
		return true;// Circles are touching
	}
	else {
		return false;
	}
}

bool AreRectsIntersecting(float r1_x, float r1_y, float r1_width, float r1_height, float r2_x, float r2_y, float r2_width, float r2_height) {
	// Check if there is an intersection
	if (std::abs(r1_x - r2_x) < ((r1_width * 0.5f) + (r2_width * 0.5f)) &&
		std::abs(r1_y - r2_y) < ((r1_height * 0.5f) + (r2_height * 0.5f))) {
		return true; // Rectangles are intersecting
	}
	else {
		return false; // Rectangles are not intersecting
	}
}

bool IsPointInRect(const AEVec2& point, const AEVec2& rectCenter, const AEVec2& rectScale) {
	// Used in the CombatPreparation scene to internally check if the mouse is over a button
    float halfWidth = rectScale.x * 0.5f;
    float halfHeight = rectScale.y * 0.5f;
    
    return (point.x >= rectCenter.x - halfWidth &&
            point.x <= rectCenter.x + halfWidth &&
            point.y >= rectCenter.y - halfHeight &&
            point.y <= rectCenter.y + halfHeight);
}

// Easing function for smoother animations
float EaseOutCubic(float t) {
	return 1.0f - powf(1.0f - t, 3.0f);
}

// Required for generic hover over collision check. - Lebon
bool CheckMouseHitGameObject(const Transform& objTranform) {
	return IsAreaClicked(objTranform.position.x, objTranform.position.y, objTranform.scale.x, objTranform.scale.y, ScreenToWorldPosition(mouseX, mouseY).x, ScreenToWorldPosition(mouseX, mouseY).y);
}
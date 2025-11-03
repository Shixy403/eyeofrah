//---------------------------------------------------------------------------------------
// file:	Utils.hpp
// author:	Leonard 
// email:	leonardjunyi.l@digipen.edu
//
// brief:	This file contains common utitlity functions to be used across the other files.
//
// Copyright � 2025 DigiPen, All rights reserved.
//---------------------------------------------------------------------------------------
#ifndef UTILS_HPP_
#define UTILS_HPP_

int CustomCeil(float num);

bool IsAreaClicked(float area_center_x, float area_center_y, float area_width, float area_height, float click_x, float click_y);

bool AreCirclesIntersecting(float c1_x, float c1_y, float r1, float c2_x, float c2_y, float r2);

bool AreRectsIntersecting(float r1_x, float r1_y, float r1_width, float r1_height, float r2_x, float r2_y, float r2_width, float r2_height);

bool IsPointInRect(const AEVec2& point, const AEVec2& rectCenter, const AEVec2& rectScale);

float EaseOutCubic(float t);

bool CheckMouseHitGameObject(const Transform& objTranform);

template <typename T>
void shuffle(T* arr, size_t size)
{
	for (size_t i = 0; i < size - 1; ++i)
	{
		T tmp = arr[i];
		size_t random_index = static_cast<size_t>(rand() % (size - 1 - i) + i);
		arr[i] = arr[random_index];
		arr[random_index] = tmp;
	}
}
#endif
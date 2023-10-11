// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

namespace AA::UnitConversions
{
	inline constexpr float MphToCms = 44.704f;
	inline constexpr float CmsToMph = 1 / MphToCms;

	inline constexpr float KphToCms = 27.77778f;
	inline constexpr float CmsToKph = 1 / KphToCms;

	inline constexpr float MphToKph = 1.609344f;
	inline constexpr float KphToMph = 1 / MphToKph;
}

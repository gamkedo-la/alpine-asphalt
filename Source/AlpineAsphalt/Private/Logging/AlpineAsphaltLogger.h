// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#if NO_LOGGING
DECLARE_LOG_CATEGORY_EXTERN(LogAlpineAsphalt, NoLogging, NoLogging);
#elif UE_BUILD_SHIPPING
DECLARE_LOG_CATEGORY_EXTERN(LogAlpineAsphalt, Warning, Warning);
#else
DECLARE_LOG_CATEGORY_EXTERN(LogAlpineAsphalt, Display, All);
#endif

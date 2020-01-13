// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "RHI/Public/RHIResources.h"
#include "RHI/Public/RHICommandList.h"

struct FSurfaceNormalPassConfig
{

};

class FSurfaceNormalPassRenderer
{

public:
	
	FSurfaceNormalPassRenderer();
	~FSurfaceNormalPassRenderer();	

private:

	FSurfaceNormalPassConfig   Config;
	bool                       bInitiated;
};

// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeTestEditor_init() {}
	static FPackageRegistrationInfo Z_Registration_Info_UPackage__Script_TestEditor;
	FORCENOINLINE UPackage* Z_Construct_UPackage__Script_TestEditor()
	{
		if (!Z_Registration_Info_UPackage__Script_TestEditor.OuterSingleton)
		{
			static const UECodeGen_Private::FPackageParams PackageParams = {
				"/Script/TestEditor",
				nullptr,
				0,
				PKG_CompiledIn | 0x00000040,
				0xE30F12F4,
				0xDEE027AE,
				METADATA_PARAMS(0, nullptr)
			};
			UECodeGen_Private::ConstructUPackage(Z_Registration_Info_UPackage__Script_TestEditor.OuterSingleton, PackageParams);
		}
		return Z_Registration_Info_UPackage__Script_TestEditor.OuterSingleton;
	}
	static FRegisterCompiledInInfo Z_CompiledInDeferPackage_UPackage__Script_TestEditor(Z_Construct_UPackage__Script_TestEditor, TEXT("/Script/TestEditor"), Z_Registration_Info_UPackage__Script_TestEditor, CONSTRUCT_RELOAD_VERSION_INFO(FPackageReloadVersionInfo, 0xE30F12F4, 0xDEE027AE));
PRAGMA_ENABLE_DEPRECATION_WARNINGS

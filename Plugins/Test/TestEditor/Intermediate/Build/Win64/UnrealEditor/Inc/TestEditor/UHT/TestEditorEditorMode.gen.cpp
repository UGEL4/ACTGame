// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TestEditorEditorMode.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

void EmptyLinkFunctionForGeneratedCodeTestEditorEditorMode() {}

// ********** Begin Cross Module References ********************************************************
TESTEDITOR_API UClass* Z_Construct_UClass_UTestEditorEditorMode();
TESTEDITOR_API UClass* Z_Construct_UClass_UTestEditorEditorMode_NoRegister();
UNREALED_API UClass* Z_Construct_UClass_UEdMode();
UPackage* Z_Construct_UPackage__Script_TestEditor();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UTestEditorEditorMode ****************************************************
void UTestEditorEditorMode::StaticRegisterNativesUTestEditorEditorMode()
{
}
FClassRegistrationInfo Z_Registration_Info_UClass_UTestEditorEditorMode;
UClass* UTestEditorEditorMode::GetPrivateStaticClass()
{
	using TClass = UTestEditorEditorMode;
	if (!Z_Registration_Info_UClass_UTestEditorEditorMode.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			StaticPackage(),
			TEXT("TestEditorEditorMode"),
			Z_Registration_Info_UClass_UTestEditorEditorMode.InnerSingleton,
			StaticRegisterNativesUTestEditorEditorMode,
			sizeof(TClass),
			alignof(TClass),
			TClass::StaticClassFlags,
			TClass::StaticClassCastFlags(),
			TClass::StaticConfigName(),
			(UClass::ClassConstructorType)InternalConstructor<TClass>,
			(UClass::ClassVTableHelperCtorCallerType)InternalVTableHelperCtorCaller<TClass>,
			UOBJECT_CPPCLASS_STATICFUNCTIONS_FORCLASS(TClass),
			&TClass::Super::StaticClass,
			&TClass::WithinClass::StaticClass
		);
	}
	return Z_Registration_Info_UClass_UTestEditorEditorMode.InnerSingleton;
}
UClass* Z_Construct_UClass_UTestEditorEditorMode_NoRegister()
{
	return UTestEditorEditorMode::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UTestEditorEditorMode_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * This class provides an example of how to extend a UEdMode to add some simple tools\n * using the InteractiveTools framework. The various UEdMode input event handlers (see UEdMode.h)\n * forward events to a UEdModeInteractiveToolsContext instance, which\n * has all the logic for interacting with the InputRouter, ToolManager, etc.\n * The functions provided here are the minimum to get started inserting some custom behavior.\n * Take a look at the UEdMode markup for more extensibility options.\n */" },
#endif
		{ "IncludePath", "TestEditorEditorMode.h" },
		{ "ModuleRelativePath", "Public/TestEditorEditorMode.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "This class provides an example of how to extend a UEdMode to add some simple tools\nusing the InteractiveTools framework. The various UEdMode input event handlers (see UEdMode.h)\nforward events to a UEdModeInteractiveToolsContext instance, which\nhas all the logic for interacting with the InputRouter, ToolManager, etc.\nThe functions provided here are the minimum to get started inserting some custom behavior.\nTake a look at the UEdMode markup for more extensibility options." },
#endif
	};
#endif // WITH_METADATA
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTestEditorEditorMode>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
UObject* (*const Z_Construct_UClass_UTestEditorEditorMode_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UEdMode,
	(UObject* (*)())Z_Construct_UPackage__Script_TestEditor,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTestEditorEditorMode_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UTestEditorEditorMode_Statics::ClassParams = {
	&UTestEditorEditorMode::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	0,
	0,
	0x000000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTestEditorEditorMode_Statics::Class_MetaDataParams), Z_Construct_UClass_UTestEditorEditorMode_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UTestEditorEditorMode()
{
	if (!Z_Registration_Info_UClass_UTestEditorEditorMode.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTestEditorEditorMode.OuterSingleton, Z_Construct_UClass_UTestEditorEditorMode_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UTestEditorEditorMode.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UTestEditorEditorMode);
// ********** End Class UTestEditorEditorMode ******************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Projects_ACTGame_Plugins_Test_TestEditor_Source_TestEditor_Public_TestEditorEditorMode_h__Script_TestEditor_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UTestEditorEditorMode, UTestEditorEditorMode::StaticClass, TEXT("UTestEditorEditorMode"), &Z_Registration_Info_UClass_UTestEditorEditorMode, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTestEditorEditorMode), 722148568U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Projects_ACTGame_Plugins_Test_TestEditor_Source_TestEditor_Public_TestEditorEditorMode_h__Script_TestEditor_692294135(TEXT("/Script/TestEditor"),
	Z_CompiledInDeferFile_FID_Projects_ACTGame_Plugins_Test_TestEditor_Source_TestEditor_Public_TestEditorEditorMode_h__Script_TestEditor_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Projects_ACTGame_Plugins_Test_TestEditor_Source_TestEditor_Public_TestEditorEditorMode_h__Script_TestEditor_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS

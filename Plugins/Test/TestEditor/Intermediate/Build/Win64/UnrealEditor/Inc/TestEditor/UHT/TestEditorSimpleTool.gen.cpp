// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Tools/TestEditorSimpleTool.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

void EmptyLinkFunctionForGeneratedCodeTestEditorSimpleTool() {}

// ********** Begin Cross Module References ********************************************************
INTERACTIVETOOLSFRAMEWORK_API UClass* Z_Construct_UClass_UInteractiveToolBuilder();
INTERACTIVETOOLSFRAMEWORK_API UClass* Z_Construct_UClass_UInteractiveToolPropertySet();
INTERACTIVETOOLSFRAMEWORK_API UClass* Z_Construct_UClass_USingleClickTool();
TESTEDITOR_API UClass* Z_Construct_UClass_UTestEditorSimpleTool();
TESTEDITOR_API UClass* Z_Construct_UClass_UTestEditorSimpleTool_NoRegister();
TESTEDITOR_API UClass* Z_Construct_UClass_UTestEditorSimpleToolBuilder();
TESTEDITOR_API UClass* Z_Construct_UClass_UTestEditorSimpleToolBuilder_NoRegister();
TESTEDITOR_API UClass* Z_Construct_UClass_UTestEditorSimpleToolProperties();
TESTEDITOR_API UClass* Z_Construct_UClass_UTestEditorSimpleToolProperties_NoRegister();
UPackage* Z_Construct_UPackage__Script_TestEditor();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UTestEditorSimpleToolBuilder *********************************************
void UTestEditorSimpleToolBuilder::StaticRegisterNativesUTestEditorSimpleToolBuilder()
{
}
FClassRegistrationInfo Z_Registration_Info_UClass_UTestEditorSimpleToolBuilder;
UClass* UTestEditorSimpleToolBuilder::GetPrivateStaticClass()
{
	using TClass = UTestEditorSimpleToolBuilder;
	if (!Z_Registration_Info_UClass_UTestEditorSimpleToolBuilder.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			StaticPackage(),
			TEXT("TestEditorSimpleToolBuilder"),
			Z_Registration_Info_UClass_UTestEditorSimpleToolBuilder.InnerSingleton,
			StaticRegisterNativesUTestEditorSimpleToolBuilder,
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
	return Z_Registration_Info_UClass_UTestEditorSimpleToolBuilder.InnerSingleton;
}
UClass* Z_Construct_UClass_UTestEditorSimpleToolBuilder_NoRegister()
{
	return UTestEditorSimpleToolBuilder::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UTestEditorSimpleToolBuilder_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Builder for UTestEditorSimpleTool\n */" },
#endif
		{ "IncludePath", "Tools/TestEditorSimpleTool.h" },
		{ "ModuleRelativePath", "Private/Tools/TestEditorSimpleTool.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Builder for UTestEditorSimpleTool" },
#endif
	};
#endif // WITH_METADATA
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTestEditorSimpleToolBuilder>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
UObject* (*const Z_Construct_UClass_UTestEditorSimpleToolBuilder_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UInteractiveToolBuilder,
	(UObject* (*)())Z_Construct_UPackage__Script_TestEditor,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTestEditorSimpleToolBuilder_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UTestEditorSimpleToolBuilder_Statics::ClassParams = {
	&UTestEditorSimpleToolBuilder::StaticClass,
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
	0x001000A8u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTestEditorSimpleToolBuilder_Statics::Class_MetaDataParams), Z_Construct_UClass_UTestEditorSimpleToolBuilder_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UTestEditorSimpleToolBuilder()
{
	if (!Z_Registration_Info_UClass_UTestEditorSimpleToolBuilder.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTestEditorSimpleToolBuilder.OuterSingleton, Z_Construct_UClass_UTestEditorSimpleToolBuilder_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UTestEditorSimpleToolBuilder.OuterSingleton;
}
UTestEditorSimpleToolBuilder::UTestEditorSimpleToolBuilder(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UTestEditorSimpleToolBuilder);
UTestEditorSimpleToolBuilder::~UTestEditorSimpleToolBuilder() {}
// ********** End Class UTestEditorSimpleToolBuilder ***********************************************

// ********** Begin Class UTestEditorSimpleToolProperties ******************************************
void UTestEditorSimpleToolProperties::StaticRegisterNativesUTestEditorSimpleToolProperties()
{
}
FClassRegistrationInfo Z_Registration_Info_UClass_UTestEditorSimpleToolProperties;
UClass* UTestEditorSimpleToolProperties::GetPrivateStaticClass()
{
	using TClass = UTestEditorSimpleToolProperties;
	if (!Z_Registration_Info_UClass_UTestEditorSimpleToolProperties.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			StaticPackage(),
			TEXT("TestEditorSimpleToolProperties"),
			Z_Registration_Info_UClass_UTestEditorSimpleToolProperties.InnerSingleton,
			StaticRegisterNativesUTestEditorSimpleToolProperties,
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
	return Z_Registration_Info_UClass_UTestEditorSimpleToolProperties.InnerSingleton;
}
UClass* Z_Construct_UClass_UTestEditorSimpleToolProperties_NoRegister()
{
	return UTestEditorSimpleToolProperties::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UTestEditorSimpleToolProperties_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Settings UObject for UTestEditorSimpleTool. This UClass inherits from UInteractiveToolPropertySet,\n * which provides an OnModified delegate that the Tool will listen to for changes in property values.\n */" },
#endif
		{ "IncludePath", "Tools/TestEditorSimpleTool.h" },
		{ "ModuleRelativePath", "Private/Tools/TestEditorSimpleTool.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Settings UObject for UTestEditorSimpleTool. This UClass inherits from UInteractiveToolPropertySet,\nwhich provides an OnModified delegate that the Tool will listen to for changes in property values." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ShowExtendedInfo_MetaData[] = {
		{ "Category", "Options" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** If enabled, dialog should display extended information about the actor clicked on. Otherwise, only basic info will be shown. */" },
#endif
		{ "DisplayName", "Show Extended Info" },
		{ "ModuleRelativePath", "Private/Tools/TestEditorSimpleTool.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "If enabled, dialog should display extended information about the actor clicked on. Otherwise, only basic info will be shown." },
#endif
	};
#endif // WITH_METADATA
	static void NewProp_ShowExtendedInfo_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ShowExtendedInfo;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTestEditorSimpleToolProperties>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
void Z_Construct_UClass_UTestEditorSimpleToolProperties_Statics::NewProp_ShowExtendedInfo_SetBit(void* Obj)
{
	((UTestEditorSimpleToolProperties*)Obj)->ShowExtendedInfo = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_UTestEditorSimpleToolProperties_Statics::NewProp_ShowExtendedInfo = { "ShowExtendedInfo", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(UTestEditorSimpleToolProperties), &Z_Construct_UClass_UTestEditorSimpleToolProperties_Statics::NewProp_ShowExtendedInfo_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ShowExtendedInfo_MetaData), NewProp_ShowExtendedInfo_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UTestEditorSimpleToolProperties_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTestEditorSimpleToolProperties_Statics::NewProp_ShowExtendedInfo,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTestEditorSimpleToolProperties_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UTestEditorSimpleToolProperties_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UInteractiveToolPropertySet,
	(UObject* (*)())Z_Construct_UPackage__Script_TestEditor,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTestEditorSimpleToolProperties_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UTestEditorSimpleToolProperties_Statics::ClassParams = {
	&UTestEditorSimpleToolProperties::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UTestEditorSimpleToolProperties_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UTestEditorSimpleToolProperties_Statics::PropPointers),
	0,
	0x001000A8u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTestEditorSimpleToolProperties_Statics::Class_MetaDataParams), Z_Construct_UClass_UTestEditorSimpleToolProperties_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UTestEditorSimpleToolProperties()
{
	if (!Z_Registration_Info_UClass_UTestEditorSimpleToolProperties.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTestEditorSimpleToolProperties.OuterSingleton, Z_Construct_UClass_UTestEditorSimpleToolProperties_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UTestEditorSimpleToolProperties.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UTestEditorSimpleToolProperties);
UTestEditorSimpleToolProperties::~UTestEditorSimpleToolProperties() {}
// ********** End Class UTestEditorSimpleToolProperties ********************************************

// ********** Begin Class UTestEditorSimpleTool ****************************************************
void UTestEditorSimpleTool::StaticRegisterNativesUTestEditorSimpleTool()
{
}
FClassRegistrationInfo Z_Registration_Info_UClass_UTestEditorSimpleTool;
UClass* UTestEditorSimpleTool::GetPrivateStaticClass()
{
	using TClass = UTestEditorSimpleTool;
	if (!Z_Registration_Info_UClass_UTestEditorSimpleTool.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			StaticPackage(),
			TEXT("TestEditorSimpleTool"),
			Z_Registration_Info_UClass_UTestEditorSimpleTool.InnerSingleton,
			StaticRegisterNativesUTestEditorSimpleTool,
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
	return Z_Registration_Info_UClass_UTestEditorSimpleTool.InnerSingleton;
}
UClass* Z_Construct_UClass_UTestEditorSimpleTool_NoRegister()
{
	return UTestEditorSimpleTool::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UTestEditorSimpleTool_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * UTestEditorSimpleTool is an example Tool that opens a message box displaying info about an actor that the user\n * clicks left mouse button. All the action is in the ::OnClicked handler.\n */" },
#endif
		{ "IncludePath", "Tools/TestEditorSimpleTool.h" },
		{ "ModuleRelativePath", "Private/Tools/TestEditorSimpleTool.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "UTestEditorSimpleTool is an example Tool that opens a message box displaying info about an actor that the user\nclicks left mouse button. All the action is in the ::OnClicked handler." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Properties_MetaData[] = {
		{ "ModuleRelativePath", "Private/Tools/TestEditorSimpleTool.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Properties;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTestEditorSimpleTool>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UTestEditorSimpleTool_Statics::NewProp_Properties = { "Properties", nullptr, (EPropertyFlags)0x0124080000000000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTestEditorSimpleTool, Properties), Z_Construct_UClass_UTestEditorSimpleToolProperties_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Properties_MetaData), NewProp_Properties_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UTestEditorSimpleTool_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTestEditorSimpleTool_Statics::NewProp_Properties,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTestEditorSimpleTool_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UTestEditorSimpleTool_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_USingleClickTool,
	(UObject* (*)())Z_Construct_UPackage__Script_TestEditor,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTestEditorSimpleTool_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UTestEditorSimpleTool_Statics::ClassParams = {
	&UTestEditorSimpleTool::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UTestEditorSimpleTool_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UTestEditorSimpleTool_Statics::PropPointers),
	0,
	0x001000A8u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTestEditorSimpleTool_Statics::Class_MetaDataParams), Z_Construct_UClass_UTestEditorSimpleTool_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UTestEditorSimpleTool()
{
	if (!Z_Registration_Info_UClass_UTestEditorSimpleTool.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTestEditorSimpleTool.OuterSingleton, Z_Construct_UClass_UTestEditorSimpleTool_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UTestEditorSimpleTool.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UTestEditorSimpleTool);
UTestEditorSimpleTool::~UTestEditorSimpleTool() {}
// ********** End Class UTestEditorSimpleTool ******************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Projects_ACTGame_Plugins_Test_TestEditor_Source_TestEditor_Private_Tools_TestEditorSimpleTool_h__Script_TestEditor_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UTestEditorSimpleToolBuilder, UTestEditorSimpleToolBuilder::StaticClass, TEXT("UTestEditorSimpleToolBuilder"), &Z_Registration_Info_UClass_UTestEditorSimpleToolBuilder, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTestEditorSimpleToolBuilder), 2307978542U) },
		{ Z_Construct_UClass_UTestEditorSimpleToolProperties, UTestEditorSimpleToolProperties::StaticClass, TEXT("UTestEditorSimpleToolProperties"), &Z_Registration_Info_UClass_UTestEditorSimpleToolProperties, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTestEditorSimpleToolProperties), 2839394415U) },
		{ Z_Construct_UClass_UTestEditorSimpleTool, UTestEditorSimpleTool::StaticClass, TEXT("UTestEditorSimpleTool"), &Z_Registration_Info_UClass_UTestEditorSimpleTool, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTestEditorSimpleTool), 3826127688U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Projects_ACTGame_Plugins_Test_TestEditor_Source_TestEditor_Private_Tools_TestEditorSimpleTool_h__Script_TestEditor_2917072701(TEXT("/Script/TestEditor"),
	Z_CompiledInDeferFile_FID_Projects_ACTGame_Plugins_Test_TestEditor_Source_TestEditor_Private_Tools_TestEditorSimpleTool_h__Script_TestEditor_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Projects_ACTGame_Plugins_Test_TestEditor_Source_TestEditor_Private_Tools_TestEditorSimpleTool_h__Script_TestEditor_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS

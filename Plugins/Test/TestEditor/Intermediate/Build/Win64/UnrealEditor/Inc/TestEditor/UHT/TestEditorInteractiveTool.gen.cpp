// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Tools/TestEditorInteractiveTool.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

void EmptyLinkFunctionForGeneratedCodeTestEditorInteractiveTool() {}

// ********** Begin Cross Module References ********************************************************
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FVector();
INTERACTIVETOOLSFRAMEWORK_API UClass* Z_Construct_UClass_UInteractiveTool();
INTERACTIVETOOLSFRAMEWORK_API UClass* Z_Construct_UClass_UInteractiveToolBuilder();
INTERACTIVETOOLSFRAMEWORK_API UClass* Z_Construct_UClass_UInteractiveToolPropertySet();
TESTEDITOR_API UClass* Z_Construct_UClass_UTestEditorInteractiveTool();
TESTEDITOR_API UClass* Z_Construct_UClass_UTestEditorInteractiveTool_NoRegister();
TESTEDITOR_API UClass* Z_Construct_UClass_UTestEditorInteractiveToolBuilder();
TESTEDITOR_API UClass* Z_Construct_UClass_UTestEditorInteractiveToolBuilder_NoRegister();
TESTEDITOR_API UClass* Z_Construct_UClass_UTestEditorInteractiveToolProperties();
TESTEDITOR_API UClass* Z_Construct_UClass_UTestEditorInteractiveToolProperties_NoRegister();
UPackage* Z_Construct_UPackage__Script_TestEditor();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UTestEditorInteractiveToolBuilder ****************************************
void UTestEditorInteractiveToolBuilder::StaticRegisterNativesUTestEditorInteractiveToolBuilder()
{
}
FClassRegistrationInfo Z_Registration_Info_UClass_UTestEditorInteractiveToolBuilder;
UClass* UTestEditorInteractiveToolBuilder::GetPrivateStaticClass()
{
	using TClass = UTestEditorInteractiveToolBuilder;
	if (!Z_Registration_Info_UClass_UTestEditorInteractiveToolBuilder.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			StaticPackage(),
			TEXT("TestEditorInteractiveToolBuilder"),
			Z_Registration_Info_UClass_UTestEditorInteractiveToolBuilder.InnerSingleton,
			StaticRegisterNativesUTestEditorInteractiveToolBuilder,
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
	return Z_Registration_Info_UClass_UTestEditorInteractiveToolBuilder.InnerSingleton;
}
UClass* Z_Construct_UClass_UTestEditorInteractiveToolBuilder_NoRegister()
{
	return UTestEditorInteractiveToolBuilder::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UTestEditorInteractiveToolBuilder_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Builder for UTestEditorInteractiveTool\n */" },
#endif
		{ "IncludePath", "Tools/TestEditorInteractiveTool.h" },
		{ "ModuleRelativePath", "Private/Tools/TestEditorInteractiveTool.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Builder for UTestEditorInteractiveTool" },
#endif
	};
#endif // WITH_METADATA
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTestEditorInteractiveToolBuilder>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
UObject* (*const Z_Construct_UClass_UTestEditorInteractiveToolBuilder_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UInteractiveToolBuilder,
	(UObject* (*)())Z_Construct_UPackage__Script_TestEditor,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTestEditorInteractiveToolBuilder_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UTestEditorInteractiveToolBuilder_Statics::ClassParams = {
	&UTestEditorInteractiveToolBuilder::StaticClass,
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
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTestEditorInteractiveToolBuilder_Statics::Class_MetaDataParams), Z_Construct_UClass_UTestEditorInteractiveToolBuilder_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UTestEditorInteractiveToolBuilder()
{
	if (!Z_Registration_Info_UClass_UTestEditorInteractiveToolBuilder.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTestEditorInteractiveToolBuilder.OuterSingleton, Z_Construct_UClass_UTestEditorInteractiveToolBuilder_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UTestEditorInteractiveToolBuilder.OuterSingleton;
}
UTestEditorInteractiveToolBuilder::UTestEditorInteractiveToolBuilder(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UTestEditorInteractiveToolBuilder);
UTestEditorInteractiveToolBuilder::~UTestEditorInteractiveToolBuilder() {}
// ********** End Class UTestEditorInteractiveToolBuilder ******************************************

// ********** Begin Class UTestEditorInteractiveToolProperties *************************************
void UTestEditorInteractiveToolProperties::StaticRegisterNativesUTestEditorInteractiveToolProperties()
{
}
FClassRegistrationInfo Z_Registration_Info_UClass_UTestEditorInteractiveToolProperties;
UClass* UTestEditorInteractiveToolProperties::GetPrivateStaticClass()
{
	using TClass = UTestEditorInteractiveToolProperties;
	if (!Z_Registration_Info_UClass_UTestEditorInteractiveToolProperties.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			StaticPackage(),
			TEXT("TestEditorInteractiveToolProperties"),
			Z_Registration_Info_UClass_UTestEditorInteractiveToolProperties.InnerSingleton,
			StaticRegisterNativesUTestEditorInteractiveToolProperties,
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
	return Z_Registration_Info_UClass_UTestEditorInteractiveToolProperties.InnerSingleton;
}
UClass* Z_Construct_UClass_UTestEditorInteractiveToolProperties_NoRegister()
{
	return UTestEditorInteractiveToolProperties::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UTestEditorInteractiveToolProperties_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Property set for the UTestEditorInteractiveTool\n */" },
#endif
		{ "IncludePath", "Tools/TestEditorInteractiveTool.h" },
		{ "ModuleRelativePath", "Private/Tools/TestEditorInteractiveTool.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Property set for the UTestEditorInteractiveTool" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_StartPoint_MetaData[] = {
		{ "Category", "Options" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** First point of measurement */" },
#endif
		{ "ModuleRelativePath", "Private/Tools/TestEditorInteractiveTool.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "First point of measurement" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_EndPoint_MetaData[] = {
		{ "Category", "Options" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Second point of measurement */" },
#endif
		{ "ModuleRelativePath", "Private/Tools/TestEditorInteractiveTool.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Second point of measurement" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Distance_MetaData[] = {
		{ "Category", "Options" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Current distance measurement */" },
#endif
		{ "ModuleRelativePath", "Private/Tools/TestEditorInteractiveTool.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Current distance measurement" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStructPropertyParams NewProp_StartPoint;
	static const UECodeGen_Private::FStructPropertyParams NewProp_EndPoint;
	static const UECodeGen_Private::FDoublePropertyParams NewProp_Distance;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTestEditorInteractiveToolProperties>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UTestEditorInteractiveToolProperties_Statics::NewProp_StartPoint = { "StartPoint", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTestEditorInteractiveToolProperties, StartPoint), Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_StartPoint_MetaData), NewProp_StartPoint_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UTestEditorInteractiveToolProperties_Statics::NewProp_EndPoint = { "EndPoint", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTestEditorInteractiveToolProperties, EndPoint), Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_EndPoint_MetaData), NewProp_EndPoint_MetaData) };
const UECodeGen_Private::FDoublePropertyParams Z_Construct_UClass_UTestEditorInteractiveToolProperties_Statics::NewProp_Distance = { "Distance", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTestEditorInteractiveToolProperties, Distance), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Distance_MetaData), NewProp_Distance_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UTestEditorInteractiveToolProperties_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTestEditorInteractiveToolProperties_Statics::NewProp_StartPoint,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTestEditorInteractiveToolProperties_Statics::NewProp_EndPoint,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTestEditorInteractiveToolProperties_Statics::NewProp_Distance,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTestEditorInteractiveToolProperties_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UTestEditorInteractiveToolProperties_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UInteractiveToolPropertySet,
	(UObject* (*)())Z_Construct_UPackage__Script_TestEditor,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTestEditorInteractiveToolProperties_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UTestEditorInteractiveToolProperties_Statics::ClassParams = {
	&UTestEditorInteractiveToolProperties::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UTestEditorInteractiveToolProperties_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UTestEditorInteractiveToolProperties_Statics::PropPointers),
	0,
	0x001000A8u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTestEditorInteractiveToolProperties_Statics::Class_MetaDataParams), Z_Construct_UClass_UTestEditorInteractiveToolProperties_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UTestEditorInteractiveToolProperties()
{
	if (!Z_Registration_Info_UClass_UTestEditorInteractiveToolProperties.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTestEditorInteractiveToolProperties.OuterSingleton, Z_Construct_UClass_UTestEditorInteractiveToolProperties_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UTestEditorInteractiveToolProperties.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UTestEditorInteractiveToolProperties);
UTestEditorInteractiveToolProperties::~UTestEditorInteractiveToolProperties() {}
// ********** End Class UTestEditorInteractiveToolProperties ***************************************

// ********** Begin Class UTestEditorInteractiveTool ***********************************************
void UTestEditorInteractiveTool::StaticRegisterNativesUTestEditorInteractiveTool()
{
}
FClassRegistrationInfo Z_Registration_Info_UClass_UTestEditorInteractiveTool;
UClass* UTestEditorInteractiveTool::GetPrivateStaticClass()
{
	using TClass = UTestEditorInteractiveTool;
	if (!Z_Registration_Info_UClass_UTestEditorInteractiveTool.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			StaticPackage(),
			TEXT("TestEditorInteractiveTool"),
			Z_Registration_Info_UClass_UTestEditorInteractiveTool.InnerSingleton,
			StaticRegisterNativesUTestEditorInteractiveTool,
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
	return Z_Registration_Info_UClass_UTestEditorInteractiveTool.InnerSingleton;
}
UClass* Z_Construct_UClass_UTestEditorInteractiveTool_NoRegister()
{
	return UTestEditorInteractiveTool::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UTestEditorInteractiveTool_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * UTestEditorInteractiveTool is an example Tool that allows the user to measure the \n * distance between two points. The first point is set by click-dragging the mouse, and\n * the second point is set by shift-click-dragging the mouse.\n */" },
#endif
		{ "IncludePath", "Tools/TestEditorInteractiveTool.h" },
		{ "ModuleRelativePath", "Private/Tools/TestEditorInteractiveTool.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "UTestEditorInteractiveTool is an example Tool that allows the user to measure the\ndistance between two points. The first point is set by click-dragging the mouse, and\nthe second point is set by shift-click-dragging the mouse." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Properties_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Properties of the tool are stored here */" },
#endif
		{ "ModuleRelativePath", "Private/Tools/TestEditorInteractiveTool.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Properties of the tool are stored here" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Properties;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTestEditorInteractiveTool>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UTestEditorInteractiveTool_Statics::NewProp_Properties = { "Properties", nullptr, (EPropertyFlags)0x0124080000000000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTestEditorInteractiveTool, Properties), Z_Construct_UClass_UTestEditorInteractiveToolProperties_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Properties_MetaData), NewProp_Properties_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UTestEditorInteractiveTool_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTestEditorInteractiveTool_Statics::NewProp_Properties,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTestEditorInteractiveTool_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UTestEditorInteractiveTool_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UInteractiveTool,
	(UObject* (*)())Z_Construct_UPackage__Script_TestEditor,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTestEditorInteractiveTool_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UTestEditorInteractiveTool_Statics::ClassParams = {
	&UTestEditorInteractiveTool::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UTestEditorInteractiveTool_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UTestEditorInteractiveTool_Statics::PropPointers),
	0,
	0x001000A8u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTestEditorInteractiveTool_Statics::Class_MetaDataParams), Z_Construct_UClass_UTestEditorInteractiveTool_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UTestEditorInteractiveTool()
{
	if (!Z_Registration_Info_UClass_UTestEditorInteractiveTool.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTestEditorInteractiveTool.OuterSingleton, Z_Construct_UClass_UTestEditorInteractiveTool_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UTestEditorInteractiveTool.OuterSingleton;
}
UTestEditorInteractiveTool::UTestEditorInteractiveTool() {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UTestEditorInteractiveTool);
UTestEditorInteractiveTool::~UTestEditorInteractiveTool() {}
// ********** End Class UTestEditorInteractiveTool *************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Projects_ACTGame_Plugins_Test_TestEditor_Source_TestEditor_Private_Tools_TestEditorInteractiveTool_h__Script_TestEditor_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UTestEditorInteractiveToolBuilder, UTestEditorInteractiveToolBuilder::StaticClass, TEXT("UTestEditorInteractiveToolBuilder"), &Z_Registration_Info_UClass_UTestEditorInteractiveToolBuilder, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTestEditorInteractiveToolBuilder), 1947300833U) },
		{ Z_Construct_UClass_UTestEditorInteractiveToolProperties, UTestEditorInteractiveToolProperties::StaticClass, TEXT("UTestEditorInteractiveToolProperties"), &Z_Registration_Info_UClass_UTestEditorInteractiveToolProperties, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTestEditorInteractiveToolProperties), 1856344801U) },
		{ Z_Construct_UClass_UTestEditorInteractiveTool, UTestEditorInteractiveTool::StaticClass, TEXT("UTestEditorInteractiveTool"), &Z_Registration_Info_UClass_UTestEditorInteractiveTool, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTestEditorInteractiveTool), 4161491756U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Projects_ACTGame_Plugins_Test_TestEditor_Source_TestEditor_Private_Tools_TestEditorInteractiveTool_h__Script_TestEditor_747273387(TEXT("/Script/TestEditor"),
	Z_CompiledInDeferFile_FID_Projects_ACTGame_Plugins_Test_TestEditor_Source_TestEditor_Private_Tools_TestEditorInteractiveTool_h__Script_TestEditor_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Projects_ACTGame_Plugins_Test_TestEditor_Source_TestEditor_Private_Tools_TestEditorInteractiveTool_h__Script_TestEditor_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS

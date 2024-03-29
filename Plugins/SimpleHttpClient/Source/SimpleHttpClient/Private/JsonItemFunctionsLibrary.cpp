// Fill out your copyright notice in the Description page of Project Settings.

#include "JsonItemFunctionsLibrary.h"
#include "YnnkHttpTypes.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool UJsonItemFunctionsLibrary::JsonItemFromString(FString InJsonString, FJsonItem& JsonItem)
{
	JsonItem.FromString(InJsonString);
	return JsonItem.IsValid();
}

bool UJsonItemFunctionsLibrary::JsonItemFromFile(FString FileName, FJsonItem& JsonItem)
{
	FString InJsonString;
	FFileHelper::LoadFileToString(InJsonString, *FileName);
	JsonItem.FromString(InJsonString);
	return JsonItem.IsValid();
}

FString UJsonItemFunctionsLibrary::JsonItemToString(const FJsonItem& JsonItem)
{
	return JsonItem.AsString();
}

bool UJsonItemFunctionsLibrary::JsonSetFieldValue_String(UPARAM(Ref) FJsonItem& JsonItem, FString Path, FString Value)
{
	return JsonItem.SetStringValue(Path, Value);
}

bool UJsonItemFunctionsLibrary::JsonSetFieldValue_Numeric(UPARAM(Ref) FJsonItem& JsonItem, FString Path, float Value)
{
	return JsonItem.SetFloatValue(Path, Value);
}

bool UJsonItemFunctionsLibrary::JsonSetFieldValue_Bool(UPARAM(Ref) FJsonItem& JsonItem, FString Path, bool Value)
{
	return JsonItem.SetBooleanValue(Path, Value);
}

FString UJsonItemFunctionsLibrary::JsonGetFieldValue_String(const FJsonItem& JsonItem, FString Path)
{
	return JsonItem.GetStringValue(Path);
}

float UJsonItemFunctionsLibrary::JsonGetFieldValue_Numeric(const FJsonItem& JsonItem, FString Path)
{
	return JsonItem.GetFloatValue(Path);
}

bool UJsonItemFunctionsLibrary::JsonGetFieldValue_Bool(const FJsonItem& JsonItem, FString Path)
{
	return JsonItem.GetBooleanValue(Path);
}

FYnnkUrlParameter UJsonItemFunctionsLibrary::GetDefaultContentAppJson()
{
	return FYnnkUrlParameter("Content-Type", "application/json");
}

FYnnkUrlParameter UJsonItemFunctionsLibrary::GetDefaultContentMPEG()
{
	return FYnnkUrlParameter("Content-Type", "audio/mpeg");
}

FYnnkUrlParameter UJsonItemFunctionsLibrary::GetDefaultContentWav()
{
	return FYnnkUrlParameter("Content-Type", "audio/wav");
}

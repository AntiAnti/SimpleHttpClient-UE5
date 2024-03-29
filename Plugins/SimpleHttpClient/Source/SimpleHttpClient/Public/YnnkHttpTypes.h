// Copyright (c) YuriNK, 2023, All Rights Reserved.
// ykasczc@gmail.com

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/Guid.h"

#include "YnnkHttpTypes.generated.h"

UENUM(BlueprintType)
enum class ERequestMethod : uint8
{
	Get		UMETA(DisplayName = "GET"),
	Post	UMETA(DisplayName = "POST")
};

/**
* Http-request header parameters
*/
USTRUCT(BlueprintType, Category = "Simple HTTP Client")
struct FYnnkUrlParameter
{
	GENERATED_BODY()

	// Header parameter name
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eleven Header")
	FString Name;

	// Header parameter value
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eleven Header")
	FString Value;

	FYnnkUrlParameter() {};
	FYnnkUrlParameter(FString InKey, FString InValue)
		: Name(InKey), Value(InValue)
	{}
	FYnnkUrlParameter(FString InKey, int32 InValue)
		: Name(InKey), Value(FString::FromInt(InValue))
	{}
	FYnnkUrlParameter(FString InKey, float InValue)
		: Name(InKey)
	{
		FString s = FString::SanitizeFloat(InValue);
		s.ReplaceCharInline(',', '.');
		Value = s;
	}
};

/**
* Simple http-request with header and body
*/
USTRUCT(BlueprintType, Category = "Simple HTTP Client")
struct FYnnkHttpRequest
{
	GENERATED_BODY()

	// ID to recognize requests in a queue
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eleven Request")
	int32 Id = INDEX_NONE;

	// Request URL
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eleven Request")
	FString URL;

	// Get/Post method
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eleven Request")
	ERequestMethod Method = ERequestMethod::Post;

	// Header parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eleven Request")
	TArray<FYnnkUrlParameter> Parameters;

	// Request body (can be empty)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eleven Request")
	FString Body;

	FYnnkHttpRequest() {};
	FYnnkHttpRequest(FString InUrl, ERequestMethod InMethod)
		: URL(InUrl), Method(InMethod)
	{}
	FYnnkHttpRequest(FString InUrl, ERequestMethod InMethod, const FYnnkUrlParameter& Param1)
		: URL(InUrl), Method(InMethod)
	{
		Parameters.Add(Param1);
	}
	FYnnkHttpRequest(FString InUrl, ERequestMethod InMethod, const FYnnkUrlParameter& Param1, const FYnnkUrlParameter& Param2)
		: URL(InUrl), Method(InMethod)
	{
		Parameters.Add(Param1); Parameters.Add(Param2);
	}
	FYnnkHttpRequest(FString InUrl, ERequestMethod InMethod, const FYnnkUrlParameter& Param1, const FYnnkUrlParameter& Param2, const FYnnkUrlParameter& Param3)
		: URL(InUrl), Method(InMethod)
	{
		Parameters.Add(Param1); Parameters.Add(Param2); Parameters.Add(Param3);
	}

	bool IsValid() const
	{
		return (Id != INDEX_NONE);
	}
	void Invalidate()
	{
		Id = INDEX_NONE;
	}
};

/**
* Simple http-request with header and body
*/
USTRUCT(BlueprintType, Category = "Simple HTTP Client")
struct SIMPLEHTTPCLIENT_API FJsonItem
{
	GENERATED_BODY()

protected:

	UPROPERTY()
	FString Body;

	bool SetValue(FString Path, EJson Format, float ValueNumeric = 0.f, const FString& ValueStr = TEXT(""), bool bValue = false);
	bool GetValue(const FString& Path, EJson Format, float* ValueNumeric = nullptr, FString* ValueStr = nullptr, bool* bValue = nullptr) const;
	bool GetChildObjectByPath(TSharedPtr<FJsonObject>& InParentObjectField, TSharedPtr<FJsonObject>& OutChildObjectField, FString FieldNameAsPath) const;
	bool SetTargetValue(TSharedPtr<FJsonObject>& InParentObjectField, const FString& FieldName, TSharedPtr<FJsonValue>& NewValue) const;

public:

	FString AsString() const
	{
		return Body;
	}

	void FromString(const FString& InData)
	{
		Body = InData;
	}

	bool IsValid() const
	{
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
		TSharedPtr<FJsonObject> JsonResponse = MakeShareable(new FJsonObject);
		return FJsonSerializer::Deserialize(Reader, JsonResponse);
	}

	bool SetStringValue(const FString& Path, const FString& Value)
	{ return SetValue(Path, EJson::String, 0.f, Value); };
	bool SetFloatValue(const FString& Path, const float Value)
	{ return SetValue(Path, EJson::Number, Value); };
	bool SetBooleanValue(const FString& Path, const bool Value)
	{ return SetValue(Path, EJson::Boolean, 0.f, "", Value); };

	FString GetStringValue(const FString& Path) const;
	float GetFloatValue(const FString& Path) const;
	bool GetBooleanValue(const FString& Path) const;

};


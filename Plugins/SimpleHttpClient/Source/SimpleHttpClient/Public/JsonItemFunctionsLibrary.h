// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YnnkHttpTypes.h"
#include "JsonItemFunctionsLibrary.generated.h"

/**
 * 
 */
UCLASS()
class SIMPLEHTTPCLIENT_API UJsonItemFunctionsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, meta=(DisplayName="Json from String"), Category = "Json")
	static bool JsonItemFromString(FString InJsonString, FJsonItem& JsonItem);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Json from File"), Category = "Json")
	static bool JsonItemFromFile(FString FileName, FJsonItem& JsonItem);

	UFUNCTION(BlueprintPure, meta=(DisplayName="To String"), Category = "Json")
	static FString JsonItemToString(const FJsonItem& JsonItem);

	// Example: JsonSetFieldValue_String(InOutJsonItem, "messages.role", "users")
	// Example: JsonSetFieldValue_String(InOutJsonItem, "messages.content", "some prompt")
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set String Field"), Category = "Json")
	static bool JsonSetFieldValue_String(UPARAM(Ref) FJsonItem& JsonItem, FString Path, FString Value);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Numeric Field"), Category = "Json")
	static bool JsonSetFieldValue_Numeric(UPARAM(Ref) FJsonItem& JsonItem, FString Path, float Value);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Boolean Field"), Category = "Json")
	static bool JsonSetFieldValue_Bool(UPARAM(Ref) FJsonItem& JsonItem, FString Path, bool Value);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add Array Item (Object)"), Category = "Json")
	static bool JsonAddArrayItem(UPARAM(Ref) FJsonItem& JsonItem, FString Path, const FJsonItem& NewItem);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add Array Item (String)"), Category = "Json")
	static bool JsonAddArrayItemString(UPARAM(Ref) FJsonItem& JsonItem, FString Path, FString NewItem);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add Array Item (Numeric)"), Category = "Json")
	static bool JsonAddArrayItemNumeric(UPARAM(Ref) FJsonItem& JsonItem, FString Path, float NewItem);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add Array Item (Boolean)"), Category = "Json")
	static bool JsonAddArrayItemBool(UPARAM(Ref) FJsonItem& JsonItem, FString Path, bool NewItem);

	// Find valid json block wrapped with {} in some text and return it as JsonItem struct
	UFUNCTION(BlueprintPure, Category = "Json")
	static bool ExtractJsonBlockFromString(const FString& InText, int32 SearchStart, int32& OutStartPos, int32& OutBlockLen, FJsonItem& OutJson);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get String Field"), Category = "Json")
	static FString JsonGetFieldValue_String(const FJsonItem& JsonItem, FString Path);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Numeric Field"), Category = "Json")
	static float JsonGetFieldValue_Numeric(const FJsonItem& JsonItem, FString Path);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Boolean Field"), Category = "Json")
	static bool JsonGetFieldValue_Bool(const FJsonItem& JsonItem, FString Path);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Content-type: application/json"), Category = "Json")
	static FYnnkUrlParameter GetDefaultContentAppJson();

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Content-type: audio/mpeg"), Category = "Json")
	static FYnnkUrlParameter GetDefaultContentMPEG();

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Content-type: audio/wav"), Category = "Json")
	static FYnnkUrlParameter GetDefaultContentWav();

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Content-type: text/html"), Category = "Json")
	static FYnnkUrlParameter GetDefaultContentHtml();

	UFUNCTION(BlueprintPure, Category = "Json")
	static FString CleanJsonResponse(const FString& InText);

private:
};

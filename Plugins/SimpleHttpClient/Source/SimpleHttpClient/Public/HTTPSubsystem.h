// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "YnnkHttpTypes.h"
#include "Runtime/Launch/Resources/Version.h"
#include "HTTPSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FHttpResponseString, const FName&, RequestName, int32, Code, const TArray<FString>&, Headers, const FString&, Data);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FHttpResponseData, const FName&, RequestName, int32, Code, const TArray<FString>&, Headers, const TArray<uint8>&, Data);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHttpResponseError, const FName&, RequestName, int32, Code);

/**
* Http-request header parameters
*/
USTRUCT()
struct FYnnkNamedHttpRequest
{
	GENERATED_BODY()

	// Name
	UPROPERTY()
	FName Name;

	// Request pointer
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> HttpRequest;
};

/**
 * 
 */
UCLASS()
class SIMPLEHTTPCLIENT_API UHTTPSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
	
public:

	virtual void Deinitialize() override;

	UPROPERTY(BlueprintReadWrite, Category = "HTTP Sybsystem")
	float Timeout = 180.f;

	UPROPERTY(BlueprintAssignable, Category = "HTTP Sybsystem")
	FHttpResponseData OnDataResponse;

	UPROPERTY(BlueprintAssignable, Category = "HTTP Sybsystem")
	FHttpResponseString OnTextResponse;

	UPROPERTY(BlueprintAssignable, Category = "HTTP Sybsystem")
	FHttpResponseError OnResponseError;

	UFUNCTION(BlueprintCallable, meta=(DisplayName="HTTP Request"), Category = "HTTP Sybsystem")
	bool SendHttpRequest(FName Keyword, FString URL, ERequestMethod Verb, const TArray<FYnnkUrlParameter>& HeaderParams, const FString& Body);

protected:
	// Active requests
	TMap<int32, FYnnkNamedHttpRequest> HttpRequests;
	// Temp. buffer
	TArray<uint8> DataBuffer;

#if ENGINE_MINOR_VERSION > 2
	FHttpRequestStreamDelegate OnAudioChunkReceived;
#endif
	bool AudioChunkReceivedWrapper(void* Ptr, int64 Length);

	/**
	* Result of http-request with text (json) data
	*/
	UFUNCTION()
	void OnStringRequestCompleted(int32 ReqId, const FString Content, const TArray<FString>& Headers, const int32 ResponseCode, const bool bWasSuccessful);

	/**
	* Result of http-request with binary data
	*/
	UFUNCTION()
	void OnDataRequestCompleted(int32 ReqId, const TArray<uint8>& Content, const TArray<FString>& Headers, const int32 ResponseCode, const bool bWasSuccessful);

	void OnHTTPRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr RequestResponse, bool bWasSuccessful, int32 ReqId);
};

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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHttpStreamString, const FName&, RequestName, const FString&, Text);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHttpStreamData, const FName&, RequestName, const TArray<uint8>&, Data);

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

	// Expected response format
	UPROPERTY()
	EExpectedResponseType ResponseFormat = EExpectedResponseType::Default;

	// Request pointer
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> HttpRequest;

	TArray<uint8> TempBinaryData;
	FString TempStringData;

	bool FormatAudio() const
	{
		return ResponseFormat == EExpectedResponseType::StreamData || ResponseFormat == EExpectedResponseType::Data;
	}
	bool FormatText() const
	{
		return ResponseFormat == EExpectedResponseType::StreamText || ResponseFormat == EExpectedResponseType::Text;
	}
	bool FormatStream() const
	{
		return ResponseFormat == EExpectedResponseType::StreamText || ResponseFormat == EExpectedResponseType::StreamData;
	}
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

	UPROPERTY(BlueprintReadWrite, Category = "HTTP Subsystem")
	float Timeout = 180.f;

	UPROPERTY(BlueprintReadWrite, Category = "HTTP Subsystem")
	bool bResponseInGameThread = false;

	UPROPERTY(BlueprintAssignable, Category = "HTTP Subsystem")
	FHttpResponseData OnDataResponse;

	UPROPERTY(BlueprintAssignable, Category = "HTTP Subsystem")
	FHttpResponseString OnTextResponse;

	UPROPERTY(BlueprintAssignable, Category = "HTTP Subsystem")
	FHttpStreamString OnTextStreamResponse;

	UPROPERTY(BlueprintAssignable, Category = "HTTP Subsystem")
	FHttpStreamData OnDataStreamResponse;

	UPROPERTY(BlueprintAssignable, Category = "HTTP Subsystem")
	FHttpResponseError OnResponseError;

	UFUNCTION(BlueprintCallable, meta=(DisplayName="HTTP Request (Text)"), Category = "HTTP Subsystem")
	bool SendHttpRequest(FName Keyword, FString URL, ERequestMethod Verb, const TArray<FYnnkUrlParameter>& HeaderParams, const FString& BodyString, EExpectedResponseType ExpectedResponseFormat = EExpectedResponseType::Default);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "HTTP Request (Data)"), Category = "HTTP Subsystem")
	bool SendHttpRequestData(FName Keyword, FString URL, ERequestMethod Verb, const TArray<FYnnkUrlParameter>& HeaderParams, const TArray<uint8>& BodyData, EExpectedResponseType ExpectedResponseFormat = EExpectedResponseType::Default);

	bool SendHttpRequestInternal(FName Keyword, FString URL, ERequestMethod Verb, const TArray<FYnnkUrlParameter>& HeaderParams, const FString& BodyString, const TArray<uint8>& BodyData, EExpectedResponseType ExpectedResponseFormat);

protected:
	// Active requests
	TMap<int32, FYnnkNamedHttpRequest> HttpRequests;
	int32 LastRequest = INDEX_NONE;
	// Temp. buffer
	TArray<uint8> DataBuffer;

#if ENGINE_MINOR_VERSION > 4
	FHttpRequestStreamDelegateV2 OnStreamChunkReceived;
#elif ENGINE_MINOR_VERSION > 2
	FHttpRequestStreamDelegate OnStreamChunkReceived;
#endif


	// before 5.5
	bool StreamChunkReceivedWrapper(void* Ptr, int64 Length);
	// since 5.5
	void StreamChunkReceivedWrapperV2(void* Ptr, int64& InOutLength);

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

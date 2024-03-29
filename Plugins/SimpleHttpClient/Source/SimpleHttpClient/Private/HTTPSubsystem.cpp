// Fill out your copyright notice in the Description page of Project Settings.

#include "HTTPSubsystem.h"
#include "YnnkHttpTypes.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Runtime/Launch/Resources/Version.h"

bool UHTTPSubsystem::SendHttpRequest(FName Keyword, FString URL, ERequestMethod Verb, const TArray<FYnnkUrlParameter>& HeaderParams, const FString& Body)
{
	const FString SendMethod = (Verb == ERequestMethod::Get ? TEXT("GET") : TEXT("POST"));

	int32 RequestId = INDEX_NONE, MaxId = INDEX_NONE;
	for (const auto& Req : HttpRequests)
	{
		MaxId = FMath::Max(Req.Key, MaxId);
		if (!Req.Value.HttpRequest.IsValid())
		{
			RequestId = Req.Key;
			break;
		}
	}
	if (!HttpRequests.Contains(RequestId))
	{
		RequestId = MaxId + 1;
		HttpRequests.Add(RequestId);
	}

	HttpRequests[RequestId].Name = Keyword;

#if ENGINE_MINOR_VERSION > 2
	if (!OnAudioChunkReceived.IsBound())
	{
		OnAudioChunkReceived.BindUObject(this, &UHTTPSubsystem::AudioChunkReceivedWrapper);
	}
#endif

	auto& HttpRequest = HttpRequests[RequestId].HttpRequest;
	HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetTimeout(Timeout);
	HttpRequest->SetURL(URL);
	HttpRequest->SetVerb(SendMethod);

	bool bContentTypeSpecified = false;
	for (const auto& Param : HeaderParams)
	{
		if (Param.Name == TEXT("Content-Type"))
		{
			bContentTypeSpecified = true;
		}
		HttpRequest->SetHeader(Param.Name, Param.Value);
	}

	if (!bContentTypeSpecified)
	{
		HttpRequest->SetHeader("Content-Type", "application/json");
	}

	if (!Body.IsEmpty())
	{
		HttpRequest->SetContentAsString(Body);
	}

	HttpRequest->OnProcessRequestComplete().BindUObject(this, &UHTTPSubsystem::OnHTTPRequestComplete, RequestId);
	//HttpRequest->SetResponseBodyReceiveStreamDelegate(OnAudioChunkReceived);
	return HttpRequest->ProcessRequest();
}

void UHTTPSubsystem::Deinitialize()
{
	Super::Deinitialize();

	for (auto& Req : HttpRequests)
	{
		if (Req.Value.HttpRequest.IsValid())
		{
			Req.Value.HttpRequest->OnProcessRequestComplete().Unbind();
			//HttpRequest->OnRequestProgress().Unbind();
		}
	}
#if ENGINE_MINOR_VERSION > 2
	OnAudioChunkReceived.Unbind();
#endif
}

void UHTTPSubsystem::OnHTTPRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr RequestResponse, bool bWasSuccessful, int32 ReqId)
{
	if (!RequestResponse.IsValid())
	{
		return;
	}

	FString Type = RequestResponse->GetContentType();
	TArray<FString> Headers = RequestResponse->GetAllHeaders();
	UE_LOG(LogTemp, Log, TEXT("HTTP-request returned audio data of type: %s. Length = %d"), *Type, RequestResponse->GetContentLength());

	if (Type.Contains(TEXT("audio/")))
	{
		OnDataRequestCompleted(ReqId, RequestResponse->GetContent(), Headers, RequestResponse->GetResponseCode(), bWasSuccessful);
	}
	else if (Type.Contains(TEXT("application/json")))
	{
		UE_LOG(LogTemp, Log, TEXT("%s"), *RequestResponse->GetContentAsString());
		OnStringRequestCompleted(ReqId, RequestResponse->GetContentAsString(), Headers, RequestResponse->GetResponseCode(), bWasSuccessful);
	}
	else
	{
		OnStringRequestCompleted(ReqId, RequestResponse->GetContentAsString(), Headers, RequestResponse->GetResponseCode(), bWasSuccessful);
	}

	if (EHttpRequestStatus::IsFinished(Request->GetStatus()))
	{
		if (HttpRequests.Contains(ReqId))
		{
			HttpRequests[ReqId].HttpRequest->OnProcessRequestComplete().Unbind();
			HttpRequests[ReqId].HttpRequest = nullptr;
		}
	}
}

bool UHTTPSubsystem::AudioChunkReceivedWrapper(void* Ptr, int64 Length)
{
	UE_LOG(LogTemp, Log, TEXT("AudioChunkReceivedWrapper: %d bytes received"), Length);
	return (Ptr != NULL && Length > 0);
}

void UHTTPSubsystem::OnStringRequestCompleted(int32 ReqId, const FString Content, const TArray<FString>& Headers, const int32 ResponseCode, const bool bWasSuccessful)
{
	const FName Keyword = HttpRequests.Contains(ReqId) ? HttpRequests[ReqId].Name : TEXT("Default");

	if (bWasSuccessful)
	{
		//UE_LOG(LogTemp, Log, TEXT("OnTextResponse(%s)"), *Content);
		OnTextResponse.Broadcast(Keyword, ResponseCode, Headers, Content);
	}
	else
	{
		OnResponseError.Broadcast(Keyword, ResponseCode);
	}
}

void UHTTPSubsystem::OnDataRequestCompleted(int32 ReqId, const TArray<uint8>& Content, const TArray<FString>& Headers, const int32 ResponseCode, const bool bWasSuccessful)
{
	const FName Keyword = HttpRequests.Contains(ReqId) ? HttpRequests[ReqId].Name : TEXT("Default");

	if (bWasSuccessful)
	{
		DataBuffer.SetNumUninitialized(Content.Num());
		FMemory::Memcpy(DataBuffer.GetData(), Content.GetData(), DataBuffer.Num());
		OnDataResponse.Broadcast(Keyword, ResponseCode, Headers, DataBuffer);
	}
	else
	{
		OnResponseError.Broadcast(Keyword, ResponseCode);
	}
}

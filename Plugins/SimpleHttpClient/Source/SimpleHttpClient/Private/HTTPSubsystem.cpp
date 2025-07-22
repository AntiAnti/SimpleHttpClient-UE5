// Fill out your copyright notice in the Description page of Project Settings.

#include "HTTPSubsystem.h"
#include "YnnkHttpTypes.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Misc/FileHelper.h"
#include "JsonItemFunctionsLibrary.h"
//#include "Async/Future.h"
#include "Async/Async.h"

bool UHTTPSubsystem::SendHttpRequestInternal(FName Keyword, FString URL, ERequestMethod Verb, const TArray<FYnnkUrlParameter>& HeaderParams, const FString& BodyString, const TArray<uint8>& BodyData, EExpectedResponseType ExpectedResponseFormat)
{
	const FString SendMethod = (Verb == ERequestMethod::Get ? TEXT("GET") : TEXT("POST"));

	int32 RequestId = INDEX_NONE, MaxId = INDEX_NONE;
	for (auto& Req : HttpRequests)
	{
		if (!Req.Value.HttpRequest.IsValid())
		{
			Req.Value.TempBinaryData.Empty();
		}
	}
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
	HttpRequests[RequestId].ResponseFormat = ExpectedResponseFormat;

	if (!OnStreamChunkReceived.IsBound())
	{
#if ENGINE_MINOR_VERSION > 4
		OnStreamChunkReceived.BindUObject(this, &UHTTPSubsystem::StreamChunkReceivedWrapperV2);
#elif ENGINE_MINOR_VERSION > 2
		OnStreamChunkReceived.BindUObject(this, &UHTTPSubsystem::StreamChunkReceivedWrapper);
#endif
	}

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

	bool bContentIsString = BodyString != TEXT("!!NULL") && !BodyString.IsEmpty();

	if (!bContentTypeSpecified)
	{
		if (bContentIsString)
		{
			HttpRequest->SetHeader("Content-Type", "application/json");
		}
		else
		{
			HttpRequest->SetHeader("Content-Type", "audio/wav");
		}
	}

	if (bContentIsString)
	{
		HttpRequest->SetContentAsString(BodyString);
	}
	else
	{
		HttpRequest->SetContent(BodyData);
	}

	HttpRequest->OnProcessRequestComplete().BindUObject(this, &UHTTPSubsystem::OnHTTPRequestComplete, RequestId);
	if (HttpRequests[RequestId].FormatStream())
	{
#if ENGINE_MINOR_VERSION > 4
		HttpRequest->SetResponseBodyReceiveStreamDelegateV2(OnStreamChunkReceived);
#elif ENGINE_MINOR_VERSION > 2
		HttpRequest->SetResponseBodyReceiveStreamDelegate(OnStreamChunkReceived);
#endif
		LastRequest = RequestId;
	}
	return HttpRequest->ProcessRequest();
}

bool UHTTPSubsystem::SendHttpRequest(FName Keyword, FString URL, ERequestMethod Verb, const TArray<FYnnkUrlParameter>& HeaderParams, const FString& BodyString, EExpectedResponseType ExpectedResponseFormat)
{
	return SendHttpRequestInternal(Keyword, URL, Verb, HeaderParams, BodyString, {}, ExpectedResponseFormat);
}

bool UHTTPSubsystem::SendHttpRequestData(FName Keyword, FString URL, ERequestMethod Verb, const TArray<FYnnkUrlParameter>& HeaderParams, const TArray<uint8>& BodyData, EExpectedResponseType ExpectedResponseFormat)
{
	return SendHttpRequestInternal(Keyword, URL, Verb, HeaderParams, TEXT("!!NULL"), BodyData, ExpectedResponseFormat);
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
	OnStreamChunkReceived.Unbind();
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

	if (Type.Contains(TEXT("audio/")) || HttpRequests[ReqId].FormatAudio())
	{
		UE_LOG(LogTemp, Log, TEXT("HTTP-request returned data of type: %s (binary). Length = %d"), *Type, RequestResponse->GetContentLength());
		OnDataRequestCompleted(ReqId, RequestResponse->GetContent(), Headers, RequestResponse->GetResponseCode(), bWasSuccessful);
	}
	else if (Type.Contains(TEXT("application/json")) || Type.StartsWith("text/") || HttpRequests[ReqId].FormatText())
	{
		UE_LOG(LogTemp, Log, TEXT("HTTP-request returned data of type: %s (text). Length = %d"), *Type, RequestResponse->GetContentLength());
		StringBuffer = RequestResponse->GetContentAsString();

		UE_LOG(LogTemp, Log, TEXT("To remove: [%s]"), *StringBuffer);
		OnStringRequestCompleted(ReqId, RequestResponse->GetContentAsString(), Headers, RequestResponse->GetResponseCode(), bWasSuccessful);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("HTTP-request returned data of type: %s (unknown). Length = %d"), *Type, RequestResponse->GetContentLength());
		OnStringRequestCompleted(ReqId, RequestResponse->GetContentAsString(), Headers, RequestResponse->GetResponseCode(), bWasSuccessful);
	}

	if (EHttpRequestStatus::IsFinished(Request->GetStatus()))
	{
		if (HttpRequests.Contains(ReqId))
		{
			HttpRequests[ReqId].HttpRequest->OnProcessRequestComplete().Unbind();
			HttpRequests[ReqId].HttpRequest = nullptr;
			LastRequest = INDEX_NONE;
		}
	}
}

bool UHTTPSubsystem::StreamChunkReceivedWrapper(void* Ptr, int64 Length)
{
	if (HttpRequests.Contains(LastRequest) && Length > 0)
	{
		auto& Req = HttpRequests[LastRequest];

		Req.TempBinaryData.SetNum(Length);
		FMemory::Memcpy(Req.TempBinaryData.GetData(), Ptr, Length);

		if (Req.FormatText())
		{
			FFileHelper::BufferToString(Req.TempStringData, Req.TempBinaryData.GetData(), Length);
			Req.TempStringData = UJsonItemFunctionsLibrary::CleanJsonResponse(Req.TempStringData);

			if (bResponseInGameThread && !IsInGameThread())
			{
				AsyncTask(ENamedThreads::GameThread, [this, ReqId = LastRequest, strDat = Req.TempStringData]()
				{
					if (HttpRequests.Contains(ReqId))
					{
						OnTextStreamResponse.Broadcast(HttpRequests[ReqId].Name, HttpRequests[ReqId].TempStringData);
					}
					else
					{
						OnTextStreamResponse.Broadcast(TEXT("Default"), strDat);
					}
				});
			}
			else
			{
				OnTextStreamResponse.Broadcast(Req.Name, Req.TempStringData);
			}
		}
		else
		{
			if (bResponseInGameThread && !IsInGameThread())
			{
				AsyncTask(ENamedThreads::GameThread, [this, ReqId = LastRequest]()
				{
					if (HttpRequests.Contains(ReqId))
					{
						OnDataStreamResponse.Broadcast(HttpRequests[ReqId].Name, HttpRequests[ReqId].TempBinaryData);
					}
					else
					{
						OnDataStreamResponse.Broadcast(TEXT("Default_ERROR"), {});
					}
				});
			}
			else
			{
				OnDataStreamResponse.Broadcast(Req.Name, Req.TempBinaryData);
			}
		}
	}

	return (Ptr != NULL && Length > 0);
}

void UHTTPSubsystem::StreamChunkReceivedWrapperV2(void* Ptr, int64& InOutLength)
{
	StreamChunkReceivedWrapper(Ptr, InOutLength);
}

void UHTTPSubsystem::OnStringRequestCompleted(int32 ReqId, const FString Content, const TArray<FString>& Headers, const int32 ResponseCode, const bool bWasSuccessful)
{
	const FName Keyword = HttpRequests.Contains(ReqId) ? HttpRequests[ReqId].Name : TEXT("Default");

	if (bWasSuccessful)
	{
		if (bResponseInGameThread && !IsInGameThread())
		{
			StringBuffer = Content;
			AsyncTask(ENamedThreads::GameThread, [this, Keyword, ResponseCode, Headers]()
			{
				OnTextResponse.Broadcast(Keyword, ResponseCode, Headers, StringBuffer);
			});
		}
		else
		{
			OnTextResponse.Broadcast(Keyword, ResponseCode, Headers, Content);
		}
	}
	else
	{
		if (bResponseInGameThread && !IsInGameThread())
		{
			AsyncTask(ENamedThreads::GameThread, [this, Keyword, ResponseCode]()
			{
				OnResponseError.Broadcast(Keyword, ResponseCode);
			});
		}
		else
		{
			OnResponseError.Broadcast(Keyword, ResponseCode);
		}
	}
}

void UHTTPSubsystem::OnDataRequestCompleted(int32 ReqId, const TArray<uint8>& Content, const TArray<FString>& Headers, const int32 ResponseCode, const bool bWasSuccessful)
{
	const FName Keyword = HttpRequests.Contains(ReqId) ? HttpRequests[ReqId].Name : TEXT("Default");

	if (bWasSuccessful)
	{
		DataBuffer.SetNum(Content.Num());
		FMemory::Memcpy(DataBuffer.GetData(), Content.GetData(), DataBuffer.Num());
		if (bResponseInGameThread && !IsInGameThread())
		{
			AsyncTask(ENamedThreads::GameThread, [this, Keyword, ResponseCode, Headers]()
			{
				OnDataResponse.Broadcast(Keyword, ResponseCode, Headers, DataBuffer);
			});
		}
		else
		{
			OnDataResponse.Broadcast(Keyword, ResponseCode, Headers, DataBuffer);
		}
	}
	else
	{
		if (bResponseInGameThread && !IsInGameThread())
		{
			AsyncTask(ENamedThreads::GameThread, [this, Keyword, ResponseCode]()
			{
				OnResponseError.Broadcast(Keyword, ResponseCode);
			});
		}
		else
		{
			OnResponseError.Broadcast(Keyword, ResponseCode);
		}
	}
}

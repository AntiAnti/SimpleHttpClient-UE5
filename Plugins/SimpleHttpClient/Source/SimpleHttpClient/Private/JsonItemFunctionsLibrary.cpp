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

bool UJsonItemFunctionsLibrary::JsonSetFieldValue_String(FJsonItem& JsonItem, FString Path, FString Value)
{
	return JsonItem.SetStringValue(Path, Value);
}

bool UJsonItemFunctionsLibrary::JsonSetFieldValue_Numeric(FJsonItem& JsonItem, FString Path, float Value)
{
	return JsonItem.SetFloatValue(Path, Value);
}

bool UJsonItemFunctionsLibrary::JsonSetFieldValue_Bool(FJsonItem& JsonItem, FString Path, bool Value)
{
	return JsonItem.SetBooleanValue(Path, Value);
}

bool UJsonItemFunctionsLibrary::JsonAddArrayItem(FJsonItem& JsonItem, FString Path, const FJsonItem& NewItem)
{
	return JsonItem.AddObjectArrayItem(Path, NewItem.AsString());
}

bool UJsonItemFunctionsLibrary::JsonAddArrayItemString(FJsonItem& JsonItem, FString Path, FString NewItem)
{
	return JsonItem.AddStringArrayItem(Path, NewItem);
}

bool UJsonItemFunctionsLibrary::JsonAddArrayItemNumeric(FJsonItem& JsonItem, FString Path, float NewItem)
{
	return JsonItem.AddFloatArrayItem(Path, NewItem);
}

bool UJsonItemFunctionsLibrary::JsonAddArrayItemBool(FJsonItem& JsonItem, FString Path, bool NewItem)
{
	return JsonItem.AddBooleanArrayItem(Path, NewItem);
}

bool UJsonItemFunctionsLibrary::ExtractJsonBlockFromString(const FString& InText, int32 SearchStart, int32& OutStartPos, int32& OutBlockLen, FJsonItem& OutJson)
{
	int32 Depth = 0;
	int32 Position = SearchStart;
	int32 PositionStart = INDEX_NONE;
	int32 PositionEnd = INDEX_NONE;

	while (Position < InText.Len())
	{
		int32 OpenPos = InText.Find(TEXT("{"), ESearchCase::IgnoreCase, ESearchDir::FromStart, Position);
		int32 EndPos = InText.Find(TEXT("}"), ESearchCase::IgnoreCase, ESearchDir::FromStart, Position);

		if ((EndPos != INDEX_NONE && OpenPos != INDEX_NONE && EndPos < OpenPos) || (EndPos != INDEX_NONE && OpenPos == INDEX_NONE))
		{
			Depth--; Position = EndPos + 1;
		}
		else if ((EndPos != INDEX_NONE && OpenPos != INDEX_NONE && EndPos > OpenPos) || (EndPos == INDEX_NONE && OpenPos != INDEX_NONE))
		{
			if (PositionStart == INDEX_NONE) PositionStart = OpenPos;
			Depth++; Position = OpenPos + 1;
		}

		if (Depth < 0 || (EndPos == INDEX_NONE && OpenPos == INDEX_NONE))
		{
			break;
		}
		if (Depth == 0)
		{
			PositionEnd = EndPos + 1;
			break;
		}
	}

	if (PositionStart == INDEX_NONE || PositionEnd == INDEX_NONE)
	{
		return false;
	}

	OutStartPos = PositionStart;
	OutBlockLen = PositionEnd - PositionStart;
	FString JsonData = InText.Mid(PositionStart, OutBlockLen);
	OutJson.FromString(JsonData);

	return OutJson.IsValid();
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

FYnnkUrlParameter UJsonItemFunctionsLibrary::GetDefaultContentHtml()
{
	return FYnnkUrlParameter("Content-Type", "text/html");
}

FString UJsonItemFunctionsLibrary::CleanJsonResponse(const FString& InText)
{
	bool bStartsWtihData = InText.StartsWith(TEXT("data: {")) || InText.StartsWith(TEXT("data:{"));
	bool bEndsWithBracket = InText.TrimEnd().EndsWith(TEXT("}")) || InText.EndsWith(TEXT("}\n")) || InText.EndsWith(TEXT("}\r\n"));
	if (bStartsWtihData && bEndsWithBracket)
	{
		int32 Ind = InText.Find("{");
		return InText.RightChop(Ind);
	}
	else
	{
		return InText;
	}
}

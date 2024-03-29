// Fill out your copyright notice in the Description page of Project Settings.

#include "YnnkHttpTypes.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

FString FJsonItem::GetStringValue(const FString& Path) const
{
	FString val;
	GetValue(Path, EJson::String, nullptr, &val);
	return val;

}

float FJsonItem::GetFloatValue(const FString& Path) const
{
	float val;
	GetValue(Path, EJson::Number, &val);
	return val;
}

bool FJsonItem::GetBooleanValue(const FString& Path) const
{
	bool val;
	GetValue(Path, EJson::Boolean, nullptr, nullptr, &val);
	return val;
}

bool FJsonItem::SetValue(FString Path, EJson Format, float ValueNumeric, const FString& ValueStr, bool bValue)
{
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
	TSharedPtr<FJsonObject> JsonResponse = MakeShareable(new FJsonObject);
	if (!FJsonSerializer::Deserialize(Reader, JsonResponse))
	{
		UE_LOG(LogTemp, Log, TEXT("FJsonItem::SetValue - invalid json body"));
		return false;
	}

	TArray<FString> PathParameters;
	Path.ParseIntoArray(PathParameters, TEXT("."));

	TSharedPtr<FJsonObject> Field = JsonResponse;

	if (PathParameters.Num() < 1)
	{
		UE_LOG(LogTemp, Log, TEXT("FJsonItem::SetValue - empty path"));
		return false;
	}
	else if (PathParameters.Num() < 2)
	{
		TSharedPtr<FJsonValue> NewValue;
		if (Format == EJson::String)
		{
			NewValue = MakeShared<FJsonValueString>(ValueStr);
		}
		else if (Format == EJson::Number)
		{
			NewValue = MakeShared<FJsonValueNumber>(ValueNumeric);
		}
		else if (Format == EJson::Boolean)
		{
			NewValue = MakeShared<FJsonValueBoolean>(bValue);
		}
		SetTargetValue(Field, PathParameters[0], NewValue);
	}
	else
	{
		for (int32 i = 0; i < PathParameters.Num() - 1; i++)
		{
			TSharedPtr<FJsonObject> CurrField = Field;
			if (!GetChildObjectByPath(CurrField, Field, PathParameters[i]))
			{
				return false;
			}
		}

		// finish?
		if (Format == EJson::String)
		{
			Field->SetStringField(PathParameters.Last(), ValueStr);
		}
		else if (Format == EJson::Number)
		{
			Field->SetNumberField(PathParameters.Last(), ValueNumeric);
		}
		else if (Format == EJson::Boolean)
		{
			Field->SetBoolField(PathParameters.Last(), bValue);
		}
	}

	FString RequestContentString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestContentString);
	FJsonSerializer::Serialize(JsonResponse.ToSharedRef(), Writer);

	FromString(RequestContentString);
	return IsValid();
}

bool FJsonItem::GetValue(const FString& Path, EJson Format, float* ValueNumeric, FString* ValueStr, bool* bValue) const
{
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(AsString());
	TSharedPtr<FJsonObject> JsonResponse = MakeShareable(new FJsonObject);
	if (!FJsonSerializer::Deserialize(Reader, JsonResponse))
	{
		return false;
	}

	TArray<FString> PathParameters;
	Path.ParseIntoArray(PathParameters, TEXT("."));
	if (PathParameters.Num() < 1)
	{
		return false;
	}

	TSharedPtr<FJsonObject> Field = JsonResponse;

	for (int32 i = 0; i < PathParameters.Num() - 1; i++)
	{
		TSharedPtr<FJsonObject> CurrField = Field;
		if (!GetChildObjectByPath(CurrField, Field, PathParameters[i]))
		{
			return false;
		}
	}

	// finish?
	if (Format == EJson::String && ValueStr)
	{
		*ValueStr = Field->GetStringField(PathParameters.Last());
	}
	else if (Format == EJson::Number && ValueNumeric)
	{
		*ValueNumeric = Field->GetNumberField(PathParameters.Last());
	}
	else if (Format == EJson::Boolean && bValue)
	{
		*bValue = Field->GetBoolField(PathParameters.Last());
	}
	else
	{
		return false;
	}

	return true;
}

bool FJsonItem::SetTargetValue(TSharedPtr<FJsonObject>& InParentObjectField, const FString& FieldName, TSharedPtr<FJsonValue>& NewValue) const
{
	if (FieldName.Right(1) == TEXT("]"))
	{
		FString ArrFieldName, ArrFieldValue;
		bool bNumeric = false;
		float NumVal = 0.f;

		FString FieldNameAsPath, a, b;
		FieldNameAsPath = FieldName;
		FieldNameAsPath.Split(TEXT("["), &a, &b);
		FieldNameAsPath = a;
		b.LeftChopInline(1);

		if (FCString::IsNumeric(*b))
		{
			// array[index]

			int32 ArrIndex = FCString::Atoi(*b);
			//UE_LOG(LogTemp, Log, TEXT("looking for array field \"%s\" with index %d"), *FieldNameAsPath, ArrIndex);

			TArray<TSharedPtr<FJsonValue>> Arr = InParentObjectField->GetArrayField(FieldNameAsPath);
			if (Arr.IsValidIndex(ArrIndex))
			{
				Arr[ArrIndex] = NewValue;
				InParentObjectField->SetArrayField(FieldNameAsPath, Arr);
				return true;
			}
		}
		else
		{
			// array[field_name=field_value]

			b.Split(TEXT("="), &ArrFieldName, &ArrFieldValue);
			// remove quotes
			if (ArrFieldValue.Left(1) == TEXT("\"") && ArrFieldValue.Right(1) == TEXT("\""))
			{
				ArrFieldValue.MidInline(1, ArrFieldValue.Len() - 2);
			}
			bNumeric = FCString::IsNumeric(*ArrFieldValue);
			if (bNumeric)
			{
				NumVal = FCString::Atof(*ArrFieldValue);
			}
			bool bBoolean = false;
			bool bBoolValue = false;
			if (ArrFieldValue.Equals(TEXT("true"), ESearchCase::IgnoreCase) || ArrFieldValue.Equals(TEXT("false"), ESearchCase::IgnoreCase))
			{
				bBoolean = true;
				bBoolValue = ArrFieldValue.Equals(TEXT("true"), ESearchCase::IgnoreCase);
			}

			TArray<TSharedPtr<FJsonValue>> Arr = InParentObjectField->GetArrayField(FieldNameAsPath);
			for (auto& ArrField : Arr)
			{
				TSharedPtr<FJsonObject>* f;
				if (ArrField->TryGetObject(f))
				{
					FString valStr = f->Get()->HasTypedField<EJson::String>(ArrFieldName) ? f->Get()->GetStringField(ArrFieldName) : "Err";
					float valFloat = f->Get()->HasTypedField<EJson::Number>(ArrFieldName) ? f->Get()->GetNumberField(ArrFieldName) : 0.f;
					bool valBoolean = f->Get()->HasTypedField<EJson::Boolean>(ArrFieldName) ? f->Get()->GetBoolField(ArrFieldName) : false;

					if (bBoolean)
					{
						if (bBoolValue == valBoolean)
						{
							ArrField = NewValue;
							InParentObjectField->SetArrayField(FieldNameAsPath, Arr);
							return true;
						}

					}
					else if (bNumeric)
					{
						if (FMath::IsNearlyEqual(valFloat, NumVal))
						{
							ArrField = NewValue;
							InParentObjectField->SetArrayField(FieldNameAsPath, Arr);
							return true;
						}
					}
					else
					{
						if (valStr == ArrFieldValue)
						{
							ArrField = NewValue;
							InParentObjectField->SetArrayField(FieldNameAsPath, Arr);
							return true;
						}
					}
				}
			}
		}
	}
	else
	{
		InParentObjectField->SetField(FieldName, NewValue);
		return true;
	}

	return false;
}

bool FJsonItem::GetChildObjectByPath(TSharedPtr<FJsonObject>& InParentObjectField, TSharedPtr<FJsonObject>& OutChildObjectField, FString FieldNameAsPath) const
{
	FString ArrFieldName, ArrFieldValue;
	bool bNumeric = false;
	float NumVal = 0.f;

	UE_LOG(LogTemp, Log, TEXT("GetChildObjectByPath: %s"), *FieldNameAsPath);

	// array?
	if (FieldNameAsPath.Right(1) == TEXT("]"))
	{
		FString a, b;
		FieldNameAsPath.Split(TEXT("["), &a, &b);
		FieldNameAsPath = a;
		b.LeftChopInline(1);

		if (FCString::IsNumeric(*b))
		{
			// array[index]

			int32 ArrIndex = FCString::Atoi(*b);
			UE_LOG(LogTemp, Log, TEXT("looking for array field \"%s\" with index %d"), *FieldNameAsPath, ArrIndex);

			TArray<TSharedPtr<FJsonValue>> Arr = InParentObjectField->GetArrayField(FieldNameAsPath);
			if (Arr.IsValidIndex(ArrIndex))
			{
				OutChildObjectField = Arr[ArrIndex]->AsObject();
				return true;
			}
		}
		else
		{
			// array[field_name=field_value]

			b.Split(TEXT("="), &ArrFieldName, &ArrFieldValue);
			// remove quotes
			if (ArrFieldValue.Left(1) == TEXT("\"") && ArrFieldValue.Right(1) == TEXT("\""))
			{
				ArrFieldValue.MidInline(1, ArrFieldValue.Len() - 2);
			}
			bNumeric = FCString::IsNumeric(*ArrFieldValue);
			if (bNumeric)
			{
				NumVal = FCString::Atof(*ArrFieldValue);
			}
			bool bBoolean = false;
			bool bBoolValue = false;
			if (ArrFieldValue.Equals(TEXT("true"), ESearchCase::IgnoreCase) || ArrFieldValue.Equals(TEXT("false"), ESearchCase::IgnoreCase))
			{
				bBoolean = true;
				bBoolValue = ArrFieldValue.Equals(TEXT("true"), ESearchCase::IgnoreCase);
			}

			UE_LOG(LogTemp, Log, TEXT(" looking for array %s with field %s = %s"), *FieldNameAsPath, *ArrFieldName, *ArrFieldValue);

			TArray<TSharedPtr<FJsonValue>> Arr = InParentObjectField->GetArrayField(FieldNameAsPath);
			for (auto& ArrField : Arr)
			{
				TSharedPtr<FJsonObject>* f;
				if (ArrField->TryGetObject(f))
				{
					FString valStr = f->Get()->HasTypedField<EJson::String>(ArrFieldName) ? f->Get()->GetStringField(ArrFieldName) : "Err";
					float valFloat = f->Get()->HasTypedField<EJson::Number>(ArrFieldName) ? f->Get()->GetNumberField(ArrFieldName) : 0.f;
					bool valBoolean = f->Get()->HasTypedField<EJson::Boolean>(ArrFieldName) ? f->Get()->GetBoolField(ArrFieldName) : false;

					UE_LOG(LogTemp, Log, TEXT("checking numeric value: [%s] = [%f | %s], IsNumeric=%d"), *ArrFieldName, valFloat, *valStr, (int)bNumeric);

					if (bBoolean)
					{
						if (bBoolValue == valBoolean)
						{
							OutChildObjectField = ArrField->AsObject();
							return true;
						}
					}
					else if (bNumeric)
					{
						if (FMath::IsNearlyEqual(valFloat, NumVal))
						{
							OutChildObjectField = ArrField->AsObject();
							return true;
						}
					}
					else
					{
						if (valStr == ArrFieldValue)
						{
							UE_LOG(LogTemp, Log, TEXT("Item found"));
							OutChildObjectField = ArrField->AsObject();
							return true;
						}
					}
				}
			}
		}

		UE_LOG(LogTemp, Log, TEXT("Item not found"));
	}
	else
	{
		if (InParentObjectField->HasTypedField<EJson::Object>(FieldNameAsPath))
		{
			OutChildObjectField = InParentObjectField->GetObjectField(FieldNameAsPath);
			return true;
		}
		else
		{
			return false;
		}
	}

	return false;
}

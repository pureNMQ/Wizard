#include "Save/WizardSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Crc.h"

namespace WizardSaveStorage
{
	static constexpr uint32 Magic = 0x575A5331;
	static constexpr int32 HeaderSize = 12;
	static constexpr int32 MaxFileSize = 1024 * 1024;

	bool Encode(UWizardSaveGame* Snapshot, TArray<uint8>& Bytes)
	{
		Bytes.Reset();
		TArray<uint8> Payload;
		if (!Snapshot || !UGameplayStatics::SaveGameToMemory(Snapshot, Payload) || Payload.IsEmpty() || Payload.Num() > MaxFileSize - HeaderSize) return false;
		const uint32 Header[] = { Magic, static_cast<uint32>(Payload.Num()), FCrc::MemCrc32(Payload.GetData(), Payload.Num()) };
		Bytes.Append(reinterpret_cast<const uint8*>(Header), HeaderSize);
		Bytes.Append(Payload);
		return true;
	}

	UWizardSaveGame* Decode(const TArray<uint8>& Bytes)
	{
		if (Bytes.Num() <= HeaderSize || Bytes.Num() > MaxFileSize) return nullptr;
		uint32 Header[3];
		FMemory::Memcpy(Header, Bytes.GetData(), HeaderSize);
		if (Header[0] != Magic || Header[1] != static_cast<uint32>(Bytes.Num() - HeaderSize)
			|| Header[2] != FCrc::MemCrc32(Bytes.GetData() + HeaderSize, Bytes.Num() - HeaderSize)) return nullptr;
		TArray<uint8> Payload;
		Payload.Append(Bytes.GetData() + HeaderSize, Bytes.Num() - HeaderSize);
		return Cast<UWizardSaveGame>(UGameplayStatics::LoadGameFromMemory(Payload));
	}

	bool Read(const FString& Path, TArray<uint8>& Bytes)
	{
		const int64 Size = IFileManager::Get().FileSize(*Path);
		return Size > HeaderSize && Size <= MaxFileSize && FFileHelper::LoadFileToArray(Bytes, *Path);
	}

	bool Write(const FString& Path, const TArray<uint8>& Bytes)
	{
		IFileManager& Files = IFileManager::Get();
		if (Bytes.IsEmpty() || !Files.MakeDirectory(*FPaths::GetPath(Path), true)) return false;
		const FString Temporary = Path + TEXT(".tmp");
		if (!FFileHelper::SaveArrayToFile(Bytes, *Temporary)) return false;
		if (Files.FileExists(*Path) && Files.Copy(*(Path + TEXT(".bak")), *Path, true, false) != COPY_OK) return false;
		return Files.Move(*Path, *Temporary, true, false, false, true);
	}
}

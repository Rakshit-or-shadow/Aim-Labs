#include "GETrainingCenter.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "GETrainingManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "GEStyle.h"

AGETrainingCenter::AGETrainingCenter()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

void AGETrainingCenter::BeginPlay()
{
	Super::BeginPlay();
	CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	GlowMaterial = LoadObject<UMaterialInterface>(nullptr, GEStyle::GlowMaterialPath);

	// Stage B4: a light haze so the room matches the dark neon look of the arena
	if (UExponentialHeightFogComponent* Fog = NewObject<UExponentialHeightFogComponent>(this, TEXT("GE_Haze")))
	{
		Fog->SetupAttachment(Root);
		Fog->RegisterComponent();
		Fog->SetFogDensity(0.004f);
		Fog->SetFogHeightFalloff(0.3f);
		Fog->SetFogInscatteringColor(GEStyle::GetTheme(GEStyle::EGETheme::Red).BgTint);
		Fog->SetFogMaxOpacity(0.6f);
	}

	BuildHub();
	BuildLane();
	if (bHasPortal)
	{
		BuildPortal();
	}

	if (UGETrainingManager* Manager = UGETrainingManager::Get(this))
	{
		Manager->RegisterCenter(this);
	}
}

/** Makes one colored box. Collision comes from a separate invisible box component. */
UStaticMeshComponent* AGETrainingCenter::AddBox(const FVector& LocalCenter, const FVector& SizeCm, const FLinearColor& Color, bool bCollide, float YawDeg, bool bGlow)
{
	const int32 Id = PartCounter++;
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("GE_Box%d"), Id));
	C->SetStaticMesh(CubeMesh);
	C->SetupAttachment(Root);
	C->SetRelativeLocation(LocalCenter);
	C->SetRelativeRotation(FRotator(0.0f, YawDeg, 0.0f));
	C->SetRelativeScale3D(SizeCm / 100.0f); // cube is 100 cm at scale 1
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->RegisterComponent();
	UMaterialInterface* Base = (bGlow && GlowMaterial) ? GlowMaterial.Get() : BaseMaterial.Get();
	if (Base)
	{
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Base, C);
		M->SetVectorParameterValue(TEXT("Color"), Color);
		C->SetMaterial(0, M);
	}
	if (bCollide)
	{
		UBoxComponent* Box = NewObject<UBoxComponent>(this, *FString::Printf(TEXT("GE_Collision%d"), Id));
		Box->SetupAttachment(Root);
		Box->SetRelativeLocation(LocalCenter);
		Box->SetRelativeRotation(FRotator(0.0f, YawDeg, 0.0f));
		Box->SetBoxExtent(SizeCm * 0.5f);
		Box->SetCollisionProfileName(TEXT("BlockAll"));
		Box->RegisterComponent();
	}
	return C;
}

void AGETrainingCenter::AddText(const FString& Text, const FVector& LocalLocation, float YawDeg, float WorldSize, const FColor& Color)
{
	UTextRenderComponent* T = NewObject<UTextRenderComponent>(this, *FString::Printf(TEXT("GE_Text%d"), PartCounter++));
	T->SetupAttachment(Root);
	T->SetRelativeLocation(LocalLocation);
	T->SetRelativeRotation(FRotator(0.0f, YawDeg, 0.0f));
	T->SetText(FText::FromString(Text));
	T->SetWorldSize(WorldSize);
	T->SetTextRenderColor(Color);
	T->SetHorizontalAlignment(EHTA_Center);
	T->SetVerticalAlignment(EVRTA_TextCenter);
	T->RegisterComponent();
}

void AGETrainingCenter::AddLight(const FVector& LocalLocation)
{
	UPointLightComponent* L = NewObject<UPointLightComponent>(this, *FString::Printf(TEXT("GE_Light%d"), PartCounter++));
	L->SetupAttachment(Root);
	L->SetRelativeLocation(LocalLocation);
	L->SetIntensity(Layout.LightIntensity);
	L->SetAttenuationRadius(3000.0f);
	L->SetCastShadows(false);
	L->RegisterComponent();
}

void AGETrainingCenter::BuildHub()
{
	const FGETrainingLayout& L = Layout;
	const float Length = L.FloorMaxX - L.FloorMinX;
	const float MidX = (L.FloorMinX + L.FloorMaxX) * 0.5f;

	// Big floor (top surface at local Z 0).
	AddBox(FVector(MidX, 0, -20), FVector(Length, L.FloorHalfWidth * 2.0f, 40), GEStyle::WithAlpha(GEStyle::FloorColor * 2.0f, 1.0f), true);
	// Spawn pad in the hub.
	AddBox(FVector(0, 0, 0.6f), FVector(500, 500, 1), GEStyle::Glow(GEStyle::GetTheme(GEStyle::EGETheme::Red).AccentDim, 1.2f), false, 0.0f, true);

	// Walls: rear, far end, two sides.
	const FLinearColor WallColor = GEStyle::WithAlpha(GEStyle::BackWallColor * 2.5f, 1.0f); // dark walls (theme background)
	AddBox(FVector(L.FloorMinX, 0, L.WallHeight * 0.5f), FVector(40, L.FloorHalfWidth * 2.0f, L.WallHeight), WallColor, true);
	AddBox(FVector(L.FloorMaxX, 0, L.WallHeight * 0.5f), FVector(40, L.FloorHalfWidth * 2.0f, L.WallHeight), WallColor, true);
	AddBox(FVector(MidX, L.FloorHalfWidth, L.WallHeight * 0.5f), FVector(Length, 40, L.WallHeight), WallColor, true);
	AddBox(FVector(MidX, -L.FloorHalfWidth, L.WallHeight * 0.5f), FVector(Length, 40, L.WallHeight), WallColor, true);

	// Lights along the room so it is not dark.
	for (float X = L.FloorMinX + 500.0f; X < L.FloorMaxX; X += L.LightSpacing)
	{
		AddLight(FVector(X, 0, L.LightHeight));
	}

	// Stage B4: neon strips along the walls (red low, cyan high), same colors as the UI themes
	const FLinearColor RedNeon = GEStyle::Glow(GEStyle::GetTheme(GEStyle::EGETheme::Red).AccentPrimary, 2.5f);
	const FLinearColor CyanNeon = GEStyle::Glow(GEStyle::GetTheme(GEStyle::EGETheme::Cyan).AccentPrimary, 3.0f);
	for (float Side : { -1.0f, 1.0f })
	{
		const float Y = Side * (L.FloorHalfWidth - 24.0f);
		AddBox(FVector(MidX, Y, 6.0f), FVector(Length - 80.0f, 6, 6), RedNeon, false, 0.0f, true);
		AddBox(FVector(MidX, Y, L.WallHeight - 10.0f), FVector(Length - 80.0f, 6, 6), CyanNeon, false, 0.0f, true);
	}
	const FColor Info = GEStyle::GetTheme(GEStyle::EGETheme::Cyan).AccentPrimary.ToFColor(true);
	AddText(TEXT("TRAINING CENTER"), FVector(L.FloorMinX + 30.0f, 0, 300), 0.0f, 80.0f, Info);
}

void AGETrainingCenter::BuildLane()
{
	const FGETrainingLayout& L = Layout;

	// Lane strip from the spawn pad to the far end.
	AddBox(FVector(2500, 0, 0.6f), FVector(5000, 600, 1), GEStyle::WithAlpha(GEStyle::FloorColor * 3.0f, 1.0f), false);

	// Distance markers: a floor strip plus a sign post with text on each side. Text faces back toward the player (yaw 180).
	for (int32 Meters : L.MarkerDistancesMeters)
	{
		const float X = Meters * 100.0f;
		AddBox(FVector(X, 0, 1.0f), FVector(30, 600, 1.5f), GEStyle::Glow(GEStyle::GetTheme(GEStyle::EGETheme::Yellow).AccentPrimary, 1.5f), false, 0.0f, true);
		for (float Side : { -1.0f, 1.0f })
		{
			const float Y = Side * 380.0f;
			AddBox(FVector(X, Y, 60), FVector(10, 10, 120), FLinearColor(0.8f, 0.8f, 0.8f), false);
			AddText(FString::Printf(TEXT("%d m"), Meters), FVector(X - 8.0f, Y, 150), 180.0f, 60.0f, GEStyle::GetTheme(GEStyle::EGETheme::Yellow).AccentPrimary.ToFColor(true));
		}
	}

	// Cover walls.
	for (const FVector2D& P : L.CoverWalls)
	{
		AddBox(FVector(P.X, P.Y, L.CoverSize.Z * 0.5f), L.CoverSize, FLinearColor(0.35f, 0.3f, 0.25f), true);
	}
	// Crates.
	for (const FVector2D& P : L.Crates)
	{
		AddBox(FVector(P.X, P.Y, 50), FVector(100, 100, 100), FLinearColor(0.5f, 0.35f, 0.15f), true);
	}
}

void AGETrainingCenter::BuildPortal()
{
	// Glowing frame next to the player start, facing the start (PortalYaw = direction the start looks).
	const FTransform T = GetActorTransform();
	const FVector Center = T.InverseTransformPosition(PortalLocation);
	const FRotator Facing(0.0f, PortalYaw, 0.0f);
	const FVector Side = Facing.RotateVector(FVector::RightVector);
	const FLinearColor Glow = GEStyle::Glow(GEStyle::GetTheme(GEStyle::EGETheme::Cyan).AccentPrimary, 3.0f);

	AddBox(Center + Side * 110.0f + FVector(0, 0, 150), FVector(20, 20, 300), Glow, false, PortalYaw, true);
	AddBox(Center - Side * 110.0f + FVector(0, 0, 150), FVector(20, 20, 300), Glow, false, PortalYaw, true);
	AddBox(Center + FVector(0, 0, 300), FVector(20, 240, 20), Glow, false, PortalYaw, true);
	// Text faces back toward the start (opposite of its looking direction).
	AddText(TEXT("TRAINING CENTER\npress T"), Center + FVector(0, 0, 360), PortalYaw + 180.0f, 40.0f, GEStyle::GetTheme(GEStyle::EGETheme::Cyan).AccentPrimary.ToFColor(true));
}

void AGETrainingCenter::GetBotSlots(EGETrainingMode Mode, TArray<FGEBotSlot>& OutSlots) const
{
	const FTransform T = GetActorTransform();
	const FVector Lateral = T.TransformVectorNoScale(FVector::RightVector);

	if (Mode == EGETrainingMode::Peek)
	{
		for (const FVector2D& P : Layout.CoverWalls)
		{
			FGEBotSlot Slot;
			const float BehindX = P.X + Layout.CoverSize.X * 0.5f + Layout.PeekBotGapBehindCover;
			Slot.Location = T.TransformPosition(FVector(BehindX, P.Y, 0));
			Slot.LateralAxis = Lateral;
			// Step out toward the middle of the lane.
			Slot.PeekDirection = Lateral * (P.Y > 0.0f ? -1.0f : 1.0f);
			OutSlots.Add(Slot);
		}
	}
	else
	{
		for (const FVector2D& P : Layout.OpenBotPositions)
		{
			FGEBotSlot Slot;
			Slot.Location = T.TransformPosition(FVector(P.X, P.Y, 0));
			Slot.LateralAxis = Lateral;
			Slot.PeekDirection = Lateral;
			OutSlots.Add(Slot);
		}
	}
}

void AGETrainingCenter::GetPlayerSpawn(FVector& OutLocation, FRotator& OutRotation) const
{
	// 100 cm up so the capsule never starts inside the floor; it settles by itself.
	OutLocation = GetActorTransform().TransformPosition(FVector(0, 0, 100));
	OutRotation = GetActorRotation();
}

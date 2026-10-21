// Arena.cpp

#include "Arena.h"
#include "RayShotStyle.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/Scene.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

// ---------- room size (cm). The player stands at X = 0, targets are at X = 1000 ----------
static const float ArenaXMin = -500.0f;   // wall behind the player
static const float ArenaXMax = 1300.0f;   // back wall (behind the targets)
static const float ArenaHalfWidth = 1000.0f;
static const float ArenaHeight = 900.0f;
static const float ArenaWallThickness = 20.0f;

AArena::AArena()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    // load the built-in shapes and materials once
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(RayShotStyle::CubeMeshPath);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Lit(RayShotStyle::LitMaterialPath);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Glow(RayShotStyle::GlowMaterialPath);

    if (Cube.Succeeded()) { CubeMesh = Cube.Object; }
    if (Lit.Succeeded())  { LitMaterial = Lit.Object; }
    if (Glow.Succeeded()) { GlowMaterial = Glow.Object; }
}

void AArena::BeginPlay()
{
    Super::BeginPlay();

    const float Length = ArenaXMax - ArenaXMin;
    const float MidX = (ArenaXMin + ArenaXMax) * 0.5f;
    const float T = ArenaWallThickness;

    // =================== the room ===================
    // floor (top surface just above z = 0 so it covers the level's own floor)
    AddBox(FVector(MidX, 0, 2 - T * 0.5f), FVector(Length, ArenaHalfWidth * 2, T), RayShotStyle::FloorColor, false);
    // ceiling
    AddBox(FVector(MidX, 0, ArenaHeight + T * 0.5f), FVector(Length, ArenaHalfWidth * 2, T), RayShotStyle::WallColor, false);
    // left + right walls
    AddBox(FVector(MidX, -ArenaHalfWidth - T * 0.5f, ArenaHeight * 0.5f), FVector(Length, T, ArenaHeight), RayShotStyle::WallColor, false);
    AddBox(FVector(MidX,  ArenaHalfWidth + T * 0.5f, ArenaHeight * 0.5f), FVector(Length, T, ArenaHeight), RayShotStyle::WallColor, false);
    // back wall (behind the targets) + front wall (behind the player)
    AddBox(FVector(ArenaXMax + T * 0.5f, 0, ArenaHeight * 0.5f), FVector(T, ArenaHalfWidth * 2, ArenaHeight), RayShotStyle::BackWallColor, false);
    AddBox(FVector(ArenaXMin - T * 0.5f, 0, ArenaHeight * 0.5f), FVector(T, ArenaHalfWidth * 2, ArenaHeight), RayShotStyle::WallColor, false);

    // =================== neon strips ===================
    const FLinearColor CyanNeon = RayShotStyle::Glow(RayShotStyle::Cyan, 3.0f);
    const FLinearColor PinkNeon = RayShotStyle::Glow(RayShotStyle::Magenta, 2.5f);
    const float Strip = 6.0f; // strip thickness

    // along the floor edges (cyan) and ceiling edges (pink)
    for (int32 Side = -1; Side <= 1; Side += 2)
    {
        float Y = Side * (ArenaHalfWidth - Strip);
        AddBox(FVector(MidX, Y, Strip), FVector(Length, Strip, Strip), CyanNeon, true);
        AddBox(FVector(MidX, Y, ArenaHeight - Strip), FVector(Length, Strip, Strip), PinkNeon, true);

        // vertical strips in the back corners
        AddBox(FVector(ArenaXMax - Strip, Y, ArenaHeight * 0.5f), FVector(Strip, Strip, ArenaHeight), PinkNeon, true);
    }

    // a glowing frame on the back wall around the target zone
    // (targets are centered at Y = 0, Z = 320)
    const float FrameX = ArenaXMax - 4.0f;
    const float FrameHalfW = 460.0f;
    const float FrameBottom = 30.0f;
    const float FrameTop = 610.0f;
    const float FrameMidZ = (FrameBottom + FrameTop) * 0.5f;
    AddBox(FVector(FrameX, 0, FrameTop),    FVector(Strip, FrameHalfW * 2, Strip), CyanNeon, true);
    AddBox(FVector(FrameX, 0, FrameBottom), FVector(Strip, FrameHalfW * 2, Strip), CyanNeon, true);
    AddBox(FVector(FrameX, -FrameHalfW, FrameMidZ), FVector(Strip, Strip, FrameTop - FrameBottom), CyanNeon, true);
    AddBox(FVector(FrameX,  FrameHalfW, FrameMidZ), FVector(Strip, Strip, FrameTop - FrameBottom), CyanNeon, true);

    // =================== floor grid ===================
    const FLinearColor GridColor = RayShotStyle::Glow(RayShotStyle::Cyan, 0.35f);
    for (float Y = -800.0f; Y <= 800.0f; Y += 200.0f)
    {
        AddBox(FVector(MidX, Y, 3), FVector(Length, 3, 2), GridColor, true);
    }
    for (float X = -400.0f; X <= 1200.0f; X += 200.0f)
    {
        AddBox(FVector(X, 0, 3), FVector(3, ArenaHalfWidth * 2, 2), GridColor, true);
    }

    // a pink square on the floor where the player stands
    const float Pad = 70.0f;
    AddBox(FVector(0,  Pad, 4), FVector(Pad * 2, 4, 3), PinkNeon, true);
    AddBox(FVector(0, -Pad, 4), FVector(Pad * 2, 4, 3), PinkNeon, true);
    AddBox(FVector( Pad, 0, 4), FVector(4, Pad * 2, 3), PinkNeon, true);
    AddBox(FVector(-Pad, 0, 4), FVector(4, Pad * 2, 3), PinkNeon, true);

    // =================== lights ===================
    const FLinearColor CoolWhite(0.75f, 0.85f, 1.0f);
    AddLight(FVector(0,    -500, 850), CoolWhite, 80.0f, 1800.0f, true);
    AddLight(FVector(0,     500, 850), CoolWhite, 80.0f, 1800.0f, true);
    AddLight(FVector(800,  -500, 850), CoolWhite, 80.0f, 1800.0f, true);
    AddLight(FVector(800,   500, 850), CoolWhite, 80.0f, 1800.0f, true);
    // soft cyan wash on the back wall
    AddLight(FVector(1150, 0, 320), RayShotStyle::Cyan, 30.0f, 800.0f, false);

    AddPostProcess();
}

void AArena::AddBox(const FVector& Center, const FVector& Size, const FLinearColor& Color, bool bGlow)
{
    if (CubeMesh == nullptr)
    {
        return;
    }

    // create a new mesh component at runtime
    UStaticMeshComponent* Box = NewObject<UStaticMeshComponent>(this);
    Box->SetupAttachment(RootComponent);
    Box->SetMobility(EComponentMobility::Movable);
    Box->SetStaticMesh(CubeMesh);
    Box->SetRelativeLocation(Center);
    Box->SetRelativeScale3D(Size / 100.0f); // the cube is 100 cm wide
    Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Box->SetCastShadow(!bGlow); // neon strips shouldn't cast shadows
    Box->RegisterComponent();

    // give it its own colored material
    UMaterialInterface* BaseMaterial = bGlow ? GlowMaterial : LitMaterial;
    if (BaseMaterial)
    {
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        Material->SetVectorParameterValue(TEXT("Color"), Color);
        Box->SetMaterial(0, Material);
    }
}

void AArena::AddLight(const FVector& Location, const FLinearColor& Color, float Candelas, float Radius, bool bShadows)
{
    UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
    Light->SetupAttachment(RootComponent);
    Light->SetMobility(EComponentMobility::Movable);
    Light->SetRelativeLocation(Location);
    Light->RegisterComponent();

    Light->SetIntensityUnits(ELightUnits::Candelas);
    Light->SetIntensity(Candelas);
    Light->SetLightColor(Color);
    Light->SetAttenuationRadius(Radius);
    Light->SetSourceRadius(30.0f); // softer shadows
    Light->SetCastShadows(bShadows);
}

void AArena::AddPostProcess()
{
    APostProcessVolume* Volume = GetWorld()->SpawnActor<APostProcessVolume>();
    if (Volume == nullptr)
    {
        return;
    }

    Volume->bUnbound = true; // affects the whole level

    FPostProcessSettings& Settings = Volume->Settings;

    // glow around bright things
    Settings.bOverride_BloomIntensity = true;
    Settings.BloomIntensity = 1.2f;

    // darker screen corners
    Settings.bOverride_VignetteIntensity = true;
    Settings.VignetteIntensity = 0.55f;

    // aim trainers should never blur when you flick
    Settings.bOverride_MotionBlurAmount = true;
    Settings.MotionBlurAmount = 0.0f;

    // a bit more color and contrast
    Settings.bOverride_ColorSaturation = true;
    Settings.ColorSaturation = FVector4(1.12f, 1.12f, 1.12f, 1.0f);
    Settings.bOverride_ColorContrast = true;
    Settings.ColorContrast = FVector4(1.06f, 1.06f, 1.06f, 1.0f);

    // tiny bit of lens color fringing at the edges
    Settings.bOverride_SceneFringeIntensity = true;
    Settings.SceneFringeIntensity = 0.25f;

    // keep the room moody instead of auto-brightening it to grey
    Settings.bOverride_AutoExposureBias = true;
    Settings.AutoExposureBias = -0.5f;
}

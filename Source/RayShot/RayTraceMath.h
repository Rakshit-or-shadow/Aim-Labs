// RayTraceMath.h
// Name - Rakshit Singh
//
// My ray-sphere intersection from my C ray tracer (src/spheres.c -> doesIntersect),
// ported to Unreal's FVector. This is the HIT DETECTION for the whole game:
// Unreal's own collision is turned off on the targets.
#pragma once

#include "CoreMinimal.h"

namespace RayTraceMath
{
    // Returns true if the ray hits the sphere.
    // OutT = how far along the ray the hit happened (in cm, if RayDir is normalized).
    inline bool DoesIntersect(const FVector& SphereCenter, double Radius,
                              const FVector& RayPos, const FVector& RayDir, double& OutT)
    {
        // vector from the sphere center to the ray start
        FVector V = RayPos - SphereCenter;

        // coefficients of the quadratic equation (same as my C code)
        double A = FVector::DotProduct(RayDir, RayDir);      // rayDir . rayDir
        double B = 2.0 * FVector::DotProduct(RayDir, V);     // 2 * (rayDir . V)
        double C = FVector::DotProduct(V, V) - Radius * Radius; // V . V - r^2

        // discriminant is negative -> no intersection
        double Discriminant = B * B - 4.0 * A * C;
        if (Discriminant < 0.0)
        {
            return false;
        }

        // the two possible solutions for t
        double SqrtD = FMath::Sqrt(Discriminant);
        double T1 = (-B - SqrtD) / (2.0 * A); // A > 0, so T1 is always the smaller one
        double T2 = (-B + SqrtD) / (2.0 * A);

        if (T1 >= 0.0) { OutT = T1; return true; } // closest hit is in front of us
        if (T2 >= 0.0) { OutT = T2; return true; } // we started inside the sphere
        return false;                               // sphere is behind the ray
    }
}

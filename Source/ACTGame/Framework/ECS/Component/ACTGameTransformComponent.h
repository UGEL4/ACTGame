#pragma once

namespace ACTGame
{
struct PositionComponent
{
    float x;
    float y;
    float z;
};

struct RotationComponent
{
    float x;
    float y;
    float z;
    float w;
};

struct ScaleComponent
{
    float x;
    float y;
    float z;
};

struct TransformComponent
{
    PositionComponent Position;
    RotationComponent Rotation;
    ScaleComponent    Scale;
};
} // namespace ACTGame

#pragma once

struct FVertexSimple
{
    float x, y, z;    // Position
    float r, g, b, a; // Color
};

enum class EPrimitiveType : unsigned char
{
    EPT_Triangle,
    EPT_Cube,
    EPT_Sphere,
    EPT_Max,
};

enum Direction
{
    Left,
    Right,
    Top,
    Bottom,
};
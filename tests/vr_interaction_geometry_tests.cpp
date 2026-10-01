#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include "../L4D2VR/sdk/vector.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "../L4D2VR/vr_interaction_geometry.h"
#include <cstdio>
#include <cstdlib>
#include <limits>

static void Check(bool condition, const char* message)
{
    if (!condition) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}
static bool Near(const Vector& a, const Vector& b) { return (a-b).LengthSqr() < 0.000001f; }

int main()
{
    using namespace l4d2vr_interaction;
    const Vector forward(1.0f,0.0f,0.0f), right(0.0f,-1.0f,0.0f), up(0.0f,0.0f,1.0f);
    Check(std::fabs(BodyYaw(Vector(0.0f,1.0f,0.0f),0.0f,90.0f,true)-90.0f)<0.001f,
        "A 90-degree stick turn must be applied once, not twice");
    Check(BodyYaw(Vector(0.0f,0.0f,-1.0f),90.0f,0.0f,true)==90.0f,
        "Looking down at ammo must retain the body's yaw");
    Check(BodyYaw(Vector(0.0f,0.0f,-1.0f),90.0f,45.0f,true)==135.0f,
        "Turning while looking down must still rotate the body once");
    Vector head(10.0f,20.0f,100.0f), offset(-0.1f,0.0f,-0.28f);
    const Vector body = BodyOrigin(head,forward,right,offset,40.0f);
    Check(Near(body,Vector(6.0f,20.0f,88.8f)),"Body origin must follow tracked head height");
    const Vector movedHead=head+Vector(20.0f,30.0f,10.0f);
    Check(Near(BodyOrigin(movedHead,forward,right,offset,40.0f)-body,Vector(20.0f,30.0f,10.0f)),
        "Room-scale head translation must carry the body reference with it");
    Vector local{}, moved{};
    Check(LocalHandPosition(Vector(8.0f,0.0f,0.0f),Vector(0.0f,0.0f,0.0f),forward,up,local),"Valid weapon basis must resolve");
    Check(LocalHandPosition(Vector(28.0f,30.0f,10.0f),Vector(20.0f,30.0f,10.0f),forward,up,moved)&&Near(local,moved),
        "Moving the gun and support hand together must not count as pumping");
    Check(LocalHandPosition(Vector(0.0f,8.0f,0.0f),Vector(0.0f,0.0f,0.0f),Vector(0.0f,1.0f,0.0f),up,moved)&&Near(local,moved),
        "Rotating the gun and support hand together must not count as pumping");
    Check(LocalHandPosition(Vector(5.0f,0.0f,0.0f),Vector(0.0f,0.0f,0.0f),forward,up,moved)&&local.x-moved.x==3.0f,
        "Rearward travel must be positive along the weapon's own axis");
    Check(ShellTouchesPort(Vector(0.0f,0.0f,0.0f),Vector(0.0f,0.0f,0.059f),0.06f),"A shell entering the loading port must be accepted");
    Check(!ShellTouchesPort(Vector(0.0f,0.0f,0.0f),Vector(0.0f,0.0f,0.061f),0.06f),"A shell outside the loading port must not add ammo");
    const float nan=std::numeric_limits<float>::quiet_NaN();
    Check(!LocalHandPosition(Vector(nan,0.0f,0.0f),head,forward,up,local),"Invalid tracking must not complete a stroke");
    Check(!ShellTouchesPort(head,head,nan),"Invalid port calibration must not load a shell");
}

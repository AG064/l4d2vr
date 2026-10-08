#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include "../L4D2VR/sdk/vector.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "../L4D2VR/vr_interaction_geometry.h"
#include "../L4D2VR/vr_body_inventory.h"
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
    l4d2vr_body_inventory::ModelPose bodyModel{};
    const Vector bodyOffset(-0.1f,0.0f,-0.28f);
    const Vector slot(0.23f,-0.22f,-0.4f);
    Vector fitted(0.0f,0.0f,0.0f), fitForward(0.0f,0.0f,0.0f), fitRight(0.0f,0.0f,0.0f);
    for (float eyeToPelvis : {0.5f,0.65f,0.8f})
    {
        const Vector pelvis=head-Vector(0.0f,0.0f,eyeToPelvis*40.0f);
        Check(l4d2vr_body_inventory::Capture(1u,2u,3u,head,pelvis,0.0f,0.0f,40.0f,100u,bodyModel),
            "A valid rendered pelvis can supply character-specific fitting");
        Check(l4d2vr_body_inventory::Resolve(bodyModel,1u,110u,head,bodyOffset,40.0f,0.0f,fitted,fitForward,fitRight),
            "A current local character sample resolves the on-body frame");
        const Vector waist=fitted+fitForward*(slot.x*40.0f)+fitRight*(slot.y*40.0f)+Vector(0.0f,0.0f,slot.z*40.0f);
        Check(std::fabs(waist.z-pelvis.z)<0.0001f,"Default waist height follows the rendered character pelvis");
        Vector movedFit(0.0f,0.0f,0.0f);
        Check(l4d2vr_body_inventory::Resolve(bodyModel,1u,120u,movedHead,bodyOffset,40.0f,0.0f,movedFit,fitForward,fitRight)&&
            Near(movedFit-fitted,movedHead-head),"Input/render head translation carries the fitted body coherently");
    }
    Check(l4d2vr_body_inventory::Resolve(bodyModel,1u,120u,head,bodyOffset,40.0f,90.0f,fitted,fitForward,fitRight)&&
        Near(fitForward,Vector(0.0f,1.0f,0.0f)),"A stick turn between model samples rotates the fitted body exactly once");
    const Vector configured=bodyOffset+Vector(0.0f,0.0f,0.1f);
    Vector adjusted(0.0f,0.0f,0.0f);
    Check(l4d2vr_body_inventory::Resolve(bodyModel,1u,120u,head,configured,40.0f,90.0f,adjusted,fitForward,fitRight)&&
        Near(adjusted-fitted,Vector(0.0f,0.0f,4.0f)),"Saved body-origin adjustment remains meaningful after pelvis fitting");
    Check(!l4d2vr_body_inventory::Resolve(bodyModel,4u,120u,head,bodyOffset,40.0f,0.0f,fitted,fitForward,fitRight),
        "A different player cannot reuse the previous body's pelvis");
    Check(!l4d2vr_body_inventory::Resolve(bodyModel,1u,401u,head,bodyOffset,40.0f,0.0f,fitted,fitForward,fitRight),
        "A stale rendered model falls back instead of freezing inventory at its old point");
    Check(!l4d2vr_body_inventory::Resolve(bodyModel,1u,99u,head,bodyOffset,40.0f,0.0f,fitted,fitForward,fitRight),
        "Clock reversal cannot validate a future model sample");
    Check(!l4d2vr_body_inventory::Capture(1u,2u,3u,head,head+Vector(0.0f,0.0f,10.0f),0.0f,0.0f,40.0f,100u,bodyModel),
        "A malformed rig with its pelvis above the head cannot move inventory");
    Check(!l4d2vr_body_inventory::Capture(1u,2u,3u,head,head+Vector(100.0f,0.0f,-20.0f),0.0f,0.0f,40.0f,100u,bodyModel),
        "An implausibly distant replacement pelvis uses fixed anchors");
    Check(!l4d2vr_body_inventory::Capture(1u,2u,3u,head,head-Vector(0.0f,0.0f,20.0f),nan,0.0f,40.0f,100u,bodyModel),
        "Invalid model orientation cannot contaminate the body sample");
    const Vector shiftedPelvis=head+Vector(4.0f,2.0f,-24.0f);
    Check(l4d2vr_body_inventory::Capture(1u,2u,4u,head,shiftedPelvis,90.0f,45.0f,40.0f,200u,bodyModel),
        "A replacement character can publish its own pelvis and body orientation");
    Check(l4d2vr_body_inventory::Resolve(bodyModel,1u,210u,head,bodyOffset,40.0f,45.0f,fitted,fitForward,fitRight)&&
        Near(fitted,shiftedPelvis+Vector(0.0f,-4.0f,16.0f)),"Rendered horizontal pelvis offset is retained in body space");
    Check(l4d2vr_body_inventory::Resolve(bodyModel,1u,220u,head,bodyOffset,40.0f,135.0f,adjusted,fitForward,fitRight)&&
        Near(fitForward,Vector(-1.0f,0.0f,0.0f)),"A later stick turn rebases the sampled model yaw without double rotation");
    Check(PointBoxDistance(Vector(0.0f,0.0f,0.0f),Vector(-2.0f,-1.0f,-1.0f),Vector(2.0f,1.0f,1.0f))==0.0f,
        "Touching an item's body must not require touching its origin");
    Check(PointBoxDistance(Vector(3.0f,0.0f,0.0f),Vector(-2.0f,-1.0f,-1.0f),Vector(2.0f,1.0f,1.0f))==1.0f,
        "Contact range must be measured from the item's surface");
    Check(!std::isfinite(PointBoxDistance(head,Vector(1.0f,0.0f,0.0f),Vector(-1.0f,1.0f,1.0f))),
        "Invalid item bounds must never offer grip pickup");
    Check(!LocalHandPosition(Vector(nan,0.0f,0.0f),head,forward,up,local),"Invalid tracking must not complete a stroke");
    Check(!ShellTouchesPort(head,head,nan),"Invalid port calibration must not load a shell");
}

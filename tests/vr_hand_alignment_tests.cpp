#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include "../L4D2VR/vr_hands/vr_hand_math.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include <cstdio>
#include <cstdlib>
static void Check(bool x) { if (!x) { std::fprintf(stderr,"Hand anchor regression failed\n"); std::abort(); } }
int main()
{
    const Vector p(30.0f,50.0f,80.0f), offset(0.0f,-0.08f,0.1f), rotation(0.0f,180.0f,0.0f);
    const QAngle angle(20.0f,75.0f,40.0f);
    const float units=43.2f;
    const auto rendered=VrHandMath::BuildControllerWorld(p,angle,units,1.2f,offset,rotation);
    const auto anchor=VrHandMath::BuildControllerWorld(p,angle,units,1.0f/units,offset,rotation);
    for (int row=0;row<3;++row)
    {
        Check(std::fabs(VrHandMath::Get(rendered,row,3)-VrHandMath::Get(anchor,row,3))<0.0001f);
        for (int col=0;col<3;++col)
            Check(std::fabs(VrHandMath::Get(rendered,row,col)/(units*1.2f)-VrHandMath::Get(anchor,row,col))<0.0001f);
    }
    // The same local palm point must land at the same world position in the
    // scaled glove mesh and in the unscaled interaction anchor.
    for (int row=0;row<3;++row)
        Check(std::fabs(VrHandMath::Get(rendered,row,3)+VrHandMath::Get(rendered,row,2)*0.07f/1.2f-
            VrHandMath::Get(anchor,row,3)-VrHandMath::Get(anchor,row,2)*0.07f*units)<0.0001f);
}

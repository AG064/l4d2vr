#include "../L4D2VR/vr_dual_pistols.h"
#include <cstdio>
#include <cstdlib>
#include <limits>

#define CHECK(value) do { if (!(value)) { std::fprintf(stderr,"Failed at line %d\n",__LINE__);std::abort(); } } while(false)
struct Matrix { float m[3][4]{}; };
static Matrix Pose(float x, float y) { Matrix p{}; p.m[0][0]=p.m[1][1]=p.m[2][2]=1; p.m[0][3]=x; p.m[1][3]=y; return p; }

int main()
{
    using namespace l4d2vr_dual;
    ReloadPulse reload;
    CHECK(!reload.Update(true,false,100u));
    CHECK(reload.Update(true,true,200u));
    CHECK(reload.Update(true,false,549u));
    CHECK(!reload.Update(true,false,550u));
    CHECK(reload.Update(true,true,0xfffffff0u));
    CHECK(reload.Update(true,false,20u));
    CHECK(!reload.Update(false,false,30u));
    CHECK(IsShotMarker(kLeftShotMarker,true,true));
    CHECK(IsShotMarker(kRightShotMarker,true,true));
    CHECK(!IsShotMarker(kLeftShotMarker,false,true));
    CHECK(!IsShotMarker(kLeftShotMarker,true,false));
    CHECK(!IsShotMarker(2,true,true)); // ordinary pistol inventory-drop marker
    TriggerRouter router;
    CHECK(router.Update(true,false,true,30)==Hand::Left);
    CHECK(router.Update(true,false,true,29)==Hand::Left);
    CHECK(router.Update(true,true,true,29)==Hand::None);
    CHECK(router.Update(true,true,true,29)==Hand::Right);
    CHECK(router.Update(true,true,true,28)==Hand::None);
    CHECK(router.Update(true,true,true,28)==Hand::Left);
    CHECK(router.Update(false,true,true,28)==Hand::None);
    CHECK(router.Update(true,false,false,28)==Hand::None);
    CHECK(router.Update(true,true,false,28)==Hand::Right);

    CommandShots cache;
    Shot left{101,Hand::Left,{-5,2,3},{0,90,0}};
    Shot right{102,Hand::Right,{5,2,3},{0,-90,0}};
    cache.Store(left); cache.Store(right);
    Shot copy{};
    CHECK(cache.Get(101,copy) && copy.hand==Hand::Left && copy.angles[1]==90);
    CHECK(cache.Get(102,copy) && copy.hand==Hand::Right && copy.angles[1]==-90);
    CHECK(cache.Get(101,copy) && copy.position[0]==-5); // backup retransmission
    CHECK(!cache.Get(103,copy));
    cache.Store({251,Hand::None,{},{}});
    CHECK(!cache.Get(101,copy));
    cache.Store({103,Hand::Left,{std::numeric_limits<float>::infinity(),0,0},{}});
    CHECK(!cache.Get(103,copy));

    Matrix rightHand=Pose(10,0), leftHand=Pose(-10,0), rightGun=Pose(11,0), oldLeft=Pose(12,5), delta{};
    const Matrix frame=ControllerFrame<Matrix>({0,0,0},{1,0,0},{0,-1,0},{0,0,1});
    CHECK(frame.m[1][0]==-1 && frame.m[2][1]==1 && frame.m[0][2]==-1);
    CHECK(Retarget(rightHand,leftHand,rightGun,oldLeft,delta));
    Matrix target=Multiply(delta,oldLeft);
    CHECK(std::fabs(target.m[0][3]+9)<0.0001f && std::fabs(target.m[1][3])<0.0001f);
    leftHand.m[0][0]=0; leftHand.m[0][1]=-1; leftHand.m[1][0]=1; leftHand.m[1][1]=0;
    CHECK(Retarget(rightHand,leftHand,rightGun,oldLeft,delta));
    target=Multiply(delta,oldLeft);
    CHECK(std::fabs(target.m[0][3]+10)<0.0001f && std::fabs(target.m[1][3]-1)<0.0001f);
    Matrix singular{};
    CHECK(!Retarget(singular,leftHand,rightGun,oldLeft,delta));
}

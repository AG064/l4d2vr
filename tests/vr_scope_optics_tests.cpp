#include <limits>
#if defined(VECTOR_PARANOIA) && !defined(VEC_T_NAN)
#define VEC_T_NAN std::numeric_limits<float>::quiet_NaN()
#endif
#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include "../L4D2VR/sdk/vector.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "../L4D2VR/vr_scope_optics.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
#define CHECK(v) do { if (!(v)) { std::fprintf(stderr, "Scope optics failed at %d\n", __LINE__); std::abort(); } } while(false)
int main()
{
    using namespace l4d2vr_optics;
    WeaponTracker weapon;
    CHECK(weapon.Observe(10u, 0x3005u, 35));
    const auto first = weapon.Current();
    CHECK(first.Valid() && !weapon.Observe(10u, 0x3005u, 35));
    ImageStamp image{first, 2.0f, 0u, 7u};
    CHECK(!ImageMatches(image, first, 2.0f, 7u, 100u)); // Submission is not completion.
    image.completedAt = 100u;
    CHECK(ImageMatches(image, first, 2.0f, 7u, 150u));
    CHECK(!ImageMatches(image, first, 4.0f, 7u, 150u)); // Old zoom cannot label a new view.
    CHECK(!ImageMatches(image, first, 2.0f, 8u, 150u)); // Device/queue lifecycle changed.
    CHECK(!ImageMatches(image, first, 2.0f, 7u, 201u));
    CHECK(!ImageMatches(image, first, 2.0f, 7u, 99u));
    CHECK(weapon.Observe(10u, 0x4005u, 35));
    CHECK(!ImageMatches(image, weapon.Current(), 2.0f, 7u, 150u)); // Same type, different gun.
    CHECK(weapon.Observe(10u, 0x3005u, 1));
    CHECK(!weapon.Current().Valid());
    CHECK(weapon.Observe(10u, 0x3005u, 35));
    CHECK(!ImageMatches(image, weapon.Current(), 2.0f, 7u, 150u)); // Drawing it again needs a new render.
    CHECK(weapon.Observe(20u, 0x3005u, 35));
    CHECK(!ImageMatches(image, weapon.Current(), 2.0f, 7u, 150u));
    image = {weapon.Current(), 4.0f, 160u, 8u};
    CHECK(ImageMatches(image, weapon.Current(), 4.0f, 8u, 180u));
    CHECK(!ImageMatches(image, weapon.Current(), std::numeric_limits<float>::quiet_NaN(), 8u, 180u));
    CHECK(weapon.Observe(20u, 0xffffffffu, 35));
    CHECK(!weapon.Current().Valid());
    for (int id = 0; id < 50; ++id)
        CHECK(HasOptic(id) == (id == 6 || id == 10 || id == 34 || id == 35 || id == 36));
    const Vector lens(10.0f, 0.0f, 0.0f), axis(1.0f, 0.0f, 0.0f);
    CHECK(EvaluateEye(Vector(0,0,0), lens, axis, 1.0f, 100.0f, 4.0f).alpha == 1.0f);
    CHECK(EvaluateEye(Vector(0,1,0), lens, axis, 1.0f, 100.0f, 4.0f).alpha == 0.0f);
    CHECK(EvaluateEye(Vector(11,0,0), lens, axis, 1.0f, 100.0f, 4.0f).alpha == 0.0f);
    CHECK(EvaluateEye(Vector(9,0,0), lens, axis, 1.0f, 100.0f, 4.0f).alpha == 0.0f);
    CHECK(EvaluateEye(Vector(-20,0,0), lens, axis, 1.0f, 100.0f, 4.0f).alpha == 0.0f);
    CHECK(EvaluateEye(Vector(0,0,0), lens, Vector(0,0,0), 1.0f, 100.0f, 4.0f).alpha == 0.0f);
    const float nan = std::numeric_limits<float>::quiet_NaN();
    Vector invalidEye(0,0,0); invalidEye.x = nan;
    CHECK(EvaluateEye(invalidEye, lens, axis, 1,100,4).alpha == 0.0f);
    CHECK(RenderFov(1,10,nan) == 0.0f);
    for (const float relief : {4.0f, 8.0f, 15.0f})
        for (const float magnification : {1.0f, 2.0f, 4.0f, 8.0f})
        {
            const float fov = RenderFov(1.0f, relief, magnification);
            const float recovered = 1.0f / (relief * std::tan(fov * 3.14159265358979323846f / 360.0f));
            CHECK(std::fabs(recovered - magnification) < 0.001f);
        }
    CHECK(Magnification(20,20,2) == 2.0f);
    CHECK(Magnification(10,20,2) > 4.0f);
    CHECK(Magnification(3,20,2) > 10.0f);
    CHECK(FindModel("models/v_models/v_snip_awp.mdl",1241414224u));
    CHECK(!FindModel("models/v_models/v_snip_awp.mdl",1u));
    CHECK(!FindModel("models/v_models/custom.mdl",1241414224u));
    CHECK(Fresh(100u,50u));
    CHECK(!Fresh(151u,50u));
    CHECK(!Fresh(49u,50u));
    CHECK(!Fresh(100u,0u));
    const Pose previous{Vector(0,0,0),Vector(1,0,0),Vector(0,-1,0),Vector(0,0,1)};
    const Pose current{Vector(5,3,0),Vector(0,1,0),Vector(1,0,0),Vector(0,0,1)};
    Vector rebased(0,0,0);
    CHECK(RebasePoint(lens,previous,current,rebased));
    CHECK((rebased-Vector(5,13,0)).Length() < 0.001f);
    CHECK(RebaseVector(axis,previous,current,rebased));
    CHECK((rebased-Vector(0,1,0)).Length() < 0.001f);
    CHECK(!RebasePoint(invalidEye,previous,current,rebased));
    std::puts("Physical optic magnification, eye relief and model identity checks passed");
}

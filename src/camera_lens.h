/* BORE 2 PS1 third-person perspective contract.
 *
 * The previous fixed 230-pixel focal length on a 320-pixel screen is a
 * 69.6-degree horizontal field of view. Near rails, stairs and buildings
 * balloon towards the edges of the picture. A 260-pixel lens is 63.2
 * degrees; moving the orbit camera back by the same ratio preserves the
 * Sim/house at the focus while reducing near/far size exaggeration.
 *
 * This is strictly an optical/camera change: world coordinates, geometry,
 * riding physics, texture UVs, 2D HUD, picking and saves are unchanged.
 * PS1 first-person keeps the established lens/near-clip behavior. On
 * Dreamcast keep its existing camera and GTE-compatible lens until its
 * own native screenshot tests are available.
 */
#ifndef BORE_CAMERA_LENS_H
#define BORE_CAMERA_LENS_H
#define BORE_CAMERA_BASE_FOCAL 230
#define BORE_CAMERA_ORBIT_FOCAL 260
static inline int bore_camera_focal(int first_person)
{
#if defined(BORE_DC) || defined(BORE_LENS_LEGACY)
    /* A/B-only: reproduce the exact original 230 px orbit. Never set
     * this in release builds; use it for pixel- and FPS-matched captures. */
    (void)first_person;
    return BORE_CAMERA_BASE_FOCAL;
#else
    return first_person ? BORE_CAMERA_BASE_FOCAL : BORE_CAMERA_ORBIT_FOCAL;
#endif
}
static inline int bore_camera_distance(int distance, int focal)
{
    /* Guard the SDK's 16-bit camera coordinates and avoid float maths on
     * the 33 MHz MIPS CPU. Distance is clamped by camera_apply first. */
    if (focal <= 0) focal = BORE_CAMERA_BASE_FOCAL;
    return (distance * focal + BORE_CAMERA_BASE_FOCAL / 2) /
           BORE_CAMERA_BASE_FOCAL;
}

// Portable signed 8.8 follow, preserving subunit position in both directions.
// The local map bounds keep products in int range; never left-shift negatives.
static inline int bore_camera_floor256(int v){
    return v/256-(v<0&&v%256!=0);
}
static inline int bore_camera_follow_axis(int current,int *acc,int target,int steps,int gain){
    if(current!=bore_camera_floor256(*acc))*acc=current*256;
    int goal=target*256;
    for(int i=0;i<steps;i++)*acc+=((goal-*acc)/16)*gain/16;
    int diff=goal-*acc;
    if(diff>-384&&diff<384)*acc=goal;
    return bore_camera_floor256(*acc);
}
static inline void bore_camera_rebase_axis(int current,int *acc,int delta){
    if(current==bore_camera_floor256(*acc))*acc+=delta*256;
}
#endif

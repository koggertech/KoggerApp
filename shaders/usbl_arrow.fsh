#version 140

uniform vec4 color;
// Compass bearing in RADIANS: 0 points north (up), increasing turns clockwise.
uniform float yaw;
// Wing half-span, in sprite radii. The dart's length is fixed; this is what makes it narrow.
uniform float halfWidth;

void main()
{
    vec2 coord = gl_PointCoord * 2.0 - 1.0;

    // gl_PointCoord's origin is UPPER-LEFT, so its y grows DOWNWARD. Flipping it here puts the
    // sprite in a screen-up frame, which is the whole reason `yaw` below can be read as a
    // bearing rather than as an arbitrary angle with an arbitrary sign.
    vec2 d = vec2(coord.x, -coord.y);

    // Into the dart's own frame, nose at +y. yaw = 0 is the identity, so the nose points up =
    // north; yaw = pi/2 sends it to screen-right = east.
    float c = cos(yaw);
    float s = sin(yaw);
    vec2 q = vec2(d.x * c - d.y * s, d.x * s + d.y * c);

    // A paper dart: nose forward, two swept-back wings, a notch cut into the tail. The notch is
    // what stops it reading as a plain triangle -- and a triangle this size reads as a generic
    // marker rather than as a heading.
    const float kNose  = 1.0;
    const float kBack  = 0.85;   // wing corners, behind the centre
    const float kNotch = 0.25;   // how far forward the tail cut reaches

    float w = max(halfWidth, 0.001);
    float ax = abs(q.x);

    // Outer edge: taper from the nose back to the wing corner at (+-w, -kBack).
    float halfAt = w * (kNose - q.y) / (kNose + kBack);
    // Notch edge: from the tail cut at (0, -kNotch) out to that same corner.
    float notchAt = -kNotch - ax * (kBack - kNotch) / w;

    if (q.y > kNose || q.y < -kBack || ax > halfAt || q.y < notchAt) {
        discard;
    }

    gl_FragColor = color;
}

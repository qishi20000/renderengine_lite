#version 300 es
precision mediump float;

// Deliberately not PBR/IBL (see ARCHITECTURE.md section 12) — a single
// directional light + fixed ambient term is indistinguishable from more
// expensive lighting for a small car-model overlay at AVM viewing distance
// and angle, and costs far less on Adreno.

uniform sampler2D u_albedo;
uniform vec4 u_lightDirWS;   // xyz = normalized direction, w unused
uniform vec4 u_ambientColor; // rgb = ambient, a unused

in vec3 v_worldNormal;
in vec2 v_uv;

out vec4 fragColor;

void main() {
    vec3 n = normalize(v_worldNormal);
    float ndotl = max(dot(n, -u_lightDirWS.xyz), 0.0);
    vec4 albedo = texture(u_albedo, v_uv);
    vec3 color = albedo.rgb * (u_ambientColor.rgb + ndotl);
    fragColor = vec4(color, albedo.a);
}

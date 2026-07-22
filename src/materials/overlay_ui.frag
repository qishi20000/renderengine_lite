#version 300 es
precision mediump float;

uniform sampler2D u_overlayTex;
uniform vec4 u_tintColor; // multiplicative tint, e.g. distance-based guide-line coloring

in vec2 v_uv;
out vec4 fragColor;

void main() {
    fragColor = texture(u_overlayTex, v_uv) * u_tintColor;
}

@include hybrid_common.glsl
@vs foliage_vs
layout(binding=3) uniform foliage_params {
    vec4 foliage_rect;
    vec4 foliage_uv;
    vec4 mask_uv;
    vec4 foliage_mode;
    vec4 foliage_tint;
    vec4 foliage_sun;
    vec4 foliage_breeze;
    vec4 foliage_fire;
};
flat out vec4 scene_tint;
flat out vec4 sun_light;
flat out vec4 breeze_state;
flat out vec4 local_fire;
out vec2 world_pos;
out vec2 palm_uv;
out vec2 original_uv;
flat out float mode;
out vec2 leaf_local;
void main() {
    const vec2 corners[6]=vec2[6](vec2(0,0),vec2(1,0),vec2(0,1),vec2(0,1),vec2(1,0),vec2(1,1));
    vec2 p=corners[gl_VertexIndex];
    vec2 pos=foliage_rect.xy+p*foliage_rect.zw;
    gl_Position=vec4(pos.x*2.0/1280.0-1.0,1.0-pos.y*2.0/720.0,0,1);
    p=mix(p,1.0-p,foliage_mode.yz);
    palm_uv=foliage_uv.xy+p*foliage_uv.zw;
    original_uv=mask_uv.xy+p*mask_uv.zw;
    breeze_state=foliage_breeze;local_fire=foliage_fire;world_pos=pos;
    mode=foliage_mode.x;leaf_local=p;scene_tint=foliage_tint;sun_light=foliage_sun;
}
@end
@fs foliage_fs
layout(binding=4) uniform texture2D palm_sheet;
layout(binding=0) uniform texture2D original_mask;
layout(binding=3) uniform sampler foliage_sampler;
layout(binding=0) uniform sampler mask_sampler;
flat in vec4 scene_tint;
flat in vec4 sun_light;
flat in vec4 breeze_state;
flat in vec4 local_fire;
in vec2 world_pos;
in vec2 palm_uv;
in vec2 original_uv;
flat in float mode;
in vec2 leaf_local;
out vec4 frag_color;
@include_block hybrid_lighting
vec3 foliage_lit(vec3 color,vec2 normal) {
    vec2 d=world_pos-local_fire.xy;
    return hybrid_lit(color,normal)+vec3(.20,.064,.008)*exp(-dot(d,d)/max(local_fire.w*local_fire.w,.01))*local_fire.z;
}
void main() {
    if(mode<.5) {
        if(texture(sampler2D(original_mask,mask_sampler),original_uv).r<.001)discard;
        // Wrap the complete bark across each curved mask row. Sampling only
        // the centre strip flattened the trunk into horizontal bands.
        float start=original_uv.x-leaf_local.x*24.0/2048.0;
        float left=24.0,right=0.0;
        for(int i=0;i<24;i++) {
            float a=texture(sampler2D(original_mask,mask_sampler),vec2(start+(float(i)+.5)/2048.0,original_uv.y)).r;
            if(a>.001) { left=min(left,float(i));right=max(right,float(i)); }
        }
        float t=clamp((leaf_local.x*24.0-left)/max(1.0,right-left+1.0),0.0,1.0);
        float bx=mix(mix(489.0,460.0,leaf_local.y),mix(533.0,564.0,leaf_local.y),t);
        vec3 bark=texture(sampler2D(palm_sheet,foliage_sampler),vec2(bx/1024.0,palm_uv.y)).rgb;
        frag_color=vec4(foliage_lit(bark,vec2(2.0*t-1.0,0)),1);
    } else if(mode<1.5) {
        // Join the leaning trunk TOP (445,148), not its foot (455,293).
        vec2 leaf=leaf_local;
        float bend=clamp(dot(leaf-vec2(79.0/152.0,26.0/69.0),leaf-vec2(79.0/152.0,26.0/69.0))*3.0,0.0,1.0);
        float edge=smoothstep(0.0,.07,leaf.x)*smoothstep(0.0,.07,1.0-leaf.x)*smoothstep(0.0,.07,leaf.y)*smoothstep(0.0,.07,1.0-leaf.y);
        float phase=6.28318530718*breeze_state.x/4096.0;
        leaf+=vec2(sin(phase*521.0+leaf.x*3.0)*1.8/152.0,sin(phase*313.0+leaf.x*5.0)*.75/69.0)*bend*edge*breeze_state.y;
        float pivot=79.0/152.0;
        float u=leaf.x<pivot ? leaf.x*.5/pivot : .5+(leaf.x-pivot)*.5/(1.0-pivot);
        float py=26.0/69.0;
        float hub=(380.0-15.0)/745.0;
        float v=leaf.y<py ? leaf.y*hub/py : hub+(leaf.y-py)*(1.0-hub)/(1.0-py);
        vec2 raw=vec2(u*1024.0,15.0+v*745.0);
        // The atlas contains a whole tree: exclude its painted stem from the
        // crown layer. The registered original mask supplies the only trunk.
        if(raw.y>460.0 && raw.x>485.0 && raw.x<541.0)discard;
        vec4 c=texture(sampler2D(palm_sheet,foliage_sampler),raw/vec2(1024,1536));
        if(c.a<.5)discard;
        frag_color=vec4(foliage_lit(c.rgb,vec2(0)),1);
    } else {
        if(texture(sampler2D(original_mask,mask_sampler),original_uv).r<.001)discard;
        frag_color=vec4(.035,.028,.016,.15);
    }
}
@end
@program foliage foliage_vs foliage_fs

@include hybrid_common.glsl
@vs character_vs
layout(binding=2) uniform character_params {
    vec4 hd_rect;
    vec4 hd_uv;
    vec4 hd_canvas_flip;
    vec4 hd_light;
    vec4 hd_tint;
    vec4 hd_sun;
    vec4 hd_projection;
};
out vec2 world_pos;
flat out vec4 scene_tint;
flat out vec4 sun_light;
flat out vec4 cell_uv;
flat out vec2 mirror_sign;
out vec2 sheet_uv;
out vec2 local_uv;
flat out vec4 lighting;
void main() {
    const vec2 corners[6]=vec2[6](vec2(0,0),vec2(1,0),vec2(0,1),vec2(0,1),vec2(1,0),vec2(1,1));
    vec2 p=corners[gl_VertexIndex];
    vec2 pos=hd_rect.xy+p*hd_rect.zw;
    if(hd_light.w>.5 && hd_light.w<1.5)pos=vec2(pos.x+hd_projection.x*(1.0-p.y)*hd_rect.w,hd_rect.y+hd_rect.w+(1.0-p.y)*hd_rect.w*.35);
    world_pos=pos;scene_tint=hd_tint;sun_light=hd_sun;cell_uv=hd_uv;mirror_sign=mix(vec2(1),vec2(-1),hd_canvas_flip.zw);
    gl_Position=vec4(pos.x*2.0/hd_canvas_flip.x-1.0,1.0-pos.y*2.0/hd_canvas_flip.y,0,1);
    sheet_uv=hd_uv.xy+vec2(mix(p.x,1.0-p.x,hd_canvas_flip.z),mix(p.y,1.0-p.y,hd_canvas_flip.w))*hd_uv.zw;
    local_uv=p;lighting=hd_light;
}
@end
@fs character_fs
layout(binding=3) uniform texture2D character_sheet;
layout(binding=2) uniform sampler character_sampler;
layout(binding=2) uniform texture2D background_plate;
layout(binding=1) uniform sampler linear_sampler;
in vec2 world_pos;
flat in vec4 scene_tint;
flat in vec4 sun_light;
flat in vec4 cell_uv;
flat in vec2 mirror_sign;
in vec2 sheet_uv;
in vec2 local_uv;
flat in vec4 lighting;
out vec4 frag_color;
@include_block hybrid_lighting
@include_block hybrid_ground
float covered(vec2 p) {
    if(any(lessThan(p,cell_uv.xy)) || any(greaterThanEqual(p,cell_uv.xy+cell_uv.zw)))return 0.0;
    return step(.5,texture(sampler2D(character_sheet,character_sampler),p).a);
}
float segment_distance(vec2 p,vec2 a,vec2 b) {
    vec2 ab=b-a;
    return length(p-a-ab*clamp(dot(p-a,ab)/dot(ab,ab),0.0,1.0));
}
float cubic_distance(vec2 p,vec2 a,vec2 b,vec2 c,vec2 d) {
    float dist=10.0;vec2 previous=a;
    for(int i=1;i<=16;i++) {
        float t=float(i)/16.0;float u=1.0-t;
        vec2 q=u*u*u*a+3.0*u*u*t*b+3.0*u*t*t*c+t*t*t*d;
        dist=min(dist,segment_distance(p,previous,q));previous=q;
    }
    return dist;
}
void main() {
    if(lighting.w>1.5) {
        // Vector punctuation: flat colors, clean outlines, pixel coverage only.
        vec2 p=(sheet_uv-.5)*vec2(lighting.x,1.0)+.5;
        float d;
        if(lighting.w<2.5) {
            d=cubic_distance(p,vec2(.27,.27),vec2(.27,.04),vec2(.75,.04),vec2(.75,.29));
            d=min(d,cubic_distance(p,vec2(.75,.29),vec2(.77,.46),vec2(.48,.47),vec2(.48,.64)))-.055;
            d=min(d,segment_distance(p,vec2(.48,.63),vec2(.48,.68))-.055);
            d=min(d,length(p-vec2(.48,.87))-.06);
        } else {
            d=segment_distance(p,vec2(.5,.10),vec2(.5,.59))-.075;
            d=min(d,length(p-vec2(.5,.83))-.075);
        }
        float aa=max(fwidth(d),.0001);
        vec3 ink=vec3(.12,.09,.09),rim=vec3(1.0,.96,.86),fill=vec3(.95,.23,.18);
        vec3 color=mix(fill,rim,smoothstep(-aa,aa,d));
        color=mix(color,ink,smoothstep(.027-aa,.027+aa,d));
        float alpha=1.0-smoothstep(.050-aa,.050+aa,d);
        if(lighting.w>2.5) {
            float rays=segment_distance(p,vec2(.25,.80),vec2(.04,.66));
            rays=min(rays,segment_distance(p,vec2(.19,.93),vec2(-.13,.90)));
            rays=min(rays,segment_distance(p,vec2(.75,.80),vec2(.96,.66)));
            rays=min(rays,segment_distance(p,vec2(.81,.93),vec2(1.13,.90)))-.012;
            float a=1.0-smoothstep(-aa,aa,rays);
            if(a>alpha){alpha=a;color=rim;}
        }
        if(alpha<.001)discard;
        frag_color=vec4(color,alpha);
    } else if(lighting.w>0.5) {
        vec2 d=max(fwidth(sheet_uv),vec2(1.0)/vec2(1536,1024));
        float alpha=covered(sheet_uv);
        float soft=(covered(sheet_uv+vec2(d.x,0))+covered(sheet_uv-vec2(d.x,0))+covered(sheet_uv+vec2(0,d.y))+covered(sheet_uv-vec2(0,d.y)))*.25;
        alpha=mix(mix(alpha,soft,.6),alpha,sun_light.z);
        frag_color=vec4(vec3(15,20,35)/255.0,alpha*sun_light.w*ground_mask(world_pos));
    } else {
        vec4 tex=texture(sampler2D(character_sheet,character_sampler),sheet_uv);
        // Cutout removes the generated sheet's faint translucent background halo.
        if(tex.a<.5)discard;
        vec2 d=max(fwidth(sheet_uv),vec2(1.0)/vec2(1536,1024));
        vec2 normal=vec2(covered(sheet_uv-vec2(d.x,0))-covered(sheet_uv+vec2(d.x,0)),covered(sheet_uv-vec2(0,d.y))-covered(sheet_uv+vec2(0,d.y)));
        frag_color=vec4(hybrid_lit(tex.rgb,normal*mirror_sign),1.0);
    }
}
@end
@program character character_vs character_fs

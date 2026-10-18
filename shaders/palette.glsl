@include hybrid_common.glsl
@vs quad_vs
layout(binding=0) uniform draw_params {
    vec4 rect;
    vec4 uv_rect;
    vec4 flags;
    vec4 canvas;
    vec4 tint;
    vec4 sun;
    vec4 projection;
    vec4 segment;
    vec4 material;
    vec4 breeze; vec4 fire_light; vec4 decor;
};
out vec2 uv;
out vec2 pixel_local;
out vec2 world_pos;
flat out vec4 sprite_bounds;
flat out vec4 options;
flat out vec4 scene_tint;
flat out vec4 sun_light;
flat out vec4 stroke_segment;
flat out vec4 sprite_material;
flat out vec4 sprite_breeze;
flat out vec4 local_fire;
flat out vec4 sprite_decor;
void main() {
    const vec2 corners[6]=vec2[6](vec2(0,0),vec2(1,0),vec2(0,1),vec2(0,1),vec2(1,0),vec2(1,1));
    vec2 p=corners[gl_VertexIndex];
    vec2 pos=rect.xy+p*rect.zw;
    if(flags.w>1.5 && flags.w<2.5) {
        // A flat body already lies in the ground plane: its contact shadow
        // follows the silhouette instead of anchoring the back to a lower hand.
        if(projection.z>1.5)pos.x+=projection.x*(pos.y-segment.y);
        else if(projection.z>.5)pos.y+=1.25;
        else pos=vec2(pos.x+projection.x*(projection.y-p.y)*rect.w,rect.y+projection.y*rect.w+(projection.y-p.y)*rect.w*.35);
    }
    gl_Position=vec4(pos.x*2.0/canvas.x-1.0,1.0-pos.y*2.0/canvas.y,0,1);
    vec2 tp=mix(p,1.0-p,flags.xy);
    uv=uv_rect.xy+tp*uv_rect.zw;
    pixel_local=tp*uv_rect.zw*vec2(2048,4096);
    sprite_bounds=uv_rect*vec4(2048,4096,2048,4096);
    stroke_segment=segment;sprite_material=material;
    sprite_breeze=breeze;local_fire=fire_light;sprite_decor=decor;
    options=flags;scene_tint=tint;sun_light=sun;world_pos=pos;
}
@end
@fs palette_fs
layout(binding=0) uniform texture2D atlas;
layout(binding=1) uniform texture2D palette;
layout(binding=2) uniform texture2D background_plate;
layout(binding=5) uniform texture2D environment_mask;
layout(binding=0) uniform sampler nearest_sampler;
layout(binding=1) uniform sampler linear_sampler;
in vec2 uv;
in vec2 pixel_local;
in vec2 world_pos;
flat in vec4 sprite_bounds;
flat in vec4 options;
flat in vec4 scene_tint;
flat in vec4 sun_light;
flat in vec4 stroke_segment;
flat in vec4 sprite_material;
flat in vec4 sprite_breeze;
flat in vec4 local_fire;
flat in vec4 sprite_decor;
out vec4 frag_color;
@include_block hybrid_lighting
@include_block hybrid_ground
float sprite_ground(vec2 world) {
    if(sprite_decor.x>.5)return texture(sampler2D(environment_mask,linear_sampler),world/vec2(1280,720)).g;
    return ground_mask(world);
}
int at(ivec2 p) {
    p=clamp(p,ivec2(0),ivec2(sprite_bounds.zw)-1);
    return int(round(texelFetch(sampler2D(atlas,nearest_sampler),ivec2(sprite_bounds.xy)+p,0).r*255.0));
}
vec4 entry(int i) {
    vec4 c=texelFetch(sampler2D(palette,nearest_sampler),ivec2(i,0),0);
    // Only identified Johnny resources receive the richer character material.
    if(sprite_material.y>.5) {
        if(i==5)c.rgb=vec3(.105,.065,.042); // beard and ink, deep warm umber
        if(i==15)c.rgb=vec3(.99,.97,.92);  // luminous cap and white shorts
        if(i==7)c.rgb=vec3(.81,.78,.72);
    }
    c.a=(options.z>.5 && i==0)?0.0:1.0;
    return vec4(c.rgb*c.a,c.a);
}
// LUT entries are encoded sRGB in an UNORM texture. Average in linear light,
// then re-encode for the existing encoded-color display pipeline.
vec3 linear_color(vec3 c) {
    return mix(c/12.92,pow((c+.055)/1.055,vec3(2.4)),step(vec3(.04045),c));
}
vec3 encoded_color(vec3 c) {
    return mix(c*12.92,1.055*pow(max(c,vec3(0)),vec3(1.0/2.4))-.055,step(vec3(.0031308),c));
}
vec4 fused(int a,int b) {
    vec4 ca=entry(a),cb=entry(b);
    vec3 color=encoded_color((linear_color(ca.rgb)+linear_color(cb.rgb))*.5);
    // Original red/white skin is encoded as a checker, not white cloth.
    // Grade the recovered material, retaining the mathematically linear mix.
    bool skin=(a==12 && b==15)||(a==15 && b==12)||(a==4 && b==15)||(a==15 && b==4);
    if(sprite_material.y>.5 && skin)color=clamp(color*vec3(1.04,.86,.70)+vec3(.012,.006,0),0.0,1.0);
    return vec4(color,1);
}
vec4 corner_entry(int index,vec4 base,bool fused_material) {
    vec4 n=entry(index);
    // Smooth coverage without reintroducing an unfused white corner.
    if(fused_material)n.rgb=base.rgb/max(base.a,.001)*n.a;
    return n;
}
bool skin_pair(int a,int b) {
    return ((a==12 || a==4) && b==15)||((b==12 || b==4) && a==15);
}
bool checker(int e,int diagonal,int side1,int side2) {
    return e==diagonal && side1==side2 && e!=side1 && entry(e).a>.99 && entry(side1).a>.99;
}
void main() {
    if(options.w<.5) {
        int i=int(round(texture(sampler2D(atlas,nearest_sampler),uv).r*255.0));
        if(options.z>.5 && i==0)discard;
        frag_color=texelFetch(sampler2D(palette,nearest_sampler),ivec2(i,0),0);
        return;
    }
    if(options.w>2.5) {
        vec2 ab=stroke_segment.zw-stroke_segment.xy;
        float t=clamp(dot(world_pos-stroke_segment.xy,ab)/max(dot(ab,ab),.0001),0.0,1.0);
        float dist=length(world_pos-stroke_segment.xy-t*ab);
        float pixel=max(length(dFdx(world_pos)),length(dFdy(world_pos)));
        float alpha=1.0-smoothstep(pixel*.20,pixel*.95,dist);
        if(alpha<.001)discard;
        frag_color=vec4(hybrid_lit(entry(int(sprite_material.z)).rgb,vec2(0)),alpha);
        return;
    }
    vec2 local=pixel_local;
    if(sprite_breeze.y>.0) {
        vec2 p=local/sprite_bounds.zw,hub=sprite_breeze.zw;
        float bend=clamp(dot(p-hub,p-hub)*3.0,0.0,1.0);
        float edge=smoothstep(0.0,.07,p.x)*smoothstep(0.0,.07,1.0-p.x)*smoothstep(0.0,.07,p.y)*smoothstep(0.0,.07,1.0-p.y);
        float phase=6.28318530718*sprite_breeze.x/4096.0;
        local+=vec2(sin(phase*521.0+p.x*3.0)*1.8,sin(phase*313.0+p.x*5.0)*.75)*bend*edge*sprite_breeze.y;
    }
    ivec2 q=ivec2(floor(local));vec2 f=fract(local);
    int A=at(q+ivec2(-1,-1)),B=at(q+ivec2(0,-1)),C=at(q+ivec2(1,-1));
    int D=at(q+ivec2(-1,0)),E=at(q),F=at(q+ivec2(1,0));
    int G=at(q+ivec2(-1,1)),H=at(q+ivec2(0,1)),I=at(q+ivec2(1,1));
    vec4 c=entry(E);float aa=max(fwidth(pixel_local.x)+fwidth(pixel_local.y),.05)*.25;
    int partner=-1,count=0;
    if(checker(E,A,B,D)){partner=B;count++;}
    if(checker(E,C,B,F)){if(partner<0 || partner==B){partner=B;count++;}}
    if(checker(E,G,D,H)){if(partner<0 || partner==D){partner=D;count++;}}
    if(checker(E,I,F,H)){if(partner<0 || partner==F){partner=F;count++;}}
    // Two adjoining 2x2 blocks (or a cardinal alternation) avoid confusing
    // isolated eyes, punctuation and one-pixel lines with a dithered surface.
    bool alternating=D==F && B==H && B==D && E!=D && entry(E).a>.99 && entry(D).a>.99;
    bool johnny=sprite_material.y>.5;
    // Small skin patches (cheek, elbow, fingers) often contain just one 2x2
    // checker. Preserve the stricter generic rule for unrelated resources.
    bool skin_checker=johnny && partner>=0 && skin_pair(E,partner);
    bool dither=partner>=0 && (count>=2 || (alternating && count>=1) || skin_checker);
    if(dither)c=fused(E,partner);
    if(johnny && !dither && E==15) {
        int neighbors[8]=int[8](A,B,C,D,F,G,H,I);
        int ink=0,skin=0,bright=0,skin_index=12;
        for(int n=0;n<8;n++) {
            int i=neighbors[n];
            ink+=int(i==5);
            bright+=int(i==7 || i==15);
            if(i==4 || i==12){skin++;skin_index=i;}
        }
        if(skin>=2 && ink<=2 && bright<=1) {
            // Recover isolated white edge texels as skin, keeping alpha exact.
            c=fused(E,skin_index);dither=true;
        } else if(ink>=5 && skin==0 && bright<=1) {
            // Quiet an isolated glint in dark material, never erase coverage.
            // An eye next to skin, or a larger cloth/eye cluster, is preserved.
            c.rgb=mix(entry(5).rgb,c.rgb,.40);
        }
    }
    // Scale2x corner agreement, continuous diagonals, no bilinear atlas fetch.
    if((!dither || johnny) && B!=H && D!=F) {
        if(D==B && (A==D || E!=A))c=mix(c,corner_entry(D,c,dither),1.0-smoothstep(.48-aa,.48+aa,f.x+f.y));
        if(B==F && (C==B || E!=C))c=mix(c,corner_entry(F,c,dither),1.0-smoothstep(.48-aa,.48+aa,1.0-f.x+f.y));
        if(D==H && (G==D || E!=G))c=mix(c,corner_entry(D,c,dither),1.0-smoothstep(.48-aa,.48+aa,f.x+1.0-f.y));
        if(H==F && (I==H || E!=I))c=mix(c,corner_entry(F,c,dither),1.0-smoothstep(.48-aa,.48+aa,2.0-f.x-f.y));
    }
    if(options.w>1.5) {
        float soft=(entry(D).a+entry(F).a+entry(B).a+entry(H).a)*.25;
        float softness=sprite_material.x>.5?.75:mix(.6,.25,sun_light.z);
        float alpha=mix(c.a,soft,softness);
        if(alpha<.001)discard;
        frag_color=vec4(vec3(15,20,35)/255.0,alpha*sun_light.w*sprite_ground(world_pos));return;
    }
    if(c.a<.001)discard;
    // Internal index changes in fused surfaces are not geometric bevels.
    vec2 normal=vec2(entry(D).a-entry(F).a,entry(B).a-entry(H).a);
    if(!johnny && !dither)normal+=vec2(float(F!=E)-float(D!=E),float(H!=E)-float(B!=E))*.20;
    normal*=mix(vec2(1),vec2(-1),options.xy);
    vec3 albedo=c.rgb/max(c.a,.001);
    // Johnny has matte fills: no bevel at every palette transition and a
    // restrained silhouette rim. Other materials retain their previous light.
    vec3 color=hybrid_lit_scaled(albedo,normal,johnny?.15:1.0);
    if(sprite_material.y>.5) {
        // Sand bounce in daylight; keep some pigment in the cool night light.
        color=mix(color,albedo*scene_tint.rgb,.45*(1.0-sun_light.z));
        float bounce=sqrt(max(dot(albedo,vec3(.299,.587,.114)),0.0));
        color+=vec3(.025,.012,.002)*sun_light.z*bounce;
    }
    float warm=exp(-dot(world_pos-local_fire.xy,world_pos-local_fire.xy)/max(local_fire.w*local_fire.w,.01));
    color+=vec3(.20,.064,.008)*warm*local_fire.z*sprite_decor.x;
    // FIRE1 is the dedicated flame/spark sheet. Its authored red/yellow
    // pixels emit light at night; Johnny's skin and white clothes do not.
    if(sprite_material.w>.5) {
        if(E==14)color=mix(color,vec3(1.0,.78,.16),.95);
        else if(E==12)color=mix(color,vec3(1.0,.32,.035),.90);
        else if(E==4)color=mix(color,vec3(.64,.11,.018),.80);
    }
    frag_color=vec4(color,c.a);
}
@end
@program indexed quad_vs palette_fs

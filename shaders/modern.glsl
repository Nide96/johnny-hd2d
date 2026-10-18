@vs modern_vs
out vec2 screen_uv;
void main() {
    const vec2 corners[3]=vec2[3](vec2(0,0),vec2(2,0),vec2(0,2));
    vec2 p=corners[gl_VertexIndex];screen_uv=p;
    gl_Position=vec4(p.x*2.0-1.0,1.0-p.y*2.0,0,1);
}
@end
@fs modern_fs
layout(binding=1) uniform modern_params {
    vec4 clock; vec4 plate_tint;
    vec4 sky_top; vec4 sky_horizon; vec4 sun; vec4 moon;
    vec4 environment; vec4 campfire;
};
layout(binding=2) uniform texture2D background_plate;
layout(binding=5) uniform texture2D environment_mask;
layout(binding=1) uniform sampler linear_sampler;
in vec2 screen_uv;
out vec4 frag_color;
const float TAU=6.28318530718;
float hash21(vec2 p) {
    vec3 q=fract(vec3(p.xyx)*vec3(.1031,.1030,.0973));
    q+=dot(q,q.yzx+33.33);return fract((q.x+q.y)*q.z);
}
// Tileable value noise: both cloud layers wrap exactly at 4096 seconds.
float noise16(vec2 p) {
    vec2 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);
    return mix(mix(hash21(mod(i,16.0)),hash21(mod(i+vec2(1,0),16.0)),f.x),
               mix(hash21(mod(i+vec2(0,1),16.0)),hash21(mod(i+vec2(1),16.0)),f.x),f.y);
}
float clouds(vec2 p) {
    float n=.55*noise16(p)+.26*noise16(p*2.0)+.13*noise16(p*4.0)+.06*noise16(p*8.0);
    return smoothstep(.46,.70,n);
}
vec2 source_uv(vec2 p) {
    float y=p.y*720.0,sy;
    if(y<240.0)sy=y*224.0/240.0;
    else if(y<425.0)sy=224.0+(y-240.0)*117.0/185.0;
    else if(y<592.0)sy=341.0+(y-425.0)*147.0/167.0;
    else sy=488.0+(y-592.0)*232.0/128.0;
    return vec2(p.x,sy/720.0);
}
vec3 legacy_water(vec2 p,vec2 sp,vec3 base) {
    float island=1.0-smoothstep(.96,1.12,length((p-vec2(.50,.706))/vec2(.31,.135)));
    float blue=smoothstep(.02,.12,base.b-base.r)*smoothstep(-.015,.04,base.g-base.r);
    float water=smoothstep(.32,.37,p.y)*(1.0-island)*blue;
    float t=clock.x,near_water=smoothstep(.33,1.0,p.y);
    vec2 ripple=vec2(sin(p.y*190.0+t*.9)+.35*sin(p.x*91.0-p.y*140.0-t*.7),.40*sin(p.y*240.0-p.x*36.0+t*.65));
    vec3 c=texture(sampler2D(background_plate,linear_sampler),clamp(sp+ripple*vec2(.0018,.00085)*water*near_water,vec2(.001),vec2(.999))).rgb;
    c+=vec3(.012,.025,.034)*sin(p.y*260.0+p.x*42.0-t*1.4)*water*near_water;
    return mix(vec3(dot(c,vec3(.299,.587,.114)))*.9,c,clock.y)*plate_tint.rgb;
}
vec3 sky(vec2 p,float t) {
    float h=clamp(p.y/.333333,0.0,1.0);
    vec3 c=mix(sky_top.rgb,sky_horizon.rgb,pow(h,1.35));
    vec2 cp=vec2(p.x*6.0,h*2.9);
    float high=clouds(cp+vec2(t/256.0,0));
    float low=clouds(cp*vec2(1.5,1.2)+vec2(t/128.0,3.7));
    float coverage=clamp(high*.80+low*.65,0.0,1.0)*(1.0-smoothstep(.72,1.0,h));
    // Fixed celestial positions: no screen-space crawl. Slow, shallow twinkle.
    vec2 grid=p*vec2(148,148),cell=floor(grid);
    float seed=hash21(cell),present=step(.979,seed);
    vec2 location=vec2(hash21(cell+17.1),hash21(cell+59.7))*.68+.16;
    float distance_to_star=length((fract(grid)-location)*vec2(1280,720)/148.0);
    float pixel=max(fwidth(p.x)*1280.0,fwidth(p.y)*720.0);
    float star=(1.0-smoothstep(.20*pixel,.95*pixel,distance_to_star))*present;
    star*=environment.y*(1.0-coverage)*(1.0-smoothstep(.23,.32,p.y));
    float twinkle=.88+.12*sin(TAU*t/128.0+seed*TAU);
    c+=mix(vec3(.65,.76,1.0),vec3(1,.89,.73),hash21(cell+4.2))*star*twinkle;
    vec2 aspect=vec2(1280.0/720.0,1);
    float sd=length((p-sun.xy)*aspect),md=length((p-moon.xy)*aspect);
    vec3 solar=mix(vec3(1,.95,.74),vec3(1,.64,.25),sun.w);
    c+=solar*exp(-sd*sd/0.0035)*sun.z*.20;
    c=mix(c,solar,(1.0-smoothstep(.014,.016,sd))*sun.z);
    c+=vec3(.33,.43,.7)*exp(-md*md/.0009)*moon.z*.10;
    float disk=1.0-smoothstep(.011,.012,md);
    float crater=.92+.08*noise16(p*180.0);
    c=mix(c,vec3(.77,.83,.94)*crater,disk*moon.z);
    vec3 cloud_color=mix(vec3(.045,.06,.11),mix(vec3(.80,.85,.89),vec3(.77,.38,.27),environment.z),environment.x);
    c=mix(c,cloud_color,coverage*.90);
    return c;
}
void main() {
    vec2 p=screen_uv,sp=source_uv(p);
    vec3 base=texture(sampler2D(background_plate,linear_sampler),sp).rgb;
    if(clock.z<.5){frag_color=vec4(legacy_water(p,sp,base),1);return;}
    float t=clock.x,phase=TAU*t/4096.0;
    if(p.y<.333333){frag_color=vec4(sky(p,t),1);return;}
    vec3 mask=texture(sampler2D(environment_mask,linear_sampler),p).rgb;
    float shore=(mask.r-.5)*64.0,land=mask.g;
    float water=(1.0-land)*(1.0-mask.b)*smoothstep(.33333,.355,p.y);
    float near_water=smoothstep(.33333,1.0,p.y);
    vec2 swell=vec2(sin(p.y*145.0-phase*173.0)+.36*sin(p.x*80.0+p.y*180.0-phase*311.0),
                      .36*sin(p.y*230.0-p.x*40.0+phase*227.0));
    vec2 q=clamp(sp+swell*vec2(.0020,.0009)*near_water*water,vec2(.001),vec2(.999));
    vec3 color=texture(sampler2D(background_plate,linear_sampler),q).rgb;
        // Remove the sunset photograph's fixed golden sea reflection before the
    // procedural sun/moon reflection is added in its actual celestial column.
    float old_column=.81-.24*near_water;
    float painted_area=exp(-pow(abs(p.x-old_column)/(.09+.04*near_water),4.0));
    float painted_gold=max(painted_area,smoothstep(.015,.17,color.r-color.b))*(1.0-land)*(1.0-mask.b);
    float clean_x=p.y<.57?q.x-.23:(p.y>.87?q.x-.27:.96);
    vec3 clean_sea=texture(sampler2D(background_plate,linear_sampler),vec2(clamp(clean_x,.01,.99),q.y)).rgb;
    float clean_luma=dot(clean_sea,vec3(.299,.587,.114));
    vec3 water_neutral=clean_luma*vec3(.51,.79,1.10);
    color=mix(color,mix(clean_sea,water_neutral,.55),painted_gold*.96);
    float crest=pow(max(0.0,sin(p.y*325.0+p.x*46.0-phase*613.0)),12.0);
    float light_x=mix(moon.x,sun.x,environment.x);
    float column=exp(-pow((p.x-light_x)/(.016+.07*near_water),2.0));
    vec3 reflection=mix(vec3(.20,.33,.58),mix(vec3(.65,.73,.70),vec3(1,.57,.21),environment.z),environment.x);
    color+=reflection*column*crest*water*(.04+.20*environment.z);
    float swash=-2.0+4.0*sin(TAU*t/8.0+.35*sin(p.x*24.0));
    float lace=.35+.65*noise16(vec2(p.x*110.0,p.y*70.0)+vec2(t/256.0,0));
    float foam=(1.0-smoothstep(.5,2.0,abs(shore-swash)))*lace;
    float secondary=(1.0-smoothstep(.6,1.4,abs(shore+11.0+2.0*sin(TAU*t/16.0+p.x*7.0))))*.16;
    float coast=1.0-smoothstep(16.0,24.0,abs(shore));
    float wet=land*(1.0-smoothstep(1.0,10.0,shore))*(.70+.30*sin(TAU*t/8.0-.8));
    color*=1.0-wet*.16;
    if(land>.001)color*=1.0-land*.045*clouds(vec2(p.x*6.0+t/256.0,p.y*3.0));
    color=mix(color,mix(vec3(.16,.24,.36),vec3(.88,.92,.92),environment.x),
              clamp((foam+secondary)*coast*(1.0-mask.b)*.62,0.0,.65));
    vec3 night_material=vec3(dot(color,vec3(.299,.587,.114)))*vec3(.50,.67,.94);
    color=mix(night_material,color,environment.x)*plate_tint.rgb*mix(.58,1.0,environment.x);
    color+=vec3(.07,.10,.15)*column*crest*water*environment.y*near_water;
    // Analytic emission halos, restricted to real fire resources. No glow on
    // white clothes, eyes or punctuation. Embers remain behind the actors.
    if(campfire.z>.001) {
    vec2 world=p*vec2(1280,720),delta=world-campfire.xy;
    float glow=exp(-dot(delta,delta)/max(campfire.w*campfire.w,.01))*campfire.z;
    color+=vec3(.30,.105,.018)*glow*land;
    for(int i=0;i<3;i++) {
        float age=fract(t/4.0+float(i)*.333333);
        vec2 ember=campfire.xy+vec2(sin(age*7.0+float(i)*2.0)*9.0,-age*48.0);
        float e=exp(-dot(world-ember,world-ember)/2.2)*sin(age*3.14159265);
        color+=vec3(1,.39,.055)*e*campfire.z*.65;
    }
    }
    frag_color=vec4(color,1);
}
@end
@program modern modern_vs modern_fs

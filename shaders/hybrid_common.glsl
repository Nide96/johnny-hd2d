@block hybrid_lighting
vec3 hybrid_lit_scaled(vec3 color,vec2 normal,float rim_scale) {
    vec2 direction=normalize(sun_light.xy);
    float bevel=clamp(dot(normal,direction),-.8,1.0);
    vec3 rim=mix(vec3(.42,.58,1.0),vec3(1.0,.81,.52),sun_light.z);
    color=mix(vec3(dot(color,vec3(.299,.587,.114)))*.9,color,sun_light.z);
    return color*scene_tint.rgb*(1.0+bevel*.09)+rim*max(bevel,0.0)*scene_tint.w*rim_scale;
}
vec3 hybrid_lit(vec3 color,vec2 normal) {
    return hybrid_lit_scaled(color,normal,1.0);
}
@end
@block hybrid_ground
float ground_mask(vec2 world) {
    vec2 delta=(world-vec2(645,505))/vec2(343,85);
    float land=1.0-smoothstep(.96,1.0,length(delta));
    float y=world.y,sy;
    if(y<240.0)sy=y*224.0/240.0;
    else if(y<425.0)sy=224.0+(y-240.0)*117.0/185.0;
    else if(y<592.0)sy=341.0+(y-425.0)*147.0/167.0;
    else sy=488.0+(y-592.0)*232.0/128.0;
    vec3 c=texture(sampler2D(background_plate,linear_sampler),vec2(world.x/1280.0,sy/720.0)).rgb;
    return land*smoothstep(.015,.08,c.r-c.g)*smoothstep(.035,.12,c.g-c.b);
}
@end

#include "layout.h"
#include "palm_layout.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static void near(float a,float b){assert(fabsf(a-b)<.001f);}
int main(void) {
 near(jc_layout_x(455),667.f);near(jc_layout_y(293),450.f);
 near(jc_layout_x(443),645.4f);near(jc_layout_y(148),189.f);
 near(jc_layout_width(24),43.2f);near(jc_layout_width(145),261.f);
 for(int origin=-300;origin<=300;origin+=31) {
   float actor=origin+455.f,tree=origin+443.f;
   near(jc_layout_x(actor-origin)-jc_layout_x(tree-origin),12.f*JC_WORLD_SCALE);
 }
 assert(jc_layout_x(440)<jc_layout_x(455)); /* Never mirror animation direction. */
 for(int origin=-300;origin<=300;origin+=31) {
   JCRenderFrame frame={.origin_x=origin,.origin_y=origin,.scenery_count=2};
   frame.scenery[0]=(JCDraw){.source_sprite=13,.x=443+origin,.y=148+origin,.w=24,.h=145,.ax=70,.ay=80};
   frame.scenery[1]=(JCDraw){.source_sprite=12,.x=366+origin,.y=122+origin,.w=152,.h=69,.ax=90,.ay=100};
   for(unsigned id=0;id<2;++id) {
     JCDraw d={.source_sprite=id,.x=(id?365:443)+origin+3,.y=(id?122:148)+origin-2,.flags=FLIP_H,.clip={1,2,30,40}};
     strcpy(d.resource,"TRUNK.BMP");
     JCDraw p=jc_palm_draw(&frame,&d), b=frame.scenery[id];
     assert(p.x==b.x+3 && p.y==b.y-2);
     assert(p.ax==b.ax && p.ay==b.ay && p.w==b.w && p.h==b.h);
     assert(p.flags==d.flags && !memcmp(&p.clip,&d.clip,sizeof d.clip));
   }
 }
 puts("layout: shared camera, original trunk footprint/contact point, origin invariance, no mirror");
}

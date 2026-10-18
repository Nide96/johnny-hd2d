# Manual Hyprland integration check: real SPIR-V, own window only.
# Requires a live Vulkan session, cc, hyprctl, grim and Python Pillow.
from pathlib import Path
import subprocess,time,json,signal,os,re,tempfile,shlex
from PIL import Image
root=Path(__file__).resolve().parents[1];out=root/'work/gpu-material';out.mkdir(parents=True,exist_ok=True)
FIXTURE=r'''
  uint8_t *patch = malloc(WIDTH*HEIGHT);
  if (!patch) goto fail;
  memset(patch,7,WIDTH*HEIGHT);
  for(int y=32;y<160;++y)for(int x=32;x<160;++x)patch[y*WIDTH+x]=((x+y)&1)?12:15;
  for(int y=32;y<160;++y)for(int x=200;x<260;++x)patch[y*WIDTH+x]=12;
  for(int y=32;y<160;++y)for(int x=300;x<360;++x)patch[y*WIDTH+x]=15;
  jc_render_frame_base(app.vm.scene.current,patch);
  uint8_t checker[64*64];
  for(int y=0;y<64;++y)for(int x=0;x<64;++x)checker[y*64+x]=((x+y)&1)?0:15;
  JCSprite sprite={64,64,checker};
  GfxCommand cmd={.op=GFX_SPRITE,.domain=GFX_ACTOR,.sprite=&sprite,.x=32,.y=240,.clip=CLIP_FULL};
  jc_render_frame_command(app.vm.scene.current,&cmd);
  uint8_t skin[128*128],white[64*64],ink[64*64];
  for(int y=0;y<128;++y)for(int x=0;x<128;++x)skin[y*128+x]=((x+y)&1)?12:15;
  memset(white,15,sizeof white);memset(ink,5,sizeof ink);
  JCSprite hero={128,128,skin},cloth={64,64,white},beard={64,64,ink};
  GfxCommand johnny={.op=GFX_SPRITE,.domain=GFX_ACTOR,.resource="JOHNWALK.BMP",.sprite=&hero,.x=400,.y=32,.clip=CLIP_FULL};
  jc_render_frame_command(app.vm.scene.current,&johnny);
  johnny.sprite=&cloth;johnny.y=200;jc_render_frame_command(app.vm.scene.current,&johnny);
  johnny.sprite=&beard;johnny.x=500;jc_render_frame_command(app.vm.scene.current,&johnny);
  uint8_t detail[128*64];memset(detail,5,sizeof detail);
  detail[16*128+16]=15; // isolated white glint in ink
  detail[16*128+32]=15;detail[16*128+33]=12; // eye beside skin
  detail[40*128+16]=12;detail[40*128+17]=15;
  detail[41*128+16]=15;detail[41*128+17]=12; // single 2x2 skin checker
  JCSprite tiny={128,64,detail};
  johnny.sprite=&tiny;johnny.x=400;johnny.y=300;
  jc_render_frame_command(app.vm.scene.current,&johnny);
  uint8_t ca[4],cb[4];
  jc_hybrid_color(jc_engine_palette(app.vm.engine)[12],ca);
  jc_hybrid_color(jc_engine_palette(app.vm.engine)[15],cb);
  fprintf(stderr,"GPU fixture LUT: %u %u %u / %u %u %u\n",ca[0],ca[1],ca[2],cb[0],cb[1],cb[2]);
  free(patch);
'''
fixture_dir=tempfile.TemporaryDirectory(prefix="johnny-material-")
fixture_root=Path(fixture_dir.name)
(fixture_root/'assets').symlink_to(root/'assets',target_is_directory=True)
src=(root/'main.c').read_text()
needle='double elapsed = stm_sec(stm_diff(now, app.last_time));'
assert needle in src and src.count('  app.ready = true;')==1
src=src.replace(needle,'double elapsed = 0.0; (void)now;')
src=src.replace('  app.ready = true;',FIXTURE+'  app.ready = true;')
(fixture_root/'fixture.c').write_text(src)
binary=fixture_root/'johnny-material-fixture'
files=['environment.c','audio.c','audio_mixer.c','vm_core.c','render_frame.c','renderer_vulkan.c','resource.c','rng.c','gfx.c','ttm.c','ads.c','island.c','walk.c','engine.c','config.c']
subprocess.run(shlex.split(os.environ.get('CC','cc'))+['-std=c99','-O1','-g','-Wall','-Wextra','-Wpedantic','-Werror','-D_POSIX_C_SOURCE=200809L','-D_DEFAULT_SOURCE','-DSOKOL_VULKAN','-I',str(root),'-isystem',str(root/'vendor'),'-pthread',str(fixture_root/'fixture.c')]+[str(root/f) for f in files]+['-lX11','-lXi','-lXcursor','-lvulkan','-lasound','-ldl','-lm','-pthread','-o',str(binary)],check=True)
log=(out/'material.log').open('w')
p=subprocess.Popen([str(binary),'--no-decor-fx','--mute','--modern','--seed','4242','--clock','1993-06-01T12:00'],stdout=log,stderr=log,env=os.environ|{'VK_INSTANCE_LAYERS':'VK_LAYER_KHRONOS_validation'})
try:
 deadline=time.monotonic()+10
 while True:
  c=next((x for x in json.loads(subprocess.check_output(['hyprctl','clients','-j'])) if x['pid']==p.pid),None)
  if c:break
  if p.poll() is not None or time.monotonic()>deadline:raise RuntimeError('Own fixture window unavailable')
  time.sleep(.1)
 time.sleep(1.8)
 c=next(x for x in json.loads(subprocess.check_output(['hyprctl','clients','-j'])) if x['pid']==p.pid)
 x,y=c['at'];w,h=c['size'];k=min(w/640,h/480);vw,vh=int(k*640),int(k*480)
 subprocess.run(['grim','-g',f'{x+(w-vw)//2},{y+(h-vh)//2} {vw}x{vh}',str(out/'material.png')],check=True)
finally:
 if p.poll() is None:p.send_signal(signal.SIGTERM)
 code=p.wait(timeout=10);log.close();assert code==0
s=(out/'material.log').read_text();assert 'Validation Error' not in s and '[error]' not in s and '[panic]' not in s
im=Image.open(out/'material.png').convert('RGB')
def pixel(x,y):return im.getpixel((int(x*vw/640),int(y*vh/480)))
colors={pixel(x+.5,y+.5) for y in range(64,120) for x in range(64,120)}
assert len(colors)==1,colors
n=list(map(int,re.search(r'GPU fixture LUT: (\d+) (\d+) (\d+) / (\d+) (\d+) (\d+)',s).groups()))
def lin(c):
 c/=255;return c/12.92 if c<=.04045 else ((c+.055)/1.055)**2.4
def enc(c):return c*12.92 if c<=.0031308 else 1.055*c**(1/2.4)-.055
expected=tuple(round(enc((lin(n[i])+lin(n[i+3]))*.5)*t*255) for i,t in enumerate([1,.98,1]))
actual=next(iter(colors));assert max(abs(a-b) for a,b in zip(expected,actual))<=2,(expected,actual)
transparent={pixel(x+.5,y+.5) for y in range(250,290) for x in range(40,80)}
assert len(transparent)>=2 and max(c[0] for c in transparent)-min(c[0] for c in transparent)>20,transparent
skin_colors={pixel(x+.5,y+.5) for y in range(64,120) for x in range(432,488)}
assert len(skin_colors)==1,skin_colors
skin=next(iter(skin_colors));white=pixel(430,230);ink=pixel(530,230)
assert skin[0]>actual[0]+10 and skin[1]<actual[1]-10 and skin[2]<actual[2]-30,(skin,actual)
assert min(white)>230 and max(ink)<40,(white,ink)
glint=pixel(416.5,316.5);eye=pixel(432.5,316.5)
small_checker=[pixel(x+.5,y+.5) for y in [340,341] for x in [416,417]]
assert max(glint)<160 and min(eye)>230,(glint,eye)
assert all(max(abs(a-b) for a,b in zip(c,skin))<=2 for c in small_checker),(small_checker,skin)
result={'isolated_glint_rgb':glint,'eye_rgb':eye,'small_skin_checker_rgb':small_checker,'johnny_skin_rgb':skin,'johnny_white_rgb':white,'johnny_ink_rgb':ink,'gpu':re.search(r'GPU: ([^;]+);',s)[1],'opaque_checker_unique_colors':len(colors),'linear_mix_expected_rgb':expected,'observed_rgb':actual,'transparent_checker_preserved':True,'validation_errors':0}
(out/'material-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))

fixture_dir.cleanup()

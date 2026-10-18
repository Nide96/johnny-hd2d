# Manual Hyprland/SPIR-V check. Requires grim, Pillow and NumPy. Captures
# only this process's focused window; no desktop or microphone recording.
from pathlib import Path
import subprocess,time,os,json,signal
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1];out=root/'work/gpu-environment';out.mkdir(exist_ok=True)
cases=[('day','12:00',0,[]),('sunset','17:00',0,[]),('night','00:00',0,[]),('wave','17:00',2,[]),('night-twinkle','00:00',32,[]),('reference','17:00',0,['--no-decor-fx']),('seam','17:00',4096,[]),('fire','00:00',0,[])]
for name,hour,t,args in cases:
 log=(out/(name+'.log')).open('w')
 p=subprocess.Popen([str(root/'johnny-hd2d'),'--modern','--palm-original','--fullscreen','--scene',('16' if name=='fire' else '34'),'--skip-steps',('400' if name=='fire' else '100'),'--seed','4242','--clock','2026-10-18T'+hour,'--freeze-scene','--decor-time',str(t),'--mute',*args],stdout=log,stderr=log,env=os.environ|{'VK_INSTANCE_LAYERS':'VK_LAYER_KHRONOS_validation'})
 try:
  deadline=time.monotonic()+12
  while True:
   c=next((x for x in json.loads(subprocess.check_output(['hyprctl','clients','-j'])) if x['pid']==p.pid),None)
   if c:break
   if p.poll() is not None or time.monotonic()>deadline:raise RuntimeError('Own window not available')
   time.sleep(.1)
  time.sleep(1.8)
  assert json.loads(subprocess.check_output(['hyprctl','activewindow','-j'])).get('pid')==p.pid
  c=next(x for x in json.loads(subprocess.check_output(['hyprctl','clients','-j'])) if x['pid']==p.pid)
  x,y=c['at'];w,h=c['size'];k=min(w/1280,h/720);vw,vh=int(k*1280),int(k*720)
  subprocess.run(['grim','-g',f'{x+(w-vw)//2},{y+(h-vh)//2} {vw}x{vh}',str(out/(name+'.png'))],check=True)
 finally:
  if p.poll() is None:p.send_signal(signal.SIGTERM)
  code=p.wait(timeout=12);log.close();assert code==0
 text=(out/(name+'.log')).read_text();assert not any(s in text for s in ['Validation Error','[error]','[panic]']),text
 print(name,'captured',flush=True)
frames=[Image.open(out/(n+'.png')).convert('RGB') for n in ['day','sunset','night']]
w,h=frames[0].size;canvas=Image.new('RGB',(w*3,h+50),(18,24,32));draw=ImageDraw.Draw(canvas)
try:font=ImageFont.truetype('/usr/share/fonts/TTF/DejaVuSans.ttf',24)
except OSError:font=ImageFont.load_default()
for i,(frame,label) in enumerate(zip(frames,['Jour — 12:00','Crépuscule — 17:00','Nuit étoilée — 00:00'])):canvas.paste(frame,(i*w,50));draw.text((i*w+20,12),label,font=font,fill='white')
canvas.save(out/'day-night.png')
frames[2].save(out/'night-preview.png')
Image.open(out/'fire.png').save(out/'fire-preview.png')

import numpy as np

def star_points(im):
    lum=im[:int(im.shape[0]*.218)].mean(axis=2)
    selected=(lum>35)&(lum<205)
    height,width=im.shape[:2]
    selected[int(height*.042):int(height*.102),int(width*.47):int(width*.53)]=False
    seen=np.zeros(selected.shape,dtype=bool);points=[]
    for y,x in zip(*np.where(selected)):
        if seen[y,x]:continue
        stack=[(int(y),int(x))];seen[y,x]=True;component=[]
        while stack:
            yy,xx=stack.pop();component.append((yy,xx))
            for dy,dx in ((-1,0),(1,0),(0,-1),(0,1)):
                ny,nx=yy+dy,xx+dx
                if 0<=ny<selected.shape[0] and 0<=nx<selected.shape[1] and selected[ny,nx] and not seen[ny,nx]:
                    seen[ny,nx]=True;stack.append((ny,nx))
        if 1<=len(component)<=12:
            points.append((round(sum(p[1] for p in component)/len(component)),round(sum(p[0] for p in component)/len(component))))
    return points

def read(name):return np.array(Image.open(out/(name+'.png')).convert('RGB'))
a,b=read('night'),read('night-twinkle');s0,s1=star_points(a),star_points(b)
common=sum(any(abs(x-u)<=1 and abs(y-v)<=1 for u,v in s1) for x,y in s0)
assert len(s0)>=20 and common>=len(s0)*.65,(len(s0),len(s1),common)
sunset,wave,seam=read('sunset'),read('wave'),read('seam')
assert np.array_equal(sunset,seam),'4096-second clock wrap discontinuity'
diff=np.abs(sunset.astype(int)-wave.astype(int));height,width=sunset.shape[:2]
water=diff[int(height*.76):int(height*.97),int(width*.26):int(width*.73)]
sky=diff[:int(height*.185)]
sand=diff[int(height*.681):int(height*.713),int(width*.573):int(width*.651)]
changed_water=int((water.max(axis=2)>2).sum());changed_sky=int((sky.max(axis=2)>0).sum())
assert changed_water>water.shape[0]*water.shape[1]*.10
assert changed_sky>100
assert float(np.percentile(sand,99))<=4,'Sand interior displaced rather than lit'
import re
assert int(re.search(r'(\d+) fire-lit frames',(out/'fire.log').read_text())[1])>0
fire=read('fire')
flames=(fire[:,:,0]>210)&(fire[:,:,1]>50)&(fire[:,:,2]<100)
assert int(flames.sum())>=5,'Night flames must remain emissive' 
result={'night_star_groups':len(s0),'stationary_star_matches':common,'seam_4096_pixel_exact':True,'water_changed_pixels':changed_water,'sky_changed_pixels':changed_sky,'sand_p99_channel_delta':float(np.percentile(sand,99)),'emissive_flame_pixels':int(flames.sum()),'validation_errors':0}
(out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result),flush=True)

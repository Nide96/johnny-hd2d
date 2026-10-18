# Diagnostic harness for the pinned Sokol revision. Timestamps measure the
# graphics pass, excluding CPU work, atlas upload and presentation latency.
# References: https://docs.vulkan.org/samples/latest/samples/api/timestamp_queries/README.html
from pathlib import Path
import subprocess,os,json,re,tempfile
root=Path(__file__).resolve().parents[1];project=root;out=root/'work/gpu-environment-bench';out.mkdir(exist_ok=True)
main=(project/'main.c').read_text()
main=main.replace('typedef struct {\n  char default_dir', 'static VkQueryPool living_queries;\nstatic unsigned living_query_frame;\nstatic double living_timestamp_ns;\ntypedef struct {\n  char default_dir',1)
needle='  app.last_time = stm_now();'
init=r'''
  VkQueryPoolCreateInfo query_info={.sType=VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,.queryType=VK_QUERY_TYPE_TIMESTAMP,.queryCount=480};
  if(vkCreateQueryPool((VkDevice)sapp_get_environment().vulkan.device,&query_info,NULL,&living_queries)!=VK_SUCCESS)goto fail;
  living_timestamp_ns=device_info.limits.timestampPeriod;
'''
main=main.replace(needle,init+needle,1)
cleanup=r'''
  if(living_queries) {
    VkDevice device=(VkDevice)sapp_get_environment().vulkan.device;
    vkDeviceWaitIdle(device);
    uint64_t stamps[480]={0};
    unsigned n=living_query_frame>1?living_query_frame-1:0;
    if(n>239)n=239;
    VkResult status=vkGetQueryPoolResults(device,living_queries,0,n*2,sizeof(stamps),stamps,sizeof(uint64_t),VK_QUERY_RESULT_64_BIT|VK_QUERY_RESULT_WAIT_BIT);
    fprintf(stderr,"GPU_TIMES status=%d ns_per_tick=%.6f count=%u",status,living_timestamp_ns,n);
    for(unsigned i=0;i<n;++i)fprintf(stderr," %.6f",(stamps[2*i+1]-stamps[2*i])*living_timestamp_ns/1e6);
    fputc('\n',stderr);vkDestroyQueryPool(device,living_queries,NULL);
  }
'''
main=main.replace('static void cleanup(void) {','static void cleanup(void) {'+cleanup)
renderer=(project/'renderer_vulkan.c').read_text()
needle='  const bool modern = r->modern_loaded;'
renderer=renderer.replace(needle,'''  if(living_query_frame>0 && living_query_frame<240)
    vkCmdWriteTimestamp(_sg.vk.frame.cmd_buf,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,living_queries,2*(living_query_frame-1));
'''+needle)
renderer=renderer.replace('  sg_apply_viewport(vx, vy, w, h, true);','  sg_apply_viewport(vx, vy, w, h, true);\n  if(living_query_frame==60)fprintf(stderr,"GPU_AREA %d %d\\n",w,h);')
renderer=renderer.replace('  sg_end_pass();','''  sg_end_pass();
  if(living_query_frame==0)vkCmdResetQueryPool(_sg.vk.frame.cmd_buf,living_queries,0,480);
  else if(living_query_frame<240)vkCmdWriteTimestamp(_sg.vk.frame.cmd_buf,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,living_queries,2*(living_query_frame-1)+1);
  ++living_query_frame;''')
(out/'bench.c').write_text(main+'\n'+renderer)
files=['vm_core','render_frame','environment','audio','audio_mixer','resource','rng','gfx','ttm','ads','island','walk','engine','config']
fixture=tempfile.TemporaryDirectory(prefix='johnny-environment-bench-')
fixture_root=Path(fixture.name)
(fixture_root/'assets').symlink_to(project/'assets',target_is_directory=True)
binary=fixture_root/'johnny-environment-bench'
subprocess.run(['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Wpedantic','-Werror','-D_POSIX_C_SOURCE=200809L','-D_DEFAULT_SOURCE','-DSOKOL_VULKAN','-I'+str(project),'-isystem',str(project/'vendor'),'-pthread',str(out/'bench.c')]+[str(project/(f+'.c')) for f in files]+['-lX11','-lXi','-lXcursor','-lvulkan','-lasound','-ldl','-lm','-pthread','-o',str(binary)],check=True)
cases=[('day',[],'12:00'),('night',[],'00:00'),('reference',['--no-decor-fx'],'17:00'),('fire',[],'00:00')]
results=[]
for name,args,hour in cases:
 p=subprocess.run([str(binary),'--modern',*(['--scene','16','--skip-steps','400'] if name=='fire' else ['--palm-original','--walk-demo']),'--fullscreen','--mute','--clock','2026-10-18T'+hour,'--seed','4242','--smoke',*args],capture_output=True,text=True,timeout=30,env=os.environ|{'VK_INSTANCE_LAYERS':'VK_LAYER_KHRONOS_validation'})
 text=p.stdout+p.stderr;(out/(name+'.log')).write_text(text)
 assert p.returncode==0 and '240 submitted frames' in text,(name,text)
 assert not any(x in text for x in ['Validation Error','[error]','[panic]']),(name,text)
 m=re.search(r'GPU_TIMES status=0 ns_per_tick=([\d.]+) count=(\d+)([^\n]+)',text);assert m,text
 raw=list(map(float,m[3].split()));assert len(raw)==239 and min(raw)>0
 area=list(map(int,re.search(r'GPU_AREA (\d+) (\d+)',text).groups()))
 # First 30 frames excluded from steady pass timing; original raw timings kept.
 values=sorted(raw[30:]);mean=sum(values)/len(values)
 result={'case':name,'render_area':area,'samples':len(values),'gpu_mean_ms':mean,'gpu_p50_ms':values[len(values)//2],'gpu_p95_ms':values[int(len(values)*.95)],'gpu_max_ms':max(values),'validation_errors':0}
 results.append(result);print(result,flush=True)
(out/'results.json').write_text(json.dumps(results,indent=2)+'\n')

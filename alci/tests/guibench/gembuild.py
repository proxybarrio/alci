#!/usr/bin/env python3
# Build Gem-elks outside its tree (the tree is only read): objects and
# programs go to OUT. Flags and file lists follow its Makefile.elks.
import sys, os, subprocess, glob
# usage: gembuild.py GEM-ELKS-TREE OUT [gembench.c]
S, OUT = sys.argv[1], sys.argv[2]
T = os.environ.get('ELKS_ROOT', os.path.abspath(os.path.join(os.path.dirname(__file__), '../../..')))
os.makedirs(OUT + '/obj', exist_ok=True); os.makedirs(OUT + '/bin', exist_ok=True)
CPP = ['-DELKS=1','-DUNIX=1','-DI8086=1','-DNDEBUG=1','-DMULTIAPP=1','-DSMALL_DATA=1',
       '-I'+S+'/src/include','-I'+S+'/src/aes','-I'+S+'/src/vdi','-I'+T+'/libc/include','-I'+T+'/elks/include']
BIND = ['-I'+S+'/src/bindings/original','-DUSER_INTIN=1']
CF = ['-Os','-mcmodel=medium','-melks-libc','-march=i8086','-mtune=i8086','-msegment-relocation-stuff',
      '-fno-inline','-ffunction-sections','-fdata-sections','-fno-builtin-printf','-fno-builtin-fprintf',
      '-std=gnu89','-w']
LF = ['-mcmodel=medium','-melks-libc','-march=i8086','-mtune=i8086','-msegment-relocation-stuff',
      '-Wl,--gc-sections','-Wl,--icf=all']
def cc(src, extra=[], tag=''):
    o = OUT + '/obj/' + tag + src.replace('/', '_') + '.o'
    subprocess.run(['ia16-elf-gcc'] + CPP + extra + CF + ['-c', S + '/' + src, '-o', o], check=True)
    return o
def ar(name, objs):
    p = OUT + '/' + name; 
    if os.path.exists(p): os.remove(p)
    subprocess.run(['ia16-elf-ar','rcs',p] + objs, check=True); return p
def ld(name, stack, heap, objs):
    subprocess.run(['ia16-elf-gcc'] + LF + ['-maout-stack=%d' % stack, '-maout-heap=%d' % heap,
                    '-o', OUT + '/bin/' + name] + objs, check=True)
vdi = [cc('src/vdi/' + f) for f in ['gem_vdi.c','gem_input.c','drivers/gem_kbd_raw.c','drivers/gem_mouse_ms.c',
       'drivers/gem_mouse_serial.c','drivers/gem_mouse_amstrad.c','drivers/gem_pcvideo.c','drivers/gem_video_common.c',
       'drivers/gem_video_planar.c','drivers/gem_video_mono.c','drivers/gem_video_cga.c','drivers/gem_video_hercules.c',
       'drivers/gem_pcvideo_asm.S']]
libvdi = ar('libvdi.a', vdi)
res = [cc(f) for f in ['src/server/gem_config.c'] + ['src/aes/' + x for x in [
 'gem_exec_elks.c','gem_aes_resident.c','gem_aes_tick.c','gem_pending.c','gem_menu_core.c','gem_event_resident.c',
 'gem_form_resident.c','gem_alert_resident.c','gem_fsel_resident.c','gem_graf_resident.c','gem_resident_memory.c',
 'gem_aes_call.c','gem_rsc.c','gem_resource_resident.c','gem_scrap_resident.c','gem_system_resource.c',
 'gem_menu_object_resident.c','gem_menu_pull_resident.c','gem_shell_resident.c','gem_startup_resident.c',
 'gem_object_resident.c','gem_window_resident.c','gem_far_resource_elks.c']] + ['src/vdi/' + x for x in [
 'gem_vdi_resident.c','gem_vdi_output.c','gem_vdi_mouse.c','gem_vdi_font.c','gem_vdi_text.c','gem_vdi_palette.c',
 'gem_vdi_sound.c']] + ['src/server/gem_main.c']]
ld('gem', 4096, 1024, res + [libvdi])
FAR = ['-DGEM_FAR_DATA=1']
binds = [cc(f[len(S)+1:], BIND + FAR, 'c_') for f in sorted(glob.glob(S + '/src/bindings/original/*.c'))]
binds += [cc('src/bindings/' + f, BIND + FAR, 'c_') for f in ['gem_direct_aes.c','gem_direct_vdi.c']]
libgem = ar('libgem.a', binds)
desk = [cc('src/aes/opengem/' + f, FAR, 'c_') for f in ['desktop.c','desk1.c','deskact.c','deskfun.c','deskwin.c',
 'deskpro.c','deskppd.c','deskobj.c','deskfpd.c','deskins.c','deskapp1.c','deskapp2.c','deskapp3.c','deskgraf.c',
 'deskrsrc.c','deskinf.c','deskdir.c','deskgsx.c','desktype.c','optimize.c','desksupp.c','deskiacc.c','gemdesk_posix.c',
 'deskacc.c']]
desk += [cc(f, FAR, 'c_') for f in ['src/desktop/gemdos_posix.c','src/bindings/gem_wordpair.c']]
main = [cc('src/desktop/gemdesk_main.c', FAR, 'c_')]
ld('gemdesk', 4096, 14336, main + desk + [libgem])
rt = [cc('src/apps/gem_app_vdi.c', FAR, 'c_'), OUT + '/obj/c_src_bindings_gem_wordpair.c.o']
ld('gemclock', 2048, 1024, [cc('src/apps/gemclock.c', FAR, 'c_')] + rt + [libgem])
ld('gemcalc', 2048, 1024, [cc('src/apps/gemcalc.c', FAR, 'c_')] + rt + [libgem])
if len(sys.argv) > 3:
    o = OUT + '/obj/gembench.o'
    subprocess.run(['ia16-elf-gcc'] + CPP + FAR + CF + ['-c', sys.argv[3], '-o', o], check=True)
    ld('gembench', 2048, 1024, [o] + rt + [libgem])
for f in os.listdir(OUT + '/bin'): print(f, os.path.getsize(OUT + '/bin/' + f))

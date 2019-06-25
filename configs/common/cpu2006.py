#Mybench.py

import m5
from m5.objects import Process
m5.util.addToPath('../common')


binary_dir = '/home/cc/spec2006_1/benchspec/CPU2006/'
data_dir = '/home/cc/spec2006_1/benchspec/CPU2006/'

#====================
#400.perlbench
perlbench = Process()
perlbench.executable =  binary_dir+'400.perlbench/exe/perlbench_base.amd64-m64-gcc43-nn'
#data=data_dir+'400.perlbench/data/ref/input/checkspam.pl'
#input='-I./lib'
perlbench.cmd = [perlbench.executable] + ['-I./lib','attrs.pl']
#perlbench.output = 'attrs.out'

#401.bzip2
bzip2 = Process()
bzip2.executable =  binary_dir+'401.bzip2/exe/bzip2_base.amd64-m64-gcc43-nn'
data=data_dir+'401.bzip2/data/ref/input/liberty.jpg'
bzip2.cmd = [bzip2.executable] + [data, '30']
#bzip2.output = 'liberty.out'

#====================
#403.gcc
gcc = Process()
gcc.executable =  binary_dir+'403.gcc/exe/gcc_base.amd64-m64-gcc43-nn'
data=data_dir+'403.gcc/data/ref/input/166.i'
output='166.s'
gcc.cmd = [gcc.executable] + [data]+['-o',output]
#gcc.output = '166.out'

#410.bwaves
bwaves = Process()
bwaves.executable =  binary_dir+'410.bwaves/exe/bwaves_base.amd64-m64-gcc43-nn'
data=data_dir+'410.bwaves/data/ref/input/bwaves.in'
output='bwaves.c'
bwaves.cmd = [bwaves.executable]+[data]+['-o',output]
#bwaves.output='bwaves.ref.out'

#====================
#416.gamess
gamess=Process()
gamess.executable =  binary_dir+'416.gamess/exe/gamess_base.amd64-m64-gcc43-nn'
gamess.cmd = [gamess.executable]
gamess.input='cytosine.2.config'
#gamess.output='cytosine.2.output'

#429.mcf
mcf = Process()
mcf.executable =  binary_dir+'429.mcf/exe/mcf_base.amd64-m64-gcc43-nn'
data=data_dir+'429.mcf/data/ref/input/inp.in'
mcf.cmd = [mcf.executable] + [data]
#mcf.output = 'inp.out'

#====================
#433.milc
milc=Process()
milc.executable = binary_dir+'433.milc/exe/milc_base.amd64-m64-gcc43-nn'
stdin=data_dir+'433.milc/data/ref/input/su3imp.in'
milc.cmd = [milc.executable]
milc.input=stdin
#milc.output='su3imp.out'

#====================
#434.zeusmp
zeusmp=Process()
zeusmp.executable =  binary_dir+'434.zeusmp/exe/zeusmp_base.amd64-m64-gcc43-nn'
zeusmp.cmd = [zeusmp.executable]
#zeusmp.output = 'zeusmp.stdout'

#====================
#435.gromacs
gromacs = Process()
gromacs.executable =  binary_dir+'435.gromacs/exe/gromacs_base.amd64-m64-gcc43-nn'
data=data_dir+'435.gromacs/data/ref/input/gromacs.tpr'
gromacs.cmd = [gromacs.executable] + ['-silent','-deffnm',data,'-nice','0']

#====================
#436.cactusADM
cactusADM = Process()
cactusADM.executable =  binary_dir+'436.cactusADM/exe/cactusADM_base.amd64-m64-gcc43-nn'
data=data_dir+'436.cactusADM/data/ref/input/benchADM.par'
cactusADM.cmd = [cactusADM.executable] + [data]
#cactusADM.output = 'benchADM.out'

#437.leslie3d
leslie3d=Process()
leslie3d.executable =  binary_dir+'437.leslie3d/exe/leslie3d_base.amd64-m64-gcc43-nn'
stdin=data_dir+'437.leslie3d/data/ref/input/leslie3d.in'
leslie3d.cmd = [leslie3d.executable]
leslie3d.input=stdin
#leslie3d.output='leslie3d.stdout'

#444.namd
namd = Process()
namd.executable =  binary_dir+'444.namd/exe/namd_base.amd64-m64-gcc43-nn'
input=data_dir+'444.namd/data/all/input/namd.input'
namd.cmd = [namd.executable] + ['--input',input,'--iterations','38','--output','namd.out']
#namd.output='namd.stdout'

#445.gobmk
gobmk=Process()
gobmk.executable =  binary_dir+'445.gobmk/exe/gobmk_base.amd64-m64-gcc43-nn'
stdin=data_dir+'445.gobmk/data/ref/input/trevord.tst'
gobmk.cmd = [gobmk.executable]+['--quiet','--mode','gtp']
gobmk.input=stdin
#gobmk.output='trevord.ref.trevord.out'

#====================
#447.dealII
dealII=Process()
dealII.executable =  binary_dir+'447.dealII/exe/dealII_base.amd64-m64-gcc43-nn'
dealII.cmd = [gobmk.executable]+['23']
#dealII.output='log'


#450.soplex
soplex=Process()
soplex.executable =  binary_dir+'450.soplex/exe/soplex_base.amd64-m64-gcc43-nn'
data=data_dir+'450.soplex/data/ref/input/pds-50.mps'
soplex.cmd = [soplex.executable]+['-s1 -e -m45000',data]
#soplex.output = 'pds-50.out'

#453.povray
povray=Process()
povray.executable =  binary_dir+'453.povray/exe/povray_base.amd64-m64-gcc43-nn'
data=data_dir+'453.povray/data/ref/input/SPEC-benchmark-ref.ini'
#povray.cmd = [povray.executable]+['SPEC-benchmark-test.ini']
povray.cmd = [povray.executable]+[data]
#povray.output = 'SPEC-benchmark-ref.stdout'

#454.calculix
calculix=Process()
calculix.executable =  binary_dir+'454.calculix/exe/calculix_base.amd64-m64-gcc43-nn'
data=data_dir+'454.calculix/data/ref/input/hyperviscoplastic'
calculix.cmd = [calculix.executable]+['-i',data]
#calculix.output = 'hyperviscoplastic.log'

#456.hmmer
hmmer=Process()
hmmer.executable =  binary_dir+'456.hmmer/exe/hmmer_base.amd64-m64-gcc43-nn'
data=data_dir+'456.hmmer/data/ref/input/retro.hmm'
hmmer.cmd = [hmmer.executable]+['--fixed', '0', '--mean', '500', '--num', '500000', '--sd', '350', '--seed', '0', data]
#hmmer.output = 'retro.out'

#458.sjeng
sjeng=Process()
sjeng.executable =  binary_dir+'458.sjeng/exe/sjeng_base.amd64-m64-gcc43-nn'
data=data_dir+'458.sjeng/data/ref/input/ref.txt'
sjeng.cmd = [sjeng.executable]+[data]
#sjeng.output = 'ref.out'

#459.GemsFDTD
GemsFDTD=Process()
GemsFDTD.executable =  binary_dir+'459.GemsFDTD/exe/GemsFDTD_base.amd64-m64-gcc43-nn'
data=data_dir+'459.GemsFDTD/data/ref/input/ref.in'
GemsFDTD.cmd = [GemsFDTD.executable]+[data]
#GemsFDTD.output = 'ref.log'

#462.libquantum
libquantum=Process()
libquantum.executable =  binary_dir+'462.libquantum/exe/libquantum_base.amd64-m64-gcc43-nn'
libquantum.cmd = [libquantum.executable],'1397','8'
#libquantum.output = 'ref.out'

#464.h264ref
h264ref=Process()
h264ref.executable =  binary_dir+'464.h264ref/exe/h264ref_base.amd64-m64-gcc43-nn'
data=data_dir+'464.h264ref/data/ref/input/foreman_ref_encoder_baseline.cfg'
h264ref.cmd = [h264ref.executable]+['-d',data]
#h264ref.output = 'foreman_ref_encoder_baseline.out'

#465.tonto
tonto=Process()
tonto.executable=binary_dir+'465.tonto/exe/tonto_base.amd64-m64-gcc43-nn'
data=data_dir+'465.tonto/data/ref/input/stdin'
tonto.cmd = [tonto.executable]+['-d',data]
#tonto.output = 'tonto.out'

#470.lbm
lbm=Process()
lbm.executable =  binary_dir+'470.lbm/exe/lbm_base.amd64-m64-gcc43-nn'
data=data_dir+'470.lbm/data/ref/input/100_100_130_ldc.of'
lbm.cmd = [lbm.executable]+['3000', 'reference.dat', '0', '0' ,data]
#lbm.output = 'lbm.out'

#471.omnetpp
omnetpp=Process()
omnetpp.executable =  binary_dir+'471.omnetpp/exe/omnetpp_base.amd64-m64-gcc43-nn'
data=data_dir+'471.omnetpp/data/ref/input/omnetpp.ini'
omnetpp.cmd = [omnetpp.executable]+[data]
#omnetpp.output = 'omnetpp.log'

#====================
#473.astar
astar=Process()
astar.executable =  binary_dir+'473.astar/exe/astar_base.amd64-m64-gcc43-nn'
data=data_dir+'473.astar/data/ref/input/BigLakes2048.cfg'
astar.cmd = [astar.executable]+[data]
#astar.output = 'BigLakes2048.out'

#====================
#481.wrf
wrf=Process()
wrf.executable =  binary_dir+'481.wrf/exe/wrf_base.amd64-m64-gcc43-nn'
data=data_dir+'481.wrf/data/ref/input/namelist.input'
wrf.cmd = [wrf.executable]+['namelist.input']
#wrf.output = 'rsl.out.0000'

#482.sphinx3
sphinx3=Process()
sphinx3.executable =  binary_dir+'482.sphinx3/exe/sphinx_livepretend_base.amd64-m64-gcc43-nn'
sphinx3.cmd = [sphinx3.executable]+['ctlfile', '.', 'args.an4']
#sphinx3.output = 'an4.out'

#483.xalancbmk
xalancbmk=Process()
xalancbmk.executable =  binary_dir+'483.xalancbmk/exe/Xalan_base.amd64-m64-gcc43-nn'
xalancbmk.cmd = [xalancbmk.executable]+['-v','t5.xml','xalanc.xsl']
#xalancbmk.output = 'ref.out'

#998.specrand
specrand_i=Process()
specrand_i.executable = binary_dir+'998.specrand/exe/specrand_base.amd64-m64-gcc43-nn'
specrand_i.cmd = [specrand_i.executable] + ['1255432124','234923']
#specrand_i.output = 'rand.234923.988.out'

#999.specrand
specrand_f=Process()
specrand_f.executable = binary_dir+'999.specrand/exe/specrand_base.amd64-m64-gcc43-nn'
specrand_f.cmd = [specrand_f.executable] + ['1255432124','234923']
#specrand_f.output = 'rand.234923.999.out'

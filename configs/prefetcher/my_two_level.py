import m5
from m5.objects import *
from caches import *

from optparse import OptionParser

parser = OptionParser()
parser.add_option('--l1i_size', help="L1 instruction cache size");
parser.add_option('--l1d_size', help="L1 data cache size");
parser.add_option('--l2_size', help="Unified L2 cache size");
parser.add_option("-p", "--prefetchers", type="string", default="tagged",
                  help="Use tagged/stride/stream/lookahead prefetchers"
                  "[default: %default]")
(options, args) = parser.parse_args()

system = System()
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = '1GHz'
system.clk_domain.voltage_domain = VoltageDomain()

system.mem_mode = 'timing'
system.mem_ranges = [AddrRange('8GB')]

system.cpu = DerivO3CPU()
system.cpu.icache = L1ICache(options)
system.cpu.dcache = L1DCache(options)
system.cpu.icache.connectCPU(system.cpu)
system.cpu.dcache.connectCPU(system.cpu)

system.l2bus = L2XBar()
system.membus = SystemXBar()
# Connect I/D cache to BUS
system.cpu.icache.connectBus(system.l2bus)
system.cpu.dcache.connectBus(system.l2bus)
# Connect I/D Cache directly to MEMBUS
#system.cpu.icache_port = system.membus.slave
#system.cpu.dcache_port = system.membus.slave

# connect L2 Cache to BUS
system.l2cache = L2Cache(options)
system.l2cache.connectCPUSideBus(system.l2bus)
system.l2cache.connectMemSideBus(system.membus)

if options.prefetchers == "tagged":
    print "Using Tagged Prefetcher!"
    system.l2cache.prefetcher = TaggedPrefetcher()
elif options.prefetchers == "stride":
    print "Using Stride Prefetcher!"
    system.l2cache.prefetcher = StridePrefetcher()
elif options.prefetchers == "stream":
    print "Using Stream Prefetcher!"
    system.l2cache.prefetcher = StreamPrefetcher()
elif options.prefetchers == "lookahead":
    print "Using Lookahead Prefetcher!"
    system.l2cache.prefetcher = LookaheadPrefetcher()

system.cpu.createInterruptController()
system.cpu.interrupts[0].pio = system.membus.master
system.cpu.interrupts[0].int_master = system.membus.slave
system.cpu.interrupts[0].int_slave = system.membus.master

system.system_port = system.membus.slave

system.mem_ctrl = DDR3_1600_8x8()
system.mem_ctrl.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.master

process = Process()
process.cmd = ['tests/test-progs/hello/bin/x86/linux/hello']
#process.cmd =['tests/test-progs/hello/bin/riscv/linux/hello']
system.cpu.workload = process
system.cpu.createThreads()

root = Root(full_system = False, system = system)
m5.instantiate()

print "Beginning simulation!"
exit_event = m5.simulate()

print 'Exiting @ tick %i because %s' % (m5.curTick(), exit_event.getCause())

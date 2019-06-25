from m5.defines import buildEnv
from m5.objects import *
from m5.objects.Prefetcher import *

class L1Cache(Cache):
    assoc = 2
    tag_latency = 2
    data_latency = 2
    response_latency = 2
    mshrs = 4
    tgts_per_mshr = 20
    is_top_level = True

    #prefetcher = StridePrefetcher(degree=8, latency=1.0)
    '''
    def __init__(self, options=None):
        super(L1Cache, self).__init__()
        pass

    '''
    def connectCPU(self, cpu):
        raise NotImplementedError

    def connectBus(self, bus):
        self.mem_side = bus.slave

class L1_ICache(L1Cache):
    size = '16kB'
    is_read_only = True
    # Writeback clean lines as well
    writeback_clean = True
    '''
    def __init__(self, options=None):
        super(L1Cache, self).__init__()
        if not options or not options.l1i_size:
            return
        self.size = options.l1i_size

    '''
    def connectCPU(self, cpu):
        self.cpu_side = cpu.icache_port

class L1_DCache(L1Cache):
    size = '64kB'
    '''
    def __init__(self, options=None):
        super(L1Cache, self).__init__()
        if not options or not options.l1d_size:
            return
        self.size = options.l1d_size
    '''

    def connectCPU(self, cpu):
        self.cpu_side = cpu.dcache_port

class L2Cache(Cache):
    size = '128kB'
    assoc = 8
    tag_latency = 20
    data_latency = 20
    response_latency = 20
    mshrs = 16
    tgts_per_mshr = 12
    # prefetch_on_access = False
    # prefetcher = StridePrefetcher(degree=8, latency=1.0)
    # prefetcher = LookaheadPrefetcher(queue_squash=False)

    '''
    def __init__(self, options=None):
        super(L2Cache, self).__init__()
        if not options or not options.l2_size:
            return
        self.size = options.l2_size

    '''
    def connectCPUSideBus(self, bus):
        self.cpu_side = bus.master

    def connectMemSideBus(self, bus):
        self.mem_side = bus.slave


class IOCache(Cache):
    assoc = 8
    tag_latency = 50
    data_latency = 50
    response_latency = 50
    mshrs = 20
    size = '1kB'
    tgts_per_mshr = 12

class PageTableWalkerCache(Cache):
    assoc = 2
    tag_latency = 2
    data_latency = 2
    response_latency = 2
    mshrs = 10
    size = '1kB'
    tgts_per_mshr = 12

    # the x86 table walker actually writes to the table-walker cache
    if buildEnv['TARGET_ISA'] == 'x86':
        is_read_only = False
    else:
        is_read_only = True
        # Writeback clean lines as well
        writeback_clean = True

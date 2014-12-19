# First read config
nconfig = read_config(dev)
nconfig[0x11] = 04
nconfig[0x12] = f0
nconfig[0x12] = 0xf0
nconfig[0x13] = 0x0e
write_config(dev,nconfig)
data = [5,0,0,0]
thisep = get_read_endpoint(dev)
dev.ctrl_transfer( 0x40,0xbc,0,0xf0,data )
dev.ctrl_transfer( 0x40,0xbf,0,0,[] )
thisep = get_read_endpoint(dev)
thisep.read(512*4)

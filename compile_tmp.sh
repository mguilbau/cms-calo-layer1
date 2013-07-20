export SOFTDIR=../softipbus
export FLAGS='-g -Wall -Iinclude -I'$SOFTDIR'/include -I/opt/xdaq/include/ -DLINUX'
export LDFLAGS='-L/opt/xdaq/lib/ -lCAENVME -llog4cplus'

g++ -o bin/vme2fd $FLAGS $LDFLAGS \
  src/vme2fd.cc \
  src/vmestream/caen.cc \
  src/vmestream/VMEController.cc \
  src/vmestream/VMEStream_PC.c \
  $SOFTDIR/src/circular_buffer.c $SOFTDIR/src/buffer.c $SOFTDIR/src/bytebuffer.c \
  src/vmestream/OrscEmulator.cc src/vmestream/OrscEchoEmulator.cc

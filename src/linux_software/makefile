CXX=g++
CC_FLAGS=-g -pthread
LD_FLAGS=-g

RM=rm -f

SRCS = sdr.cc
OBJS = $(subst .cc,.o,$(SRCS))


all: sdr
sdr: $(OBJS)
	$(CXX) $(CC_FLAGS) -o sdr $(OBJS) $(LD_FLAGS)

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CC_FLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) *~ .depend

include .depend
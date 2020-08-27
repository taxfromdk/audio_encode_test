CUDA_PATH ?= /opt/cuda

GCC ?= g++

CCFLAGS := -std=c++11
# Debug build flags
CCFLAGS += -g


LDFLAGS += -lavformat -lavutil -lavcodec -lswresample

encode_audio: encode_audio.cpp
	$(GCC) $(CCFLAGS) $(LDFLAGS) $(INCLUDES) -o $@ $^ 	

##############################################################
#                                                            #
#      Tests                                                 #
#                                                            #
##############################################################


test: encode_audio
	./encode_audio
	ffplay AACinADTS.aac -autoexit
	
clean:
	rm -rf encode_audio *.aac

.PHONY: clean test

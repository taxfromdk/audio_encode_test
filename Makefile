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
	./encode_audio mp3
	ffplay raw.mp3 -autoexit
	./encode_audio aac
	ffplay raw.aac -autoexit

	
clean:
	rm -rf encode_audio *.aac *.mp3

.PHONY: clean test

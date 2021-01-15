ROOT_DIR ?= $(abspath ../../)
COMPILE_PREX ?=
USER_LINK_FLAGS ?=

-include ./build/build_param

#AR = $(COMPILE_PREX)ar
#CC = $(COMPILE_PREX)gcc
#NM = $(COMPILE_PREX)nm
#CPP = $(COMPILE_PREX)g++
#OBJCOPY = $(COMPILE_PREX)objcopy
#OBJDUMP = $(COMPILE_PREX)objdump
#APP_PACK = ./build/pack.sh

#编译所用变量
OUTPUT_DIR = $(ROOT_DIR)/output/$(APP_BIN_NAME)_$(USER_SW_VER)
OUTPUT_DIR_OBJS = $(OUTPUT_DIR)/.objs

#sdk头文件路径
SDK_INC_BASE_DIR = $(ROOT_DIR)/sdk/include
SDK_LIB_DIR = $(ROOT_DIR)/sdk/lib
SDK_INCS =  $(addprefix -I ,  $(shell find $(SDK_INC_BASE_DIR) -type d) )

#user app基准路径
USER_SRC_BASE_DIR = ./src
USER_INC_BASE_DIR = ./include
USER_INC_BASE_DIR += $(shell find $(ROOT_DIR)/sdk -name include -type d)

#user app 编译文件
USER_SRC_DIRS = $(shell find $(USER_SRC_BASE_DIR) -type d)
USER_SRCS += $(foreach dir, $(USER_SRC_DIRS), $(wildcard $(dir)/*.c)) 
USER_SRCS += $(foreach dir, $(USER_SRC_DIRS), $(wildcard $(dir)/*.cpp)) 
USER_SRCS += $(foreach dir, $(USER_SRC_DIRS), $(wildcard $(dir)/*.s)) 
USER_SRCS += $(foreach dir, $(USER_SRC_DIRS), $(wildcard $(dir)/*.S)) 
#user头文件
USER_INCS = $(addprefix -I ,  $(shell find $(USER_INC_BASE_DIR) -type d) )
#user的obj命令
USER_OBJS = $(addsuffix .o, $(USER_SRCS))
#user的实际obj地址
USER_OBJS_OUT =  $(addprefix $(OUTPUT_DIR_OBJS)/, $(USER_OBJS))


LINKFLAGS = \
        -L$(SDK_LIB_DIR) -ltuya_iot -pthread -lm  

LINKFLAGS += $(USER_LINK_FLAGS)

CCFLAGS = \
	-g -fPIC

DEFINES = -DAPP_BIN_NAME=\"$(APP_BIN_NAME)\" \
		  -DUSER_SW_VER=\"$(USER_SW_VER)\"

CFLAGS = $(CCFLAGS) $(DEFINES) $(SDK_INCS) $(USER_INCS) $(USER_CFLAGS)


all: pack

pack: build_app
ifeq ($(APP_PACK),$(wildcard $(APP_PACK)))
	sh $(APP_PACK) $(OUTPUT_DIR) $(ROOT_DIR) $(APP_BIN_NAME) $(USER_SW_VER) $(COMPILE_PREX)
endif

build_app: $(USER_OBJS_OUT)
	$(CC) $(USER_OBJS_OUT) $(CFLAGS) $(LINKFLAGS) -o $(OUTPUT_DIR)/$(APP_BIN_NAME)
	@cp -f boot.sh $(OUTPUT_DIR)
	@echo "Build APP Finish"

$(OUTPUT_DIR_OBJS)/%.c.o: %.c
	@mkdir -p $(dir $@);
	$(CC) $(CFLAGS) -o $@ -c $<

$(OUTPUT_DIR_OBJS)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@);
	$(CC) $(CFLAGS) -o $@ -c $<

$(OUTPUT_DIR_OBJS)/%.s.o: %.s
	@mkdir -p $(dir $@);
	$(CC) $(CFLAGS) -o $@ -c $<

$(OUTPUT_DIR_OBJS)/%.S.o: %.S
	@mkdir -p $(dir $@);
	$(CC) $(CFLAGS) -D__ASSEMBLER__ -o $@ -c $<


.PHONY: all clean SHOWARGS build_app pack
clean:
	rm -rf $(OUTPUT_DIR)

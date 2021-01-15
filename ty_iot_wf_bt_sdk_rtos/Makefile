# 包含编译参数配置定义文件
include build/build_param

############################################################
# 默认编译目标
############################################################

all: sdk

############################################################
# 包含开发环境定义
############################################################
-include $(TOOLCHAIN_PATH)/platform.mk
CFLAGS += $(TUYA_PLATFORM_CFLAGS)

############################################################
# 包含所有SDK组件的头文件目录
############################################################

SDK_INCLUDE_DIRS := $(shell find sdk -name include -type d)
SDK_INCLUDE_ALL_SUBDIRS := $(foreach dir,$(SDK_INCLUDE_DIRS),$(shell find $(dir) -type d))
CFLAGS += $(addprefix -I,$(SDK_INCLUDE_ALL_SUBDIRS))

############################################################
# 包含所有SDK组件的源文件目录
############################################################

SDK_SRC_DIRS := $(shell find sdk -name src -type d)
SDK_SRC_ALL_SUBDIRS := $(foreach dir,$(SDK_SRC_DIRS),$(shell find $(dir) -type d))

############################################################
# 使用 xmake 且包含所有SDK组件的 local.mk
############################################################

XMAKE_OUTDIR := $(OUTPUT_DIR)
include scripts/xmake.mk
-include $(shell find sdk -name local.mk)

############################################################
# 生成单个SDK静态库
############################################################
# 权限产物目录
CI_DOWNLOAD_DIR := $(ROOT_DIR)/tmp/$(PROJECT_NAME)_$(TARGET_PLATFORM)_$(SDK_FULL_VER)

# 生成目标文件列表
STATIC_OBJS_DIR := $(call static-objects-dir)
SDK_STATIC_OBJS_DIRS := $(STATIC_OBJS_DIR)/sdk

# 生成命令
sdk_static:
	@mkdir -p $(SDK_STATIC_OBJS_DIRS)
	@if [ -d $(CI_DOWNLOAD_DIR)/sdk/lib/ ]; then \
		cd $(CI_DOWNLOAD_DIR)/sdk/lib/;	\
		$(AR) -x $(CI_DOWNLOAD_DIR)/sdk/lib/libtuya_iot.a;	\
		cd -; \
		echo $(SDK_STATIC_OBJS_DIRS); \
		rm -f $(SDK_STATIC_OBJS_DIRS)/*.o; \
		mv -f $(CI_DOWNLOAD_DIR)/sdk/lib/*.o $(SDK_STATIC_OBJS_DIRS); \
		fi	
	@$(call clear-cxx-object-by-dir, $(SDK_STATIC_OBJS_DIRS))
	@$(call build-static-library-by-dirs,$(OUTPUT_DIR)/lib/libtuya_iot.a,$(SDK_STATIC_OBJS_DIRS))
	@cp $(OUTPUT_DIR)/lib/libtuya_iot.a $(OUTPUT_DIR)/lib/libtuya_iot.a.stripped
	@$(STRIP) --strip-debug $(OUTPUT_DIR)/lib/libtuya_iot.a.stripped

############################################################
# 生成单个SDK动态库
############################################################

# 生成目标文件列表
SHARED_OBJS_DIR := $(call shared-objects-dir)
SDK_SHARED_OBJS_DIRS := $(SHARED_OBJS_DIR)/sdk

# 生成命令
sdk_shared:
	@mkdir -p $(SDK_SHARED_OBJS_DIRS)
	@if [ -d $(CI_DOWNLOAD_DIR)/sdk/lib/ ]; then \
		cd $(CI_DOWNLOAD_DIR)/sdk/lib/;	\
		$(AR) -x $(CI_DOWNLOAD_DIR)/sdk/lib/libtuya_iot.a;	\
		cd -; \
		rm -f $(SDK_SHARED_OBJS_DIRS)/*.o; \
		mv -f $(CI_DOWNLOAD_DIR)/sdk/lib/*.o $(SDK_SHARED_OBJS_DIRS); \
		fi	
	@$(call clear-cxx-object-by-dir, $(SDK_SHARED_OBJS_DIRS))
	@$(call build-shared-library-by-dirs,$(OUTPUT_DIR)/lib/libtuya_iot.so,$(SDK_SHARED_OBJS_DIRS))
	@cp $(OUTPUT_DIR)/lib/libtuya_iot.so $(OUTPUT_DIR)/lib/libtuya_iot.so.stripped
	@$(STRIP) --strip-debug $(OUTPUT_DIR)/lib/libtuya_iot.so.stripped

############################################################
# SDK 打包
############################################################
# 生成SDK包文件目录
SDK_FILES_PATH_NAME=$(PROJECT_NAME)_$(TARGET_PLATFORM)_$(SDK_FULL_VER)
SDK_FILES_DIR=$(dir $(OUTPUT_DIR))/$(SDK_FILES_PATH_NAME)
sdk_files:
	@echo "====== build sdk files begin ======="
	# 清空目录
	@rm -fr $(SDK_FILES_DIR)
	@mkdir -p $(SDK_FILES_DIR)/apps
	@mkdir -p $(SDK_FILES_DIR)/platforms
	@mkdir -p $(SDK_FILES_DIR)/sdk/lib
	@mkdir -p $(SDK_FILES_DIR)/sdk/include
	# 复制demo
	@if [ -d apps/$(DEMO_NAME) ]; then cp -a apps/$(DEMO_NAME) $(SDK_FILES_DIR)/apps/; fi
ifeq ($(findstring beta,$(SDK_BETA_VER)),)		# beta 版本不把开发环境打进去
	# 复制工具链
	#@sh ./scripts/export_platform.sh platforms/$(TARGET_PLATFORM) $(SDK_FILES_DIR)/platforms/$(TARGET_PLATFORM)
endif
	# 复制对外头文件
	@for dir in `ls -1 sdk`; do		\
		if [ -d sdk/$${dir}/include ]; then \
			cp -a sdk/$${dir}/include/* $(SDK_FILES_DIR)/sdk/include/; fi ;\
	done

	# 复制CI下载下来的产物的头文件
	@if [ -d sdk/include ]; then cp -arf sdk/include/* $(SDK_FILES_DIR)/sdk/include/; fi
	# 复制旧版兼容头文件
	@if [ -d $(OUTPUT_DIR)/compat/include ]; then cp -a $(OUTPUT_DIR)/compat/include $(SDK_FILES_DIR)/; fi
	# 复制库文件
	@cp $(OUTPUT_DIR)/lib/libtuya_iot.* $(SDK_FILES_DIR)/sdk/lib/
	# 加入 change log
	@cp CHANGELOG.md $(SDK_FILES_DIR)/
	@cp README.md $(SDK_FILES_DIR)/
	# 生成应用构建脚本
	@cp ./scripts/build_app.user.sh $(SDK_FILES_DIR)/build_app.sh
	@sed -i 's,####TARGET_PLATFORM####,$(TARGET_PLATFORM),g' $(SDK_FILES_DIR)/build_app.sh
	@sed -i 's,####TARGET_PLATFORM_REPO####,$(TARGET_PLATFORM_REPO),g' $(SDK_FILES_DIR)/build_app.sh
	@sed -i 's,####TARGET_PLATFORM_VERSION####,$(TARGET_PLATFORM_VERSION),g' $(SDK_FILES_DIR)/build_app.sh
	@sed -i 's,####WIFI_GW####,$(WIFI_GW),g' $(SDK_FILES_DIR)/build_app.sh
	@chmod a+x $(SDK_FILES_DIR)/build_app.sh
	@echo "------ build sdk files end ---------"

# SDK产物包全路径名称
ifneq ($(CI_PACKAGE_FULLNAME),)
PACKAGE_FULLNAME := $(CI_PACKAGE_FULLNAME)
else
PACKAGE_FULLNAME := $(dir $(OUTPUT_DIR))/dist/$(SDK_FILES_PATH_NAME).tar.gz
endif

# SDK 打包
sdk_pack:
	@echo "====== pack begin ======="
	@rm -fr $(SDK_FILES_DIR)/../sdk.tar.gz
	@cd $(SDK_FILES_DIR)/../ && tar zcvf sdk.tar.gz $(SDK_FILES_PATH_NAME)
	@rm -fr $(PACKAGE_FULLNAME)
	@mkdir -p `dirname $(PACKAGE_FULLNAME)`
	@mv $(SDK_FILES_DIR)/../sdk.tar.gz $(PACKAGE_FULLNAME)
	@echo "------ pack end ---------"

############################################################
# 编译 demo app
############################################################

# 在生成的SDK包环境中编译
sdk_demo:
	@echo "====== build sdk demo begin ======="
	@cd $(SDK_FILES_DIR); sh ./build_app.sh apps/$(DEMO_NAME) $(DEMO_NAME) $(USER_SW_VER)
	@echo "------ build sdk demo end ---------"


# 在当前环境中编译
demo:
	@echo "====== build demo $$_demos begin =======";
	@rm -rf sdk/include;
	@mkdir -p sdk/lib/;
	@mkdir -p sdk/include/;	
	@ln -bs $(OUTPUT_DIR)/lib/libtuya_iot.a sdk/lib/libtuya_iot.a;
	@ln -bs $(OUTPUT_DIR)/lib/libtuya_iot.a.stripped sdk/lib/libtuya_iot.a.stripped;
	
	# 复制对外头文件
	@for dir in `ls -1 sdk`; do		\
		if [ -d sdk/$${dir}/include ]; then \
			cp -a sdk/$${dir}/include/* sdk/include/; fi ;\
		done
		
	# 开始编译demo
	_demos=`sh ./scripts/get_sub_dir.sh apps`;	\
		echo $${_demos};	\
		cd ./apps/$${_demos}/; \
		if [ -f build.sh ]; then	\
			sh ./build.sh $${_demos} 1.0.0 $(TARGET_PLATFORM);	\
		else	\
			make APP_BIN_NAME=$${_demos} USER_SW_VER=1.0.0 all;	\
		fi
		
	echo "------ build demo end ---------"


############################################################
# 编译运行单元测试
############################################################

ut_prepare:
	@make -C ut prepare

ut: ut_prepare
	@echo "====== build unit test begin ======="
	make -C ut ut
	@echo "------ build unit test end ---------"

ut_run: ut_prepare
	@echo "====== run unit test begin ======="
	make -C ut ut_run
	@echo "------ run unit test end ------"
ifeq ($(COV), 1)
	make -C ut coverage
endif

# 生成UT包文件目录
UT_FILES_PATH_NAME?=sdk_ut_log
# UT产物包全路径名称
UT_PACKAGE_FULLNAME := $(dir $(OUTPUT_DIR))/dist/$(UT_FILES_PATH_NAME).tar.gz
ut_pack:
	make -C ut ut_pack UT_FILES_PATH_NAME=$(UT_FILES_PATH_NAME) UT_PACKAGE_FULLNAME=$(UT_PACKAGE_FULLNAME)

clean_ut:
	make -C ut clean

############################################################
# 使用cppcheck进行静态检测
############################################################
# cppcheck时需要定义的宏 -D开头
CPPCHECK_D = -Dbool_t=int
# cppcheck时不需要定义的宏 -U开头
CPPCHECK_U = -U__cplusplus

# 全部检测
# 有error级别结果的会报错
cppcheck:
	@echo "====== run cppcheck begin ======="
	cppcheck --enable=warning,style,missingInclude $(CPPCHECK_D) $(CPPCHECK_U)  --error-exitcode=99\
	$(addprefix -I,$(SDK_INCLUDE_ALL_SUBDIRS)) \
	$(SDK_SRC_ALL_SUBDIRS)
	@echo "------ run cppcheck end ---------"

# 对指定组件进行检测
# 不会对error级别的结果敏感
# --force会对所有未定义的宏进行定义后检测
cppcheck_one:
	@echo "====== run cppcheck_one begin ======="
ifdef CPPCHECK
	cppcheck --enable=warning,style,missingInclude $(CPPCHECK_D) $(CPPCHECK_U) --force \
	$(addprefix -I,$(SDK_INCLUDE_ALL_SUBDIRS)) \
	$(patsubst %, ./sdk/%/src/, $(CPPCHECK))
else
	@echo "need: make cppcheck_one CPPCHECK=<name1 name2 ...>"
endif
	@echo "------ run cppcheck_one end ---------"

############################################################
# 其他目标和依赖关系
############################################################

.PHONY: all clean sdk demo sdk_static sdk_shared sdk_compat_headers sdk_files sdk_pack cppcheck cppcheck_one test

# 清空中间文件
clean:
	rm -fr $(OUTPUT_DIR)
	rm -fr ./sdk/include
	rm -fr ./sdk/lib
auto:clean config sdk demo

ut: config
ut_run: config
pack: sdk_pack
demo: sdk
sdk_demo: sdk_files
sdk_pack: sdk_files
sdk_files: sdk sdk_compat_headers
ifeq ($(BUILD_STATIC), 1)
sdk: sdk_static
endif
ifeq ($(BUILD_SHARED), 1)
sdk: sdk_shared
endif
sdk_static: xmake_static
sdk_shared: xmake_shared
config: generate_iot_config
clean_iot_config:
clean: clean_ut  clean_iot_config


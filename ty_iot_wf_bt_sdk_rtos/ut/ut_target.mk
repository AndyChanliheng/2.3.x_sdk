
# ----------------------------------------------------
# 为 MOCK 生成规则
# ----------------------------------------------------

# 清除 LOCAL_xxx 变量
include $(CLEAR_VARS)

# 定义目标名称和源码列表
LOCAL_MODULE := $(UT_MODULE)_mock
LOCAL_SRC_FILES := $(UT_MOCK_FILES)

# 通用选项
LOCAL_CFLAGS += -I$(TOPDIR)/ut/externals/gtest/include

# 生成静态库
include $(BUILD_STATIC_LIBRARY)

# ----------------------------------------------------
# 为单元测试可执行程序生成规则
# ----------------------------------------------------

# 清除 LOCAL_xxx 变量
include $(CLEAR_VARS)

# 定义单元测试目标和源码列表
LOCAL_MODULE := ut_$(UT_MODULE)
LOCAL_SRC_FILES := $(UT_TEST_FILES)
UT_TARGET := $(XMAKE_OUTDIR)/bin/$(LOCAL_MODULE)
UT_TARGETS += $(UT_TARGET)

# 通用选项
LOCAL_CFLAGS += -I$(TOPDIR)/ut/externals/gtest/include 
LOCAL_LDFLAGS += $(UT_MOCK_INCLUDE)
LOCAL_LDFLAGS += -Lexternals/gtest/lib -lgtest -lgmock
LOCAL_LDFLAGS += -lpthread

# 添加依赖的SDK组件
UT_SDK_DEPENDS_LIBS := $(patsubst %,$(XMAKE_OUTDIR)/lib/lib%.a,$(UT_SDK_DEPENDS))
$(UT_TARGET): $(UT_SDK_DEPENDS_LIBS)

# 添加依赖的MOCK
UT_MOCK_DEPENDS_LIBS := $(patsubst %,$(XMAKE_OUTDIR)/lib/lib%_mock.a,$(UT_MOCK_DEPENDS))
$(UT_TARGET): $(UT_MOCK_DEPENDS_LIBS)

# 生成可执行程序
include $(BUILD_EXECUTABLE)

# ----------------------------------------------------
